#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Team.h"
#include "World.h"
#include "City.h"
#include "InsideLocation.h"
#include "InsideLocationLevel.h"
#include "Level.h"
#include "GroundItem.h"
#include "Spell.h"
#include "PlayerInfo.h"
#include "AIController.h"
#include "QuestManager.h"
#include "Quest.h"
#include "Quest_Secret.h"
#include "Quest_Scripted.h"
#include "Quest_Tournament.h"
#include "GameStats.h"
#include "GameGui.h"
#include "Journal.h"
#include "WorldMapGui.h"
#include "GameMessages.h"
#include "Notifications.h"
#include "LevelGui.h"
#include "ServerPanel.h"
#include "MpBox.h"
#include "DialogContext.h"
#include "EntityInterpolator.h"
#include "Game.h"
#include "FOV.h"
#include "ScriptManager.h"
#include "ItemHelper.h"
#include "CommandParser.h"
#include "Arena.h"
#include "Door.h"
#include "SoundManager.h"
#include "LobbyApi.h"
#include "GameFile.h"
#include "SaveState.h"

const float CHANGE_LEVEL_TIMER = 5.f;

//=================================================================================================
void Net::InitServer()
{
	Info("Creating server (port %d)...", port);

	OpenPeer();

	uint max_connections = max_players - 1;
	if(!server_lan)
		++max_connections;

	SocketDescriptor sd(port, nullptr);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(max_connections, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create server: SLikeNet error %d.", r);
		throw Format(txCreateServerFailed, r);
	}

	if(!password.empty())
	{
		Info("Set server password.");
		peer->SetIncomingPassword(password.c_str(), password.length());
	}
	else
		peer->SetIncomingPassword(nullptr, 0);

	peer->SetMaximumIncomingConnections((word)max_players - 1);
	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));

	if(!server_lan)
	{
		ConnectionAttemptResult result = peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0, nullptr, 0, 6);
		if(result == CONNECTION_ATTEMPT_STARTED)
			master_server_state = MasterServerState::Connecting;
		else
		{
			master_server_state = MasterServerState::NotConnected;
			Error("Failed to connect to master server: SLikeNet error %d.", result);
		}
	}
	master_server_adr = UNASSIGNED_SYSTEM_ADDRESS;

	Info("Server created. Waiting for connection.");

	SetMode(Mode::Server);
}

//=================================================================================================
void Net::OnNewGameServer()
{
	active_players = 1;
	DeleteElements(players);
	team->my_id = 0;
	team->leader_id = 0;
	last_id = 0;
	game->paused = false;
	game->hardcore_mode = false;

	PlayerInfo* info = new PlayerInfo;
	players.push_back(info);

	PlayerInfo& sp = *info;
	sp.name = game->player_name;
	sp.id = 0;
	sp.state = PlayerInfo::IN_LOBBY;
	sp.left = PlayerInfo::LEFT_NO;

	game->skip_id_counter = 0;
	update_timer = 0.f;
	game->arena->Reset();
	team->anyone_talking = false;
	warps.clear();
	if(!mp_load)
		changes.clear(); // przy wczytywaniu jest czyszczone przed wczytaniem i w net_changes s¹ zapisane quest_items
	if(!net_strs.empty())
		StringPool.Free(net_strs);
	game_gui->server->max_players = max_players;
	game_gui->server->server_name = server_name;
	game_gui->server->UpdateServerInfo();
	game_gui->server->Show();
	if(!mp_quickload)
	{
		game_gui->mp_box->Reset();
		game_gui->mp_box->visible = true;
	}

	if(!mp_load)
		game_gui->server->CheckAutopick();
	else
	{
		// search for saved character
		PlayerInfo* old = FindOldPlayer(game->player_name.c_str());
		if(old)
		{
			sp.devmode = old->devmode;
			sp.clas = old->clas;
			sp.hd.CopyFrom(old->hd);
			sp.loaded = true;
			game_gui->server->UseLoadedCharacter(true);
		}
		else
		{
			game_gui->server->UseLoadedCharacter(false);
			game_gui->server->CheckAutopick();
		}
	}

	game->ChangeTitle();
}

//=================================================================================================
void Net::UpdateServer(float dt)
{
	if(game->game_state == GS_LEVEL)
	{
		for(PlayerInfo& info : players)
		{
			if(info.left == PlayerInfo::LEFT_NO && !info.pc->is_local)
				info.pc->UpdateCooldown(dt);
		}

		InterpolatePlayers(dt);
		UpdateFastTravel(dt);
		game->pc->unit->changed = true;
	}

	Packet* packet;
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		BitStreamReader reader(packet);
		PlayerInfo* ptr_info = FindPlayer(packet->systemAddress);
		if(!ptr_info || ptr_info->left != PlayerInfo::LEFT_NO)
		{
			if(packet->data[0] != ID_UNCONNECTED_PING)
				Info("Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}

		PlayerInfo& info = *ptr_info;
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			Info(msg_id == ID_CONNECTION_LOST ? "Lost connection with player %s." : "Player %s has disconnected.", info.name.c_str());
			--active_players;
			players_left = true;
			info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			break;
		case ID_SAY:
			Server_Say(reader, info);
			break;
		case ID_WHISPER:
			Server_Whisper(reader, info);
			break;
		case ID_CONTROL:
			if(!ProcessControlMessageServer(reader, info))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("UpdateServer: Unknown packet from %s: %u.", info.name.c_str(), msg_id);
			break;
		}
	}

	ProcessLeftPlayers();

	update_timer += dt;
	if(update_timer >= TICK && active_players > 1)
	{
		bool last_anyone_talking = team->anyone_talking;
		team->anyone_talking = team->IsAnyoneTalking();
		if(last_anyone_talking != team->anyone_talking)
			PushChange(NetChange::CHANGE_FLAGS);

		update_timer = 0;
		BitStreamWriter f;
		f << ID_CHANGES;

		// dodaj zmiany pozycji jednostek i ai_mode
		if(game->game_state == GS_LEVEL)
		{
			for(LevelArea& area : game_level->ForEachArea())
				ServerProcessUnits(area.units);
		}

		// wyœlij odkryte kawa³ki minimapy
		if(!game_level->minimap_reveal_mp.empty())
		{
			if(game->game_state == GS_LEVEL)
				PushChange(NetChange::REVEAL_MINIMAP);
			else
				game_level->minimap_reveal_mp.clear();
		}

		// changes
		WriteServerChanges(f);
		changes.clear();
		assert(net_strs.empty());
		SendAll(f, HIGH_PRIORITY, RELIABLE_ORDERED);

		for(PlayerInfo& info : players)
		{
			if(info.id == team->my_id || info.left != PlayerInfo::LEFT_NO)
				continue;

			// update stats
			if(info.u->player->stat_flags != 0)
			{
				NetChangePlayer& c = Add1(info.changes);
				c.id = info.u->player->stat_flags;
				c.type = NetChangePlayer::PLAYER_STATS;
				info.u->player->stat_flags = 0;
			}

			// write & send updates
			if(!info.changes.empty() || info.update_flags)
			{
				BitStreamWriter f;
				WriteServerChangesForPlayer(f, info);
				SendServer(f, HIGH_PRIORITY, RELIABLE_ORDERED, info.adr);
				info.update_flags = 0;
				info.changes.clear();
			}
		}
	}
}

//=================================================================================================
void Net::InterpolatePlayers(float dt)
{
	for(PlayerInfo& info : players)
	{
		if(!info.pc->is_local && info.left == PlayerInfo::LEFT_NO)
			info.u->interp->Update(dt, info.u->visual_pos, info.u->rot);
	}
}

//=================================================================================================
void Net::UpdateFastTravel(float dt)
{
	if(!fast_travel)
		return;

	if(!game_level->CanFastTravel())
	{
		CancelFastTravel(FAST_TRAVEL_NOT_SAFE, 0);
		return;
	}

	int player_id = -1;
	bool ok = true;
	for(PlayerInfo& info : players)
	{
		if(info.left == PlayerInfo::LEFT_NO && !info.fast_travel)
		{
			player_id = info.id;
			ok = false;
			break;
		}
	}

	if(ok)
	{
		NetChange& c = Add1(changes);
		c.type = NetChange::FAST_TRAVEL;
		c.id = FAST_TRAVEL_IN_PROGRESS;

		if(fast_travel_notif)
		{
			fast_travel_notif->Close();
			fast_travel_notif = nullptr;
		}

		game->ExitToMap();
	}
	else
	{
		fast_travel_timer += dt;
		if(fast_travel_timer >= 5.f)
			CancelFastTravel(FAST_TRAVEL_DENY, player_id);
	}
}

//=================================================================================================
void Net::ServerProcessUnits(vector<Unit*>& units)
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->changed)
		{
			NetChange& c = Add1(changes);
			c.type = NetChange::UNIT_POS;
			c.unit = *it;
			(*it)->changed = false;
		}
		if((*it)->IsAI() && (*it)->ai->change_ai_mode)
		{
			NetChange& c = Add1(changes);
			c.type = NetChange::CHANGE_AI_MODE;
			c.unit = *it;
			(*it)->ai->change_ai_mode = false;
		}
	}
}

//=================================================================================================
void Net::UpdateWarpData(float dt)
{
	LoopAndRemove(warps, [dt](WarpData& warp)
	{
		if((warp.timer -= dt * 2) > 0.f)
			return false;

		game_level->WarpUnit(warp.u, warp.where);
		warp.u->frozen = FROZEN::NO;

		NetChangePlayer& c = Add1(warp.u->player->player_info->changes);
		c.type = NetChangePlayer::SET_FROZEN;
		c.id = 0;

		return true;
	});
}

//=================================================================================================
void Net::ProcessLeftPlayers()
{
	if(!players_left)
		return;

	LoopAndRemove(players, [this](PlayerInfo& info)
	{
		if(info.left == PlayerInfo::LEFT_NO)
			return false;

		// order of changes is important here
		NetChange& c = Add1(changes);
		c.type = NetChange::REMOVE_PLAYER;
		c.id = info.id;
		c.count = (int)info.left;

		RemovePlayer(info);

		if(team->leader_id == c.id)
		{
			team->leader_id = team->my_id;
			team->leader = game->pc->unit;
			NetChange& c2 = Add1(changes);
			c2.type = NetChange::CHANGE_LEADER;
			c2.id = team->my_id;

			if(game_gui->world_map->dialog_enc)
				game_gui->world_map->dialog_enc->bts[0].state = Button::NONE;

			game_gui->AddMsg(game->txYouAreLeader);
		}

		team->CheckCredit();
		team->CalculatePlayersLevel();
		delete &info;

		return true;
	});

	players_left = false;
}

//=================================================================================================
void Net::RemovePlayer(PlayerInfo& info)
{
	switch(info.left)
	{
	case PlayerInfo::LEFT_TIMEOUT:
		{
			Info("Player %s kicked due to timeout.", info.name.c_str());
			game_gui->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_KICK:
		{
			Info("Player %s kicked from server.", info.name.c_str());
			game_gui->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_DISCONNECTED:
		{
			Info("Player %s disconnected from server.", info.name.c_str());
			game_gui->AddMsg(Format(game->txPlayerDisconnected, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_QUIT:
		{
			Info("Player %s quit game.", info.name.c_str());
			game_gui->AddMsg(Format(game->txPlayerQuit, info.name.c_str()));
		}
		break;
	default:
		assert(0);
		break;
	}

	if(!info.u)
		return;

	Unit* unit = info.u;
	RemoveElementOrder(team->members.ptrs, unit);
	RemoveElementOrder(team->active_members.ptrs, unit);
	if(game->game_state == GS_WORLDMAP)
	{
		if(IsLocal() && !game_level->is_open)
			game->DeleteUnit(unit);
	}
	else
	{
		game_level->to_remove.push_back(unit);
		unit->to_remove = true;
	}
	info.u = nullptr;
}

//=================================================================================================
bool Net::ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info)
{
	bool move_info;
	f >> move_info;
	if(!f)
	{
		Error("UpdateServer: Broken packet ID_CONTROL from %s.", info.name.c_str());
		return true;
	}

	Unit& unit = *info.u;
	PlayerController& player = *info.u->player;

	// movement/animation info
	if(move_info)
	{
		if(!info.warping && game->game_state == GS_LEVEL)
		{
			Vec3 new_pos;
			float rot;
			Animation ani;

			f >> new_pos;
			f >> rot;
			f >> unit.mesh_inst->groups[0].speed;
			f.ReadCasted<byte>(ani);
			if(!f)
			{
				Error("UpdateServer: Broken packet ID_CONTROL(2) from %s.", info.name.c_str());
				return true;
			}

			float dist = Vec3::Distance(unit.pos, new_pos);
			if(dist >= 10.f)
			{
				// too big change in distance, warp unit to old position
				Warn("UpdateServer: Invalid unit movement from %s ((%g,%g,%g) -> (%g,%g,%g)).", info.name.c_str(), unit.pos.x, unit.pos.y, unit.pos.z,
					new_pos.x, new_pos.y, new_pos.z);
				game_level->WarpUnit(unit, unit.pos);
				unit.interp->Add(unit.pos, rot);
			}
			else
			{
				unit.player->TrainMove(dist);
				if(player.noclip || unit.usable || CheckMove(unit, new_pos))
				{
					// update position
					if(!unit.pos.Equal(new_pos) && !game_level->location->outside)
					{
						// reveal minimap
						Int2 new_tile(int(new_pos.x / 2), int(new_pos.z / 2));
						if(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)) != new_tile)
							FOV::DungeonReveal(new_tile, game_level->minimap_reveal);
					}
					unit.pos = new_pos;
					unit.UpdatePhysics();
					unit.interp->Add(unit.pos, rot);
					unit.changed = true;
				}
				else
				{
					// player is now stuck inside something, unstuck him
					unit.interp->Add(unit.pos, rot);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::UNSTUCK;
					c.pos = unit.pos;
				}
			}

			// animation
			if(unit.animation != ANI_PLAY && ani != ANI_PLAY)
				unit.animation = ani;
		}
		else
		{
			// player is warping or not in level, skip movement
			f.Skip(sizeof(Vec3) + sizeof(float) * 2 + sizeof(byte));
			if(!f)
			{
				Error("UpdateServer: Broken packet ID_CONTROL(3) from %s.", info.name.c_str());
				return true;
			}
		}
	}

	// count of changes
	byte changes_count;
	f >> changes_count;
	if(!f)
	{
		Error("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str());
		return true;
	}

	// process changes
	for(byte change_i = 0; change_i < changes_count; ++change_i)
	{
		// change type
		NetChange::TYPE type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			Error("UpdateServer: Broken packet ID_CONTROL(5) from %s.", info.name.c_str());
			return true;
		}

		switch(type)
		{
		// player change equipped items
		case NetChange::CHANGE_EQUIPMENT:
			{
				int i_index;
				f >> i_index;
				if(!f)
					Error("Update server: Broken CHANGE_EQUIPMENT from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(i_index >= 0)
					{
						// equipping item
						if(uint(i_index) >= unit.items.size())
						{
							Error("Update server: CHANGE_EQUIPMENT from %s, invalid index %d.", info.name.c_str(), i_index);
							break;
						}

						ItemSlot& slot = unit.items[i_index];
						if(!unit.CanWear(slot.item))
						{
							Error("Update server: CHANGE_EQUIPMENT from %s, item at index %d (%s) is not wearable.",
								info.name.c_str(), i_index, slot.item->id.c_str());
							break;
						}

						ITEM_SLOT slot_type = ItemTypeToSlot(slot.item->type);
						if(slot_type == SLOT_RING1)
						{
							if(unit.slots[slot_type])
							{
								if(!unit.slots[SLOT_RING2] || unit.player->last_ring)
									slot_type = SLOT_RING2;
							}
							unit.player->last_ring = (slot_type == SLOT_RING2);
						}
						if(unit.slots[slot_type])
						{
							unit.RemoveItemEffects(unit.slots[slot_type], slot_type);
							unit.ApplyItemEffects(slot.item, slot_type);
							std::swap(unit.slots[slot_type], slot.item);
							SortItems(unit.items);
						}
						else
						{
							unit.ApplyItemEffects(slot.item, slot_type);
							unit.slots[slot_type] = slot.item;
							unit.items.erase(unit.items.begin() + i_index);
						}

						// send to other players
						if(active_players > 2 && IsVisible(slot_type))
						{
							NetChange& c = Add1(changes);
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = &unit;
							c.id = slot_type;
						}
					}
					else
					{
						// removing item
						ITEM_SLOT slot = IIndexToSlot(i_index);

						if(slot < SLOT_WEAPON || slot >= SLOT_MAX)
							Error("Update server: CHANGE_EQUIPMENT from %s, invalid slot type %d.", info.name.c_str(), slot);
						else if(!unit.slots[slot])
							Error("Update server: CHANGE_EQUIPMENT from %s, empty slot type %d.", info.name.c_str(), slot);
						else
						{
							unit.RemoveItemEffects(unit.slots[slot], slot);
							unit.AddItem(unit.slots[slot], 1u, false);
							unit.weight -= unit.slots[slot]->weight;
							unit.slots[slot] = nullptr;

							// send to other players
							if(active_players > 2 && IsVisible(slot))
							{
								NetChange& c = Add1(changes);
								c.type = NetChange::CHANGE_EQUIPMENT;
								c.unit = &unit;
								c.id = slot;
							}
						}
					}
				}
			}
			break;
		// player take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				bool hide;
				WeaponType weapon_type;
				f >> hide;
				f.ReadCasted<byte>(weapon_type);
				if(!f)
					Error("Update server: Broken TAKE_WEAPON from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					unit.mesh_inst->groups[1].speed = 1.f;
					unit.SetWeaponState(!hide, weapon_type);
					// send to other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.unit = &unit;
						c.type = NetChange::TAKE_WEAPON;
						c.id = (hide ? 1 : 0);
					}
				}
			}
			break;
		// player attacks
		case NetChange::ATTACK:
			{
				byte typeflags;
				float attack_speed, yspeed;
				f >> typeflags;
				f >> attack_speed;
				if((typeflags & 0xF) == AID_Shoot)
					f >> yspeed;
				if(!f)
					Error("Update server: Broken ATTACK from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					byte type = (typeflags & 0xF);

					// force taken weapon in hand
					unit.weapon_state = WS_TAKEN;

					switch(type)
					{
					case AID_Attack:
						if(unit.action == A_ATTACK && unit.animation_state == 0)
						{
							unit.attack_power = unit.mesh_inst->groups[1].time / unit.GetAttackFrame(0);
							unit.animation_state = 1;
							unit.mesh_inst->groups[1].speed = (unit.attack_power + unit.GetAttackSpeed()) * unit.GetStaminaAttackSpeedMod();
							unit.attack_power += 1.f;
						}
						else
						{
							if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
								game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.f;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 1;
							unit.hitted = false;
						}
						unit.player->Train(TrainWhat::AttackStart, 0.f, 0);
						break;
					case AID_PrepareAttack:
						{
							if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
								game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.f;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 0;
							unit.hitted = false;
							unit.RemoveStamina(unit.GetWeapon().GetInfo().stamina);
							unit.timer = 0.f;
						}
						break;
					case AID_Shoot:
					case AID_StartShoot:
						if(unit.action == A_SHOOT && unit.animation_state == 0)
							unit.animation_state = 1;
						else
						{
							unit.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.action = A_SHOOT;
							unit.animation_state = (type == AID_Shoot ? 1 : 0);
							unit.hitted = false;
							if(!unit.bow_instance)
								unit.bow_instance = game_level->GetBowInstance(unit.GetBow().mesh);
							unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
							unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[1].speed;
							unit.RemoveStamina(Unit::STAMINA_BOW_ATTACK);
						}
						if(type == AID_Shoot)
							info.yspeed = yspeed;
						break;
					case AID_Block:
						{
							unit.action = A_BLOCK;
							unit.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = 1.f;
							unit.mesh_inst->groups[1].blend_max = attack_speed;
							unit.animation_state = 0;
						}
						break;
					case AID_Bash:
						{
							unit.action = A_BASH;
							unit.animation_state = 0;
							unit.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.mesh_inst->frame_end_info2 = false;
							unit.hitted = false;
							unit.player->Train(TrainWhat::BashStart, 0.f, 0);
							unit.RemoveStamina(Unit::STAMINA_BASH_ATTACK);
						}
						break;
					case AID_RunningAttack:
						{
							if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
								game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
							unit.action = A_ATTACK;
							unit.attack_id = ((typeflags & 0xF0) >> 4);
							unit.attack_power = 1.5f;
							unit.run_attack = true;
							unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
							unit.mesh_inst->groups[1].speed = attack_speed;
							unit.animation_state = 1;
							unit.hitted = false;
							unit.RemoveStamina(unit.GetWeapon().GetInfo().stamina * 1.5f);
						}
						break;
					case AID_StopBlock:
						{
							unit.action = A_NONE;
							unit.mesh_inst->frame_end_info2 = false;
							unit.mesh_inst->Deactivate(1);
						}
						break;
					}

					// send to other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.unit = &unit;
						c.type = NetChange::ATTACK;
						c.id = typeflags;
						c.f[1] = attack_speed;
					}
				}
			}
			break;
		// player drops item
		case NetChange::DROP_ITEM:
			{
				int i_index, count;
				f >> i_index;
				f >> count;
				if(!f)
					Error("Update server: Broken DROP_ITEM from %s.", info.name.c_str());
				else if(count == 0)
					Error("Update server: DROP_ITEM from %s, count %d.", info.name.c_str(), count);
				else if(game->game_state == GS_LEVEL)
				{
					GroundItem* item;

					if(i_index >= 0)
					{
						// dropping unequipped item
						if(i_index >= (int)unit.items.size())
						{
							Error("Update server: DROP_ITEM from %s, invalid index %d (count %d).", info.name.c_str(), i_index, count);
							break;
						}

						ItemSlot& sl = unit.items[i_index];
						if(count > (int)sl.count)
						{
							Error("Update server: DROP_ITEM from %s, index %d (count %d) have only %d count.", info.name.c_str(), i_index,
								count, sl.count);
							count = sl.count;
						}
						sl.count -= count;
						unit.weight -= sl.item->weight*count;
						item = new GroundItem;
						item->Register();
						item->item = sl.item;
						item->count = count;
						item->team_count = min(count, (int)sl.team_count);
						sl.team_count -= item->team_count;
						if(sl.count == 0)
							unit.items.erase(unit.items.begin() + i_index);
					}
					else
					{
						// dropping equipped item
						ITEM_SLOT slot_type = IIndexToSlot(i_index);
						if(!IsValid(slot_type))
						{
							Error("Update server: DROP_ITEM from %s, invalid slot %d.", info.name.c_str(), slot_type);
							break;
						}

						const Item*& slot = unit.slots[slot_type];
						if(!slot)
						{
							Error("Update server: DROP_ITEM from %s, empty slot %d.", info.name.c_str(), slot_type);
							break;
						}

						unit.RemoveItemEffects(slot, slot_type);
						unit.weight -= slot->weight*count;
						item = new GroundItem;
						item->Register();
						item->item = slot;
						item->count = 1;
						item->team_count = 0;
						slot = nullptr;

						// send info about changing equipment to other players
						if(active_players > 2 && IsVisible(slot_type))
						{
							NetChange& c = Add1(changes);
							c.type = NetChange::CHANGE_EQUIPMENT;
							c.unit = &unit;
							c.id = slot_type;
						}
					}

					unit.action = A_ANIMATION;
					unit.mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot)*0.25f;
					item->pos.z -= cos(unit.rot)*0.25f;
					item->rot = Random(MAX_ANGLE);
					if(!quest_mgr->quest_secret->CheckMoonStone(item, unit))
						game_level->AddGroundItem(*unit.area, item);

					// send to other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::DROP_ITEM;
						c.unit = &unit;
					}
				}
			}
			break;
		// player wants to pick up item
		case NetChange::PICKUP_ITEM:
			{
				int id;
				f >> id;
				if(!f)
				{
					Error("Update server: Broken PICKUP_ITEM from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				LevelArea* area;
				GroundItem* item = game_level->FindGroundItem(id, &area);
				if(!item)
				{
					Error("Update server: PICKUP_ITEM from %s, missing item %d.", info.name.c_str(), id);
					break;
				}

				// add item
				unit.AddItem(item->item, item->count, item->team_count);

				// start animation
				bool up_animation = (item->pos.y > unit.pos.y + 0.5f);
				unit.action = A_PICKUP;
				unit.animation = ANI_PLAY;
				unit.mesh_inst->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
				unit.mesh_inst->groups[0].speed = 1.f;
				unit.mesh_inst->frame_end_info = false;

				// send pickup acceptation
				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::PICKUP;
				c.id = item->count;
				c.count = item->team_count;

				// send remove item to all players
				NetChange& c2 = Add1(changes);
				c2.type = NetChange::REMOVE_ITEM;
				c2.id = item->id;

				// send info to other players about picking item
				if(active_players > 2)
				{
					NetChange& c3 = Add1(changes);
					c3.type = NetChange::PICKUP_ITEM;
					c3.unit = &unit;
					c3.count = (up_animation ? 1 : 0);
				}

				// remove item
				if(game->pc->data.before_player == BP_ITEM && game->pc->data.before_player_ptr.item == item)
					game->pc->data.before_player = BP_NONE;
				RemoveElement(area->items, item);

				// event
				for(Event& event : game_level->location->events)
				{
					if(event.type == EVENT_PICKUP)
					{
						ScriptEvent e(EVENT_PICKUP);
						e.item = item;
						event.quest->FireEvent(e);
					}
				}

				delete item;
			}
			break;
		// player consume item
		case NetChange::CONSUME_ITEM:
			{
				int index;
				f >> index;
				if(!f)
					Error("Update server: Broken CONSUME_ITEM from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
					unit.ConsumeItem(index);
			}
			break;
		// player wants to loot unit
		case NetChange::LOOT_UNIT:
			{
				int id;
				f >> id;
				if(!f)
				{
					Error("Update server: Broken LOOT_UNIT from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Unit* looted_unit = game_level->FindUnit(id);
				if(!looted_unit)
				{
					Error("Update server: LOOT_UNIT from %s, missing unit %d.", info.name.c_str(), id);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::LOOT;
				if(looted_unit->busy == Unit::Busy_Looted)
				{
					// someone else is already looting unit
					c.id = 0;
				}
				else
				{
					// start looting unit
					c.id = 1;
					looted_unit->busy = Unit::Busy_Looted;
					player.action = PlayerAction::LootUnit;
					player.action_unit = looted_unit;
					player.chest_trade = &looted_unit->items;
				}
			}
			break;
		// player wants to loot chest
		case NetChange::USE_CHEST:
			{
				int id;
				f >> id;
				if(!f)
				{
					Error("Update server: Broken USE_CHEST from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Chest* chest = game_level->FindChest(id);
				if(!chest)
				{
					Error("Update server: USE_CHEST from %s, missing chest %d.", info.name.c_str(), id);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::LOOT;
				if(chest->GetUser())
				{
					// someone else is already looting this chest
					c.id = 0;
				}
				else
				{
					// start looting chest
					c.id = 1;
					player.action = PlayerAction::LootChest;
					player.action_chest = chest;
					player.chest_trade = &chest->items;
					chest->OpenClose(player.unit);
				}
			}
			break;
		// player gets item from unit or container
		case NetChange::GET_ITEM:
			{
				int i_index, count;
				f >> i_index;
				f >> count;
				if(!f)
				{
					Error("Update server: Broken GET_ITEM from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				if(!player.IsTrading())
				{
					Error("Update server: GET_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(i_index >= 0)
				{
					// getting not equipped item
					if(i_index >= (int)player.chest_trade->size())
					{
						Error("Update server: GET_ITEM from %s, invalid index %d.", info.name.c_str(), i_index);
						break;
					}

					ItemSlot& slot = player.chest_trade->at(i_index);
					if(count < 1 || count >(int)slot.count)
					{
						Error("Update server: GET_ITEM from %s, invalid item count %d (have %d).", info.name.c_str(), count, slot.count);
						break;
					}

					if(slot.item->type == IT_GOLD)
					{
						// special handling of gold
						uint team_count = min(slot.team_count, (uint)count);
						if(team_count == 0)
						{
							unit.gold += slot.count;
							info.UpdateGold();
						}
						else
						{
							team->AddGold(team_count);
							uint count = slot.count - team_count;
							if(count)
							{
								unit.gold += count;
								info.UpdateGold();
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chest_trade->erase(player.chest_trade->begin() + i_index);
						else
							slot.team_count -= team_count;
					}
					else
					{
						// player get item from corpse/chest/npc or bought from trader
						uint team_count = (player.action == PlayerAction::Trade ? 0 : min((uint)count, slot.team_count));
						unit.AddItem2(slot.item, (uint)count, team_count, false, false);
						if(player.action == PlayerAction::Trade)
						{
							int price = ItemHelper::GetItemPrice(slot.item, unit, true) * count;
							unit.gold -= price;
							player.Train(TrainWhat::Trade, (float)price, 0);
						}
						else if(player.action == PlayerAction::ShareItems && slot.item->type == IT_CONSUMABLE)
						{
							const Consumable& pot = slot.item->ToConsumable();
							if(pot.ai_type == ConsumableAiType::Healing)
								player.action_unit->ai->have_potion = HavePotion::Check;
							else if(pot.ai_type == ConsumableAiType::Mana)
								player.action_unit->ai->have_mp_potion = HavePotion::Check;
						}
						if(player.action != PlayerAction::LootChest && player.action != PlayerAction::LootContainer)
						{
							player.action_unit->weight -= slot.item->weight*count;
							if(player.action == PlayerAction::LootUnit)
							{
								if(slot.item == player.action_unit->used_item)
								{
									player.action_unit->used_item = nullptr;
									// removed item from hand, send info to other players
									if(active_players > 2)
									{
										NetChange& c = Add1(changes);
										c.type = NetChange::REMOVE_USED_ITEM;
										c.unit = player.action_unit;
									}
								}
								if(IsSet(slot.item->flags, ITEM_IMPORTANT))
								{
									player.action_unit->mark = false;
									NetChange& c = Add1(changes);
									c.type = NetChange::MARK_UNIT;
									c.unit = player.action_unit;
									c.id = 0;
								}
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chest_trade->erase(player.chest_trade->begin() + i_index);
						else
							slot.team_count -= team_count;
					}
				}
				else
				{
					// getting equipped item
					ITEM_SLOT type = IIndexToSlot(i_index);
					if(player.action == PlayerAction::LootChest || player.action == PlayerAction::LootContainer
						|| type < SLOT_WEAPON || type >= SLOT_MAX || !player.action_unit->slots[type])
					{
						Error("Update server: GET_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
						break;
					}

					// get equipped item from unit
					const Item*& slot = player.action_unit->slots[type];
					unit.AddItem2(slot, 1u, 1u, false, false);
					if(player.action == PlayerAction::LootUnit && type == SLOT_WEAPON && slot == player.action_unit->used_item)
					{
						player.action_unit->used_item = nullptr;
						// removed item from hand, send info to other players
						if(active_players > 2)
						{
							NetChange& c = Add1(changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = player.action_unit;
						}
					}
					player.action_unit->RemoveItemEffects(slot, type);
					player.action_unit->weight -= slot->weight;
					slot = nullptr;

					// send info about changing equipment of looted unit
					if(active_players > 2 && IsVisible(type))
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = player.action_unit;
						c.id = type;
					}
				}
			}
			break;
		// player puts item into unit or container
		case NetChange::PUT_ITEM:
			{
				int i_index;
				uint count;
				f >> i_index;
				f >> count;
				if(!f)
				{
					Error("Update server: Broken PUT_ITEM from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				if(!player.IsTrading())
				{
					Error("Update server: PUT_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(i_index >= 0)
				{
					// put not equipped item
					if(i_index >= (int)unit.items.size())
					{
						Error("Update server: PUT_ITEM from %s, invalid index %d.", info.name.c_str(), i_index);
						break;
					}

					ItemSlot& slot = unit.items[i_index];
					if(count < 1 || count > slot.count)
					{
						Error("Update server: PUT_ITEM from %s, invalid count %u (have %u).", info.name.c_str(), count, slot.count);
						break;
					}

					uint team_count = min(count, slot.team_count);

					// add item
					if(player.action == PlayerAction::LootChest)
						player.action_chest->AddItem(slot.item, count, team_count, false);
					else if(player.action == PlayerAction::LootContainer)
						player.action_usable->container->AddItem(slot.item, count, team_count);
					else if(player.action == PlayerAction::Trade)
					{
						InsertItem(*player.chest_trade, slot.item, count, team_count);
						int price = ItemHelper::GetItemPrice(slot.item, unit, false);
						player.Train(TrainWhat::Trade, (float)price * count, 0);
						if(team_count)
							team->AddGold(price * team_count);
						uint normal_count = count - team_count;
						if(normal_count)
						{
							unit.gold += price * normal_count;
							info.UpdateGold();
						}
					}
					else
					{
						Unit* t = player.action_unit;
						uint add_as_team = team_count;
						if(player.action == PlayerAction::GiveItems)
						{
							add_as_team = 0;
							int price = slot.item->value / 2;
							if(slot.team_count > 0)
							{
								t->hero->credit += price;
								if(IsLocal())
									team->CheckCredit(true);
							}
							else if(t->gold >= price)
							{
								t->gold -= price;
								unit.gold += price;
							}
						}
						t->AddItem2(slot.item, count, add_as_team, false, false);
						if(player.action == PlayerAction::ShareItems || player.action == PlayerAction::GiveItems)
						{
							if(slot.item->type == IT_CONSUMABLE)
							{
								const Consumable& pot = slot.item->ToConsumable();
								if(pot.ai_type == ConsumableAiType::Healing)
									t->ai->have_potion = HavePotion::Yes;
								else if(pot.ai_type == ConsumableAiType::Mana)
									t->ai->have_mp_potion = HavePotion::Yes;
							}
							if(player.action == PlayerAction::GiveItems)
							{
								t->UpdateInventory();
								NetChangePlayer& c = Add1(info.changes);
								c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
								c.unit = t;
							}
						}
					}

					// remove item
					unit.weight -= slot.item->weight * count;
					slot.count -= count;
					if(slot.count == 0)
						unit.items.erase(unit.items.begin() + i_index);
					else
						slot.team_count -= team_count;
				}
				else
				{
					// put equipped item
					ITEM_SLOT type = IIndexToSlot(i_index);
					if(type < SLOT_WEAPON || type >= SLOT_MAX || !unit.slots[type])
					{
						Error("Update server: PUT_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
						break;
					}

					const Item*& slot = unit.slots[type];
					int price = ItemHelper::GetItemPrice(slot, unit, false);
					// add new item
					if(player.action == PlayerAction::LootChest)
						player.action_chest->AddItem(slot, 1u, 0u, false);
					else if(player.action == PlayerAction::LootContainer)
						player.action_usable->container->AddItem(slot, 1u, 0u);
					else if(player.action == PlayerAction::Trade)
					{
						InsertItem(*player.chest_trade, slot, 1u, 0u);
						unit.gold += price;
						player.Train(TrainWhat::Trade, (float)price, 0);
					}
					else
					{
						player.action_unit->AddItem2(slot, 1u, 0u, false, false);
						if(player.action == PlayerAction::GiveItems)
						{
							price = slot->value / 2;
							if(player.action_unit->gold >= price)
							{
								// sold for gold
								player.action_unit->gold -= price;
								unit.gold += price;
							}
							player.action_unit->UpdateInventory();
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
							c.unit = player.action_unit;
						}
					}
					// remove equipped
					unit.RemoveItemEffects(slot, type);
					unit.weight -= slot->weight;
					slot = nullptr;
					// send info about changing equipment
					if(active_players > 2 && IsVisible(type))
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::CHANGE_EQUIPMENT;
						c.unit = &unit;
						c.id = type;
					}
				}
			}
			break;
		// player picks up all items from corpse/chest
		case NetChange::GET_ALL_ITEMS:
			{
				if(game->game_state != GS_LEVEL)
					break;
				if(!player.IsTrading())
				{
					Error("Update server: GET_ALL_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				// slots
				bool any = false;
				if(player.action != PlayerAction::LootChest && player.action != PlayerAction::LootContainer)
				{
					const Item** slots = player.action_unit->slots;
					for(int i = 0; i < SLOT_MAX; ++i)
					{
						if(slots[i])
						{
							player.action_unit->RemoveItemEffects(slots[i], (ITEM_SLOT)i);
							InsertItemBare(unit.items, slots[i]);
							slots[i] = nullptr;
							any = true;

							// send info about changing equipment
							if(active_players > 2 && IsVisible((ITEM_SLOT)i))
							{
								NetChange& c = Add1(changes);
								c.type = NetChange::CHANGE_EQUIPMENT;
								c.unit = player.action_unit;
								c.id = i;
							}
						}
					}

					// reset weight
					player.action_unit->weight = 0;
				}

				// not equipped items
				for(ItemSlot& slot : *player.chest_trade)
				{
					if(!slot.item)
						continue;

					if(slot.item->type == IT_GOLD)
						unit.AddItem(Item::gold, slot.count, slot.team_count);
					else
					{
						InsertItemBare(unit.items, slot.item, slot.count, slot.team_count);
						any = true;
					}
				}
				player.chest_trade->clear();

				if(any)
					SortItems(unit.items);
			}
			break;
		// player ends trade
		case NetChange::STOP_TRADE:
			if(game->game_state != GS_LEVEL)
				break;
			if(!player.IsTrading())
			{
				Error("Update server: STOP_TRADE, player %s is not trading.", info.name.c_str());
				break;
			}

			if(player.action == PlayerAction::LootChest)
				player.action_chest->OpenClose(nullptr);
			else if(player.action == PlayerAction::LootContainer)
			{
				unit.UseUsable(nullptr);

				NetChange& c = Add1(changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = player.action_usable->id;
				c.count = USE_USABLE_END;
			}
			else if(player.action_unit)
			{
				player.action_unit->busy = Unit::Busy_No;
				player.action_unit->look_target = nullptr;
			}
			player.action = PlayerAction::None;
			break;
		// player takes item for credit
		case NetChange::TAKE_ITEM_CREDIT:
			{
				int index;
				f >> index;
				if(!f)
				{
					Error("Update server: Broken TAKE_ITEM_CREDIT from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				if(index < 0 || index >= (int)unit.items.size())
				{
					Error("Update server: TAKE_ITEM_CREDIT from %s, invalid index %d.", info.name.c_str(), index);
					break;
				}

				ItemSlot& slot = unit.items[index];
				if(unit.CanWear(slot.item) && slot.team_count != 0)
				{
					slot.team_count = 0;
					player.credit += slot.item->value / 2;
					team->CheckCredit(true);
				}
				else
				{
					Error("Update server: TAKE_ITEM_CREDIT from %s, item %s (count %u, team count %u).", info.name.c_str(), slot.item->id.c_str(),
						slot.count, slot.team_count);
				}
			}
			break;
		// unit plays idle animation
		case NetChange::IDLE:
			{
				byte index;
				f >> index;
				if(!f)
					Error("Update server: Broken IDLE from %s.", info.name.c_str());
				else if(index >= unit.data->idles->anims.size())
					Error("Update server: IDLE from %s, invalid animation index %u.", info.name.c_str(), index);
				else if(game->game_state == GS_LEVEL)
				{
					unit.mesh_inst->Play(unit.data->idles->anims[index].c_str(), PLAY_ONCE, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;
					unit.animation = ANI_IDLE;
					// send info to other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::IDLE;
						c.unit = &unit;
						c.id = index;
					}
				}
			}
			break;
		// player start dialog
		case NetChange::TALK:
			{
				int id;
				f >> id;
				if(!f)
				{
					Error("Update server: Broken TALK from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Unit* talk_to = game_level->FindUnit(id);
				if(!talk_to)
				{
					Error("Update server: TALK from %s, missing unit %d.", info.name.c_str(), id);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::START_DIALOG;
				if(talk_to->busy != Unit::Busy_No || !talk_to->CanTalk(unit))
				{
					// can't talk to unit
					c.id = -1;
				}
				else if(talk_to->area != unit.area)
				{
					// unit left/entered building
					c.id = -2;
				}
				else
				{
					// start dialog
					c.id = talk_to->id;
					player.dialog_ctx->StartDialog(talk_to);
				}
			}
			break;
		// player selected dialog choice
		case NetChange::CHOICE:
			{
				byte choice;
				f >> choice;
				if(!f)
					Error("Update server: Broken CHOICE from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(player.dialog_ctx->show_choices && choice < player.dialog_ctx->choices.size())
						player.dialog_ctx->choice_selected = choice;
					else
						Error("Update server: CHOICE from %s, not in dialog or invalid choice %u.", info.name.c_str(), choice);
				}
			}
			break;
		// pomijanie dialogu przez gracza
		case NetChange::SKIP_DIALOG:
			{
				int skip_id;
				f >> skip_id;
				if(!f)
					Error("Update server: Broken SKIP_DIALOG from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					DialogContext& ctx = *player.dialog_ctx;
					if(ctx.dialog_wait > 0.f && ctx.dialog_mode && !ctx.show_choices && ctx.skip_id == skip_id && ctx.can_skip)
						ctx.choice_selected = 1;
				}
			}
			break;
		// player wants to enter building
		case NetChange::ENTER_BUILDING:
			{
				byte building_index;
				f >> building_index;
				if(!f)
					Error("Update server: Broken ENTER_BUILDING from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(game_level->city_ctx && building_index < game_level->city_ctx->inside_buildings.size())
					{
						WarpData& warp = Add1(warps);
						warp.u = &unit;
						warp.where = building_index;
						warp.timer = 1.f;
						unit.frozen = FROZEN::YES;
					}
					else
						Error("Update server: ENTER_BUILDING from %s, invalid building index %u.", info.name.c_str(), building_index);
				}
			}
			break;
		// player wants to exit building
		case NetChange::EXIT_BUILDING:
			if(game->game_state == GS_LEVEL)
			{
				if(unit.area->area_type == LevelArea::Type::Building)
				{
					WarpData& warp = Add1(warps);
					warp.u = &unit;
					warp.where = LevelArea::OUTSIDE_ID;
					warp.timer = 1.f;
					unit.frozen = FROZEN::YES;
				}
				else
					Error("Update server: EXIT_BUILDING from %s, unit not in building.", info.name.c_str());
			}
			break;
		// notify about warping
		case NetChange::WARP:
			if(game->game_state == GS_LEVEL)
				info.warping = false;
			break;
		// player added note to journal
		case NetChange::ADD_NOTE:
			{
				string& str = Add1(info.notes);
				f.ReadString1(str);
				if(!f)
				{
					info.notes.pop_back();
					Error("Update server: Broken ADD_NOTE from %s.", info.name.c_str());
				}
			}
			break;
		// get random number for player
		case NetChange::RANDOM_NUMBER:
			{
				byte number;
				f >> number;
				if(!f)
					Error("Update server: Broken RANDOM_NUMBER from %s.", info.name.c_str());
				else
				{
					game_gui->AddMsg(Format(game->txRolledNumber, info.name.c_str(), number));
					// send to other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::RANDOM_NUMBER;
						c.unit = info.u;
						c.id = number;
					}
				}
			}
			break;
		// player wants to start/stop using usable
		case NetChange::USE_USABLE:
			{
				int usable_id;
				USE_USABLE_STATE state;
				f >> usable_id;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					Error("Update server: Broken USE_USABLE from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Usable* usable = game_level->FindUsable(usable_id);
				if(!usable)
				{
					Error("Update server: USE_USABLE from %s, missing usable %d.", info.name.c_str(), usable_id);
					break;
				}

				if(state == USE_USABLE_START)
				{
					// use usable
					if(usable->user)
					{
						// someone else is already using this
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::USE_USABLE;
						break;
					}
					else
					{
						BaseUsable& base = *usable->base;
						if(!IsSet(base.use_flags, BaseUsable::CONTAINER))
						{
							unit.action = A_ANIMATION2;
							unit.animation = ANI_PLAY;
							unit.mesh_inst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
							unit.mesh_inst->groups[0].speed = 1.f;
							unit.target_pos = unit.pos;
							unit.target_pos2 = usable->pos;
							if(usable->base->limit_rot == 4)
								unit.target_pos2 -= Vec3(sin(usable->rot)*1.5f, 0, cos(usable->rot)*1.5f);
							unit.timer = 0.f;
							unit.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
							unit.use_rot = Vec3::LookAtAngle(unit.pos, usable->pos);
							unit.used_item = base.item;
							if(unit.used_item)
							{
								unit.weapon_taken = W_NONE;
								unit.weapon_state = WS_HIDDEN;
							}
						}
						else
						{
							// start looting container
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::LOOT;
							c.id = 1;

							player.action = PlayerAction::LootContainer;
							player.action_usable = usable;
							player.chest_trade = &usable->container->items;
						}

						unit.UseUsable(usable);
					}
				}
				else
				{
					// stop using usable
					if(usable->user == unit)
					{
						if(state == USE_USABLE_STOP)
						{
							BaseUsable& base = *usable->base;
							if(!IsSet(base.use_flags, BaseUsable::CONTAINER))
							{
								unit.action = A_NONE;
								unit.animation = ANI_STAND;
								if(unit.live_state == Unit::ALIVE)
									unit.used_item = nullptr;
							}
						}
						else if(state == USE_USABLE_END)
							unit.UseUsable(nullptr);
					}
				}

				// send info to players
				NetChange& c = Add1(changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = usable_id;
				c.count = state;
			}
			break;
		// player used cheat 'suicide'
		case NetChange::CHEAT_SUICIDE:
			if(info.devmode)
			{
				if(game->game_state == GS_LEVEL)
					game->GiveDmg(unit, unit.hpmax);
			}
			else
				Error("Update server: Player %s used CHEAT_SUICIDE without devmode.", info.name.c_str());
			break;
		// player used cheat 'godmode'
		case NetChange::CHEAT_GODMODE:
			{
				bool state;
				f >> state;
				if(!f)
					Error("Update server: Broken CHEAT_GODMODE from %s.", info.name.c_str());
				else if(info.devmode)
					player.godmode = state;
				else
					Error("Update server: Player %s used CHEAT_GODMODE without devmode.", info.name.c_str());
			}
			break;
		// player stands up
		case NetChange::STAND_UP:
			if(game->game_state != GS_LEVEL)
				break;
			unit.Standup();
			// send to other players
			if(active_players > 2)
			{
				NetChange& c = Add1(changes);
				c.type = NetChange::STAND_UP;
				c.unit = &unit;
			}
			break;
		// player used cheat 'noclip'
		case NetChange::CHEAT_NOCLIP:
			{
				bool state;
				f >> state;
				if(!f)
					Error("Update server: Broken CHEAT_NOCLIP from %s.", info.name.c_str());
				else if(info.devmode)
					player.noclip = state;
				else
					Error("Update server: Player %s used CHEAT_NOCLIP without devmode.", info.name.c_str());
			}
			break;
		// player used cheat 'invisible'
		case NetChange::CHEAT_INVISIBLE:
			{
				bool state;
				f >> state;
				if(!f)
					Error("Update server: Broken CHEAT_INVISIBLE from %s.", info.name.c_str());
				else if(info.devmode)
					player.invisible = state;
				else
					Error("Update server: Player %s used CHEAT_INVISIBLE without devmode.", info.name.c_str());
			}
			break;
		// player used cheat 'scare'
		case NetChange::CHEAT_SCARE:
			if(info.devmode)
			{
				if(game->game_state != GS_LEVEL)
					break;
				for(AIController* ai : game->ais)
				{
					if(ai->unit->IsEnemy(unit) && Vec3::Distance(ai->unit->pos, unit.pos) < ALERT_RANGE && game_level->CanSee(*ai->unit, unit))
					{
						ai->morale = -10;
						ai->target_last_pos = unit.pos;
					}
				}
			}
			else
				Error("Update server: Player %s used CHEAT_SCARE without devmode.", info.name.c_str());
			break;
		// player used cheat 'killall'
		case NetChange::CHEAT_KILLALL:
			{
				int ignored_id;
				byte mode;
				f >> ignored_id;
				f >> mode;
				if(!f)
				{
					Error("Update server: Broken CHEAT_KILLALL from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				if(!info.devmode)
				{
					Error("Update server: Player %s used CHEAT_KILLALL without devmode.", info.name.c_str());
					break;
				}

				Unit* ignored;
				if(ignored_id == -1)
					ignored = nullptr;
				else
				{
					ignored = game_level->FindUnit(ignored_id);
					if(!ignored)
					{
						Error("Update server: CHEAT_KILLALL from %s, missing unit %d.", info.name.c_str(), ignored_id);
						break;
					}
				}

				if(!game_level->KillAll(mode, unit, ignored))
					Error("Update server: CHEAT_KILLALL from %s, invalid mode %u.", info.name.c_str(), mode);
			}
			break;
		// client checks if item is better for npc
		case NetChange::IS_BETTER_ITEM:
			{
				int i_index;
				f >> i_index;
				if(!f)
				{
					Error("Update server: Broken IS_BETTER_ITEM from %s.", info.name.c_str());
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::IS_BETTER_ITEM;
				c.id = 0;

				if(game->game_state != GS_LEVEL)
					break;

				if(player.action == PlayerAction::GiveItems)
				{
					const Item* item = unit.GetIIndexItem(i_index);
					if(item)
					{
						if(player.action_unit->IsBetterItem(item))
							c.id = 1;
					}
					else
						Error("Update server: IS_BETTER_ITEM from %s, invalid i_index %d.", info.name.c_str(), i_index);
				}
				else
					Error("Update server: IS_BETTER_ITEM from %s, player is not giving items.", info.name.c_str());
			}
			break;
		// player used cheat 'citizen'
		case NetChange::CHEAT_CITIZEN:
			if(info.devmode)
			{
				if(team->is_bandit || team->crazies_attack)
				{
					team->is_bandit = false;
					team->crazies_attack = false;
					PushChange(NetChange::CHANGE_FLAGS);
				}
			}
			else
				Error("Update server: Player %s used CHEAT_CITIZEN without devmode.", info.name.c_str());
			break;
		// player used cheat 'heal'
		case NetChange::CHEAT_HEAL:
			if(info.devmode)
			{
				if(game->game_state == GS_LEVEL)
					cmdp->HealUnit(unit);
			}
			else
				Error("Update server: Player %s used CHEAT_HEAL without devmode.", info.name.c_str());
			break;
		// player used cheat 'kill'
		case NetChange::CHEAT_KILL:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHEAT_KILL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_KILL without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(!target)
						Error("Update server: CHEAT_KILL from %s, missing unit %d.", info.name.c_str(), id);
					else if(target->IsAlive())
						game->GiveDmg(*target, target->hpmax);
				}
			}
			break;
		// player used cheat 'heal_unit'
		case NetChange::CHEAT_HEAL_UNIT:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHEAT_HEAL_UNIT from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_HEAL_UNIT without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(!target)
						Error("Update server: CHEAT_HEAL_UNIT from %s, missing unit %d.", info.name.c_str(), id);
					else
						cmdp->HealUnit(*target);
				}
			}
			break;
		// player used cheat 'reveal'
		case NetChange::CHEAT_REVEAL:
			if(info.devmode)
				world->Reveal();
			else
				Error("Update server: Player %s used CHEAT_REVEAL without devmode.", info.name.c_str());
			break;
		// player used cheat 'goto_map'
		case NetChange::CHEAT_GOTO_MAP:
			if(info.devmode)
			{
				if(game->game_state != GS_LEVEL)
					break;
				game->ExitToMap();
				return false;
			}
			else
				Error("Update server: Player %s used CHEAT_GOTO_MAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'reveal_minimap'
		case NetChange::CHEAT_REVEAL_MINIMAP:
			if(info.devmode)
			{
				if(game->game_state == GS_LEVEL)
					game_level->RevealMinimap();
			}
			else
				Error("Update server: Player %s used CHEAT_REVEAL_MINIMAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'add_gold' or 'add_team_gold'
		case NetChange::CHEAT_ADD_GOLD:
			{
				bool is_team;
				int count;
				f >> is_team;
				f >> count;
				if(!f)
					Error("Update server: Broken CHEAT_ADD_GOLD from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_ADD_GOLD without devmode.", info.name.c_str());
				else
				{
					if(is_team)
					{
						if(count <= 0)
							Error("Update server: CHEAT_ADD_GOLD from %s, invalid count %d.", info.name.c_str(), count);
						else
							team->AddGold(count);
					}
					else
					{
						unit.gold = max(unit.gold + count, 0);
						info.UpdateGold();
					}
				}
			}
			break;
		// player used cheat 'add_item' or 'add_team_item'
		case NetChange::CHEAT_ADD_ITEM:
			{
				int count;
				bool is_team;
				const string& item_id = f.ReadString1();
				f >> count;
				f >> is_team;
				if(!f)
					Error("Update server: Broken CHEAT_ADD_ITEM from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_ADD_ITEM without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					const Item* item = Item::TryGet(item_id);
					if(item && count)
						info.u->AddItem2(item, count, is_team ? count : 0u, false);
					else
						Error("Update server: CHEAT_ADD_ITEM from %s, missing item %s or invalid count %u.", info.name.c_str(), item_id.c_str(), count);
				}
			}
			break;
		// player used cheat 'skip_days'
		case NetChange::CHEAT_SKIP_DAYS:
			{
				int count;
				f >> count;
				if(!f)
					Error("Update server: Broken CHEAT_SKIP_DAYS from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_SKIP_DAYS without devmode.", info.name.c_str());
				else
					world->Update(count, World::UM_SKIP);
			}
			break;
		// player used cheat 'warp'
		case NetChange::CHEAT_WARP:
			{
				byte building_index;
				f >> building_index;
				if(!f)
					Error("Update server: Broken CHEAT_WARP from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_WARP without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(unit.frozen != FROZEN::NO)
						Error("Update server: CHEAT_WARP from %s, unit is frozen.", info.name.c_str());
					else if(!game_level->city_ctx || building_index >= game_level->city_ctx->inside_buildings.size())
						Error("Update server: CHEAT_WARP from %s, invalid inside building index %u.", info.name.c_str(), building_index);
					else
					{
						WarpData& warp = Add1(warps);
						warp.u = &unit;
						warp.where = building_index;
						warp.timer = 1.f;
						unit.frozen = (unit.usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
						NetChangePlayer& c = Add1(info.u->player->player_info->changes);
						c.type = NetChangePlayer::PREPARE_WARP;
					}
				}
			}
			break;
		// player used cheat 'spawn_unit'
		case NetChange::CHEAT_SPAWN_UNIT:
			{
				byte count;
				char level, in_arena;
				const string& unit_id = f.ReadString1();
				f >> count;
				f >> level;
				f >> in_arena;
				if(!f)
					Error("Update server: Broken CHEAT_SPAWN_UNIT from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_SPAWN_UNIT without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					UnitData* data = UnitData::TryGet(unit_id);
					if(!data)
						Error("Update server: CHEAT_SPAWN_UNIT from %s, invalid unit id %s.", info.name.c_str(), unit_id.c_str());
					else
					{
						if(in_arena < -1 || in_arena > 1)
							in_arena = -1;

						LevelArea& area = *info.u->area;
						Vec3 pos = info.u->GetFrontPos();

						for(byte i = 0; i < count; ++i)
						{
							Unit* spawned = game_level->SpawnUnitNearLocation(area, pos, *data, &unit.pos, level);
							if(!spawned)
							{
								Warn("Update server: CHEAT_SPAWN_UNIT from %s, no free space for unit.", info.name.c_str());
								break;
							}
							else if(in_arena != -1)
							{
								spawned->in_arena = in_arena;
								game->arena->units.push_back(spawned);
							}
						}
					}
				}
			}
			break;
		// player used cheat 'set_stat' or 'mod_stat'
		case NetChange::CHEAT_SET_STAT:
		case NetChange::CHEAT_MOD_STAT:
			{
				cstring name = (type == NetChange::CHEAT_SET_STAT ? "CHEAT_SET_STAT" : "CHEAT_MOD_STAT");

				byte what;
				bool is_skill;
				char value;
				f >> what;
				f >> is_skill;
				f >> value;
				if(!f)
					Error("Update server: Broken %s from %s.", name, info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used %s without devmode.", info.name.c_str(), name);
				else if(game->game_state == GS_LEVEL)
				{
					if(is_skill)
					{
						if(what < (int)SkillId::MAX)
						{
							int num = +value;
							if(type == NetChange::CHEAT_MOD_STAT)
								num += info.u->stats->skill[what];
							int v = Clamp(num, Skill::MIN, Skill::MAX);
							if(v != info.u->stats->skill[what])
							{
								info.u->Set((SkillId)what, v);
								NetChangePlayer& c = Add1(player.player_info->changes);
								c.type = NetChangePlayer::STAT_CHANGED;
								c.id = (int)ChangedStatType::SKILL;
								c.a = what;
								c.count = v;
							}
						}
						else
							Error("Update server: %s from %s, invalid skill %d.", name, info.name.c_str(), what);
					}
					else
					{
						if(what < (int)AttributeId::MAX)
						{
							int num = +value;
							if(type == NetChange::CHEAT_MOD_STAT)
								num += info.u->stats->attrib[what];
							int v = Clamp(num, Attribute::MIN, Attribute::MAX);
							if(v != info.u->stats->attrib[what])
							{
								info.u->Set((AttributeId)what, v);
								NetChangePlayer& c = Add1(player.player_info->changes);
								c.type = NetChangePlayer::STAT_CHANGED;
								c.id = (int)ChangedStatType::ATTRIBUTE;
								c.a = what;
								c.count = v;
							}
						}
						else
							Error("Update server: %s from %s, invalid attribute %d.", name, info.name.c_str(), what);
					}
				}
			}
			break;
		// response to pvp request
		case NetChange::PVP:
			{
				bool accepted;
				f >> accepted;
				if(!f)
					Error("Update server: Broken PVP from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
					game->arena->HandlePvpResponse(info, accepted);
			}
			break;
		// leader wants to leave location
		case NetChange::LEAVE_LOCATION:
			{
				char type;
				f >> type;
				if(!f)
					Error("Update server: Broken LEAVE_LOCATION from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: LEAVE_LOCATION from %s, player is not leader.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					CanLeaveLocationResult result = game_level->CanLeaveLocation(*info.u);
					if(result == CanLeaveLocationResult::Yes)
					{
						PushChange(NetChange::LEAVE_LOCATION);
						if(type == ENTER_FROM_OUTSIDE)
							game->fallback_type = FALLBACK::EXIT;
						else if(type == ENTER_FROM_UP_LEVEL)
						{
							game->fallback_type = FALLBACK::CHANGE_LEVEL;
							game->fallback_1 = -1;
						}
						else if(type == ENTER_FROM_DOWN_LEVEL)
						{
							game->fallback_type = FALLBACK::CHANGE_LEVEL;
							game->fallback_1 = +1;
						}
						else
						{
							if(game_level->location->TryGetPortal(type))
							{
								game->fallback_type = FALLBACK::USE_PORTAL;
								game->fallback_1 = type;
							}
							else
							{
								Error("Update server: LEAVE_LOCATION from %s, invalid type %d.", type);
								break;
							}
						}

						game->fallback_t = -1.f;
						for(Unit& team_member : team->members)
							team_member.frozen = FROZEN::YES;
					}
					else
					{
						// can't leave
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::CANT_LEAVE_LOCATION;
						c.id = (int)result;
					}
				}
			}
			break;
		// player open/close door
		case NetChange::USE_DOOR:
			{
				int id;
				bool is_closing;
				f >> id;
				f >> is_closing;
				if(!f)
				{
					Error("Update server: Broken USE_DOOR from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Door* door = game_level->FindDoor(id);
				if(!door)
				{
					Error("Update server: USE_DOOR from %s, missing door %d.", info.name.c_str(), id);
					break;
				}

				bool ok = true;
				if(is_closing)
				{
					// closing door
					if(door->state == Door::Open)
					{
						door->state = Door::Closing;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Opening)
					{
						door->state = Door::Closing2;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Opening2)
					{
						door->state = Door::Closing;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else
						ok = false;
				}
				else
				{
					// opening door
					if(door->state == Door::Closed)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Closing)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening2;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else if(door->state == Door::Closing2)
					{
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
						door->mesh_inst->frame_end_info = false;
					}
					else
						ok = false;
				}

				if(ok && Rand() % 2 == 0)
				{
					Sound* sound;
					if(is_closing && Rand() % 2 == 0)
						sound = game->sDoorClose;
					else
						sound = game->sDoor[Rand() % 3];
					sound_mgr->PlaySound3d(sound, door->GetCenter(), Door::SOUND_DIST);
				}

				// send to other players
				NetChange& c = Add1(changes);
				c.type = NetChange::USE_DOOR;
				c.id = id;
				c.count = (is_closing ? 1 : 0);
			}
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					Error("Update server: Broken TRAVEL from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(game->game_state == GS_WORLDMAP)
				{
					world->Travel(loc, true);
					game_gui->world_map->StartTravel();
				}
			}
			break;
		// leader wants to travel to pos
		case NetChange::TRAVEL_POS:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					Error("Update server: Broken TRAVEL_POS from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: TRAVEL_POS from %s, player is not leader.", info.name.c_str());
				else if(game->game_state == GS_WORLDMAP)
				{
					world->TravelPos(pos, true);
					game_gui->world_map->StartTravel();
				}
			}
			break;
		// leader stopped travel
		case NetChange::STOP_TRAVEL:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					Error("Update server: Broken STOP_TRAVEL from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: STOP_TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(game->game_state == GS_WORLDMAP)
					world->StopTravel(pos, true);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game->game_state == GS_WORLDMAP)
				world->EndTravel();
			break;
		// enter current location
		case NetChange::ENTER_LOCATION:
			if(game->game_state == GS_WORLDMAP && world->GetState() == World::State::ON_MAP && team->IsLeader(info.u))
			{
				game->EnterLocation();
				return false;
			}
			else
				Error("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str());
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(game->game_state == GS_WORLDMAP)
			{
				if(game_gui->world_map->dialog_enc)
				{
					gui->CloseDialog(game_gui->world_map->dialog_enc);
					game_gui->world_map->dialog_enc = nullptr;
				}
				PushChange(NetChange::CLOSE_ENCOUNTER);
				game->Event_RandomEncounter(0);
				return false;
			}
			break;
		// player used cheat to change level (<>+shift+ctrl)
		case NetChange::CHEAT_CHANGE_LEVEL:
			{
				bool is_down;
				f >> is_down;
				if(!f)
					Error("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str());
				else if(game_level->location->outside)
					Error("Update server:CHEAT_CHANGE_LEVEL from %s, outside location.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					game->ChangeLevel(is_down ? +1 : -1);
					return false;
				}
			}
			break;
		// player used cheat to warp to stairs (<>+shift)
		case NetChange::CHEAT_WARP_TO_STAIRS:
			{
				bool is_down;
				f >> is_down;
				if(!f)
					Error("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					InsideLocation* inside = (InsideLocation*)game_level->location;
					InsideLocationLevel& lvl = inside->GetLevelData();

					if(!is_down)
					{
						Int2 tile = lvl.GetUpStairsFrontTile();
						unit.rot = DirToRot(lvl.staircase_up_dir);
						game_level->WarpUnit(unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
					else
					{
						Int2 tile = lvl.GetDownStairsFrontTile();
						unit.rot = DirToRot(lvl.staircase_down_dir);
						game_level->WarpUnit(unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
				}
			}
			break;
		// player used cheat 'noai'
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				f >> state;
				if(!f)
					Error("Update server: Broken CHEAT_NOAI from %s.", info.name.c_str());
				else if(info.devmode)
				{
					if(game->noai != state)
					{
						game->noai = state;
						NetChange& c = Add1(changes);
						c.type = NetChange::CHEAT_NOAI;
						c.id = (state ? 1 : 0);
					}
				}
				else
					Error("Update server: Player %s used CHEAT_NOAI without devmode.", info.name.c_str());
			}
			break;
		// player rest in inn
		case NetChange::REST:
			{
				byte days;
				f >> days;
				if(!f)
					Error("Update server: Broken REST from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					player.Rest(days, true);
					player.UseDays(days);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::END_FALLBACK;
				}
			}
			break;
		// player trains
		case NetChange::TRAIN:
			{
				byte type, stat_type;
				f >> type;
				f >> stat_type;
				if(!f)
					Error("Update server: Broken TRAIN from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(type == 2)
						quest_mgr->quest_tournament->Train(player);
					else
					{
						cstring error = nullptr;
						if(type == 0)
						{
							if(stat_type >= (byte)AttributeId::MAX)
								error = "attribute";
						}
						else if(type == 1)
						{
							if(stat_type >= (byte)SkillId::MAX)
								error = "skill";
						}
						else
						{
							if(stat_type >= (byte)Perk::Max)
								error = "perk";
						}

						if(error)
						{
							Error("Update server: TRAIN from %s, invalid %d %u.", info.name.c_str(), error, stat_type);
							break;
						}
						if(type == 3)
						{
							player.AddPerk((Perk)stat_type, -1);
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::GAME_MESSAGE;
							c.id = GMS_LEARNED_PERK;
						}
						else
							player.Train(type == 1, stat_type);
					}
					player.Rest(10, false);
					player.UseDays(10);
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::END_FALLBACK;
				}
			}
			break;
		// player wants to change leader
		case NetChange::CHANGE_LEADER:
			{
				byte id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHANGE_LEADER from %s.", info.name.c_str());
				else if(team->leader_id != info.id)
					Error("Update server: CHANGE_LEADER from %s, player is not leader.", info.name.c_str());
				else
				{
					PlayerInfo* new_leader = TryGetPlayer(id);
					if(!new_leader)
					{
						Error("Update server: CHANGE_LEADER from %s, invalid player id %u.", id);
						break;
					}

					team->leader_id = id;
					team->leader = new_leader->u;

					if(team->leader_id == team->my_id)
						game_gui->AddMsg(game->txYouAreLeader);
					else
						game_gui->AddMsg(Format(game->txPcIsLeader, team->leader->player->name.c_str()));

					NetChange& c = Add1(changes);
					c.type = NetChange::CHANGE_LEADER;
					c.id = id;
				}
			}
			break;
		// player pays credit
		case NetChange::PAY_CREDIT:
			{
				int count;
				f >> count;
				if(!f)
					Error("Update server: Broken PAY_CREDIT from %s.", info.name.c_str());
				else if(unit.gold < count || player.credit < count || count < 0)
				{
					Error("Update server: PAY_CREDIT from %s, invalid count %d (gold %d, credit %d).",
						info.name.c_str(), count, unit.gold, player.credit);
				}
				else
				{
					unit.gold -= count;
					player.PayCredit(count);
				}
			}
			break;
		// player give gold to unit
		case NetChange::GIVE_GOLD:
			{
				int id, count;
				f >> id;
				f >> count;
				if(!f)
				{
					Error("Update server: Broken GIVE_GOLD from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				Unit* target = team->FindActiveTeamMember(id);
				if(!target)
					Error("Update server: GIVE_GOLD from %s, missing unit %d.", info.name.c_str(), id);
				else if(target == &unit)
					Error("Update server: GIVE_GOLD from %s, wants to give gold to himself.", info.name.c_str());
				else if(count > unit.gold || count < 0)
					Error("Update server: GIVE_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold);
				else
				{
					// give gold
					target->gold += count;
					unit.gold -= count;
					if(target->IsPlayer())
					{
						// message about getting gold
						if(target->player != game->pc)
						{
							NetChangePlayer& c = Add1(target->player->player_info->changes);
							c.type = NetChangePlayer::GOLD_RECEIVED;
							c.id = info.id;
							c.count = count;
							target->player->player_info->UpdateGold();
						}
						else
						{
							game_gui->mp_box->Add(Format(game->txReceivedGold, count, info.name.c_str()));
							sound_mgr->PlaySound2d(game->sCoins);
						}
					}
					else if(player.IsTradingWith(target))
					{
						// message about changing trader gold
						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
						c.id = target->id;
						c.count = target->gold;
						info.UpdateGold();
					}
				}
			}
			break;
		// player drops gold on group
		case NetChange::DROP_GOLD:
			{
				int count;
				f >> count;
				if(!f)
					Error("Update server: Broken DROP_GOLD from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(count > 0 && count <= unit.gold && unit.IsStanding() && unit.action == A_NONE)
					{
						unit.gold -= count;

						// animation
						unit.action = A_ANIMATION;
						unit.mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
						unit.mesh_inst->groups[0].speed = 1.f;
						unit.mesh_inst->frame_end_info = false;

						// create item
						GroundItem* item = new GroundItem;
						item->Register();
						item->item = Item::gold;
						item->count = count;
						item->team_count = 0;
						item->pos = unit.pos;
						item->pos.x -= sin(unit.rot)*0.25f;
						item->pos.z -= cos(unit.rot)*0.25f;
						item->rot = Random(MAX_ANGLE);
						game_level->AddGroundItem(*info.u->area, item);

						// send info to other players
						if(active_players > 2)
						{
							NetChange& c = Add1(changes);
							c.type = NetChange::DROP_ITEM;
							c.unit = &unit;
						}
					}
					else
						Error("Update server: DROP_GOLD from %s, invalid count %d or busy.", info.name.c_str());
				}
			}
			break;
		// player puts gold into container
		case NetChange::PUT_GOLD:
			{
				int count;
				f >> count;
				if(!f)
					Error("Update server: Broken PUT_GOLD from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					if(count < 0 || count > unit.gold)
						Error("Update server: PUT_GOLD from %s, invalid count %d (have %d).", info.name.c_str(), count, unit.gold);
					else if(player.action != PlayerAction::LootChest && player.action != PlayerAction::LootUnit
						&& player.action != PlayerAction::LootContainer)
					{
						Error("Update server: PUT_GOLD from %s, player is not trading.", info.name.c_str());
					}
					else
					{
						InsertItem(*player.chest_trade, Item::gold, count, 0);
						unit.gold -= count;
					}
				}
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte location_index;
				f >> location_index;
				if(!f)
					Error("Update server: Broken CHEAT_TRAVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_TRAVEL without devmode.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: CHEAT_TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(!world->VerifyLocation(location_index))
					Error("Update server: CHEAT_TRAVEL from %s, invalid location index %u.", info.name.c_str(), location_index);
				else if(game->game_state == GS_WORLDMAP)
				{
					world->Warp(location_index);
					game_gui->world_map->StartTravel();

					// inform other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::CHEAT_TRAVEL;
						c.id = location_index;
					}
				}
			}
			break;
		//  player used cheat for fast travel to pos on map
		case NetChange::CHEAT_TRAVEL_POS:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					Error("Update server: Broken CHEAT_TRAVEL_POS from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_TRAVEL_POS without devmode.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: CHEAT_TRAVEL_POS from %s, player is not leader.", info.name.c_str());
				else if(game->game_state == GS_WORLDMAP)
				{
					world->WarpPos(pos);
					game_gui->world_map->StartTravel();

					// inform other players
					if(active_players > 2)
					{
						NetChange& c = Add1(changes);
						c.type = NetChange::CHEAT_TRAVEL_POS;
						c.pos.x = pos.x;
						c.pos.y = pos.y;
					}
				}
			}
			break;
		// player used cheat 'hurt'
		case NetChange::CHEAT_HURT:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHEAT_HURT from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_HURT without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(target)
						game->GiveDmg(*target, 100.f);
					else
						Error("Update server: CHEAT_HURT from %s, missing unit %d.", info.name.c_str(), id);
				}
			}
			break;
		// player used cheat 'break_action'
		case NetChange::CHEAT_BREAK_ACTION:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHEAT_BREAK_ACTION from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_BREAK_ACTION without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(target)
						target->BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
					else
						Error("Update server: CHEAT_BREAK_ACTION from %s, missing unit %d.", info.name.c_str(), id);
				}
			}
			break;
		// player used cheat 'fall'
		case NetChange::CHEAT_FALL:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update server: Broken CHEAT_FALL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_FALL without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(target)
						target->Fall();
					else
						Error("Update server: CHEAT_FALL from %s, missing unit %d.", info.name.c_str(), id);
				}
			}
			break;
		// player yell to move ai
		case NetChange::YELL:
			if(game->game_state == GS_LEVEL)
				player.Yell();
			break;
		// player used cheat 'stun'
		case NetChange::CHEAT_STUN:
			{
				int id;
				float length;
				f >> id;
				f >> length;
				if(!f)
					Error("Update server: Broken CHEAT_STUN from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_STUN without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(target && length > 0)
						target->ApplyStun(length);
					else
						Error("Update server: CHEAT_STUN from %s, missing unit %d.", info.name.c_str(), id);
				}
			}
			break;
		// player used action
		case NetChange::PLAYER_ACTION:
			{
				int netid;
				Vec3 pos;
				f >> netid;
				f >> pos;
				if(!f)
					Error("Update server: Broken PLAYER_ACTION from %s.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(netid);
					if(!target && netid != -1)
						Error("Update server: PLAYER_ACTION, invalid target %d from %s.", netid, info.name.c_str());
					else
						info.pc->UseAction(false, &pos, target);
				}
			}
			break;
		// player used cheat 'refresh_cooldown'
		case NetChange::CHEAT_REFRESH_COOLDOWN:
			if(!info.devmode)
				Error("Update server: Player %s used CHEAT_REFRESH_COOLDOWN without devmode.", info.name.c_str());
			else if(game->game_state == GS_LEVEL)
				player.RefreshCooldown();
			break;
		// client fallback ended
		case NetChange::END_FALLBACK:
			info.u->frozen = FROZEN::NO;
			break;
		// run script
		case NetChange::RUN_SCRIPT:
			{
				LocalString code;
				int target_id;
				f.ReadString4(*code);
				f >> target_id;
				if(!f)
				{
					Error("Update server: Broken RUN_SCRIPT from %s.", info.name.c_str());
					break;
				}

				if(game->game_state != GS_LEVEL)
					break;

				if(!info.devmode)
				{
					Error("Update server: Player %s used RUN_SCRIPT without devmode.", info.name.c_str());
					break;
				}

				Unit* target = game_level->FindUnit(target_id);
				if(!target && target_id != -1)
				{
					Error("Update server: RUN_SCRIPT, invalid target %d from %s.", target_id, info.name.c_str());
					break;
				}

				string& output = script_mgr->OpenOutput();
				ScriptContext& ctx = script_mgr->GetContext();
				ctx.pc = info.pc;
				ctx.target = target;
				script_mgr->RunScript(code->c_str());

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::RUN_SCRIPT_RESULT;
				if(output.empty())
					c.str = nullptr;
				else
				{
					c.str = code.Pin();
					*c.str = output;
				}

				ctx.pc = nullptr;
				ctx.target = nullptr;
				script_mgr->CloseOutput();
			}
			break;
		// player set next action
		case NetChange::SET_NEXT_ACTION:
			{
				f.ReadCasted<byte>(player.next_action);
				if(!f)
				{
					Error("Update server: Broken SET_NEXT_ACTION from '%s'.", info.name.c_str());
					player.next_action = NA_NONE;
					break;
				}
				switch(player.next_action)
				{
				case NA_NONE:
					break;
				case NA_REMOVE:
				case NA_DROP:
					f.ReadCasted<byte>(player.next_action_data.slot);
					if(!f)
					{
						Error("Update server: Broken SET_NEXT_ACTION(2) from '%s'.", info.name.c_str());
						player.next_action = NA_NONE;
					}
					else if(game->game_state != GS_LEVEL)
						player.next_action = NA_NONE;
					else if(!IsValid(player.next_action_data.slot) || !unit.slots[player.next_action_data.slot])
					{
						Error("Update server: SET_NEXT_ACTION, invalid slot %d from '%s'.", player.next_action_data.slot, info.name.c_str());
						player.next_action = NA_NONE;
					}
					break;
				case NA_EQUIP:
				case NA_CONSUME:
				case NA_EQUIP_DRAW:
					{
						f >> player.next_action_data.index;
						const string& item_id = f.ReadString1();
						if(!f)
						{
							Error("Update server: Broken SET_NEXT_ACTION(3) from '%s'.", info.name.c_str());
							player.next_action = NA_NONE;
						}
						else if(game->game_state != GS_LEVEL)
							player.next_action = NA_NONE;
						else if(player.next_action_data.index < 0 || (uint)player.next_action_data.index >= info.u->items.size())
						{
							Error("Update server: SET_NEXT_ACTION, invalid index %d from '%s'.", player.next_action_data.index, info.name.c_str());
							player.next_action = NA_NONE;
						}
						else
						{
							player.next_action_data.item = Item::TryGet(item_id);
							if(!player.next_action_data.item)
							{
								Error("Update server: SET_NEXT_ACTION, invalid item '%s' from '%s'.", item_id.c_str(), info.name.c_str());
								player.next_action = NA_NONE;
							}
						}
					}
					break;
				case NA_USE:
					{
						int id;
						f >> id;
						if(!f)
						{
							Error("Update server: Broken SET_NEXT_ACTION(4) from '%s'.", info.name.c_str());
							player.next_action = NA_NONE;
						}
						else if(game->game_state == GS_LEVEL)
						{
							player.next_action_data.usable = game_level->FindUsable(id);
							if(!player.next_action_data.usable)
							{
								Error("Update server: SET_NEXT_ACTION, invalid usable %d from '%s'.", id, info.name.c_str());
								player.next_action = NA_NONE;
							}
						}
					}
					break;
				default:
					Error("Update server: SET_NEXT_ACTION, invalid action %d from '%s'.", player.next_action, info.name.c_str());
					player.next_action = NA_NONE;
					break;
				}
			}
			break;
		// player toggle always run - notify to save it
		case NetChange::CHANGE_ALWAYS_RUN:
			{
				bool always_run;
				f >> always_run;
				if(!f)
					Error("Update server: Broken CHANGE_ALWAYS_RUN from %s.", info.name.c_str());
				else
					player.always_run = always_run;
			}
			break;
		// player used generic command
		case NetChange::GENERIC_CMD:
			{
				byte len;
				f >> len;
				if(!f.Ensure(len))
					Error("Update server: Broken GENERIC_CMD (length %u) from %s.", len, info.name.c_str());
				else if(!info.devmode)
				{
					Error("Update server: Player %s used GENERIC_CMD without devmode.", info.name.c_str());
					f.Skip(len);
				}
				else if(game->game_state != GS_LEVEL)
					f.Skip(len);
				else if(!cmdp->ParseStream(f, info))
					Error("Update server: Failed to parse GENERIC_CMD (length %u) from %s.", len, info.name.c_str());
			}
			break;
		// player used item
		case NetChange::USE_ITEM:
			{
				int index;
				f >> index;
				if(!f)
				{
					Error("Update server: Broken USE_ITEM from %s.", info.name.c_str());
					break;
				}
				if(game->game_state != GS_LEVEL)
					break;
				if(index < 0 || index >= (int)unit.items.size())
				{
					Error("Update server: USE_ITEM, invalid index %d from %s.", index, info.name.c_str());
					break;
				}
				ItemSlot& slot = unit.items[index];
				if(!slot.item || slot.item->type != IT_BOOK || !IsSet(slot.item->flags, ITEM_MAGIC_SCROLL))
				{
					Error("Update server: USE_ITEM, invalid item '%s' at index %d from %s.",
						slot.item ? slot.item->id.c_str() : "null", index, info.name.c_str());
					break;
				}
				unit.action = A_USE_ITEM;
				unit.used_item = slot.item;
				unit.mesh_inst->frame_end_info2 = false;
				unit.mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);

				NetChange& c = Add1(changes);
				c.type = NetChange::USE_ITEM;
				c.unit = &unit;
			}
			break;
		// player used cheat 'arena'
		case NetChange::CHEAT_ARENA:
			{
				const string& str = f.ReadString1();
				if(!f)
					Error("Update server: Broken CHEAT_ARENA from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_ARENA without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
					cmdp->ParseStringCommand(CMD_ARENA, str, info);
			}
			break;
		// clean level from blood and corpses
		case NetChange::CLEAN_LEVEL:
			{
				int building_id;
				f >> building_id;
				if(!f)
					Error("Update server: Broken CLEAN_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CLEAN_LEVEL without devmode.", info.name.c_str());
				else if(game->game_state == GS_LEVEL)
					game_level->CleanLevel(building_id);
			}
			break;
		// player set shortcut
		case NetChange::SET_SHORTCUT:
			{
				int index, value;
				Shortcut::Type type;
				f.ReadCasted<byte>(index);
				f.ReadCasted<byte>(type);
				if(type == Shortcut::TYPE_SPECIAL)
					f.ReadCasted<byte>(value);
				else if(type == Shortcut::TYPE_ITEM)
				{
					const string& item_id = f.ReadString1();
					if(f)
					{
						value = (int)Item::TryGet(item_id);
						if(value == 0)
						{
							Error("Update server: SET_SHORTCUT invalid item '%s' from %s.", item_id.c_str(), info.name.c_str());
							break;
						}
					}
				}
				else
					value = 0;
				if(!f)
					Error("Update server: Broken SET_SHORTCUT from %s.", info.name.c_str());
				else if(index < 0 || index >= Shortcut::MAX)
					Error("Update server: SET_SHORTCUT invalid index %d from %s.", index, info.name.c_str());
				else
				{
					Shortcut& shortcut = info.pc->shortcuts[index];
					shortcut.type = type;
					shortcut.value = value;
				}
			}
			break;
		// fast travel request/response
		case NetChange::FAST_TRAVEL:
			{
				FAST_TRAVEL option;
				f.ReadCasted<byte>(option);
				if(!f)
				{
					Error("Update server: Broken FAST_TRAVEL from %s.", info.name.c_str());
					break;
				}

				switch(option)
				{
				case FAST_TRAVEL_START:
					if(!team->IsLeader(info.u))
						Error("Update server: FAST_TRAVEL start - %s is not leader.", info.name.c_str());
					else if(IsFastTravel())
						Error("Update server: FAST_TRAVEL already started from %s.", info.name.c_str());
					else if(!game_level->CanFastTravel())
						Error("Update server: FAST_TRAVEL start from %s, can't.", info.name.c_str());
					else
						StartFastTravel(1);
					break;
				case FAST_TRAVEL_CANCEL:
					if(!team->IsLeader(info.u))
						Error("Update server: FAST_TRAVEL cancel - %s is not leader.", info.name.c_str());
					else if(!IsFastTravel())
						Error("Update server: FAST_TRAVEL cancel not started from %s.", info.name.c_str());
					else
						CancelFastTravel(FAST_TRAVEL_CANCEL, info.id);
					break;
				case FAST_TRAVEL_ACCEPT:
					if(info.fast_travel)
						Error("Update server: FAST_TRAVEL already accepted from %s.", info.name.c_str());
					else
					{
						info.fast_travel = true;

						NetChange& c = Add1(changes);
						c.type = NetChange::FAST_TRAVEL_VOTE;
						c.id = info.id;
					}
					break;
				case FAST_TRAVEL_DENY:
					if(info.fast_travel)
						Error("Update server: FAST_TRAVEL cancel already accepted from %s.", info.name.c_str());
					else
						CancelFastTravel(FAST_TRAVEL_DENY, info.id);
					break;
				}
			}
			break;
		// skip current cutscene
		case NetChange::CUTSCENE_SKIP:
			if(!game->cutscene)
				Error("Update server: CUTSCENE_SKIP no cutscene from %s.", info.name.c_str());
			else if(!team->IsLeader(info.u))
				Error("Update server: CUTSCENE_SKIP not a leader from %s.", info.name.c_str());
			else
				game->CutsceneEnded(true);
			break;
		// invalid change
		default:
			Error("Update server: Invalid change type %u from %s.", type, info.name.c_str());
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			Error("Update server: Invalid checksum from %s (%u).", info.name.c_str(), change_i);
			return true;
		}
	}

	return true;
}

//=================================================================================================
bool Net::CheckMove(Unit& unit, const Vec3& pos)
{
	game_level->global_col.clear();

	const float radius = unit.GetUnitRadius();
	Level::IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { &unit, nullptr };
	const void* ignored_objects[2] = { nullptr, nullptr };
	if(unit.usable)
		ignored_objects[0] = unit.usable;
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignored_objects = ignored_objects;

	game_level->GatherCollisionObjects(*unit.area, game_level->global_col, pos, radius, &ignore);

	if(game_level->global_col.empty())
		return true;

	return !game_level->Collide(game_level->global_col, pos, radius);
}

//=================================================================================================
void Net::WriteServerChanges(BitStreamWriter& f)
{
	// count
	f.WriteCasted<word>(changes.size());
	if(changes.size() >= 0xFFFF)
		Error("Too many changes %d!", changes.size());

	// changes
	for(NetChange& c : changes)
	{
		f.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::UNIT_POS:
			{
				Unit& unit = *c.unit;
				f << unit.id;
				f << unit.pos;
				f << unit.rot;
				f << unit.mesh_inst->groups[0].speed;
				f.WriteCasted<byte>(unit.animation);
			}
			break;
		case NetChange::CHANGE_EQUIPMENT:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			f << c.unit->slots[c.id];
			break;
		case NetChange::TAKE_WEAPON:
			{
				Unit& u = *c.unit;
				f << u.id;
				f << (c.id != 0);
				f.WriteCasted<byte>(c.id == 0 ? u.weapon_taken : u.weapon_hiding);
			}
			break;
		case NetChange::ATTACK:
			{
				Unit&u = *c.unit;
				f << u.id;
				byte b = (byte)c.id;
				b |= ((u.attack_id & 0xF) << 4);
				f << b;
				f << c.f[1];
			}
			break;
		case NetChange::CHANGE_FLAGS:
			{
				byte b = 0;
				if(team->is_bandit)
					b |= 0x01;
				if(team->crazies_attack)
					b |= 0x02;
				if(team->anyone_talking)
					b |= 0x04;
				f << b;
			}
			break;
		case NetChange::UPDATE_HP:
			f << c.unit->id;
			f << c.unit->GetHpp();
			break;
		case NetChange::UPDATE_MP:
			f << c.unit->id;
			f << c.unit->GetMpp();
			break;
		case NetChange::UPDATE_STAMINA:
			f << c.unit->id;
			f << c.unit->GetStaminap();
			break;
		case NetChange::SPAWN_BLOOD:
			f.WriteCasted<byte>(c.id);
			f << c.pos;
			break;
		case NetChange::DIE:
		case NetChange::FALL:
		case NetChange::DROP_ITEM:
		case NetChange::STUNNED:
		case NetChange::STAND_UP:
		case NetChange::CREATE_DRAIN:
		case NetChange::HERO_LEAVE:
		case NetChange::REMOVE_USED_ITEM:
		case NetChange::USABLE_SOUND:
		case NetChange::BREAK_ACTION:
		case NetChange::PLAYER_ACTION:
		case NetChange::USE_ITEM:
			f << c.unit->id;
			break;
		case NetChange::TELL_NAME:
			f << c.unit->id;
			f << (c.id == 1);
			if(c.id == 1)
				f << c.unit->hero->name;
			break;
		case NetChange::UNIT_SOUND:
		case NetChange::CAST_SPELL:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::PICKUP_ITEM:
			f << c.unit->id;
			f << (c.count != 0);
			break;
		case NetChange::SPAWN_ITEM:
			c.item->Write(f);
			break;
		case NetChange::CONSUME_ITEM:
			{
				const Item* item = (const Item*)c.id;
				f << c.unit->id;
				f << item->id;
				f << (c.count != 0);
			}
			break;
		case NetChange::HIT_SOUND:
			f << c.pos;
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::SHOOT_ARROW:
			{
				f << (c.unit ? c.unit->id : -1);
				f << c.pos;
				f << c.vec3;
				f << c.extra_f;
			}
			break;
		case NetChange::UPDATE_CREDIT:
			{
				f << (byte)team->GetActiveTeamSize();
				for(Unit& unit : team->active_members)
				{
					f << unit.id;
					f << unit.GetCredit();
				}
			}
			break;
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count = 0;
				uint pos = f.BeginPatch(count);
				for(PlayerInfo& info : players)
				{
					if(info.left == PlayerInfo::LEFT_NO)
					{
						f << info.u->id;
						f << info.u->player->free_days;
						++count;
					}
				}
				f.Patch(pos, count);
			}
			break;
		case NetChange::IDLE:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::ALL_QUESTS_COMPLETED:
		case NetChange::GAME_OVER:
		case NetChange::LEAVE_LOCATION:
		case NetChange::EXIT_TO_MAP:
		case NetChange::EVIL_SOUND:
		case NetChange::CLOSE_ENCOUNTER:
		case NetChange::CLOSE_PORTAL:
		case NetChange::CLEAN_ALTAR:
		case NetChange::CHEAT_REVEAL_MINIMAP:
		case NetChange::END_OF_GAME:
		case NetChange::GAME_SAVED:
		case NetChange::END_TRAVEL:
		case NetChange::CUTSCENE_SKIP:
			break;
		case NetChange::KICK_NPC:
		case NetChange::REMOVE_UNIT:
		case NetChange::REMOVE_TRAP:
		case NetChange::TRIGGER_TRAP:
		case NetChange::CLEAN_LEVEL:
		case NetChange::REMOVE_ITEM:
			f << c.id;
			break;
		case NetChange::TALK:
			f << c.unit->id;
			f << (byte)c.id;
			f << c.count;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(net_strs, c.str);
			break;
		case NetChange::TALK_POS:
			f << c.pos;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(net_strs, c.str);
			break;
		case NetChange::CHANGE_LOCATION_STATE:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::ADD_RUMOR:
			f << game_gui->journal->GetRumors()[c.id];
			break;
		case NetChange::HAIR_COLOR:
			f << c.unit->id;
			f << c.unit->human_data->hair_color;
			break;
		case NetChange::WARP:
			f << c.unit->id;
			f.WriteCasted<char>(c.unit->area->area_id);
			f << c.unit->pos;
			f << c.unit->rot;
			break;
		case NetChange::REGISTER_ITEM:
			{
				f << c.base_item->id;
				f << c.item2->id;
				f << c.item2->name;
				f << c.item2->desc;
				f << c.item2->quest_id;
			}
			break;
		case NetChange::ADD_QUEST:
			{
				Quest* q = quest_mgr->FindQuest(c.id, false);
				f << q->id;
				f << q->name;
				f.WriteString2(q->msgs[0]);
				f.WriteString2(q->msgs[1]);
			}
			break;
		case NetChange::UPDATE_QUEST:
			{
				Quest* q = quest_mgr->FindQuest(c.id, false);
				f << q->id;
				f.WriteCasted<byte>(q->state);
				f.WriteCasted<byte>(c.count);
				for(int i = 0; i < c.count; ++i)
					f.WriteString2(q->msgs[q->msgs.size() - c.count + i]);
			}
			break;
		case NetChange::RENAME_ITEM:
			{
				const Item* item = c.base_item;
				f << item->quest_id;
				f << item->id;
				f << item->name;
			}
			break;
		case NetChange::CHANGE_LEADER:
		case NetChange::ARENA_SOUND:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REMOVE_CAMP:
		case NetChange::FAST_TRAVEL_VOTE:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::PAUSED:
		case NetChange::CHEAT_NOAI:
			f << (c.id != 0);
			break;
		case NetChange::RANDOM_NUMBER:
			f.WriteCasted<byte>(c.unit->player->id);
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::REMOVE_PLAYER:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::USE_USABLE:
			f << c.unit->id;
			f << c.id;
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::RECRUIT_NPC:
			f << c.unit->id;
			f << (c.unit->hero->type != HeroType::Normal);
			break;
		case NetChange::SPAWN_UNIT:
			c.unit->Write(f);
			break;
		case NetChange::CHANGE_ARENA_STATE:
			f << c.unit->id;
			f.WriteCasted<char>(c.unit->in_arena);
			break;
		case NetChange::WORLD_TIME:
			world->WriteTime(f);
			break;
		case NetChange::USE_DOOR:
			f << c.id;
			f << (c.count != 0);
			break;
		case NetChange::CREATE_EXPLOSION:
			f << c.spell->id;
			f << c.pos;
			break;
		case NetChange::ENCOUNTER:
			f << *c.str;
			StringPool.Free(c.str);
			break;
		case NetChange::ADD_LOCATION:
			{
				Location& loc = *world->GetLocation(c.id);
				f.WriteCasted<byte>(c.id);
				f.WriteCasted<byte>(loc.type);
				if(loc.type == L_DUNGEON)
					f.WriteCasted<byte>(loc.GetLastLevel() + 1);
				f.WriteCasted<byte>(loc.state);
				f.WriteCasted<byte>(loc.target);
				f << loc.pos;
				f << loc.name;
				f.WriteCasted<byte>(loc.image);
			}
			break;
		case NetChange::CHANGE_AI_MODE:
			f << c.unit->id;
			f << c.unit->GetAiMode();
			break;
		case NetChange::CHANGE_UNIT_BASE:
			f << c.unit->id;
			f << c.unit->data->id;
			break;
		case NetChange::CREATE_SPELL_BALL:
			f << c.spell->id;
			f << c.pos;
			f << c.f[0];
			f << c.f[1];
			f << c.extra_id;
			break;
		case NetChange::SPELL_SOUND:
			f << c.spell->id;
			f << c.pos;
			break;
		case NetChange::CREATE_ELECTRO:
			f << c.e_id;
			f << c.pos;
			f << c.vec3;
			break;
		case NetChange::UPDATE_ELECTRO:
			f << c.e_id;
			f << c.pos;
			break;
		case NetChange::ELECTRO_HIT:
		case NetChange::RAISE_EFFECT:
		case NetChange::HEAL_EFFECT:
			f << c.pos;
			break;
		case NetChange::REVEAL_MINIMAP:
			f.WriteCasted<word>(game_level->minimap_reveal_mp.size());
			for(vector<Int2>::iterator it2 = game_level->minimap_reveal_mp.begin(), end2 = game_level->minimap_reveal_mp.end(); it2 != end2; ++it2)
			{
				f.WriteCasted<byte>(it2->x);
				f.WriteCasted<byte>(it2->y);
			}
			game_level->minimap_reveal_mp.clear();
			break;
		case NetChange::CHANGE_MP_VARS:
			WriteNetVars(f);
			break;
		case NetChange::SECRET_TEXT:
			f << Quest_Secret::GetNote().desc;
			break;
		case NetChange::UPDATE_MAP_POS:
			f << world->GetWorldPos();
			break;
		case NetChange::GAME_STATS:
			f << game_stats->total_kills;
			break;
		case NetChange::STUN:
			f << c.unit->id;
			f << c.f[0];
			break;
		case NetChange::MARK_UNIT:
			f << c.unit->id;
			f << (c.id != 0);
			break;
		case NetChange::TRAVEL_POS:
		case NetChange::STOP_TRAVEL:
		case NetChange::CHEAT_TRAVEL_POS:
			f << c.pos.x;
			f << c.pos.y;
			break;
		case NetChange::CHANGE_LOCATION_IMAGE:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(world->GetLocation(c.id)->image);
			break;
		case NetChange::CHANGE_LOCATION_NAME:
			f.WriteCasted<byte>(c.id);
			f << world->GetLocation(c.id)->name;
			break;
		case NetChange::USE_CHEST:
			f << c.id;
			f << c.count;
			break;
		case NetChange::FAST_TRAVEL:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::CUTSCENE_START:
			f << (c.id == 1);
			break;
		case NetChange::CUTSCENE_IMAGE:
		case NetChange::CUTSCENE_TEXT:
			f << *c.str;
			f << c.f[0];
			StringPool.Free(c.str);
			break;
		default:
			Error("Update server: Unknown change %d.", c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Net::WriteServerChangesForPlayer(BitStreamWriter& f, PlayerInfo& info)
{
	PlayerController& player = *info.pc;

	f.WriteCasted<byte>(ID_PLAYER_CHANGES);
	f.WriteCasted<byte>(info.update_flags);

	// variables
	if(IsSet(info.update_flags, PlayerInfo::UF_POISON_DAMAGE))
		f << player.last_dmg_poison;
	if(IsSet(info.update_flags, PlayerInfo::UF_GOLD))
		f << info.u->gold;
	if(IsSet(info.update_flags, PlayerInfo::UF_ALCOHOL))
		f << info.u->alcohol;
	if(IsSet(info.update_flags, PlayerInfo::UF_LEARNING_POINTS))
		f << info.pc->learning_points;
	if(IsSet(info.update_flags, PlayerInfo::UF_LEVEL))
		f << info.u->level;

	// changes
	f.WriteCasted<byte>(info.changes.size());
	if(info.changes.size() > 0xFF)
		Error("Update server: Too many changes for player %s.", info.name.c_str());

	for(NetChangePlayer& c : info.changes)
	{
		f.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChangePlayer::PICKUP:
			f << c.id;
			f << c.count;
			break;
		case NetChangePlayer::LOOT:
			f << (c.id != 0);
			if(c.id != 0)
				f.WriteItemListTeam(*player.chest_trade);
			break;
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				Unit& u = *player.action_unit;
				f << u.weight;
				f << u.weight_max;
				f << u.gold;
				f.WriteItemListTeam(u.items);
			}
			break;
		case NetChangePlayer::START_DIALOG:
			f << c.id;
			break;
		case NetChangePlayer::SHOW_DIALOG_CHOICES:
			{
				DialogContext& ctx = *player.dialog_ctx;
				f.WriteCasted<byte>(ctx.choices.size());
				f.WriteCasted<char>(ctx.dialog_esc);
				for(DialogChoice& choice : ctx.choices)
					f << choice.msg;
			}
			break;
		case NetChangePlayer::END_DIALOG:
		case NetChangePlayer::USE_USABLE:
		case NetChangePlayer::PREPARE_WARP:
		case NetChangePlayer::ENTER_ARENA:
		case NetChangePlayer::START_ARENA_COMBAT:
		case NetChangePlayer::EXIT_ARENA:
		case NetChangePlayer::END_FALLBACK:
			break;
		case NetChangePlayer::START_TRADE:
			f << c.id;
			f.WriteItemList(*player.chest_trade);
			break;
		case NetChangePlayer::SET_FROZEN:
		case NetChangePlayer::DEVMODE:
		case NetChangePlayer::PVP:
		case NetChangePlayer::NO_PVP:
		case NetChangePlayer::CANT_LEAVE_LOCATION:
		case NetChangePlayer::REST:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChangePlayer::IS_BETTER_ITEM:
			f << (c.id != 0);
			break;
		case NetChangePlayer::REMOVE_QUEST_ITEM:
		case NetChangePlayer::LOOK_AT:
			f << c.id;
			break;
		case NetChangePlayer::ADD_ITEMS:
			{
				f << c.id;
				f << c.count;
				f << c.item->id;
				if(c.item->id[0] == '$')
					f << c.item->quest_id;
			}
			break;
		case NetChangePlayer::TRAIN:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.count);
			break;
		case NetChangePlayer::UNSTUCK:
			f << c.pos;
			break;
		case NetChangePlayer::GOLD_RECEIVED:
			f.WriteCasted<byte>(c.id);
			f << c.count;
			break;
		case NetChangePlayer::ADD_ITEMS_TRADER:
			f << c.id;
			f << c.count;
			f << c.a;
			f << c.item;
			break;
		case NetChangePlayer::ADD_ITEMS_CHEST:
			f << c.id;
			f << c.count;
			f << c.a;
			f << c.item;
			break;
		case NetChangePlayer::REMOVE_ITEMS:
			f << c.id;
			f << c.count;
			break;
		case NetChangePlayer::REMOVE_ITEMS_TRADER:
			f << c.id;
			f << c.count;
			f << c.a;
			break;
		case NetChangePlayer::UPDATE_TRADER_GOLD:
			f << c.id;
			f << c.count;
			break;
		case NetChangePlayer::UPDATE_TRADER_INVENTORY:
			f << c.unit->id;
			f.WriteItemListTeam(c.unit->items);
			break;
		case NetChangePlayer::PLAYER_STATS:
			f.WriteCasted<byte>(c.id);
			if(IsSet(c.id, STAT_KILLS))
				f << player.kills;
			if(IsSet(c.id, STAT_DMG_DONE))
				f << player.dmg_done;
			if(IsSet(c.id, STAT_DMG_TAKEN))
				f << player.dmg_taken;
			if(IsSet(c.id, STAT_KNOCKS))
				f << player.knocks;
			if(IsSet(c.id, STAT_ARENA_FIGHTS))
				f << player.arena_fights;
			break;
		case NetChangePlayer::STAT_CHANGED:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.a);
			f << c.count;
			break;
		case NetChangePlayer::ADD_PERK:
		case NetChangePlayer::REMOVE_PERK:
			f.WriteCasted<char>(c.id);
			f.WriteCasted<char>(c.count);
			break;
		case NetChangePlayer::GAME_MESSAGE:
			f << c.id;
			break;
		case NetChangePlayer::RUN_SCRIPT_RESULT:
			if(c.str)
			{
				f.WriteString4(*c.str);
				StringPool.Free(c.str);
			}
			else
				f << 0u;
			break;
		case NetChangePlayer::GENERIC_CMD_RESPONSE:
			f.WriteString4(*c.str);
			StringPool.Free(c.str);
			break;
		case NetChangePlayer::ADD_EFFECT:
			f.WriteCasted<char>(c.id);
			f.WriteCasted<char>(c.count);
			f.WriteCasted<char>(c.a1);
			f.WriteCasted<char>(c.a2);
			f << c.pos.x;
			f << c.pos.y;
			break;
		case NetChangePlayer::REMOVE_EFFECT:
			f.WriteCasted<char>(c.id);
			f.WriteCasted<char>(c.count);
			f.WriteCasted<char>(c.a1);
			f.WriteCasted<char>(c.a2);
			break;
		case NetChangePlayer::ON_REST:
			f.WriteCasted<byte>(c.count);
			break;
		case NetChangePlayer::GAME_MESSAGE_FORMATTED:
			f << c.id;
			f << c.a;
			f << c.count;
			break;
		case NetChangePlayer::SOUND:
			f << c.id;
			break;
		default:
			Error("Update server: Unknown player %s change %d.", info.name.c_str(), c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Net::Server_Say(BitStreamReader& f, PlayerInfo& info)
{
	byte id;
	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Server_Say: Broken packet from %s: %s.", info.name.c_str());
	else
	{
		// id gracza jest ignorowane przez serwer ale mo¿na je sprawdziæ
		assert(id == info.id);

		cstring str = Format("%s: %s", info.name.c_str(), text.c_str());
		game_gui->AddMsg(str);

		// przeœlij dalej
		if(active_players > 2)
			peer->Send(&f.GetBitStream(), MEDIUM_PRIORITY, RELIABLE, 0, info.adr, true);

		if(game->game_state == GS_LEVEL)
			game_gui->level_gui->AddSpeechBubble(info.u, text.c_str());
	}
}

//=================================================================================================
void Net::Server_Whisper(BitStreamReader& f, PlayerInfo& info)
{
	byte id;
	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Server_Whisper: Broken packet from %s.", info.name.c_str());
	else
	{
		if(id == team->my_id)
		{
			// wiadomoœæ do mnie
			cstring str = Format("%s@: %s", info.name.c_str(), text.c_str());
			game_gui->AddMsg(str);
		}
		else
		{
			// wiadomoœæ do kogoœ innego
			PlayerInfo* info2 = TryGetPlayer(id);
			if(!info2)
				Error("Server_Whisper: Broken packet from %s to missing player %d.", info.name.c_str(), id);
			else
			{
				BitStream& bs = f.GetBitStream();
				bs.GetData()[1] = (byte)info.id;
				peer->Send(&bs, MEDIUM_PRIORITY, RELIABLE, 0, info2->adr, false);
			}
		}
	}
}

//=================================================================================================
void Net::ClearChanges()
{
	for(NetChange& c : changes)
	{
		switch(c.type)
		{
		case NetChange::TALK:
		case NetChange::TALK_POS:
			if(IsServer() && c.str)
			{
				StringPool.Free(c.str);
				RemoveElement(net_strs, c.str);
				c.str = nullptr;
			}
			break;
		case NetChange::RUN_SCRIPT:
		case NetChange::CHEAT_ARENA:
			StringPool.Free(c.str);
			break;
		}
	}
	changes.clear();

	for(PlayerInfo& info : players)
		info.changes.clear();
}

//=================================================================================================
void Net::FilterServerChanges()
{
	for(vector<NetChange>::iterator it = changes.begin(), end = changes.end(); it != end;)
	{
		if(FilterOut(*it))
		{
			if(it + 1 == end)
			{
				changes.pop_back();
				break;
			}
			else
			{
				std::iter_swap(it, end - 1);
				changes.pop_back();
				end = changes.end();
			}
		}
		else
			++it;
	}

	for(PlayerInfo& info : players)
	{
		for(vector<NetChangePlayer>::iterator it = info.changes.begin(), end = info.changes.end(); it != end;)
		{
			if(FilterOut(*it))
			{
				if(it + 1 == end)
				{
					info.changes.pop_back();
					break;
				}
				else
				{
					std::iter_swap(it, end - 1);
					info.changes.pop_back();
					end = info.changes.end();
				}
			}
			else
				++it;
		}
	}
}

//=================================================================================================
bool Net::FilterOut(NetChangePlayer& c)
{
	switch(c.type)
	{
	case NetChangePlayer::DEVMODE:
	case NetChangePlayer::GOLD_RECEIVED:
	case NetChangePlayer::GAME_MESSAGE:
	case NetChangePlayer::RUN_SCRIPT_RESULT:
	case NetChangePlayer::GENERIC_CMD_RESPONSE:
	case NetChangePlayer::GAME_MESSAGE_FORMATTED:
		return false;
	default:
		return true;
	}
}

//=================================================================================================
void Net::WriteNetVars(BitStreamWriter& f)
{
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Net::WriteWorldData(BitStreamWriter& f)
{
	Info("Preparing world data.");

	f << ID_WORLD_DATA;

	// world
	world->Write(f);

	// quests
	quest_mgr->Write(f);

	// rumors
	f.WriteStringArray<byte, word>(game_gui->journal->GetRumors());

	// stats
	game_stats->Write(f);

	// mp vars
	WriteNetVars(f);
	if(!net_strs.empty())
		StringPool.Free(net_strs);

	// secret note text
	f << Quest_Secret::GetNote().desc;

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info)
{
	f << ID_PLAYER_START_DATA;

	// flags
	f << info.devmode;
	f << game->noai;

	// player
	info.u->player->WriteStart(f);

	// notes
	f.WriteStringArray<word, word>(info.notes);

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WriteLevelData(BitStreamWriter& f, bool loaded_resources)
{
	f << ID_LEVEL_DATA;
	f << mp_load;
	f << loaded_resources;

	// level
	game_level->Write(f);

	// items preload
	std::set<const Item*>& items_load = game->items_load;
	f << items_load.size();
	for(const Item* item : items_load)
	{
		f << item->id;
		if(item->IsQuest())
			f << item->quest_id;
	}

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WritePlayerData(BitStreamWriter& f, PlayerInfo& info)
{
	Unit& unit = *info.u;

	f << ID_PLAYER_DATA;
	f << unit.id;

	// items
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(unit.slots[i])
			f << unit.slots[i]->id;
		else
			f.Write0();
	}
	f.WriteItemListTeam(unit.items);

	// data
	unit.stats->Write(f);
	f << unit.gold;
	f << unit.effects;
	unit.player->Write(f);

	// other team members
	f.WriteCasted<byte>(team->members.size() - 1);
	for(Unit& other_unit : team->members)
	{
		if(&other_unit != &unit)
			f << other_unit.id;
	}
	f.WriteCasted<byte>(team->leader_id);

	// multiplayer load data
	if(mp_load)
	{
		int flags = 0;
		if(unit.run_attack)
			flags |= 0x01;
		if(unit.used_item_is_team)
			flags |= 0x02;
		f << unit.attack_power;
		f << unit.raise_timer;
		if(unit.action == A_CAST)
			f << unit.action_unit;
		f.WriteCasted<byte>(flags);
	}

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::SendServer(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability, const SystemAddress& adr)
{
	assert(IsServer());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, adr, false);
}

//=================================================================================================
uint Net::SendAll(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability)
{
	assert(IsServer());
	if(active_players <= 1)
		return 0;
	uint ack = peer->Send(&f.GetBitStream(), priority, reliability, 0, master_server_adr, true);
	return ack;
}

//=================================================================================================
void Net::Save(GameWriter& f)
{
	f << server_name;
	f << password;
	f << active_players;
	f << max_players;
	f << last_id;
	uint count = 0;
	for(PlayerInfo& info : players)
	{
		if(info.left == PlayerInfo::LEFT_NO)
			++count;
	}
	f << count;
	for(PlayerInfo& info : players)
	{
		if(info.left == PlayerInfo::LEFT_NO)
			info.Save(f);
	}
	f << mp_use_interp;
	f << mp_interp;
}

//=================================================================================================
void Net::Load(GameReader& f)
{
	f >> server_name;
	f >> password;
	f >> active_players;
	f >> max_players;
	f >> last_id;
	uint count;
	f >> count;
	DeleteElements(old_players);
	old_players.ptrs.resize(count);
	for(uint i = 0; i < count; ++i)
	{
		old_players.ptrs[i] = new PlayerInfo;
		old_players.ptrs[i]->Load(f);
	}
	if(LOAD_VERSION < V_0_12)
		f.Skip(sizeof(int) * 7); // old netid_counters
	f >> mp_use_interp;
	f >> mp_interp;
}

//=================================================================================================
int Net::GetServerFlags()
{
	int flags = 0;
	if(!password.empty())
		flags |= SERVER_PASSWORD;
	if(mp_load)
		flags |= SERVER_SAVED;
	return flags;
}

//=================================================================================================
int Net::GetNewPlayerId()
{
	while(true)
	{
		last_id = (last_id + 1) % 256;
		bool ok = true;
		for(PlayerInfo& info : players)
		{
			if(info.id == last_id)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return last_id;
	}
}

//=================================================================================================
PlayerInfo* Net::FindOldPlayer(cstring nick)
{
	assert(nick);

	for(PlayerInfo& info : old_players)
	{
		if(info.name == nick)
			return &info;
	}

	return nullptr;
}

//=================================================================================================
void Net::DeleteOldPlayers()
{
	const bool in_level = game_level->is_open;
	for(PlayerInfo& info : old_players)
	{
		if(!info.loaded && info.u)
		{
			if(in_level)
				RemoveElement(info.u->area->units, info.u);
			if(info.u->cobj)
			{
				delete info.u->cobj->getCollisionShape();
				phy_world->removeCollisionObject(info.u->cobj);
				delete info.u->cobj;
			}
			delete info.u;
		}
	}
	DeleteElements(old_players);
}

//=================================================================================================
void Net::KickPlayer(PlayerInfo& info)
{
	// send kick message
	BitStreamWriter f;
	f << ID_SERVER_CLOSE;
	f << (byte)ServerClose_Kicked;
	SendServer(f, MEDIUM_PRIORITY, RELIABLE, info.adr);

	info.state = PlayerInfo::REMOVING;

	ServerPanel* server_panel = game_gui->server;
	if(server_panel->visible)
	{
		server_panel->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		Info("Player %s was kicked.", info.name.c_str());

		if(active_players > 2)
			server_panel->AddLobbyUpdate(Int2(Lobby_KickPlayer, info.id));

		server_panel->CheckReady();
		server_panel->UpdateServerInfo();
	}
	else
	{
		info.left = PlayerInfo::LEFT_KICK;
		players_left = true;
	}
}

//=================================================================================================
void Net::OnChangeLevel(int level)
{
	BitStreamWriter f;
	f << ID_CHANGE_LEVEL;
	f << (byte)game_level->location_index;
	f << (byte)level;
	f << false;

	uint ack = SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
	for(PlayerInfo& info : players)
	{
		if(info.id == team->my_id)
			info.state = PlayerInfo::IN_GAME;
		else
		{
			info.state = PlayerInfo::WAITING_FOR_RESPONSE;
			info.ack = ack;
			info.timer = CHANGE_LEVEL_TIMER;
		}
	}
}
