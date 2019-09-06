#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GameStats.h"
#include "BitStreamFunc.h"
#include "Version.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "ErrorHandler.h"
#include "Content.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"
#include "Quest_Secret.h"
#include "Quest.h"
#include "City.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "Cave.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "LevelGui.h"
#include "WorldMapGui.h"
#include "MpBox.h"
#include "AIController.h"
#include "Spell.h"
#include "Team.h"
#include "Action.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "Portal.h"
#include "EntityInterpolator.h"
#include "World.h"
#include "Level.h"
#include "LocationGeneratorFactory.h"
#include "GameMessages.h"
#include "Arena.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"
#include "ItemHelper.h"
#include "GameGui.h"
#include "Console.h"
#include "FOV.h"
#include "PlayerInfo.h"
#include "CommandParser.h"
#include "Quest_Scripted.h"

vector<NetChange> Net::changes;
Net::Mode Net::mode;

//=================================================================================================
inline void WriteItemList(BitStreamWriter& f, vector<ItemSlot>& items)
{
	f << items.size();
	for(ItemSlot& slot : items)
	{
		f << *slot.item;
		f << slot.count;
	}
}

//=================================================================================================
inline void WriteItemListTeam(BitStreamWriter& f, vector<ItemSlot>& items)
{
	f << items.size();
	for(ItemSlot& slot : items)
	{
		f << *slot.item;
		f << slot.count;
		f << slot.team_count;
	}
}

//=================================================================================================
bool Game::ReadItemList(BitStreamReader& f, vector<ItemSlot>& items)
{
	const int MIN_SIZE = 5;

	uint count = f.Read<uint>();
	if(!f.Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(f, slot.item) < 1)
			return false;
		f >> slot.count;
		if(!f)
			return false;
		PreloadItem(slot.item);
		slot.team_count = 0;
	}

	return true;
}

//=================================================================================================
bool Game::ReadItemListTeam(BitStreamReader& f, vector<ItemSlot>& items, bool skip)
{
	const int MIN_SIZE = 9;

	uint count;
	f >> count;
	if(!f.Ensure(count * MIN_SIZE))
		return false;

	items.resize(count);
	for(ItemSlot& slot : items)
	{
		if(ReadItemAndFind(f, slot.item) < 1)
			return false;
		f >> slot.count;
		f >> slot.team_count;
		if(!f)
			return false;
		if(!skip)
			PreloadItem(slot.item);
	}

	return true;
}

//=================================================================================================
void Game::AddServerMsg(cstring msg)
{
	assert(msg);
	cstring s = Format(txServer, msg);
	if(game_gui->server->visible)
		game_gui->server->AddMsg(s);
	else
	{
		game_gui->messages->AddGameMsg(msg, 2.f + float(strlen(msg)) / 10);
		AddMultiMsg(s);
	}
}

//=================================================================================================
void Game::SendPlayerData(PlayerInfo& info)
{
	Unit& unit = *info.u;
	BitStreamWriter f;

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
	WriteItemListTeam(f, unit.items);

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
	if(net->mp_load)
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

	net->SendServer(f, HIGH_PRIORITY, RELIABLE, info.adr);
}

//=================================================================================================
bool Game::ReadPlayerData(BitStreamReader& f)
{
	PlayerInfo& info = net->GetMe();

	int id = f.Read<int>();
	if(!f)
	{
		Error("Read player data: Broken packet.");
		return false;
	}

	Unit* unit = game_level->FindUnit(id);
	if(!unit)
	{
		Error("Read player data: Missing unit %d.", id);
		return false;
	}
	info.u = unit;
	pc = unit->player;
	pc->player_info = &info;
	info.pc = pc;
	game_gui->level_gui->Setup();

	// items
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read player data: Broken item %d.", i);
			return false;
		}
		if(!item_id.empty())
		{
			unit->slots[i] = Item::TryGet(item_id);
			if(!unit->slots[i])
				return false;
		}
		else
			unit->slots[i] = nullptr;
	}
	if(!ReadItemListTeam(f, unit->items))
	{
		Error("Read player data: Broken item list.");
		return false;
	}

	int credit = unit->player->credit,
		free_days = unit->player->free_days;

	unit->player->Init(*unit, true);

	unit->stats = new UnitStats;
	unit->stats->fixed = false;
	unit->stats->subprofile.value = 0;
	unit->stats->Read(f);
	f >> unit->gold;
	f >> unit->effects;
	if(!f || !pc->Read(f))
	{
		Error("Read player data: Broken stats.");
		return false;
	}

	unit->prev_speed = 0.f;
	unit->run_attack = false;

	unit->weight = 0;
	unit->CalculateLoad();
	unit->RecalculateWeight();
	unit->hpmax = unit->CalculateMaxHp();
	unit->hp *= unit->hpmax;
	unit->mpmax = unit->CalculateMaxMp();
	unit->mp *= unit->mpmax;
	unit->stamina_max = unit->CalculateMaxStamina();
	unit->stamina *= unit->stamina_max;

	unit->player->credit = credit;
	unit->player->free_days = free_days;
	unit->player->is_local = true;

	// other team members
	team->members.clear();
	team->active_members.clear();
	team->members.push_back(unit);
	team->active_members.push_back(unit);
	byte count;
	f >> count;
	if(!f.Ensure(sizeof(int) * count))
	{
		Error("Read player data: Broken team members.");
		return false;
	}
	for(byte i = 0; i < count; ++i)
	{
		f >> id;
		Unit* team_member = game_level->FindUnit(id);
		if(!team_member)
		{
			Error("Read player data: Missing team member %d.", id);
			return false;
		}
		team->members.push_back(team_member);
		if(team_member->IsPlayer() || team_member->hero->type == HeroType::Normal)
			team->active_members.push_back(team_member);
	}
	f.ReadCasted<byte>(team->leader_id);
	if(!f)
	{
		Error("Read player data: Broken team leader.");
		return false;
	}
	PlayerInfo* leader_info = net->TryGetPlayer(team->leader_id);
	if(!leader_info)
	{
		Error("Read player data: Missing player %d.", team->leader_id);
		return false;
	}
	team->leader = leader_info->u;

	dialog_context.pc = unit->player;

	// multiplayer load data
	if(net->mp_load)
	{
		byte flags;
		f >> unit->attack_power;
		f >> unit->raise_timer;
		if(unit->action == A_CAST)
			f >> unit->action_unit;
		f >> flags;
		if(!f)
		{
			Error("Read player data: Broken multiplayer data.");
			return false;
		}
		unit->run_attack = IsSet(flags, 0x01);
		unit->used_item_is_team = IsSet(flags, 0x02);
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read player data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
void Game::UpdateServer(float dt)
{
	if(game_state == GS_LEVEL)
	{
		net->InterpolatePlayers(dt);
		net->UpdateFastTravel(dt);
		pc->unit->changed = true;
	}

	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader reader(packet);
		PlayerInfo* ptr_info = net->FindPlayer(packet->systemAddress);
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
			--net->active_players;
			net->players_left = true;
			info.left = (msg_id == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
			break;
		case ID_SAY:
			Server_Say(reader, info, packet);
			break;
		case ID_WHISPER:
			Server_Whisper(reader, info, packet);
			break;
		case ID_CONTROL:
			if(!ProcessControlMessageServer(reader, info))
			{
				net->peer->DeallocatePacket(packet);
				return;
			}
			break;
		default:
			Warn("UpdateServer: Unknown packet from %s: %u.", info.name.c_str(), msg_id);
			break;
		}
	}

	ProcessLeftPlayers();

	net->update_timer += dt;
	if(net->update_timer >= TICK && net->active_players > 1)
	{
		bool last_anyone_talking = anyone_talking;
		anyone_talking = IsAnyoneTalking();
		if(last_anyone_talking != anyone_talking)
			Net::PushChange(NetChange::CHANGE_FLAGS);

		net->update_timer = 0;
		BitStreamWriter f;
		f << ID_CHANGES;

		// dodaj zmiany pozycji jednostek i ai_mode
		if(game_state == GS_LEVEL)
		{
			for(LevelArea& area : game_level->ForEachArea())
				ServerProcessUnits(area.units);
		}

		// wyœlij odkryte kawa³ki minimapy
		if(!game_level->minimap_reveal_mp.empty())
		{
			if(game_state == GS_LEVEL)
				Net::PushChange(NetChange::REVEAL_MINIMAP);
			else
				game_level->minimap_reveal_mp.clear();
		}

		// changes
		WriteServerChanges(f);
		Net::changes.clear();
		assert(net->net_strs.empty());
		net->SendAll(f, HIGH_PRIORITY, RELIABLE_ORDERED);

		for(PlayerInfo& info : net->players)
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
				net->SendServer(f, HIGH_PRIORITY, RELIABLE_ORDERED, info.adr);
				info.update_flags = 0;
				info.changes.clear();
			}
		}
	}
}

//=================================================================================================
bool Game::ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info)
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
		if(!info.warping && game_state == GS_LEVEL)
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
				if(player.noclip || unit.usable || CheckMoveNet(unit, new_pos))
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
	byte changes;
	f >> changes;
	if(!f)
	{
		Error("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str());
		return true;
	}

	// process changes
	for(byte change_i = 0; change_i < changes; ++change_i)
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
				else if(game_state == GS_LEVEL)
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
						if(net->active_players > 2 && IsVisible(slot_type))
						{
							NetChange& c = Add1(Net::changes);
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
							if(net->active_players > 2 && IsVisible(slot))
							{
								NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
				{
					unit.mesh_inst->groups[1].speed = 1.f;
					unit.SetWeaponState(!hide, weapon_type);
					// send to other players
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
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
								PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
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
								PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
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
								unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
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
								PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
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
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
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
						if(net->active_players > 2 && IsVisible(slot_type))
						{
							NetChange& c = Add1(Net::changes);
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
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
				NetChange& c2 = Add1(Net::changes);
				c2.type = NetChange::REMOVE_ITEM;
				c2.id = item->id;

				// send info to other players about picking item
				if(net->active_players > 2)
				{
					NetChange& c3 = Add1(Net::changes);
					c3.type = NetChange::PICKUP_ITEM;
					c3.unit = &unit;
					c3.count = (up_animation ? 1 : 0);
				}

				// remove item
				if(pc_data.before_player == BP_ITEM && pc_data.before_player_ptr.item == item)
					pc_data.before_player = BP_NONE;
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
				else if(game_state == GS_LEVEL)
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

				if(game_state != GS_LEVEL)
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

				if(game_state != GS_LEVEL)
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

				if(game_state != GS_LEVEL)
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
									if(net->active_players > 2)
									{
										NetChange& c = Add1(Net::changes);
										c.type = NetChange::REMOVE_USED_ITEM;
										c.unit = player.action_unit;
									}
								}
								if(IsSet(slot.item->flags, ITEM_IMPORTANT))
								{
									player.action_unit->mark = false;
									NetChange& c = Add1(Net::changes);
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
						if(net->active_players > 2)
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::REMOVE_USED_ITEM;
							c.unit = player.action_unit;
						}
					}
					player.action_unit->RemoveItemEffects(slot, type);
					player.action_unit->weight -= slot->weight;
					slot = nullptr;

					// send info about changing equipment of looted unit
					if(net->active_players > 2 && IsVisible(type))
					{
						NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
								if(Net::IsLocal())
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
					if(net->active_players > 2 && IsVisible(type))
					{
						NetChange& c = Add1(Net::changes);
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
				if(game_state != GS_LEVEL)
					break;
				if(!player.IsTrading())
				{
					Error("Update server: GET_ALL_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				bool changes = false;

				// slots
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
							changes = true;

							// send info about changing equipment
							if(net->active_players > 2 && IsVisible((ITEM_SLOT)i))
							{
								NetChange& c = Add1(Net::changes);
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
						changes = true;
					}
				}
				player.chest_trade->clear();

				if(changes)
					SortItems(unit.items);
			}
			break;
		// player ends trade
		case NetChange::STOP_TRADE:
			if(game_state != GS_LEVEL)
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

				NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
				{
					unit.mesh_inst->Play(unit.data->idles->anims[index].c_str(), PLAY_ONCE, 0);
					unit.mesh_inst->groups[0].speed = 1.f;
					unit.mesh_inst->frame_end_info = false;
					unit.animation = ANI_IDLE;
					// send info to other players
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
				{
					if(game_level->city_ctx && building_index < game_level->city_ctx->inside_buildings.size())
					{
						WarpData& warp = Add1(mp_warps);
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
			if(game_state == GS_LEVEL)
			{
				if(unit.area->area_type == LevelArea::Type::Building)
				{
					WarpData& warp = Add1(mp_warps);
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
			if(game_state == GS_LEVEL)
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
					AddMsg(Format(txRolledNumber, info.name.c_str(), number));
					// send to other players
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
				NetChange& c = Add1(Net::changes);
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
				if(game_state == GS_LEVEL)
					GiveDmg(unit, unit.hpmax);
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
			if(game_state != GS_LEVEL)
				break;
			unit.Standup();
			// send to other players
			if(net->active_players > 2)
			{
				NetChange& c = Add1(Net::changes);
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
				if(game_state != GS_LEVEL)
					break;
				for(AIController* ai : ais)
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

				if(game_state != GS_LEVEL)
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

				if(game_state != GS_LEVEL)
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
					Net::PushChange(NetChange::CHANGE_FLAGS);
				}
			}
			else
				Error("Update server: Player %s used CHEAT_CITIZEN without devmode.", info.name.c_str());
			break;
		// player used cheat 'heal'
		case NetChange::CHEAT_HEAL:
			if(info.devmode)
			{
				if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(!target)
						Error("Update server: CHEAT_KILL from %s, missing unit %d.", info.name.c_str(), id);
					else if(target->IsAlive())
						GiveDmg(*target, target->hpmax);
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
				else if(game_state == GS_LEVEL)
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
				if(game_state != GS_LEVEL)
					break;
				ExitToMap();
				return false;
			}
			else
				Error("Update server: Player %s used CHEAT_GOTO_MAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'reveal_minimap'
		case NetChange::CHEAT_REVEAL_MINIMAP:
			if(info.devmode)
			{
				if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
				{
					if(unit.frozen != FROZEN::NO)
						Error("Update server: CHEAT_WARP from %s, unit is frozen.", info.name.c_str());
					else if(!game_level->city_ctx || building_index >= game_level->city_ctx->inside_buildings.size())
						Error("Update server: CHEAT_WARP from %s, invalid inside building index %u.", info.name.c_str(), building_index);
					else
					{
						WarpData& warp = Add1(mp_warps);
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
				else if(game_state == GS_LEVEL)
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
								arena->units.push_back(spawned);
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
					arena->HandlePvpResponse(info, accepted);
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
				else if(game_state == GS_LEVEL)
				{
					CanLeaveLocationResult result = game_level->CanLeaveLocation(*info.u);
					if(result == CanLeaveLocationResult::Yes)
					{
						Net::PushChange(NetChange::LEAVE_LOCATION);
						if(type == ENTER_FROM_OUTSIDE)
							fallback_type = FALLBACK::EXIT;
						else if(type == ENTER_FROM_UP_LEVEL)
						{
							fallback_type = FALLBACK::CHANGE_LEVEL;
							fallback_1 = -1;
						}
						else if(type == ENTER_FROM_DOWN_LEVEL)
						{
							fallback_type = FALLBACK::CHANGE_LEVEL;
							fallback_1 = +1;
						}
						else
						{
							if(game_level->location->TryGetPortal(type))
							{
								fallback_type = FALLBACK::USE_PORTAL;
								fallback_1 = type;
							}
							else
							{
								Error("Update server: LEAVE_LOCATION from %s, invalid type %d.", type);
								break;
							}
						}

						fallback_t = -1.f;
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

				if(game_state != GS_LEVEL)
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
						sound = sDoorClose;
					else
						sound = sDoor[Rand() % 3];
					sound_mgr->PlaySound3d(sound, door->GetCenter(), Door::SOUND_DIST);
				}

				// send to other players
				NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_WORLDMAP)
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
				else if(game_state == GS_WORLDMAP)
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
				else if(game_state == GS_WORLDMAP)
					world->StopTravel(pos, true);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game_state == GS_WORLDMAP)
				world->EndTravel();
			break;
		// enter current location
		case NetChange::ENTER_LOCATION:
			if(game_state == GS_WORLDMAP && world->GetState() == World::State::ON_MAP && team->IsLeader(info.u))
			{
				EnterLocation();
				return false;
			}
			else
				Error("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str());
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(game_state == GS_WORLDMAP)
			{
				if(game_gui->world_map->dialog_enc)
				{
					gui->CloseDialog(game_gui->world_map->dialog_enc);
					game_gui->world_map->dialog_enc = nullptr;
				}
				Net::PushChange(NetChange::CLOSE_ENCOUNTER);
				Event_RandomEncounter(0);
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
				else if(game_state == GS_LEVEL)
				{
					ChangeLevel(is_down ? +1 : -1);
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
				else if(game_state == GS_LEVEL)
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
					if(noai != state)
					{
						noai = state;
						NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
					PlayerInfo* new_leader = net->TryGetPlayer(id);
					if(!new_leader)
					{
						Error("Update server: CHANGE_LEADER from %s, invalid player id %u.", id);
						break;
					}

					team->leader_id = id;
					team->leader = new_leader->u;

					if(team->leader_id == team->my_id)
						AddMsg(txYouAreLeader);
					else
						AddMsg(Format(txPcIsLeader, team->leader->player->name.c_str()));

					NetChange& c = Add1(Net::changes);
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

				if(game_state != GS_LEVEL)
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
						if(target->player != pc)
						{
							NetChangePlayer& c = Add1(target->player->player_info->changes);
							c.type = NetChangePlayer::GOLD_RECEIVED;
							c.id = info.id;
							c.count = count;
							target->player->player_info->UpdateGold();
						}
						else
						{
							AddMultiMsg(Format(txReceivedGold, count, info.name.c_str()));
							sound_mgr->PlaySound2d(sCoins);
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
				else if(game_state == GS_LEVEL)
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
						if(net->active_players > 2)
						{
							NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_WORLDMAP)
				{
					world->Warp(location_index);
					game_gui->world_map->StartTravel();

					// inform other players
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_WORLDMAP)
				{
					world->WarpPos(pos);
					game_gui->world_map->StartTravel();

					// inform other players
					if(net->active_players > 2)
					{
						NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(target)
						GiveDmg(*target, 100.f);
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
			if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(netid);
					if(!target && netid != -1)
						Error("Update server: PLAYER_ACTION, invalid target %d from %s.", netid, info.name.c_str());
					else
						UseAction(info.pc, false, &pos, target);
				}
			}
			break;
		// player used cheat 'refresh_cooldown'
		case NetChange::CHEAT_REFRESH_COOLDOWN:
			if(!info.devmode)
				Error("Update server: Player %s used CHEAT_REFRESH_COOLDOWN without devmode.", info.name.c_str());
			else if(game_state == GS_LEVEL)
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

				if(game_state != GS_LEVEL)
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
					else if(game_state != GS_LEVEL)
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
						else if(game_state != GS_LEVEL)
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
						else if(game_state == GS_LEVEL)
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
				else if(game_state != GS_LEVEL)
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
				if(game_state != GS_LEVEL)
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

				NetChange& c = Add1(Net::changes);
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
				else if(game_state == GS_LEVEL)
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
				else if(game_state == GS_LEVEL)
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
					else if(net->IsFastTravel())
						Error("Update server: FAST_TRAVEL already started from %s.", info.name.c_str());
					else if(!game_level->CanFastTravel())
						Error("Update server: FAST_TRAVEL start from %s, can't.", info.name.c_str());
					else
						net->StartFastTravel(1);
					break;
				case FAST_TRAVEL_CANCEL:
					if(!team->IsLeader(info.u))
						Error("Update server: FAST_TRAVEL cancel - %s is not leader.", info.name.c_str());
					else if(!net->IsFastTravel())
						Error("Update server: FAST_TRAVEL cancel not started from %s.", info.name.c_str());
					else
						net->CancelFastTravel(FAST_TRAVEL_CANCEL, info.id);
					break;
				case FAST_TRAVEL_ACCEPT:
					if(info.fast_travel)
						Error("Update server: FAST_TRAVEL already accepted from %s.", info.name.c_str());
					else
					{
						info.fast_travel = true;

						NetChange& c = Add1(Net::changes);
						c.type = NetChange::FAST_TRAVEL_VOTE;
						c.id = info.id;
					}
					break;
				case FAST_TRAVEL_DENY:
					if(info.fast_travel)
						Error("Update server: FAST_TRAVEL cancel already accepted from %s.", info.name.c_str());
					else
						net->CancelFastTravel(FAST_TRAVEL_DENY, info.id);
					break;
				}
			}
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
void Game::WriteServerChanges(BitStreamWriter& f)
{
	// count
	f.WriteCasted<word>(Net::changes.size());
	if(Net::changes.size() >= 0xFFFF)
		Error("Too many changes %d!", Net::changes.size());

	// changes
	for(NetChange& c : Net::changes)
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
				if(anyone_talking)
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
				for(PlayerInfo& info : net->players)
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
			RemoveElement(net->net_strs, c.str);
			break;
		case NetChange::TALK_POS:
			f << c.pos;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(net->net_strs, c.str);
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
				if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
					f.WriteCasted<byte>(loc.GetLastLevel() + 1);
				f.WriteCasted<byte>(loc.state);
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
			net->WriteNetVars(f);
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
		default:
			Error("Update server: Unknown change %d.", c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Game::WriteServerChangesForPlayer(BitStreamWriter& f, PlayerInfo& info)
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
				WriteItemListTeam(f, *player.chest_trade);
			break;
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				Unit& u = *player.action_unit;
				f << u.weight;
				f << u.weight_max;
				f << u.gold;
				WriteItemListTeam(f, u.items);
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
			WriteItemList(f, *player.chest_trade);
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
			WriteItemListTeam(f, c.unit->items);
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
void Game::UpdateClient(float dt)
{
	if(game_state == GS_LEVEL)
	{
		// interpolacja pozycji gracza
		if(interpolate_timer > 0.f)
		{
			interpolate_timer -= dt;
			if(interpolate_timer >= 0.f)
				pc->unit->visual_pos = Vec3::Lerp(pc->unit->visual_pos, pc->unit->pos, (0.1f - interpolate_timer) * 10);
			else
				pc->unit->visual_pos = pc->unit->pos;
		}

		// interpolacja pozycji/obrotu postaci
		net->InterpolateUnits(dt);
	}

	bool exit_from_server = false;

	Packet* packet;
	for(packet = net->peer->Receive(); packet; net->peer->DeallocatePacket(packet), packet = net->peer->Receive())
	{
		BitStreamReader reader(packet);
		byte msg_id;
		reader >> msg_id;

		switch(msg_id)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			{
				cstring text, text_eng;
				if(packet->data[0] == ID_CONNECTION_LOST)
				{
					text = txLostConnection;
					text_eng = "Update client: Lost connection with server.";
				}
				else
				{
					text = txYouDisconnected;
					text_eng = "Update client: You have been disconnected.";
				}
				Info(text_eng);
				net->peer->DeallocatePacket(packet);
				ExitToMenu();
				gui->SimpleDialog(text, nullptr);
				return;
			}
		case ID_SAY:
			Client_Say(reader);
			break;
		case ID_WHISPER:
			Client_Whisper(reader);
			break;
		case ID_SERVER_SAY:
			Client_ServerSay(reader);
			break;
		case ID_SERVER_CLOSE:
			{
				byte reason = (packet->length == 2 ? packet->data[1] : 0);
				cstring reason_text, reason_text_int;
				if(reason == ServerClose_Kicked)
				{
					reason_text = "You have been kicked out.";
					reason_text_int = txYouKicked;
				}
				else
				{
					reason_text = "Server was closed.";
					reason_text_int = txServerClosed;
				}
				Info("Update client: %s", reason_text);
				net->peer->DeallocatePacket(packet);
				ExitToMenu();
				gui->SimpleDialog(reason_text_int, nullptr);
				return;
			}
		case ID_CHANGE_LEVEL:
			{
				byte loc, level;
				bool encounter;
				reader >> loc;
				reader >> level;
				reader >> encounter;
				if(!reader)
					Error("Update client: Broken ID_CHANGE_LEVEL.");
				else
				{
					world->ChangeLevel(loc, encounter);
					game_level->dungeon_level = level;
					Info("Update client: Level change to %s (%d, level %d).", game_level->location->name.c_str(), game_level->location_index, game_level->dungeon_level);
					game_gui->info_box->Show(txGeneratingLocation);
					LeaveLevel();
					net_mode = NM_TRANSFER;
					net_state = NetState::Client_ChangingLevel;
					clear_color = Color::Black;
					game_gui->load_screen->visible = true;
					game_gui->level_gui->visible = false;
					game_gui->world_map->Hide();
					arena->ClosePvpDialog();
					if(game_gui->world_map->dialog_enc)
					{
						gui->CloseDialog(game_gui->world_map->dialog_enc);
						game_gui->world_map->dialog_enc = nullptr;
					}
					net->peer->DeallocatePacket(packet);
					net->FilterClientChanges();
					LoadingStart(4);
					return;
				}
			}
			break;
		case ID_CHANGES:
			if(!ProcessControlMessageClient(reader, exit_from_server))
			{
				net->peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_PLAYER_CHANGES:
			if(!ProcessControlMessageClientForMe(reader))
			{
				net->peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_LOADING:
			{
				Info("Quickloading server game.");
				net->mp_quickload = true;
				ClearGame();
				reader >> net->mp_load_worldmap;
				LoadingStart(net->mp_load_worldmap ? 4 : 9);
				game_gui->info_box->Show(txLoadingSaveByServer);
				game_gui->world_map->Hide();
				net_mode = Game::NM_TRANSFER;
				net_state = NetState::Client_BeforeTransfer;
				game_state = GS_LOAD;
				items_load.clear();
			}
			break;
		default:
			Warn("UpdateClient: Unknown packet from server: %u.", msg_id);
			break;
		}
	}

	// wyœli moj¹ pozycjê/akcjê
	net->update_timer += dt;
	if(net->update_timer >= TICK)
	{
		net->update_timer = 0;
		BitStreamWriter f;
		f << ID_CONTROL;
		if(game_state == GS_LEVEL)
		{
			f << true;
			f << pc->unit->pos;
			f << pc->unit->rot;
			f << pc->unit->mesh_inst->groups[0].speed;
			f.WriteCasted<byte>(pc->unit->animation);
		}
		else
			f << false;
		WriteClientChanges(f);
		Net::changes.clear();
		net->SendClient(f, HIGH_PRIORITY, RELIABLE_ORDERED);
	}

	if(exit_from_server)
		net->peer->Shutdown(1000);
}

//=================================================================================================
bool Game::ProcessControlMessageClient(BitStreamReader& f, bool& exit_from_server)
{
	// read count
	word changes;
	f >> changes;
	if(!f)
	{
		Error("Update client: Broken ID_CHANGES.");
		return true;
	}

	// read changes
	for(word change_i = 0; change_i < changes; ++change_i)
	{
		// read type
		NetChange::TYPE type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			Error("Update client: Broken ID_CHANGES(2).");
			return true;
		}

		// process
		switch(type)
		{
		// unit position/rotation/animation
		case NetChange::UNIT_POS:
			{
				int id;
				Vec3 pos;
				float rot, ani_speed;
				Animation ani;
				f >> id;
				f >> pos;
				f >> rot;
				f >> ani_speed;
				f.ReadCasted<byte>(ani);
				if(!f)
				{
					Error("Update client: Broken UNIT_POS.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Unit* unit = game_level->FindUnit(id);
				if(!unit)
					Error("Update client: UNIT_POS, missing unit %d.", id);
				else if(unit != pc->unit)
				{
					unit->pos = pos;
					unit->mesh_inst->groups[0].speed = ani_speed;
					assert(ani < ANI_MAX);
					if(unit->animation != ANI_PLAY && ani != ANI_PLAY)
						unit->animation = ani;
					unit->UpdatePhysics();
					unit->interp->Add(pos, rot);
				}
			}
			break;
		// unit changed equipped item
		case NetChange::CHANGE_EQUIPMENT:
			{
				int id;
				ITEM_SLOT type;
				const Item* item;
				f >> id;
				f.ReadCasted<byte>(type);
				if(!f || ReadItemAndFind(f, item) < 0)
					Error("Update client: Broken CHANGE_EQUIPMENT.");
				else if(!IsValid(type))
					Error("Update client: CHANGE_EQUIPMENT, invalid slot %d.", type);
				else if(game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(!target)
						Error("Update client: CHANGE_EQUIPMENT, missing unit %d.", id);
					else
					{
						if(item)
							PreloadItem(item);
						target->slots[type] = item;
					}
				}
			}
			break;
		// unit take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				int id;
				bool hide;
				WeaponType type;
				f >> id;
				f >> hide;
				f.ReadCasted<byte>(type);
				if(!f)
					Error("Update client: Broken TAKE_WEAPON.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: TAKE_WEAPON, missing unit %d.", id);
					else if(unit != pc->unit)
					{
						if(unit->mesh_inst->mesh->head.n_groups > 1)
							unit->mesh_inst->groups[1].speed = 1.f;
						unit->SetWeaponState(!hide, type);
					}
				}
			}
			break;
		// unit attack
		case NetChange::ATTACK:
			{
				int id;
				byte typeflags;
				float attack_speed;
				f >> id;
				f >> typeflags;
				f >> attack_speed;
				if(!f)
				{
					Error("Update client: Broken ATTACK.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Unit* unit_ptr = game_level->FindUnit(id);
				if(!unit_ptr)
				{
					Error("Update client: ATTACK, missing unit %d.", id);
					break;
				}

				// don't start attack if this is local unit
				if(unit_ptr == pc->unit)
					break;

				Unit& unit = *unit_ptr;
				byte type = (typeflags & 0xF);
				int group = unit.mesh_inst->mesh->head.n_groups - 1;
				unit.weapon_state = WS_TAKEN;

				switch(type)
				{
				case AID_Attack:
					if(unit.action == A_ATTACK && unit.animation_state == 0)
					{
						unit.animation_state = 1;
						unit.mesh_inst->groups[1].speed = attack_speed;
					}
					else
					{
						if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_PrepareAttack:
					{
						if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 0;
						unit.hitted = false;
					}
					break;
				case AID_Shoot:
				case AID_StartShoot:
					if(unit.action == A_SHOOT && unit.animation_state == 0)
						unit.animation_state = 1;
					else
					{
						unit.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.action = A_SHOOT;
						unit.animation_state = (type == AID_Shoot ? 1 : 0);
						unit.hitted = false;
						if(!unit.bow_instance)
							unit.bow_instance = GetBowInstance(unit.GetBow().mesh);
						unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
						unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[group].speed;
					}
					break;
				case AID_Block:
					{
						unit.action = A_BLOCK;
						unit.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, group);
						unit.mesh_inst->groups[1].speed = 1.f;
						unit.mesh_inst->groups[1].blend_max = attack_speed;
						unit.animation_state = 0;
					}
					break;
				case AID_Bash:
					{
						unit.action = A_BASH;
						unit.animation_state = 0;
						unit.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, group);
						unit.mesh_inst->groups[1].speed = attack_speed;
						unit.mesh_inst->frame_end_info2 = false;
						unit.hitted = false;
					}
					break;
				case AID_RunningAttack:
					{
						if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
							PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.5f;
						unit.run_attack = true;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_StopBlock:
					{
						unit.action = A_NONE;
						unit.mesh_inst->frame_end_info2 = false;
						unit.mesh_inst->Deactivate(group);
						unit.mesh_inst->groups[1].speed = 1.f;
					}
					break;
				}
			}
			break;
		// change of game flags
		case NetChange::CHANGE_FLAGS:
			{
				byte flags;
				f >> flags;
				if(!f)
					Error("Update client: Broken CHANGE_FLAGS.");
				else
				{
					team->is_bandit = IsSet(flags, 0x01);
					team->crazies_attack = IsSet(flags, 0x02);
					anyone_talking = IsSet(flags, 0x04);
				}
			}
			break;
		// update unit hp
		case NetChange::UPDATE_HP:
			{
				int id;
				float hpp;
				f >> id;
				f >> hpp;
				if(!f)
					Error("Update client: Broken UPDATE_HP.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: UPDATE_HP, missing unit %d.", id);
					else if(unit == pc->unit)
					{
						// handling of previous hp
						float hp_dif = hpp - unit->GetHpp();
						if(hp_dif < 0.f)
							pc->last_dmg += -hp_dif * unit->hpmax;
						unit->hp = hpp * unit->hpmax;
					}
					else
						unit->hp = hpp;
				}
			}
			break;
		// update unit mp
		case NetChange::UPDATE_MP:
			{
				int netid;
				float mpp;
				f >> netid;
				f >> mpp;
				if(!f)
					Error("Update client: Broken UPDATE_MP.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(netid);
					if(!unit)
						Error("Update client: UPDATE_MP, missing unit %d.", netid);
					else if(unit == pc->unit)
						unit->mp = mpp * unit->mpmax;
					else
						unit->mp = mpp;
				}
			}
			break;
		// update unit stamina
		case NetChange::UPDATE_STAMINA:
			{
				int netid;
				float staminap;
				f >> netid;
				f >> staminap;
				if(!f)
					Error("Update client: Broken UPDATE_STAMINA.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(netid);
					if(!unit)
						Error("Update client: UPDATE_STAMINA, missing unit %d.", netid);
					else if(unit == pc->unit)
						unit->stamina = staminap * unit->stamina_max;
					else
						unit->stamina = staminap;
				}
			}
			break;
		// spawn blood
		case NetChange::SPAWN_BLOOD:
			{
				byte type;
				Vec3 pos;
				f >> type;
				f >> pos;
				if(!f)
					Error("Update client: Broken SPAWN_BLOOD.");
				else if(game_state == GS_LEVEL)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tKrew[type];
					pe->emision_interval = 0.01f;
					pe->life = 5.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 10;
					pe->spawn_max = 15;
					pe->max_particles = 15;
					pe->pos = pos;
					pe->speed_min = Vec3(-1, 0, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
					pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
					pe->size = 0.3f;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 0.9f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 0;
					pe->Init();
					game_level->GetArea(pos).tmp->pes.push_back(pe);
				}
			}
			break;
		// play unit sound
		case NetChange::UNIT_SOUND:
			{
				int id;
				SOUND_ID sound_id;
				f >> id;
				f.ReadCasted<byte>(sound_id);
				if(!f)
					Error("Update client: Broken UNIT_SOUND.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: UNIT_SOUND, missing unit %d.", id);
					else if(Sound* sound = unit->GetSound(sound_id))
					{
						float dist;
						switch(sound_id)
						{
						default:
						case SOUND_SEE_ENEMY:
							dist = Unit::ALERT_SOUND_DIST;
							break;
						case SOUND_PAIN:
							dist = Unit::PAIN_SOUND_DIST;
							break;
						case SOUND_DEATH:
							dist = Unit::DIE_SOUND_DIST;
							break;
						case SOUND_ATTACK:
							dist = Unit::ATTACK_SOUND_DIST;
							break;
						case SOUND_TALK:
							dist = Unit::TALK_SOUND_DIST;
							break;
						}
						PlayAttachedSound(*unit, sound, dist);
					}
				}
			}
			break;
		// unit dies
		case NetChange::DIE:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken DIE.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: DIE, missing unit %d.", id);
					else
						unit->Die(nullptr);
				}
			}
			break;
		// unit falls on ground
		case NetChange::FALL:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken FALL.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: FALL, missing unit %d.", id);
					else
						unit->Fall();
				}
			}
			break;
		// unit drops item animation
		case NetChange::DROP_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken DROP_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: DROP_ITEM, missing unit %d.", id);
					else if(unit != pc->unit)
					{
						unit->action = A_ANIMATION;
						unit->mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
					}
				}
			}
			break;
		// spawn item on ground
		case NetChange::SPAWN_ITEM:
			{
				GroundItem* item = new GroundItem;
				if(!item->Read(f))
				{
					Error("Update client: Broken SPAWN_ITEM.");
					delete item;
				}
				else if(game_state != GS_LEVEL)
					delete item;
				else
				{
					PreloadItem(item->item);
					game_level->GetArea(item->pos).items.push_back(item);
				}
			}
			break;
		// unit picks up item
		case NetChange::PICKUP_ITEM:
			{
				int id;
				bool up_animation;
				f >> id;
				f >> up_animation;
				if(!f)
					Error("Update client: Broken PICKUP_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: PICKUP_ITEM, missing unit %d.", id);
					else if(unit != pc->unit)
					{
						unit->action = A_PICKUP;
						unit->animation = ANI_PLAY;
						unit->mesh_inst->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
					}
				}
			}
			break;
		// remove ground item (picked by someone)
		case NetChange::REMOVE_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken REMOVE_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					LevelArea* area;
					GroundItem* item = game_level->FindGroundItem(id, &area);
					if(!item)
						Error("Update client: REMOVE_ITEM, missing ground item %d.", id);
					else
					{
						RemoveElement(area->items, item);
						if(pc_data.before_player == BP_ITEM && pc_data.before_player_ptr.item == item)
							pc_data.before_player = BP_NONE;
						if(pc_data.picking_item_state == 1 && pc_data.picking_item == item)
							pc_data.picking_item_state = 2;
						else
							delete item;
					}
				}
			}
			break;
		// unit consume item
		case NetChange::CONSUME_ITEM:
			{
				int id;
				bool force;
				const Item* item;
				f >> id;
				bool ok = (ReadItemAndFind(f, item) > 0);
				f >> force;
				if(!f || !ok)
					Error("Update client: Broken CONSUME_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CONSUME_ITEM, missing unit %d.", id);
					else if(item->type != IT_CONSUMABLE)
						Error("Update client: CONSUME_ITEM, %s is not consumable.", item->id.c_str());
					else if(unit != pc->unit || force)
					{
						PreloadItem(item);
						unit->ConsumeItem(item->ToConsumable(), false, false);
					}
				}
			}
			break;
		// play hit sound
		case NetChange::HIT_SOUND:
			{
				Vec3 pos;
				MATERIAL_TYPE mat1, mat2;
				f >> pos;
				f.ReadCasted<byte>(mat1);
				f.ReadCasted<byte>(mat2);
				if(!f)
					Error("Update client: Broken HIT_SOUND.");
				else if(game_state == GS_LEVEL)
					sound_mgr->PlaySound3d(GetMaterialSound(mat1, mat2), pos, HIT_SOUND_DIST);
			}
			break;
		// unit get stunned
		case NetChange::STUNNED:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken STUNNED.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: STUNNED, missing unit %d.", id);
					else
					{
						unit->BreakAction();

						if(unit->action != A_POSITION)
							unit->action = A_PAIN;
						else
							unit->animation_state = 1;

						if(unit->mesh_inst->mesh->head.n_groups == 2)
						{
							unit->mesh_inst->frame_end_info2 = false;
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						}
						else
						{
							unit->mesh_inst->frame_end_info = false;
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
							unit->animation = ANI_PLAY;
						}
					}
				}
			}
			break;
		// create shooted arrow
		case NetChange::SHOOT_ARROW:
			{
				int id;
				Vec3 pos;
				float rotX, rotY, speedY, speed;
				f >> id;
				f >> pos;
				f >> rotY;
				f >> speedY;
				f >> rotX;
				f >> speed;
				if(!f)
					Error("Update client: Broken SHOOT_ARROW.");
				else if(game_state == GS_LEVEL)
				{
					Unit* owner;
					if(id == -1)
						owner = nullptr;
					else
					{
						owner = game_level->FindUnit(id);
						if(!owner)
						{
							Error("Update client: SHOOT_ARROW, missing unit %d.", id);
							break;
						}
					}

					LevelArea& area = game_level->GetArea(pos);

					Bullet& b = Add1(area.tmp->bullets);
					b.mesh = aArrow;
					b.pos = pos;
					b.start_pos = pos;
					b.rot = Vec3(rotX, rotY, 0);
					b.yspeed = speedY;
					b.owner = nullptr;
					b.pe = nullptr;
					b.remove = false;
					b.speed = speed;
					b.spell = nullptr;
					b.tex = nullptr;
					b.tex_size = 0.f;
					b.timer = ARROW_TIMER;
					b.owner = owner;

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
					tpe->Init(50);
					area.tmp->tpes.push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = Vec4(1, 1, 1, 0.5f);
					tpe2->color2 = Vec4(1, 1, 1, 0);
					tpe2->Init(50);
					area.tmp->tpes.push_back(tpe2);
					b.trail2 = tpe2;

					sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, ARROW_HIT_SOUND_DIST);
				}
			}
			break;
		// update team credit
		case NetChange::UPDATE_CREDIT:
			{
				byte count;
				f >> count;
				if(!f)
					Error("Update client: Broken UPDATE_CREDIT.");
				else if(game_state != GS_LEVEL)
				{
					f.Skip(sizeof(int) * 2 * count);
					if(!f)
						Error("Update client: Broken UPDATE_CREDIT(3).");
				}
				else
				{
					for(byte i = 0; i < count; ++i)
					{
						int id, credit;
						f >> id;
						f >> credit;
						if(!f)
							Error("Update client: Broken UPDATE_CREDIT(2).");
						else
						{
							Unit* unit = game_level->FindUnit(id);
							if(!unit)
								Error("Update client: UPDATE_CREDIT, missing unit %d.", id);
							else
							{
								if(unit->IsPlayer())
									unit->player->credit = credit;
								else
									unit->hero->credit = credit;
							}
						}
					}
				}
			}
			break;
		// unit playes idle animation
		case NetChange::IDLE:
			{
				int id;
				byte animation_index;
				f >> id;
				f >> animation_index;
				if(!f)
					Error("Update client: Broken IDLE.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: IDLE, missing unit %d.", id);
					else if(animation_index >= unit->data->idles->anims.size())
						Error("Update client: IDLE, invalid animation index %u (count %u).", animation_index, unit->data->idles->anims.size());
					else
					{
						unit->mesh_inst->Play(unit->data->idles->anims[animation_index].c_str(), PLAY_ONCE, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->mesh_inst->frame_end_info = false;
						unit->animation = ANI_IDLE;
					}
				}
			}
			break;
		// info about completing all unique quests
		case NetChange::ALL_QUESTS_COMPLETED:
			quest_mgr->unique_completed_show = true;
			break;
		// unit talks
		case NetChange::TALK:
			{
				int id, skip_id;
				byte animation;
				f >> id;
				f >> animation;
				f >> skip_id;
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken TALK.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: TALK, missing unit %d.", id);
					else
					{
						game_gui->level_gui->AddSpeechBubble(unit, text.c_str());
						unit->bubble->skip_id = skip_id;

						if(animation != 0)
						{
							unit->mesh_inst->Play(animation == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
							unit->animation = ANI_PLAY;
							unit->action = A_ANIMATION;
						}

						if(dialog_context.dialog_mode && dialog_context.talker == unit)
						{
							dialog_context.dialog_s_text = text;
							dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
							dialog_context.dialog_wait = 1.f;
							dialog_context.skip_id = skip_id;
						}
						else if(pc->action == PlayerAction::Talk && pc->action_unit == unit)
						{
							predialog = text;
							dialog_context.skip_id = skip_id;
						}
					}
				}
			}
			break;
		// show talk text at position
		case NetChange::TALK_POS:
			{
				Vec3 pos;
				f >> pos;
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken TALK_POS.");
				else if(game_state == GS_LEVEL)
					game_gui->level_gui->AddSpeechBubble(pos, text.c_str());
			}
			break;
		// change location state
		case NetChange::CHANGE_LOCATION_STATE:
			{
				byte location_index;
				f >> location_index;
				if(!f)
					Error("Update client: Broken CHANGE_LOCATION_STATE.");
				else if(!world->VerifyLocation(location_index))
					Error("Update client: CHANGE_LOCATION_STATE, invalid location %u.", location_index);
				else
				{
					Location* loc = world->GetLocation(location_index);
					loc->SetKnown();
				}
			}
			break;
		// add rumor to journal
		case NetChange::ADD_RUMOR:
			{
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken ADD_RUMOR.");
				else
				{
					game_gui->messages->AddGameMsg3(GMS_ADDED_RUMOR);
					game_gui->journal->GetRumors().push_back(text);
					game_gui->journal->NeedUpdate(Journal::Rumors);
				}
			}
			break;
		// hero tells his name
		case NetChange::TELL_NAME:
			{
				int id;
				bool set_name;
				f >> id;
				f >> set_name;
				if(set_name)
					f.ReadString1();
				if(!f)
					Error("Update client: Broken TELL_NAME.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: TELL_NAME, missing unit %d.", id);
					else if(!unit->IsHero())
						Error("Update client: TELL_NAME, unit %d (%s) is not a hero.", id, unit->data->id.c_str());
					else
					{
						unit->hero->know_name = true;
						if(set_name)
							unit->hero->name = f.GetStringBuffer();
					}
				}
			}
			break;
		// change unit hair color
		case NetChange::HAIR_COLOR:
			{
				int id;
				Vec4 hair_color;
				f >> id;
				f >> hair_color;
				if(!f)
					Error("Update client: Broken HAIR_COLOR.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: HAIR_COLOR, missing unit %d.", id);
					else if(unit->data->type != UNIT_TYPE::HUMAN)
						Error("Update client: HAIR_COLOR, unit %d (%s) is not human.", id, unit->data->id.c_str());
					else
						unit->human_data->hair_color = hair_color;
				}
			}
			break;
		// warp unit
		case NetChange::WARP:
			{
				int id;
				char area_id;
				Vec3 pos;
				float rot;
				f >> id;
				f >> area_id;
				f >> pos;
				f >> rot;

				if(!f)
				{
					Error("Update client: Broken WARP.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Unit* unit = game_level->FindUnit(id);
				if(!unit)
				{
					Error("Update client: WARP, missing unit %d.", id);
					break;
				}

				LevelArea* area = game_level->GetAreaById(area_id);
				if(!area)
				{
					Error("Update client: WARP, invalid area %d.", area_id);
					break;
				}

				unit->BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, false, true);

				LevelArea* old_area = unit->area;
				unit->area = area;
				unit->pos = pos;
				unit->rot = rot;

				unit->visual_pos = unit->pos;
				if(unit->interp)
					unit->interp->Reset(unit->pos, unit->rot);

				if(old_area != unit->area)
				{
					RemoveElement(old_area->units, unit);
					area->units.push_back(unit);
				}
				if(unit == pc->unit)
				{
					if(fallback_type == FALLBACK::WAIT_FOR_WARP)
						fallback_type = FALLBACK::NONE;
					else if(fallback_type == FALLBACK::ARENA)
					{
						pc->unit->frozen = FROZEN::ROTATE;
						fallback_type = FALLBACK::NONE;
					}
					else if(fallback_type == FALLBACK::ARENA_EXIT)
					{
						pc->unit->frozen = FROZEN::NO;
						fallback_type = FALLBACK::NONE;

						if(pc->unit->hp <= 0.f)
						{
							pc->unit->HealPoison();
							pc->unit->live_state = Unit::ALIVE;
							pc->unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
							pc->unit->mesh_inst->groups[0].speed = 1.f;
							pc->unit->action = A_ANIMATION;
							pc->unit->animation = ANI_PLAY;
						}
					}
					Net::PushChange(NetChange::WARP);
					interpolate_timer = 0.f;
					pc_data.rot_buf = 0.f;
					game_level->camera.Reset();
					pc_data.rot_buf = 0.f;
				}
			}
			break;
		// register new item
		case NetChange::REGISTER_ITEM:
			{
				const string& item_id = f.ReadString1();
				if(!f)
					Error("Update client: Broken REGISTER_ITEM.");
				else
				{
					const Item* base;
					base = Item::TryGet(item_id);
					if(!base)
					{
						Error("Update client: REGISTER_ITEM, missing base item %s.", item_id.c_str());
						f.SkipString1(); // id
						f.SkipString1(); // name
						f.SkipString1(); // desc
						f.Skip<int>(); // ref
						if(!f)
							Error("Update client: Broken REGISTER_ITEM(2).");
					}
					else
					{
						Item* item = base->CreateCopy();
						f >> item->id;
						f >> item->name;
						f >> item->desc;
						f >> item->quest_id;
						if(!f)
							Error("Update client: Broken REGISTER_ITEM(3).");
						else
							quest_mgr->quest_items.push_back(item);
					}
				}
			}
			break;
		// added quest
		case NetChange::ADD_QUEST:
			{
				int id;
				f >> id;
				const string& quest_name = f.ReadString1();
				if(!f)
				{
					Error("Update client: Broken ADD_QUEST.");
					break;
				}

				PlaceholderQuest* quest = new PlaceholderQuest;
				quest->quest_index = quest_mgr->quests.size();
				quest->name = quest_name;
				quest->id = id;
				quest->msgs.resize(2);

				f.ReadString2(quest->msgs[0]);
				f.ReadString2(quest->msgs[1]);
				if(!f)
				{
					Error("Update client: Broken ADD_QUEST(2).");
					delete quest;
					break;
				}

				quest->state = Quest::Started;
				game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
				game_gui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
				quest_mgr->quests.push_back(quest);
			}
			break;
		// update quest
		case NetChange::UPDATE_QUEST:
			{
				int id;
				byte state, count;
				f >> id;
				f >> state;
				f >> count;
				if(!f)
				{
					Error("Update client: Broken UPDATE_QUEST.");
					break;
				}

				Quest* quest = quest_mgr->FindQuest(id, false);
				if(!quest)
				{
					Error("Update client: UPDATE_QUEST, missing quest %d.", id);
					f.SkipStringArray<byte, word>();
					if(!f)
						Error("Update client: Broken UPDATE_QUEST(2).");
				}
				else
				{
					quest->state = (Quest::State)state;
					for(byte i = 0; i < count; ++i)
					{
						f.ReadString2(Add1(quest->msgs));
						if(!f)
						{
							Error("Update client: Broken UPDATE_QUEST(3) on index %u.", i);
							quest->msgs.pop_back();
							break;
						}
					}
					game_gui->journal->NeedUpdate(Journal::Quests, quest->quest_index);
					game_gui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
				}
			}
			break;
		// item rename
		case NetChange::RENAME_ITEM:
			{
				int quest_id;
				f >> quest_id;
				const string& item_id = f.ReadString1();
				if(!f)
					Error("Update client: Broken RENAME_ITEM.");
				else
				{
					bool found = false;
					for(Item* item : quest_mgr->quest_items)
					{
						if(item->quest_id == quest_id && item->id == item_id)
						{
							f >> item->name;
							if(!f)
								Error("Update client: Broken RENAME_ITEM(2).");
							found = true;
							break;
						}
					}
					if(!found)
					{
						Error("Update client: RENAME_ITEM, missing quest item %d.", quest_id);
						f.SkipString1();
					}
				}
			}
			break;
		// change leader notification
		case NetChange::CHANGE_LEADER:
			{
				byte id;
				f >> id;
				if(!f)
					Error("Update client: Broken CHANGE_LEADER.");
				else
				{
					PlayerInfo* info = net->TryGetPlayer(id);
					if(info)
					{
						team->leader_id = id;
						if(team->leader_id == team->my_id)
							AddMsg(txYouAreLeader);
						else
							AddMsg(Format(txPcIsLeader, info->name.c_str()));
						team->leader = info->u;

						if(game_gui->world_map->dialog_enc)
							game_gui->world_map->dialog_enc->bts[0].state = (team->IsLeader() ? Button::NONE : Button::DISABLED);
					}
					else
						Error("Update client: CHANGE_LEADER, missing player %u.", id);
				}
			}
			break;
		// player get random number
		case NetChange::RANDOM_NUMBER:
			{
				byte player_id, number;
				f >> player_id;
				f >> number;
				if(!f)
					Error("Update client: Broken RANDOM_NUMBER.");
				else if(player_id != team->my_id)
				{
					PlayerInfo* info = net->TryGetPlayer(player_id);
					if(info)
						AddMsg(Format(txRolledNumber, info->name.c_str(), number));
					else
						Error("Update client: RANDOM_NUMBER, missing player %u.", player_id);
				}
			}
			break;
		// remove player from game
		case NetChange::REMOVE_PLAYER:
			{
				byte player_id;
				PlayerInfo::LeftReason reason;
				f >> player_id;
				f.ReadCasted<byte>(reason);
				if(!f)
					Error("Update client: Broken REMOVE_PLAYER.");
				else
				{
					PlayerInfo* info = net->TryGetPlayer(player_id);
					if(!info)
						Error("Update client: REMOVE_PLAYER, missing player %u.", player_id);
					else if(player_id != team->my_id)
					{
						info->left = reason;
						RemovePlayer(*info);
						net->players.erase(info->GetIndex());
						delete info;
					}
				}
			}
			break;
		// unit uses usable object
		case NetChange::USE_USABLE:
			{
				int id, usable_id;
				USE_USABLE_STATE state;
				f >> id;
				f >> usable_id;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					Error("Update client: Broken USE_USABLE.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Unit* unit = game_level->FindUnit(id);
				if(!unit)
				{
					Error("Update client: USE_USABLE, missing unit %d.", id);
					break;
				}

				Usable* usable = game_level->FindUsable(usable_id);
				if(!usable)
				{
					Error("Update client: USE_USABLE, missing usable %d.", usable_id);
					break;
				}

				BaseUsable& base = *usable->base;
				if(state == USE_USABLE_START || state == USE_USABLE_START_SPECIAL)
				{
					if(!IsSet(base.use_flags, BaseUsable::CONTAINER))
					{
						unit->action = A_ANIMATION2;
						unit->animation = ANI_PLAY;
						unit->mesh_inst->Play(state == USE_USABLE_START_SPECIAL ? "czyta_papiery" : base.anim.c_str(), PLAY_PRIO1, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->target_pos = unit->pos;
						unit->target_pos2 = usable->pos;
						if(base.limit_rot == 4)
							unit->target_pos2 -= Vec3(sin(usable->rot)*1.5f, 0, cos(usable->rot)*1.5f);
						unit->timer = 0.f;
						unit->animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
						unit->use_rot = Vec3::LookAtAngle(unit->pos, usable->pos);
						unit->used_item = base.item;
						if(unit->used_item)
						{
							PreloadItem(unit->used_item);
							unit->weapon_taken = W_NONE;
							unit->weapon_state = WS_HIDDEN;
						}
					}
					else
						unit->action = A_NONE;

					unit->UseUsable(usable);
					if(pc_data.before_player == BP_USABLE && pc_data.before_player_ptr.usable == usable)
						pc_data.before_player = BP_NONE;
				}
				else
				{
					if(unit->player != pc && !IsSet(base.use_flags, BaseUsable::CONTAINER))
					{
						unit->action = A_NONE;
						unit->animation = ANI_STAND;
						if(unit->live_state == Unit::ALIVE)
							unit->used_item = nullptr;
					}

					if(usable->user != unit)
					{
						Error("Update client: USE_USABLE, unit %d not using %d.", id, usable_id);
						unit->usable = nullptr;
					}
					else if(state == USE_USABLE_END)
						unit->UseUsable(nullptr);
				}
			}
			break;
		// unit stands up
		case NetChange::STAND_UP:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken STAND_UP.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: STAND_UP, missing unit %d.", id);
					else
						unit->Standup();
				}
			}
			break;
		// game over
		case NetChange::GAME_OVER:
			Info("Update client: Game over - all players died.");
			SetMusic(MusicType::Death);
			game_gui->CloseAllPanels(true);
			++death_screen;
			death_fade = 0;
			death_solo = false;
			exit_from_server = true;
			break;
		// recruit npc to team
		case NetChange::RECRUIT_NPC:
			{
				int id;
				bool free;
				f >> id;
				f >> free;
				if(!f)
					Error("Update client: Broken RECRUIT_NPC.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: RECRUIT_NPC, missing unit %d.", id);
					else if(!unit->IsHero())
						Error("Update client: RECRUIT_NPC, unit %d (%s) is not a hero.", id, unit->data->id.c_str());
					else
					{
						unit->hero->team_member = true;
						unit->hero->type = free ? HeroType::Visitor : HeroType::Normal;
						unit->hero->credit = 0;
						team->members.push_back(unit);
						if(!free)
							team->active_members.push_back(unit);
						if(game_gui->team->visible)
							game_gui->team->Changed();
					}
				}
			}
			break;
		// kick npc out of team
		case NetChange::KICK_NPC:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken KICK_NPC.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: KICK_NPC, missing unit %d.", id);
					else if(!unit->IsHero() || !unit->hero->team_member)
						Error("Update client: KICK_NPC, unit %d (%s) is not a team member.", id, unit->data->id.c_str());
					else
					{
						unit->hero->team_member = false;
						RemoveElementOrder(team->members.ptrs, unit);
						if(unit->hero->type == HeroType::Normal)
							RemoveElementOrder(team->active_members.ptrs, unit);
						if(game_gui->team->visible)
							game_gui->team->Changed();
					}
				}
			}
			break;
		// remove unit from game
		case NetChange::REMOVE_UNIT:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken REMOVE_UNIT.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: REMOVE_UNIT, missing unit %d.", id);
					else
						game_level->RemoveUnit(unit);
				}
			}
			break;
		// spawn new unit
		case NetChange::SPAWN_UNIT:
			{
				Unit* unit = new Unit;
				if(!unit->Read(f))
				{
					Error("Update client: Broken SPAWN_UNIT.");
					delete unit;
				}
				else if(game_state == GS_LEVEL)
				{
					LevelArea& area = game_level->GetArea(unit->pos);
					area.units.push_back(unit);
					unit->area = &area;
					if(unit->summoner)
						SpawnUnitEffect(*unit);
				}
				else
					delete unit;
			}
			break;
		// change unit arena state
		case NetChange::CHANGE_ARENA_STATE:
			{
				int id;
				char state;
				f >> id;
				f >> state;
				if(!f)
					Error("Update client: Broken CHANGE_ARENA_STATE.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CHANGE_ARENA_STATE, missing unit %d.", id);
					else
					{
						if(state < -1 || state > 1)
							state = -1;
						unit->in_arena = state;
						if(unit == pc->unit && state >= 0)
							pc->RefreshCooldown();
					}
				}
			}
			break;
		// plays arena sound
		case NetChange::ARENA_SOUND:
			{
				byte type;
				f >> type;
				if(!f)
					Error("Update client: Broken ARENA_SOUND.");
				else if(game_state == GS_LEVEL && game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveArena)
					&& game_level->GetArena() == pc->unit->area)
				{
					Sound* sound;
					if(type == 0)
						sound = sArenaFight;
					else if(type == 1)
						sound = sArenaWin;
					else
						sound = sArenaLost;
					sound_mgr->PlaySound2d(sound);
				}
			}
			break;
		// leaving notification
		case NetChange::LEAVE_LOCATION:
			if(game_state == GS_LEVEL)
			{
				fallback_type = FALLBACK::WAIT_FOR_WARP;
				fallback_t = -1.f;
				pc->unit->frozen = FROZEN::YES;
			}
			break;
		// exit to map
		case NetChange::EXIT_TO_MAP:
			if(game_state == GS_LEVEL)
				ExitToMap();
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					Error("Update client: Broken TRAVEL.");
				else if(game_state == GS_WORLDMAP)
				{
					world->Travel(loc, false);
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
					Error("Update client: Broken TRAVEL_POS.");
				else if(game_state == GS_WORLDMAP)
				{
					world->TravelPos(pos, false);
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
					Error("Update client: Broken STOP_TRAVEL.");
				else if(game_state == GS_WORLDMAP)
					world->StopTravel(pos, false);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game_state == GS_WORLDMAP)
				world->EndTravel();
			break;
		// change world time
		case NetChange::WORLD_TIME:
			world->ReadTime(f);
			if(!f)
				Error("Update client: Broken WORLD_TIME.");
			break;
		// someone open/close door
		case NetChange::USE_DOOR:
			{
				int id;
				bool is_closing;
				f >> id;
				f >> is_closing;
				if(!f)
					Error("Update client: Broken USE_DOOR.");
				else if(game_state == GS_LEVEL)
				{
					Door* door = game_level->FindDoor(id);
					if(!door)
						Error("Update client: USE_DOOR, missing door %d.", id);
					else
					{
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
								sound = sDoorClose;
							else
								sound = sDoor[Rand() % 3];
							sound_mgr->PlaySound3d(sound, door->GetCenter(), Door::SOUND_DIST);
						}
					}
				}
			}
			break;
		// create explosion effect
		case NetChange::CREATE_EXPLOSION:
			{
				Vec3 pos;
				const string& spell_id = f.ReadString1();
				f >> pos;
				if(!f)
				{
					Error("Update client: Broken CREATE_EXPLOSION.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Spell* spell_ptr = Spell::TryGet(spell_id);
				if(!spell_ptr)
				{
					Error("Update client: CREATE_EXPLOSION, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Spell& spell = *spell_ptr;
				if(!IsSet(spell.flags, Spell::Explode))
				{
					Error("Update client: CREATE_EXPLOSION, spell '%s' is not explosion.", spell_id.c_str());
					break;
				}

				Explo* explo = new Explo;
				explo->pos = pos;
				explo->size = 0.f;
				explo->sizemax = 2.f;
				explo->tex = spell.tex_explode;

				sound_mgr->PlaySound3d(spell.sound_hit, explo->pos, spell.sound_hit_dist);

				game_level->GetArea(pos).tmp->explos.push_back(explo);
			}
			break;
		// remove trap
		case NetChange::REMOVE_TRAP:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken REMOVE_TRAP.");
				else  if(game_state == GS_LEVEL)
				{
					if(!game_level->RemoveTrap(id))
						Error("Update client: REMOVE_TRAP, missing trap %d.", id);
				}
			}
			break;
		// trigger trap
		case NetChange::TRIGGER_TRAP:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken TRIGGER_TRAP.");
				else if(game_state == GS_LEVEL)
				{
					Trap* trap = game_level->FindTrap(id);
					if(trap)
						trap->trigger = true;
					else
						Error("Update client: TRIGGER_TRAP, missing trap %d.", id);
				}
			}
			break;
		// play evil sound
		case NetChange::EVIL_SOUND:
			sound_mgr->PlaySound2d(sEvil);
			break;
		// start encounter on world map
		case NetChange::ENCOUNTER:
			{
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken ENCOUNTER.");
				else if(game_state == GS_WORLDMAP)
				{
					DialogInfo info;
					info.event = [this](int)
					{
						game_gui->world_map->dialog_enc = nullptr;
						Net::PushChange(NetChange::CLOSE_ENCOUNTER);
					};
					info.name = "encounter";
					info.order = ORDER_TOP;
					info.parent = nullptr;
					info.pause = true;
					info.type = DIALOG_OK;
					info.text = text;

					game_gui->world_map->dialog_enc = gui->ShowDialog(info);
					if(!team->IsLeader())
						game_gui->world_map->dialog_enc->bts[0].state = Button::DISABLED;
					assert(world->GetState() == World::State::TRAVEL);
					world->SetState(World::State::ENCOUNTER);
				}
			}
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(game_state == GS_WORLDMAP && game_gui->world_map->dialog_enc)
			{
				gui->CloseDialog(game_gui->world_map->dialog_enc);
				game_gui->world_map->dialog_enc = nullptr;
			}
			break;
		// close portal in location
		case NetChange::CLOSE_PORTAL:
			if(game_state == GS_LEVEL)
			{
				if(game_level->location && game_level->location->portal)
				{
					delete game_level->location->portal;
					game_level->location->portal = nullptr;
				}
				else
					Error("Update client: CLOSE_PORTAL, missing portal.");
			}
			break;
		// clean altar in evil quest
		case NetChange::CLEAN_ALTAR:
			if(game_state == GS_LEVEL)
			{
				// change object
				BaseObject* base_obj = BaseObject::Get("bloody_altar");
				Object* obj = game_level->local_area->FindObject(base_obj);
				obj->base = BaseObject::Get("altar");
				obj->mesh = obj->base->mesh;
				res_mgr->Load(obj->mesh);

				// remove particles
				float best_dist = 999.f;
				ParticleEmitter* best_pe = nullptr;
				for(ParticleEmitter* pe : game_level->local_area->tmp->pes)
				{
					if(pe->tex == tKrew[BLOOD_RED])
					{
						float dist = Vec3::Distance(pe->pos, obj->pos);
						if(dist < best_dist)
						{
							best_dist = dist;
							best_pe = pe;
						}
					}
				}
				assert(best_pe);
				best_pe->destroy = true;
			}
			break;
		// add new location
		case NetChange::ADD_LOCATION:
			{
				byte location_index;
				LOCATION type;
				f >> location_index;
				f.ReadCasted<byte>(type);
				if(!f)
				{
					Error("Update client: Broken ADD_LOCATION.");
					break;
				}

				Location* loc;
				if(type == L_DUNGEON || type == L_CRYPT)
				{
					byte levels;
					f >> levels;
					if(!f)
					{
						Error("Update client: Broken ADD_LOCATION(2).");
						break;
					}
					if(levels == 1)
						loc = new SingleInsideLocation;
					else
						loc = new MultiInsideLocation(levels);
				}
				else if(type == L_CAVE)
					loc = new Cave;
				else
					loc = new OutsideLocation;
				loc->type = type;
				loc->index = location_index;

				f.ReadCasted<byte>(loc->state);
				f >> loc->pos;
				f >> loc->name;
				f.ReadCasted<byte>(loc->image);
				if(!f)
				{
					Error("Update client: Broken ADD_LOCATION(3).");
					delete loc;
					break;
				}

				world->AddLocationAtIndex(loc);
			}
			break;
		// remove camp
		case NetChange::REMOVE_CAMP:
			{
				byte camp_index;
				f >> camp_index;
				if(!f)
					Error("Update client: Broken REMOVE_CAMP.");
				else if(!world->VerifyLocation(camp_index) || world->GetLocation(camp_index)->type != L_CAMP)
					Error("Update client: REMOVE_CAMP, invalid location %u.", camp_index);
				else
					world->RemoveLocation(camp_index);
			}
			break;
		// change unit ai mode
		case NetChange::CHANGE_AI_MODE:
			{
				int id;
				byte mode;
				f >> id;
				f >> mode;
				if(!f)
					Error("Update client: Broken CHANGE_AI_MODE.");
				else
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CHANGE_AI_MODE, missing unit %d.", id);
					else
						unit->ai_mode = mode;
				}
			}
			break;
		// change unit base type
		case NetChange::CHANGE_UNIT_BASE:
			{
				int id;
				f >> id;
				const string& unit_id = f.ReadString1();
				if(!f)
					Error("Update client: Broken CHANGE_UNIT_BASE.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					UnitData* ud = UnitData::TryGet(unit_id);
					if(unit && ud)
						unit->data = ud;
					else if(!unit)
						Error("Update client: CHANGE_UNIT_BASE, missing unit %d.", id);
					else
						Error("Update client: CHANGE_UNIT_BASE, missing base unit '%s'.", unit_id.c_str());
				}
			}
			break;
		// unit casts spell
		case NetChange::CAST_SPELL:
			{
				int id;
				byte attack_id;
				f >> id;
				f >> attack_id;
				if(!f)
					Error("Update client: Broken CAST_SPELL.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CAST_SPELL, missing unit %d.", id);
					else
					{
						unit->action = A_CAST;
						unit->attack_id = attack_id;
						unit->animation_state = 0;

						if(unit->mesh_inst->mesh->head.n_groups == 2)
						{
							unit->mesh_inst->frame_end_info2 = false;
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
						}
						else
						{
							unit->mesh_inst->frame_end_info = false;
							unit->animation = ANI_PLAY;
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
							unit->mesh_inst->groups[0].speed = 1.f;
						}
					}
				}
			}
			break;
		// create ball - spell effect
		case NetChange::CREATE_SPELL_BALL:
			{
				int id;
				Vec3 pos;
				float rotY, speedY;
				const string& spell_id = f.ReadString1();
				f >> pos;
				f >> rotY;
				f >> speedY;
				f >> id;
				if(!f)
				{
					Error("Update client: Broken CREATE_SPELL_BALL.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Spell* spell_ptr = Spell::TryGet(spell_id);
				if(!spell_ptr)
				{
					Error("Update client: CREATE_SPELL_BALL, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Unit* unit = nullptr;
				if(id != -1)
				{
					unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CREATE_SPELL_BALL, missing unit %d.", id);
				}

				Spell& spell = *spell_ptr;
				LevelArea& area = game_level->GetArea(pos);

				Bullet& b = Add1(area.tmp->bullets);

				b.pos = pos;
				b.rot = Vec3(0, rotY, 0);
				b.mesh = spell.mesh;
				b.tex = spell.tex;
				b.tex_size = spell.size;
				b.speed = spell.speed;
				b.timer = spell.range / (spell.speed - 1);
				b.remove = false;
				b.trail = nullptr;
				b.trail2 = nullptr;
				b.pe = nullptr;
				b.spell = &spell;
				b.start_pos = b.pos;
				b.owner = unit;
				b.yspeed = speedY;

				if(spell.tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.1f;
					pe->life = -1;
					pe->particle_life = 0.5f;
					pe->emisions = -1;
					pe->spawn_min = 3;
					pe->spawn_max = 4;
					pe->max_particles = 50;
					pe->pos = b.pos;
					pe->speed_min = Vec3(-1, -1, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					area.tmp->pes.push_back(pe);
					b.pe = pe;
				}
			}
			break;
		// play spell sound
		case NetChange::SPELL_SOUND:
			{
				Vec3 pos;
				const string& spell_id = f.ReadString1();
				f >> pos;
				if(!f)
				{
					Error("Update client: Broken SPELL_SOUND.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Spell* spell_ptr = Spell::TryGet(spell_id);
				if(!spell_ptr)
				{
					Error("Update client: SPELL_SOUND, missing spell '%s'.", spell_id.c_str());
					break;
				}

				Spell& spell = *spell_ptr;
				sound_mgr->PlaySound3d(spell.sound_cast, pos, spell.sound_cast_dist);
			}
			break;
		// drain blood effect
		case NetChange::CREATE_DRAIN:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken CREATE_DRAIN.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CREATE_DRAIN, missing unit %d.", id);
					else
					{
						TmpLevelArea& tmp_area = *unit->area->tmp;
						if(tmp_area.pes.empty())
							Error("Update client: CREATE_DRAIN, missing blood.");
						else
						{
							Drain& drain = Add1(tmp_area.drains);
							drain.from = nullptr;
							drain.to = unit;
							drain.pe = tmp_area.pes.back();
							drain.t = 0.f;
							drain.pe->manual_delete = 1;
							drain.pe->speed_min = Vec3(-3, 0, -3);
							drain.pe->speed_max = Vec3(3, 3, 3);
						}
					}
				}
			}
			break;
		// create electro effect
		case NetChange::CREATE_ELECTRO:
			{
				int id;
				Vec3 p1, p2;
				f >> id;
				f >> p1;
				f >> p2;
				if(!f)
					Error("Update client: Broken CREATE_ELECTRO.");
				else if(game_state == GS_LEVEL)
				{
					Electro* e = new Electro;
					e->id = id;
					e->Register();
					e->spell = Spell::TryGet("thunder_bolt");
					e->start_pos = p1;
					e->AddLine(p1, p2);
					e->valid = true;
					game_level->GetArea(p1).tmp->electros.push_back(e);
				}
			}
			break;
		// update electro effect
		case NetChange::UPDATE_ELECTRO:
			{
				int id;
				Vec3 pos;
				f >> id;
				f >> pos;
				if(!f)
					Error("Update client: Broken UPDATE_ELECTRO.");
				else if(game_state == GS_LEVEL)
				{
					Electro* e = game_level->FindElectro(id);
					if(!e)
						Error("Update client: UPDATE_ELECTRO, missing electro %d.", id);
					else
					{
						Vec3 from = e->lines.back().pts.back();
						e->AddLine(from, pos);
					}
				}
			}
			break;
		// electro hit effect
		case NetChange::ELECTRO_HIT:
			{
				Vec3 pos;
				f >> pos;
				if(!f)
					Error("Update client: Broken ELECTRO_HIT.");
				else if(game_state == GS_LEVEL)
				{
					Spell* spell = Spell::TryGet("thunder_bolt");

					// sound
					if(spell->sound_hit)
						sound_mgr->PlaySound3d(spell->sound_hit, pos, spell->sound_hit_dist);

					// particles
					if(spell->tex_particle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = spell->tex_particle;
						pe->emision_interval = 0.01f;
						pe->life = 0.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 8;
						pe->spawn_max = 12;
						pe->max_particles = 12;
						pe->pos = pos;
						pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
						pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
						pe->pos_min = Vec3(-spell->size, -spell->size, -spell->size);
						pe->pos_max = Vec3(spell->size, spell->size, spell->size);
						pe->size = spell->size_particle;
						pe->op_size = POP_LINEAR_SHRINK;
						pe->alpha = 1.f;
						pe->op_alpha = POP_LINEAR_SHRINK;
						pe->mode = 1;
						pe->Init();

						game_level->GetArea(pos).tmp->pes.push_back(pe);
					}
				}
			}
			break;
		// raise spell effect
		case NetChange::RAISE_EFFECT:
			{
				Vec3 pos;
				f >> pos;
				if(!f)
					Error("Update client: Broken RAISE_EFFECT.");
				else if(game_state == GS_LEVEL)
				{
					Spell& spell = *Spell::TryGet("raise");

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = pos;
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();

					game_level->GetArea(pos).tmp->pes.push_back(pe);
				}
			}
			break;
		// heal spell effect
		case NetChange::HEAL_EFFECT:
			{
				Vec3 pos;
				f >> pos;
				if(!f)
					Error("Update client: Broken HEAL_EFFECT.");
				else if(game_state == GS_LEVEL)
				{
					Spell& spell = *Spell::TryGet("heal");

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = pos;
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();

					game_level->GetArea(pos).tmp->pes.push_back(pe);
				}
			}
			break;
		// someone used cheat 'reveal_minimap'
		case NetChange::CHEAT_REVEAL_MINIMAP:
			game_level->RevealMinimap();
			break;
		// revealing minimap
		case NetChange::REVEAL_MINIMAP:
			{
				word count;
				f >> count;
				if(!f.Ensure(count * sizeof(byte) * 2))
					Error("Update client: Broken REVEAL_MINIMAP.");
				else if(game_state == GS_LEVEL)
				{
					for(word i = 0; i < count; ++i)
					{
						byte x, y;
						f.Read(x);
						f.Read(y);
						game_level->minimap_reveal.push_back(Int2(x, y));
					}
				}
			}
			break;
		// 'noai' cheat change
		case NetChange::CHEAT_NOAI:
			{
				bool state;
				f >> state;
				if(!f)
					Error("Update client: Broken CHEAT_NOAI.");
				else
					noai = state;
			}
			break;
		// end of game, time run out
		case NetChange::END_OF_GAME:
			Info("Update client: Game over - time run out.");
			game_gui->CloseAllPanels(true);
			end_of_game = true;
			death_fade = 0.f;
			exit_from_server = true;
			break;
		// update players free days
		case NetChange::UPDATE_FREE_DAYS:
			{
				byte count;
				f >> count;
				if(!f.Ensure(sizeof(int) * 2 * count))
					Error("Update client: Broken UPDATE_FREE_DAYS.");
				else
				{
					for(byte i = 0; i < count; ++i)
					{
						int id, days;
						f >> id;
						f >> days;

						Unit* unit = game_level->FindUnit(id);
						if(!unit || !unit->IsPlayer())
						{
							Error("Update client: UPDATE_FREE_DAYS, missing unit %d or is not a player (0x%p).", id, unit);
							f.Skip(sizeof(int) * 2 * (count - i - 1));
							break;
						}

						unit->player->free_days = days;
					}
				}
			}
			break;
		// multiplayer vars changed
		case NetChange::CHANGE_MP_VARS:
			net->ReadNetVars(f);
			if(!f)
				Error("Update client: Broken CHANGE_MP_VARS.");
			break;
		// game saved notification
		case NetChange::GAME_SAVED:
			AddMultiMsg(txGameSaved);
			game_gui->messages->AddGameMsg3(GMS_GAME_SAVED);
			break;
		// ai left team due too many team members
		case NetChange::HERO_LEAVE:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken HERO_LEAVE.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: HERO_LEAVE, missing unit %d.", id);
					else
						AddMultiMsg(Format(txMpNPCLeft, unit->GetName()));
				}
			}
			break;
		// game paused/resumed
		case NetChange::PAUSED:
			{
				bool is_paused;
				f >> is_paused;
				if(!f)
					Error("Update client: Broken PAUSED.");
				else
				{
					paused = is_paused;
					AddMultiMsg(paused ? txGamePaused : txGameResumed);
				}
			}
			break;
		// secret letter text update
		case NetChange::SECRET_TEXT:
			f >> Quest_Secret::GetNote().desc;
			if(!f)
				Error("Update client: Broken SECRET_TEXT.");
			break;
		// update position on world map
		case NetChange::UPDATE_MAP_POS:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					Error("Update client: Broken UPDATE_MAP_POS.");
				else
					world->SetWorldPos(pos);
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte location_index;
				f >> location_index;
				if(!f)
					Error("Update client: Broken CHEAT_TRAVEL.");
				else if(!world->VerifyLocation(location_index))
					Error("Update client: CHEAT_TRAVEL, invalid location index %u.", location_index);
				else if(game_state == GS_WORLDMAP)
				{
					world->Warp(location_index);
					game_gui->world_map->StartTravel();
				}
			}
			break;
		// player used cheat for fast travel to pos on map
		case NetChange::CHEAT_TRAVEL_POS:
			{
				Vec2 pos;
				f >> pos;
				if(!f)
					Error("Update client: Broken CHEAT_TRAVEL_POS.");
				else if(game_state == GS_WORLDMAP)
				{
					world->WarpPos(pos);
					game_gui->world_map->StartTravel();
				}
			}
			break;
		// remove used item from unit hand
		case NetChange::REMOVE_USED_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken REMOVE_USED_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: REMOVE_USED_ITEM, missing unit %d.", id);
					else
						unit->used_item = nullptr;
				}
			}
			break;
		// game stats showed at end of game
		case NetChange::GAME_STATS:
			f >> game_stats->total_kills;
			if(!f)
				Error("Update client: Broken GAME_STATS.");
			break;
		// play usable object sound for unit
		case NetChange::USABLE_SOUND:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken USABLE_SOUND.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: USABLE_SOUND, missing unit %d.", id);
					else if(!unit->usable)
						Error("Update client: USABLE_SOUND, unit %d (%s) don't use usable.", id, unit->data->id.c_str());
					else if(unit != pc->unit)
						sound_mgr->PlaySound3d(unit->usable->base->sound, unit->GetCenter(), Usable::SOUND_DIST);
				}
			}
			break;
		// break unit action
		case NetChange::BREAK_ACTION:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken BREAK_ACTION.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(unit)
						unit->BreakAction();
					else
						Error("Update client: BREAK_ACTION, missing unit %d.", id);
				}
			}
			break;
		// player used action
		case NetChange::PLAYER_ACTION:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken PLAYER_ACTION.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(unit && unit->player)
					{
						if(unit->player != pc)
							UseAction(unit->player, true);
					}
					else
						Error("Update client: PLAYER_ACTION, invalid player unit %d.", id);
				}
			}
			break;
		// unit stun - not shield bash
		case NetChange::STUN:
			{
				int id;
				float length;
				f >> id;
				f >> length;
				if(!f)
					Error("Update client: Broken STUN.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: STUN, missing unit %d.", id);
					else
					{
						if(length > 0)
							unit->ApplyStun(length);
						else
							unit->RemoveEffect(EffectId::Stun);
					}
				}
			}
			break;
		// player used item
		case NetChange::USE_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken USE_ITEM.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: USE_ITEM, missing unit %d.", id);
					else
					{
						unit->action = A_USE_ITEM;
						unit->mesh_inst->frame_end_info2 = false;
						unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
					}
				}
			}
			break;
		// mark unit
		case NetChange::MARK_UNIT:
			{
				int id;
				bool mark;
				f >> id;
				f >> mark;
				if(!f)
					Error("Update client: Broken MARK_UNIT.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: MARK_UNIT, missing unit %d.", id);
					else
						unit->mark = mark;
				}
			}
			break;
		// someone used cheat 'clean_level'
		case NetChange::CLEAN_LEVEL:
			{
				int building_id;
				f >> building_id;
				if(!f)
					Error("Update client: Broken CLEAN_LEVEL.");
				else if(game_state == GS_LEVEL)
					game_level->CleanLevel(building_id);
			}
			break;
		// change location image
		case NetChange::CHANGE_LOCATION_IMAGE:
			{
				int index;
				LOCATION_IMAGE image;
				f.ReadCasted<byte>(index);
				f.ReadCasted<byte>(image);
				if(!f)
					Error("Update client: Broken CHANGE_LOCATION_IMAGE.");
				else if(!world->VerifyLocation(index))
					Error("Update client: CHANGE_LOCATION_IMAGE, invalid location %d.", index);
				else
					world->GetLocation(index)->SetImage(image);
			}
			break;
		// change location name
		case NetChange::CHANGE_LOCATION_NAME:
			{
				int index;
				f.ReadCasted<byte>(index);
				const string& name = f.ReadString1();
				if(!f)
					Error("Update client: Broken CHANGE_LOCATION_NAME.");
				else if(!world->VerifyLocation(index))
					Error("Update client: CHANGE_LOCATION_NAME, invalid location %d.", index);
				else
					world->GetLocation(index)->SetName(name.c_str());
			}
			break;
		// unit uses chest
		case NetChange::USE_CHEST:
			{
				int chest_id, unit_id;
				f >> chest_id;
				f >> unit_id;
				if(!f)
				{
					Error("Update client: Broken USE_CHEST.");
					break;
				}
				Chest* chest = game_level->FindChest(chest_id);
				if(!chest)
					Error("Update client: USE_CHEST, missing chest %d.", chest_id);
				else if(unit_id == -1)
					chest->OpenClose(nullptr);
				else
				{
					Unit* unit = game_level->FindUnit(unit_id);
					if(!unit)
						Error("Update client: USE_CHEST, missing unit %d.", unit_id);
					else
						chest->OpenClose(unit);
				}
			}
			break;
		// fast travel request/response
		case NetChange::FAST_TRAVEL:
			{
				FAST_TRAVEL option;
				byte id;
				f.ReadCasted<byte>(option);
				f >> id;
				if(!f)
				{
					Error("Update client: Broken FAST_TRAVEL.");
					break;
				}
				switch(option)
				{
				case FAST_TRAVEL_START:
					net->StartFastTravel(2);
					break;
				case FAST_TRAVEL_CANCEL:
				case FAST_TRAVEL_DENY:
				case FAST_TRAVEL_NOT_SAFE:
				case FAST_TRAVEL_IN_PROGRESS:
					net->CancelFastTravel(option, id);
					break;
				}
			}
			break;
		// update player vote for fast travel
		case NetChange::FAST_TRAVEL_VOTE:
			{
				byte id;
				f >> id;
				if(!f)
				{
					Error("Update client: Broken FAST_TRAVEL_VOTE.");
					break;
				}
				PlayerInfo* info = net->TryGetPlayer(id);
				if(!info)
				{
					Error("Update client: FAST_TRAVEL_VOTE invaid player %u.", id);
					break;
				}
				info->fast_travel = true;
			}
			break;
		// invalid change
		default:
			Warn("Update client: Unknown change type %d.", type);
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			Error("Update client: Invalid checksum (%u).", change_i);
			return true;
		}
	}

	return true;
}

//=================================================================================================
bool Game::ProcessControlMessageClientForMe(BitStreamReader& f)
{
	// flags
	byte flags;
	f >> flags;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES.");
		return true;
	}

	// variables
	if(IsSet(flags, PlayerInfo::UF_POISON_DAMAGE))
		f >> pc->last_dmg_poison;
	if(IsSet(flags, PlayerInfo::UF_GOLD))
		f >> pc->unit->gold;
	if(IsSet(flags, PlayerInfo::UF_ALCOHOL))
		f >> pc->unit->alcohol;
	if(IsSet(flags, PlayerInfo::UF_LEARNING_POINTS))
		f >> pc->learning_points;
	if(IsSet(flags, PlayerInfo::UF_LEVEL))
		f >> pc->unit->level;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES(2).");
		return true;
	}

	// changes
	byte changes;
	f >> changes;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES at changes.");
		return true;
	}

	for(byte change_i = 0; change_i < changes; ++change_i)
	{
		NetChangePlayer::TYPE type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			Error("Update single client: Broken ID_PLAYER_CHANGES at UF_NET_CHANGES(2).");
			return true;
		}

		switch(type)
		{
		// item picked up
		case NetChangePlayer::PICKUP:
			{
				int count, team_count;
				f >> count;
				f >> team_count;
				if(!f)
					Error("Update single client: Broken PICKUP.");
				else if(game_state == GS_LEVEL)
				{
					pc->unit->AddItem2(pc_data.picking_item->item, (uint)count, (uint)team_count, false);
					if(pc_data.picking_item->item->type == IT_GOLD)
						sound_mgr->PlaySound2d(sCoins);
					if(pc_data.picking_item_state == 2)
						delete pc_data.picking_item;
					pc_data.picking_item_state = 0;
				}
			}
			break;
		// response to looting
		case NetChangePlayer::LOOT:
			{
				bool can_loot;
				f >> can_loot;
				if(!f)
				{
					Error("Update single client: Broken LOOT.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				if(pc->unit->action == A_PREPARE)
					pc->unit->action = A_NONE;
				if(!can_loot)
				{
					game_gui->messages->AddGameMsg3(GMS_IS_LOOTED);
					pc->action = PlayerAction::None;
					break;
				}

				// read items
				if(!ReadItemListTeam(f, *pc->chest_trade))
				{
					Error("Update single client: Broken LOOT(2).");
					break;
				}

				// start trade
				if(pc->action == PlayerAction::LootUnit)
					game_gui->inventory->StartTrade(I_LOOT_BODY, *pc->action_unit);
				else if(pc->action == PlayerAction::LootContainer)
					game_gui->inventory->StartTrade(I_LOOT_CONTAINER, pc->action_usable->container->items);
				else
					game_gui->inventory->StartTrade(I_LOOT_CHEST, pc->action_chest->items);
			}
			break;
		// start dialog with unit or is busy
		case NetChangePlayer::START_DIALOG:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken START_DIALOG.");
				else if(id == -1)
				{
					// unit is busy
					pc->action = PlayerAction::None;
					game_gui->messages->AddGameMsg3(GMS_UNIT_BUSY);
				}
				else if(id != -2 && game_state == GS_LEVEL) // not entered/left building
				{
					// start dialog
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: START_DIALOG, missing unit %d.", id);
					else
					{
						pc->action = PlayerAction::Talk;
						pc->action_unit = unit;
						dialog_context.StartDialog(unit);
						if(!predialog.empty())
						{
							dialog_context.dialog_s_text = predialog;
							dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
							dialog_context.dialog_wait = 1.f;
							predialog.clear();
						}
						else if(unit->bubble)
						{
							dialog_context.dialog_s_text = unit->bubble->text;
							dialog_context.dialog_text = dialog_context.dialog_s_text.c_str();
							dialog_context.dialog_wait = 1.f;
							dialog_context.skip_id = unit->bubble->skip_id;
						}
						pc_data.before_player = BP_NONE;
					}
				}
			}
			break;
		// show dialog choices
		case NetChangePlayer::SHOW_DIALOG_CHOICES:
			{
				byte count;
				char escape;
				f >> count;
				f >> escape;
				if(!f.Ensure(count))
					Error("Update single client: Broken SHOW_DIALOG_CHOICES.");
				else if(game_state != GS_LEVEL)
				{
					for(byte i = 0; i < count; ++i)
					{
						f.SkipString1();
						if(!f)
							Error("Update single client: Broken SHOW_DIALOG_CHOICES(2) at %u index.", i);
					}
				}
				else
				{
					dialog_context.choice_selected = 0;
					dialog_context.show_choices = true;
					dialog_context.dialog_esc = escape;
					dialog_choices.resize(count);
					for(byte i = 0; i < count; ++i)
					{
						f >> dialog_choices[i];
						if(!f)
						{
							Error("Update single client: Broken SHOW_DIALOG_CHOICES at %u index.", i);
							break;
						}
					}
					game_gui->level_gui->UpdateScrollbar(dialog_choices.size());
				}
			}
			break;
		// end of dialog
		case NetChangePlayer::END_DIALOG:
			if(game_state == GS_LEVEL)
			{
				dialog_context.dialog_mode = false;
				if(pc->action == PlayerAction::Talk)
					pc->action = PlayerAction::None;
				pc->unit->look_target = nullptr;
			}
			break;
		// start trade
		case NetChangePlayer::START_TRADE:
			{
				int id;
				f >> id;
				if(!ReadItemList(f, chest_trade))
				{
					Error("Update single client: Broken START_TRADE.");
					break;
				}

				if(game_state != GS_LEVEL)
					break;

				Unit* trader = game_level->FindUnit(id);
				if(!trader)
				{
					Error("Update single client: START_TRADE, missing unit %d.", id);
					break;
				}
				if(!trader->data->trader)
				{
					Error("Update single client: START_TRADER, unit '%s' is not a trader.", trader->data->id.c_str());
					break;
				}

				game_gui->inventory->StartTrade(I_TRADE, chest_trade, trader);
			}
			break;
		// add multiple same items to inventory
		case NetChangePlayer::ADD_ITEMS:
			{
				int team_count, count;
				const Item* item;
				f >> team_count;
				f >> count;
				if(ReadItemAndFind(f, item) <= 0)
					Error("Update single client: Broken ADD_ITEMS.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS, invalid count %d or team count %d.", count, team_count);
				else if(game_state == GS_LEVEL)
					pc->unit->AddItem2(item, (uint)count, (uint)team_count, false);
			}
			break;
		// add items to trader which is trading with player
		case NetChangePlayer::ADD_ITEMS_TRADER:
			{
				int id, count, team_count;
				const Item* item;
				f >> id;
				f >> count;
				f >> team_count;
				if(ReadItemAndFind(f, item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_TRADER.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS_TRADER, invalid count %d or team count %d.", count, team_count);
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: ADD_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc->IsTradingWith(unit))
						Error("Update single client: ADD_ITEMS_TRADER, unit %d (%s) is not trading with player.", id, unit->data->id.c_str());
					else
						unit->AddItem2(item, (uint)count, (uint)team_count, false);
				}
			}
			break;
		// add items to chest which is open by player
		case NetChangePlayer::ADD_ITEMS_CHEST:
			{
				int id, count, team_count;
				const Item* item;
				f >> id;
				f >> count;
				f >> team_count;
				if(ReadItemAndFind(f, item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_CHEST.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS_CHEST, invalid count %d or team count %d.", count, team_count);
				else if(game_state == GS_LEVEL)
				{
					Chest* chest = game_level->FindChest(id);
					if(!chest)
						Error("Update single client: ADD_ITEMS_CHEST, missing chest %d.", id);
					else if(pc->action != PlayerAction::LootChest || pc->action_chest != chest)
						Error("Update single client: ADD_ITEMS_CHEST, chest %d is not opened by player.", id);
					else
					{
						PreloadItem(item);
						chest->AddItem(item, (uint)count, (uint)team_count, false);
					}
				}
			}
			break;
		// remove items from inventory
		case NetChangePlayer::REMOVE_ITEMS:
			{
				int i_index, count;
				f >> i_index;
				f >> count;
				if(!f)
					Error("Update single client: Broken REMOVE_ITEMS.");
				else if(count <= 0)
					Error("Update single client: REMOVE_ITEMS, invalid count %d.", count);
				else if(game_state == GS_LEVEL)
					pc->unit->RemoveItem(i_index, (uint)count);
			}
			break;
		// remove items from traded inventory which is trading with player
		case NetChangePlayer::REMOVE_ITEMS_TRADER:
			{
				int id, i_index, count;
				f >> id;
				f >> count;
				f >> i_index;
				if(!f)
					Error("Update single client: Broken REMOVE_ITEMS_TRADER.");
				else if(count <= 0)
					Error("Update single client: REMOVE_ITEMS_TRADE, invalid count %d.", count);
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: REMOVE_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc->IsTradingWith(unit))
						Error("Update single client: REMOVE_ITEMS_TRADER, unit %d (%s) is not trading with player.", id, unit->data->id.c_str());
					else
						unit->RemoveItem(i_index, (uint)count);
				}
			}
			break;
		// change player frozen state
		case NetChangePlayer::SET_FROZEN:
			{
				FROZEN frozen;
				f.ReadCasted<byte>(frozen);
				if(!f)
					Error("Update single client: Broken SET_FROZEN.");
				else if(game_state == GS_LEVEL)
					pc->unit->frozen = frozen;
			}
			break;
		// remove quest item from inventory
		case NetChangePlayer::REMOVE_QUEST_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken REMOVE_QUEST_ITEM.");
				else if(game_state == GS_LEVEL)
					pc->unit->RemoveQuestItem(id);
			}
			break;
		// someone else is using usable
		case NetChangePlayer::USE_USABLE:
			if(game_state == GS_LEVEL)
			{
				game_gui->messages->AddGameMsg3(GMS_USED);
				if(pc->action == PlayerAction::LootContainer)
					pc->action = PlayerAction::None;
				if(pc->unit->action == A_PREPARE)
					pc->unit->action = A_NONE;
			}
			break;
		// change development mode for player
		case NetChangePlayer::DEVMODE:
			{
				bool allowed;
				f >> allowed;
				if(!f)
					Error("Update single client: Broken CHEATS.");
				else if(devmode != allowed)
				{
					AddMsg(allowed ? txDevmodeOn : txDevmodeOff);
					devmode = allowed;
				}
			}
			break;
		// start sharing/giving items
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				cstring name = (type == NetChangePlayer::START_SHARE ? "START_SHARE" : "START_GIVE");
				if(pc->action_unit && pc->action_unit->IsTeamMember() && game_state == GS_LEVEL)
				{
					Unit& unit = *pc->action_unit;
					f >> unit.weight;
					f >> unit.weight_max;
					f >> unit.gold;
					unit.stats = unit.data->GetStats(unit.level);
					if(!ReadItemListTeam(f, unit.items))
						Error("Update single client: Broken %s.", name);
					else
						game_gui->inventory->StartTrade(type == NetChangePlayer::START_SHARE ? I_SHARE : I_GIVE, unit);
				}
				else
				{
					if(game_state == GS_LEVEL)
						Error("Update single client: %s, player is not talking with team member.", name);
					// try to skip
					UnitStats stats;
					vector<ItemSlot> items;
					f.Skip(sizeof(int) * 3);
					stats.Read(f);
					if(!ReadItemListTeam(f, items, true))
						Error("Update single client: Broken %s(2).", name);
				}
			}
			break;
		// response to is IS_BETTER_ITEM
		case NetChangePlayer::IS_BETTER_ITEM:
			{
				bool is_better;
				f >> is_better;
				if(!f)
					Error("Update single client: Broken IS_BETTER_ITEM.");
				else if(game_state == GS_LEVEL)
					game_gui->inventory->inv_trade_mine->IsBetterItemResponse(is_better);
			}
			break;
		// question about pvp
		case NetChangePlayer::PVP:
			{
				byte player_id;
				f >> player_id;
				if(!f)
					Error("Update single client: Broken PVP.");
				else if(game_state == GS_LEVEL)
				{
					PlayerInfo* info = net->TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: PVP, invalid player id %u.", player_id);
					else
						arena->ShowPvpRequest(info->u);
				}
			}
			break;
		// preparing to warp
		case NetChangePlayer::PREPARE_WARP:
			if(game_state == GS_LEVEL)
			{
				fallback_type = FALLBACK::WAIT_FOR_WARP;
				fallback_t = -1.f;
				pc->unit->frozen = (pc->unit->usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
			}
			break;
		// entering arena
		case NetChangePlayer::ENTER_ARENA:
			if(game_state == GS_LEVEL)
			{
				fallback_type = FALLBACK::ARENA;
				fallback_t = -1.f;
				pc->unit->frozen = FROZEN::YES;
			}
			break;
		// start of arena combat
		case NetChangePlayer::START_ARENA_COMBAT:
			if(game_state == GS_LEVEL)
				pc->unit->frozen = FROZEN::NO;
			break;
		// exit from arena
		case NetChangePlayer::EXIT_ARENA:
			if(game_state == GS_LEVEL)
			{
				fallback_type = FALLBACK::ARENA_EXIT;
				fallback_t = -1.f;
				pc->unit->frozen = FROZEN::YES;
			}
			break;
		// player refused to pvp
		case NetChangePlayer::NO_PVP:
			{
				byte player_id;
				f >> player_id;
				if(!f)
					Error("Update single client: Broken NO_PVP.");
				else if(game_state == GS_LEVEL)
				{
					PlayerInfo* info = net->TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: NO_PVP, invalid player id %u.", player_id);
					else
						AddMsg(Format(txPvpRefuse, info->name.c_str()));
				}
			}
			break;
		// can't leave location message
		case NetChangePlayer::CANT_LEAVE_LOCATION:
			{
				CanLeaveLocationResult reason;
				f.ReadCasted<byte>(reason);
				if(!f)
					Error("Update single client: Broken CANT_LEAVE_LOCATION.");
				else if(reason != CanLeaveLocationResult::InCombat && reason != CanLeaveLocationResult::TeamTooFar)
					Error("Update single client: CANT_LEAVE_LOCATION, invalid reason %u.", (byte)reason);
				else if(game_state == GS_LEVEL)
					game_gui->messages->AddGameMsg3(reason == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
			}
			break;
		// force player to look at unit
		case NetChangePlayer::LOOK_AT:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken LOOK_AT.");
				else if(game_state == GS_LEVEL)
				{
					if(id == -1)
						pc->unit->look_target = nullptr;
					else
					{
						Unit* unit = game_level->FindUnit(id);
						if(!unit)
							Error("Update single client: LOOK_AT, missing unit %d.", id);
						else
							pc->unit->look_target = unit;
					}
				}
			}
			break;
		// end of fallback
		case NetChangePlayer::END_FALLBACK:
			if(game_state == GS_LEVEL && fallback_type == FALLBACK::CLIENT)
				fallback_type = FALLBACK::CLIENT2;
			break;
		// response to rest in inn
		case NetChangePlayer::REST:
			{
				byte days;
				f >> days;
				if(!f)
					Error("Update single client: Broken REST.");
				else if(game_state == GS_LEVEL)
				{
					fallback_type = FALLBACK::REST;
					fallback_t = -1.f;
					fallback_1 = days;
					pc->unit->frozen = FROZEN::YES;
				}
			}
			break;
		// response to training
		case NetChangePlayer::TRAIN:
			{
				byte type, stat_type;
				f >> type;
				f >> stat_type;
				if(!f)
					Error("Update single client: Broken TRAIN.");
				else if(game_state == GS_LEVEL)
				{
					fallback_type = FALLBACK::TRAIN;
					fallback_t = -1.f;
					fallback_1 = type;
					fallback_2 = stat_type;
					pc->unit->frozen = FROZEN::YES;
				}
			}
			break;
		// warped player to not stuck position
		case NetChangePlayer::UNSTUCK:
			{
				Vec3 new_pos;
				f >> new_pos;
				if(!f)
					Error("Update single client: Broken UNSTUCK.");
				else if(game_state == GS_LEVEL)
				{
					pc->unit->pos = new_pos;
					interpolate_timer = 0.1f;
				}
			}
			break;
		// message about receiving gold from another player
		case NetChangePlayer::GOLD_RECEIVED:
			{
				byte player_id;
				int count;
				f >> player_id;
				f >> count;
				if(!f)
					Error("Update single client: Broken GOLD_RECEIVED.");
				else if(count <= 0)
					Error("Update single client: GOLD_RECEIVED, invalid count %d.", count);
				else
				{
					PlayerInfo* info = net->TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: GOLD_RECEIVED, invalid player id %u.", player_id);
					else
					{
						AddMultiMsg(Format(txReceivedGold, count, info->name.c_str()));
						sound_mgr->PlaySound2d(sCoins);
					}
				}
			}
			break;
		// update trader gold
		case NetChangePlayer::UPDATE_TRADER_GOLD:
			{
				int id, count;
				f >> id;
				f >> count;
				if(!f)
					Error("Update single client: Broken UPDATE_TRADER_GOLD.");
				else if(count < 0)
					Error("Update single client: UPDATE_TRADER_GOLD, invalid count %d.", count);
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: UPDATE_TRADER_GOLD, missing unit %d.", id);
					else if(!pc->IsTradingWith(unit))
						Error("Update single client: UPDATE_TRADER_GOLD, unit %d (%s) is not trading with player.", id, unit->data->id.c_str());
					else
						unit->gold = count;
				}
			}
			break;
		// update trader inventory after getting item
		case NetChangePlayer::UPDATE_TRADER_INVENTORY:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken UPDATE_TRADER_INVENTORY.");
				else if(game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					bool ok = true;
					if(!unit)
					{
						Error("Update single client: UPDATE_TRADER_INVENTORY, missing unit %d.", id);
						ok = false;
					}
					else if(!pc->IsTradingWith(unit))
					{
						Error("Update single client: UPDATE_TRADER_INVENTORY, unit %d (%s) is not trading with player.",
							id, unit->data->id.c_str());
						ok = false;
					}
					if(ok)
					{
						if(!ReadItemListTeam(f, unit->items))
							Error("Update single client: Broken UPDATE_TRADER_INVENTORY(2).");
					}
					else
					{
						vector<ItemSlot> items;
						if(!ReadItemListTeam(f, items, true))
							Error("Update single client: Broken UPDATE_TRADER_INVENTORY(3).");
					}
				}
			}
			break;
		// update player statistics
		case NetChangePlayer::PLAYER_STATS:
			{
				byte flags;
				f >> flags;
				if(!f)
					Error("Update single client: Broken PLAYER_STATS.");
				else if(flags == 0 || (flags & ~STAT_MAX) != 0)
					Error("Update single client: PLAYER_STATS, invalid flags %u.", flags);
				else
				{
					int set_flags = CountBits(flags);
					// read to buffer
					f.Read(BUF, sizeof(int)*set_flags);
					if(!f)
						Error("Update single client: Broken PLAYER_STATS(2).");
					else
					{
						int* buf = (int*)BUF;
						if(IsSet(flags, STAT_KILLS))
							pc->kills = *buf++;
						if(IsSet(flags, STAT_DMG_DONE))
							pc->dmg_done = *buf++;
						if(IsSet(flags, STAT_DMG_TAKEN))
							pc->dmg_taken = *buf++;
						if(IsSet(flags, STAT_KNOCKS))
							pc->knocks = *buf++;
						if(IsSet(flags, STAT_ARENA_FIGHTS))
							pc->arena_fights = *buf++;
					}
				}
			}
			break;
		// player stat changed
		case NetChangePlayer::STAT_CHANGED:
			{
				byte type, what;
				int value;
				f >> type;
				f >> what;
				f >> value;
				if(!f)
					Error("Update single client: Broken STAT_CHANGED.");
				else
				{
					switch((ChangedStatType)type)
					{
					case ChangedStatType::ATTRIBUTE:
						if(what >= (byte)AttributeId::MAX)
							Error("Update single client: STAT_CHANGED, invalid attribute %u.", what);
						else
							pc->unit->Set((AttributeId)what, value);
						break;
					case ChangedStatType::SKILL:
						if(what >= (byte)SkillId::MAX)
							Error("Update single client: STAT_CHANGED, invalid skill %u.", what);
						else
							pc->unit->Set((SkillId)what, value);
						break;
					default:
						Error("Update single client: STAT_CHANGED, invalid change type %u.", type);
						break;
					}
				}
			}
			break;
		// add perk to player
		case NetChangePlayer::ADD_PERK:
			{
				Perk perk;
				int value;
				f.ReadCasted<char>(perk);
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken ADD_PERK.");
				else
					pc->AddPerk(perk, value);
			}
			break;
		// remove perk from player
		case NetChangePlayer::REMOVE_PERK:
			{
				Perk perk;
				int value;
				f.ReadCasted<char>(perk);
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken REMOVE_PERK.");
				else
					pc->RemovePerk(perk, value);
			}
			break;
		// show game message
		case NetChangePlayer::GAME_MESSAGE:
			{
				int gm_id;
				f >> gm_id;
				if(!f)
					Error("Update single client: Broken GAME_MESSAGE.");
				else
					game_gui->messages->AddGameMsg3((GMS)gm_id);
			}
			break;
		// run script result
		case NetChangePlayer::RUN_SCRIPT_RESULT:
			{
				string* output = StringPool.Get();
				f.ReadString4(*output);
				if(!f)
					Error("Update single client: Broken RUN_SCRIPT_RESULT.");
				else
					game_gui->console->AddMsg(output->c_str());
				StringPool.Free(output);
			}
			break;
		// generic command result
		case NetChangePlayer::GENERIC_CMD_RESPONSE:
			{
				string* output = StringPool.Get();
				f.ReadString4(*output);
				if(!f)
					Error("Update single client: Broken GENERIC_CMD_RESPONSE.");
				else
					cmdp->Msg(output->c_str());
				StringPool.Free(output);
			}
			break;
		// add effect to player
		case NetChangePlayer::ADD_EFFECT:
			{
				Effect e;
				f.ReadCasted<char>(e.effect);
				f.ReadCasted<char>(e.source);
				f.ReadCasted<char>(e.source_id);
				f.ReadCasted<char>(e.value);
				f >> e.power;
				f >> e.time;
				if(!f)
					Error("Update single client: Broken ADD_EFFECT.");
				else
					pc->unit->AddEffect(e);
			}
			break;
		// remove effect from player
		case NetChangePlayer::REMOVE_EFFECT:
			{
				EffectId effect;
				EffectSource source;
				int source_id, value;
				f.ReadCasted<char>(effect);
				f.ReadCasted<char>(source);
				f.ReadCasted<char>(source_id);
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken REMOVE_EFFECT.");
				else
					pc->unit->RemoveEffects(effect, source, source_id, value);
			}
			break;
		// player is resting
		case NetChangePlayer::ON_REST:
			{
				int days;
				f.ReadCasted<byte>(days);
				if(!f)
					Error("Update single client: Broken ON_REST.");
				else
					pc->Rest(days, false, false);
			}
			break;
		// add formatted message
		case NetChangePlayer::GAME_MESSAGE_FORMATTED:
			{
				GMS id;
				int subtype, value;
				f >> id;
				f >> subtype;
				f >> value;
				if(!f)
					Error("Update single client: Broken GAME_MESSAGE_FORMATTED.");
				else
					game_gui->messages->AddFormattedMessage(pc, id, subtype, value);
			}
			break;
		// play sound
		case NetChangePlayer::SOUND:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken SOUND.");
				else if(id == 0)
					sound_mgr->PlaySound2d(sCoins);
			}
			break;
		default:
			Warn("Update single client: Unknown player change type %d.", type);
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			Error("Update single client: Invalid checksum (%u).", change_i);
			return true;
		}
	}

	return true;
}

//=================================================================================================
void Game::WriteClientChanges(BitStreamWriter& f)
{
	// count
	f.WriteCasted<byte>(Net::changes.size());
	if(Net::changes.size() > 255)
		Error("Update client: Too many changes %u!", Net::changes.size());

	// changes
	for(NetChange& c : Net::changes)
	{
		f.WriteCasted<byte>(c.type);

		switch(c.type)
		{
		case NetChange::CHANGE_EQUIPMENT:
		case NetChange::IS_BETTER_ITEM:
		case NetChange::CONSUME_ITEM:
		case NetChange::USE_ITEM:
			f << c.id;
			break;
		case NetChange::TAKE_WEAPON:
			f << (c.id != 0);
			f.WriteCasted<byte>(c.id == 0 ? pc->unit->weapon_taken : pc->unit->weapon_hiding);
			break;
		case NetChange::ATTACK:
			{
				byte b = (byte)c.id;
				b |= ((pc->unit->attack_id & 0xF) << 4);
				f << b;
				f << c.f[1];
				if(c.id == 2)
					f << PlayerAngleY() * 12;
			}
			break;
		case NetChange::DROP_ITEM:
			f << c.id;
			f << c.count;
			break;
		case NetChange::IDLE:
		case NetChange::CHOICE:
		case NetChange::ENTER_BUILDING:
		case NetChange::CHANGE_LEADER:
		case NetChange::RANDOM_NUMBER:
		case NetChange::CHEAT_WARP:
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REST:
		case NetChange::FAST_TRAVEL:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_WARP_TO_STAIRS:
		case NetChange::CHEAT_CHANGE_LEVEL:
		case NetChange::CHEAT_NOAI:
		case NetChange::PVP:
		case NetChange::CHEAT_GODMODE:
		case NetChange::CHEAT_INVISIBLE:
		case NetChange::CHEAT_NOCLIP:
		case NetChange::CHANGE_ALWAYS_RUN:
			f << (c.id != 0);
			break;
		case NetChange::PICKUP_ITEM:
		case NetChange::LOOT_UNIT:
		case NetChange::TALK:
		case NetChange::USE_CHEST:
		case NetChange::SKIP_DIALOG:
		case NetChange::CHEAT_SKIP_DAYS:
		case NetChange::PAY_CREDIT:
		case NetChange::DROP_GOLD:
		case NetChange::TAKE_ITEM_CREDIT:
		case NetChange::CLEAN_LEVEL:
			f << c.id;
			break;
		case NetChange::CHEAT_ADD_GOLD:
			f << (c.id == 1);
			f << c.count;
			break;
		case NetChange::STOP_TRADE:
		case NetChange::GET_ALL_ITEMS:
		case NetChange::EXIT_BUILDING:
		case NetChange::WARP:
		case NetChange::CHEAT_SUICIDE:
		case NetChange::STAND_UP:
		case NetChange::CHEAT_SCARE:
		case NetChange::CHEAT_CITIZEN:
		case NetChange::CHEAT_HEAL:
		case NetChange::CHEAT_REVEAL:
		case NetChange::CHEAT_GOTO_MAP:
		case NetChange::CHEAT_REVEAL_MINIMAP:
		case NetChange::ENTER_LOCATION:
		case NetChange::CLOSE_ENCOUNTER:
		case NetChange::YELL:
		case NetChange::CHEAT_REFRESH_COOLDOWN:
		case NetChange::END_FALLBACK:
		case NetChange::END_TRAVEL:
			break;
		case NetChange::ADD_NOTE:
			f << game_gui->journal->GetNotes().back();
			break;
		case NetChange::USE_USABLE:
			f << c.id;
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::CHEAT_KILLALL:
			f << (c.unit ? c.unit->id : -1);
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_KILL:
		case NetChange::CHEAT_HEAL_UNIT:
		case NetChange::CHEAT_HURT:
		case NetChange::CHEAT_BREAK_ACTION:
		case NetChange::CHEAT_FALL:
			f << c.unit->id;
			break;
		case NetChange::CHEAT_ADD_ITEM:
			f << c.base_item->id;
			f << c.count;
			f << (c.id != 0);
			break;
		case NetChange::CHEAT_SPAWN_UNIT:
			f << c.base_unit->id;
			f.WriteCasted<byte>(c.count);
			f.WriteCasted<char>(c.id);
			f.WriteCasted<char>(c.i);
			break;
		case NetChange::CHEAT_SET_STAT:
		case NetChange::CHEAT_MOD_STAT:
			f.WriteCasted<byte>(c.id);
			f << (c.count != 0);
			f.WriteCasted<char>(c.i);
			break;
		case NetChange::LEAVE_LOCATION:
			f.WriteCasted<char>(c.id);
			break;
		case NetChange::USE_DOOR:
			f << c.id;
			f << (c.count != 0);
			break;
		case NetChange::TRAIN:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::GIVE_GOLD:
		case NetChange::GET_ITEM:
		case NetChange::PUT_ITEM:
			f << c.id;
			f << c.count;
			break;
		case NetChange::PUT_GOLD:
			f << c.count;
			break;
		case NetChange::PLAYER_ACTION:
			f << (c.unit ? c.unit->id : -1);
			f << c.pos;
			break;
		case NetChange::CHEAT_STUN:
			f << c.unit->id;
			f << c.f[0];
			break;
		case NetChange::RUN_SCRIPT:
			f.WriteString4(*c.str);
			StringPool.Free(c.str);
			f << c.id;
			break;
		case NetChange::SET_NEXT_ACTION:
			f.WriteCasted<byte>(pc->next_action);
			switch(pc->next_action)
			{
			case NA_NONE:
				break;
			case NA_REMOVE:
			case NA_DROP:
				f.WriteCasted<byte>(pc->next_action_data.slot);
				break;
			case NA_EQUIP:
			case NA_CONSUME:
			case NA_EQUIP_DRAW:
				f << pc->next_action_data.index;
				f << pc->next_action_data.item->id;
				break;
			case NA_USE:
				f << pc->next_action_data.usable->id;
				break;
			default:
				assert(0);
				break;
			}
			break;
		case NetChange::GENERIC_CMD:
			{
				byte* content = ((byte*)&c) + sizeof(NetChange::TYPE);
				byte size = *content++;
				f << size;
				f.Write(content, size);
			}
			break;
		case NetChange::CHEAT_ARENA:
			f << *c.str;
			StringPool.Free(c.str);
			break;
		case NetChange::TRAVEL_POS:
		case NetChange::STOP_TRAVEL:
		case NetChange::CHEAT_TRAVEL_POS:
			f << c.pos.x;
			f << c.pos.y;
			break;
		case NetChange::SET_SHORTCUT:
			{
				const Shortcut& shortcut = pc->shortcuts[c.id];
				f.WriteCasted<byte>(c.id);
				f.WriteCasted<byte>(shortcut.type);
				if(shortcut.type == Shortcut::TYPE_SPECIAL)
					f.WriteCasted<byte>(shortcut.value);
				else if(shortcut.type == Shortcut::TYPE_ITEM)
					f << shortcut.item->id;
			}
			break;
		default:
			Error("UpdateClient: Unknown change %d.", c.type);
			assert(0);
			break;
		}

		f.WriteCasted<byte>(0xFF);
	}
}

//=================================================================================================
void Game::Client_Say(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_Say: Broken packet.");
	else
	{
		PlayerInfo* info = net->TryGetPlayer(id);
		if(!info)
			Error("Client_Say: Broken packet, missing player %d.", id);
		else
		{
			cstring s = Format("%s: %s", info->name.c_str(), text.c_str());
			AddMsg(s);
			if(game_state == GS_LEVEL)
				game_gui->level_gui->AddSpeechBubble(info->u, text.c_str());
		}
	}
}

//=================================================================================================
void Game::Client_Whisper(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_Whisper: Broken packet.");
	else
	{
		PlayerInfo* info = net->TryGetPlayer(id);
		if(!info)
			Error("Client_Whisper: Broken packet, missing player %d.", id);
		else
		{
			cstring s = Format("%s@: %s", info->name.c_str(), text.c_str());
			AddMsg(s);
		}
	}
}

//=================================================================================================
void Game::Client_ServerSay(BitStreamReader& f)
{
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_ServerSay: Broken packet.");
	else
		AddServerMsg(text.c_str());
}

//=================================================================================================
void Game::Server_Say(BitStreamReader& f, PlayerInfo& info, Packet* packet)
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
		AddMsg(str);

		// przeœlij dalej
		if(net->active_players > 2)
			net->peer->Send(&f.GetBitStream(), MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, true);

		if(game_state == GS_LEVEL)
			game_gui->level_gui->AddSpeechBubble(info.u, text.c_str());
	}
}

//=================================================================================================
void Game::Server_Whisper(BitStreamReader& f, PlayerInfo& info, Packet* packet)
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
			AddMsg(str);
		}
		else
		{
			// wiadomoœæ do kogoœ innego
			PlayerInfo* info2 = net->TryGetPlayer(id);
			if(!info2)
				Error("Server_Whisper: Broken packet from %s to missing player %d.", info.name.c_str(), id);
			else
			{
				packet->data[1] = (byte)info.id;
				net->peer->Send((cstring)packet->data, packet->length, MEDIUM_PRIORITY, RELIABLE, 0, info2->adr, false);
			}
		}
	}
}

//=================================================================================================
void Game::ServerProcessUnits(vector<Unit*>& units)
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->changed)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::UNIT_POS;
			c.unit = *it;
			(*it)->changed = false;
		}
		if((*it)->IsAI() && (*it)->ai->change_ai_mode)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_AI_MODE;
			c.unit = *it;
			(*it)->ai->change_ai_mode = false;
		}
	}
}

//=================================================================================================
void Game::UpdateWarpData(float dt)
{
	for(vector<WarpData>::iterator it = mp_warps.begin(), end = mp_warps.end(); it != end;)
	{
		if((it->timer -= dt * 2) <= 0.f)
		{
			game_level->WarpUnit(it->u, it->where);

			NetChangePlayer& c = Add1(it->u->player->player_info->changes);
			c.type = NetChangePlayer::SET_FROZEN;
			c.id = 0;

			it->u->frozen = FROZEN::NO;

			it = mp_warps.erase(it);
			end = mp_warps.end();
		}
		else
			++it;
	}
}

//=================================================================================================
void Game::Net_OnNewGameClient()
{
	DeleteElements(quest_mgr->quest_items);
	devmode = default_devmode;
	train_move = 0.f;
	anyone_talking = false;
	interpolate_timer = 0.f;
	Net::changes.clear();
	if(!net->net_strs.empty())
		StringPool.Free(net->net_strs);
	paused = false;
	hardcore_mode = false;
	if(!net->mp_quickload)
	{
		game_gui->mp_box->Reset();
		game_gui->mp_box->visible = true;
	}
}

//=================================================================================================
void Game::Net_OnNewGameServer()
{
	net->active_players = 1;
	DeleteElements(net->players);
	team->my_id = 0;
	team->leader_id = 0;
	net->last_id = 0;
	paused = false;
	hardcore_mode = false;

	PlayerInfo* info = new PlayerInfo;
	net->players.push_back(info);

	PlayerInfo& sp = *info;
	sp.name = player_name;
	sp.id = 0;
	sp.state = PlayerInfo::IN_LOBBY;
	sp.left = PlayerInfo::LEFT_NO;

	skip_id_counter = 0;
	net->update_timer = 0.f;
	arena->Reset();
	anyone_talking = false;
	mp_warps.clear();
	if(!net->mp_load)
		Net::changes.clear(); // przy wczytywaniu jest czyszczone przed wczytaniem i w net_changes s¹ zapisane quest_items
	if(!net->net_strs.empty())
		StringPool.Free(net->net_strs);
	game_gui->server->max_players = net->max_players;
	game_gui->server->server_name = net->server_name;
	game_gui->server->UpdateServerInfo();
	game_gui->server->Show();
	if(!net->mp_quickload)
	{
		game_gui->mp_box->Reset();
		game_gui->mp_box->visible = true;
	}

	if(!net->mp_load)
		game_gui->server->CheckAutopick();
	else
	{
		// search for saved character
		PlayerInfo* old = net->FindOldPlayer(player_name.c_str());
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

	ChangeTitle();
}

//=================================================================================================
// -2 read error, -1 not found, 0 empty, 1 found
int Game::ReadItemAndFind(BitStreamReader& f, const Item*& item) const
{
	item = nullptr;

	const string& item_id = f.ReadString1();
	if(!f)
		return -2;

	if(item_id.empty())
		return 0;

	if(item_id[0] == '$')
	{
		int quest_id = f.Read<int>();
		if(!f)
			return -2;

		item = quest_mgr->FindQuestItemClient(item_id.c_str(), quest_id);
		if(!item)
		{
			Warn("Missing quest item '%s' (%d).", item_id.c_str(), quest_id);
			return -1;
		}
		else
			return 1;
	}
	else
	{
		item = Item::TryGet(item_id);
		if(!item)
		{
			Warn("Missing item '%s'.", item_id.c_str());
			return -1;
		}
		else
			return 1;
	}
}

//=================================================================================================
bool Game::CheckMoveNet(Unit& unit, const Vec3& pos)
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
void Game::ProcessLeftPlayers()
{
	if(!net->players_left)
		return;

	LoopAndRemove(net->players, [this](PlayerInfo& info)
	{
		if(info.left == PlayerInfo::LEFT_NO)
			return false;

		// order of changes is important here
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_PLAYER;
		c.id = info.id;
		c.count = (int)info.left;

		RemovePlayer(info);

		if(team->leader_id == c.id)
		{
			team->leader_id = team->my_id;
			team->leader = pc->unit;
			NetChange& c2 = Add1(Net::changes);
			c2.type = NetChange::CHANGE_LEADER;
			c2.id = team->my_id;

			if(game_gui->world_map->dialog_enc)
				game_gui->world_map->dialog_enc->bts[0].state = Button::NONE;

			AddMsg(txYouAreLeader);
		}

		team->CheckCredit();
		team->CalculatePlayersLevel();
		delete &info;

		return true;
	});

	net->players_left = false;
}

//=================================================================================================
void Game::RemovePlayer(PlayerInfo& info)
{
	switch(info.left)
	{
	case PlayerInfo::LEFT_TIMEOUT:
		{
			Info("Player %s kicked due to timeout.", info.name.c_str());
			AddMsg(Format(txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_KICK:
		{
			Info("Player %s kicked from server.", info.name.c_str());
			AddMsg(Format(txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_DISCONNECTED:
		{
			Info("Player %s disconnected from server.", info.name.c_str());
			AddMsg(Format(txPlayerDisconnected, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_QUIT:
		{
			Info("Player %s quit game.", info.name.c_str());
			AddMsg(Format(txPlayerQuit, info.name.c_str()));
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
	if(game_state == GS_WORLDMAP)
	{
		if(Net::IsLocal() && !game_level->is_open)
			DeleteUnit(unit);
	}
	else
	{
		game_level->to_remove.push_back(unit);
		unit->to_remove = true;
	}
	info.u = nullptr;
}
