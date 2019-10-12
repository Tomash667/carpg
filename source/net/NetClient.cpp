#include "Pch.h"
#include "GameCore.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Game.h"
#include "Level.h"
#include "GroundItem.h"
#include "Door.h"
#include "Spell.h"
#include "Trap.h"
#include "Object.h"
#include "Portal.h"
#include "ResourceManager.h"
#include "Team.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "Unit.h"
#include "GameGui.h"
#include "Journal.h"
#include "LevelGui.h"
#include "GameMessages.h"
#include "WorldMapGui.h"
#include "TeamPanel.h"
#include "Inventory.h"
#include "Console.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MpBox.h"
#include "EntityInterpolator.h"
#include "ParticleSystem.h"
#include "SoundManager.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest.h"
#include "World.h"
#include "City.h"
#include "MultiInsideLocation.h"
#include "Cave.h"
#include "DialogBox.h"
#include "GameStats.h"
#include "Arena.h"
#include "CommandParser.h"
#include "GameResources.h"

//=================================================================================================
void Net::InitClient()
{
	Info("Initlializing client...");

	OpenPeer();

	SocketDescriptor sd;
	sd.socketFamily = AF_INET;
	// maxConnections is 2 - one for server, one for punchthrough proxy (Connect can fail if this is 1)
	StartupResult r = peer->Startup(2, &sd, 1);
	if(r != RAKNET_STARTED)
	{
		Error("Failed to create client: SLikeNet error %d.", r);
		throw Format(txInitConnectionFailed, r);
	}
	peer->SetMaximumIncomingConnections(0);

	SetMode(Mode::Client);

	DEBUG_DO(peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS));
}

//=================================================================================================
void Net::OnNewGameClient()
{
	DeleteElements(quest_mgr->quest_items);
	game->devmode = game->default_devmode;
	game->train_move = 0.f;
	team->anyone_talking = false;
	interpolate_timer = 0.f;
	changes.clear();
	if(!net_strs.empty())
		StringPool.Free(net_strs);
	game->paused = false;
	game->hardcore_mode = false;
	if(!mp_quickload)
	{
		game_gui->mp_box->Reset();
		game_gui->mp_box->visible = true;
	}
}
//=================================================================================================
void Net::UpdateClient(float dt)
{
	if(game->game_state == GS_LEVEL)
	{
		// interpolacja pozycji gracza
		if(interpolate_timer > 0.f)
		{
			interpolate_timer -= dt;
			if(interpolate_timer >= 0.f)
				game->pc->unit->visual_pos = Vec3::Lerp(game->pc->unit->visual_pos, game->pc->unit->pos, (0.1f - interpolate_timer) * 10);
			else
				game->pc->unit->visual_pos = game->pc->unit->pos;
		}

		// interpolacja pozycji/obrotu postaci
		InterpolateUnits(dt);
	}

	bool exit_from_server = false;

	Packet* packet;
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
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
					text = game->txLostConnection;
					text_eng = "Update client: Lost connection with server.";
				}
				else
				{
					text = game->txYouDisconnected;
					text_eng = "Update client: You have been disconnected.";
				}
				Info(text_eng);
				peer->DeallocatePacket(packet);
				game->ExitToMenu();
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
					reason_text_int = game->txYouKicked;
				}
				else
				{
					reason_text = "Server was closed.";
					reason_text_int = game->txServerClosed;
				}
				Info("Update client: %s", reason_text);
				peer->DeallocatePacket(packet);
				game->ExitToMenu();
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
					game_gui->info_box->Show(game->txGeneratingLocation);
					game->LeaveLevel();
					game->net_mode = Game::NM_TRANSFER;
					game->net_state = NetState::Client_ChangingLevel;
					game->clear_color = Color::Black;
					game_gui->load_screen->visible = true;
					game_gui->level_gui->visible = false;
					game_gui->world_map->Hide();
					game->arena->ClosePvpDialog();
					if(game_gui->world_map->dialog_enc)
					{
						gui->CloseDialog(game_gui->world_map->dialog_enc);
						game_gui->world_map->dialog_enc = nullptr;
					}
					peer->DeallocatePacket(packet);
					FilterClientChanges();
					game->LoadingStart(4);
					return;
				}
			}
			break;
		case ID_CHANGES:
			if(!ProcessControlMessageClient(reader, exit_from_server))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_PLAYER_CHANGES:
			if(!ProcessControlMessageClientForMe(reader))
			{
				peer->DeallocatePacket(packet);
				return;
			}
			break;
		case ID_LOADING:
			{
				Info("Quickloading server game.");
				mp_quickload = true;
				game->ClearGame();
				reader >> mp_load_worldmap;
				game->LoadingStart(mp_load_worldmap ? 4 : 9);
				game_gui->info_box->Show(game->txLoadingSaveByServer);
				game_gui->world_map->Hide();
				game->net_mode = Game::NM_TRANSFER;
				game->net_state = NetState::Client_BeforeTransfer;
				game->game_state = GS_LOAD;
				game->items_load.clear();
			}
			break;
		default:
			Warn("UpdateClient: Unknown packet from server: %u.", msg_id);
			break;
		}
	}

	// wyœli moj¹ pozycjê/akcjê
	update_timer += dt;
	if(update_timer >= TICK)
	{
		update_timer = 0;
		BitStreamWriter f;
		f << ID_CONTROL;
		if(game->game_state == GS_LEVEL)
		{
			f << true;
			f << game->pc->unit->pos;
			f << game->pc->unit->rot;
			f << game->pc->unit->mesh_inst->groups[0].speed;
			f.WriteCasted<byte>(game->pc->unit->animation);
		}
		else
			f << false;
		WriteClientChanges(f);
		changes.clear();
		SendClient(f, HIGH_PRIORITY, RELIABLE_ORDERED);
	}

	if(exit_from_server)
		peer->Shutdown(1000);
}

//=================================================================================================
void Net::InterpolateUnits(float dt)
{
	for(LevelArea& area : game_level->ForEachArea())
	{
		for(Unit* unit : area.units)
		{
			if(!unit->IsLocalPlayer())
				unit->interp->Update(dt, unit->visual_pos, unit->rot);
			if(unit->mesh_inst->mesh->head.n_groups == 1)
			{
				if(!unit->mesh_inst->groups[0].anim)
				{
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
				}
			}
			else
			{
				if(!unit->mesh_inst->groups[0].anim && !unit->mesh_inst->groups[1].anim)
				{
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
				}
			}
		}
	}
}

//=================================================================================================
void Net::WriteClientChanges(BitStreamWriter& f)
{
	PlayerController& pc = *game->pc;

	// count
	f.WriteCasted<byte>(changes.size());
	if(changes.size() > 255)
		Error("Update client: Too many changes %u!", changes.size());

	// changes
	for(NetChange& c : changes)
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
			f.WriteCasted<byte>(c.id == 0 ? pc.unit->weapon_taken : pc.unit->weapon_hiding);
			break;
		case NetChange::ATTACK:
			{
				byte b = (byte)c.id;
				b |= ((pc.unit->attack_id & 0xF) << 4);
				f << b;
				f << c.f[1];
				if(c.id == 2)
					f << pc.GetShootAngle() * 12;
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
		case NetChange::CUTSCENE_SKIP:
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
			f.WriteCasted<byte>(pc.next_action);
			switch(pc.next_action)
			{
			case NA_NONE:
				break;
			case NA_REMOVE:
			case NA_DROP:
				f.WriteCasted<byte>(pc.next_action_data.slot);
				break;
			case NA_EQUIP:
			case NA_CONSUME:
			case NA_EQUIP_DRAW:
				f << pc.next_action_data.index;
				f << pc.next_action_data.item->id;
				break;
			case NA_USE:
				f << pc.next_action_data.usable->id;
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
				const Shortcut& shortcut = pc.shortcuts[c.id];
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
bool Net::ProcessControlMessageClient(BitStreamReader& f, bool& exit_from_server)
{
	PlayerController& pc = *game->pc;

	// read count
	word changes_count;
	f >> changes_count;
	if(!f)
	{
		Error("Update client: Broken ID_CHANGES.");
		return true;
	}

	// read changes
	for(word change_i = 0; change_i < changes_count; ++change_i)
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

				if(game->game_state != GS_LEVEL)
					break;

				Unit* unit = game_level->FindUnit(id);
				if(!unit)
					Error("Update client: UNIT_POS, missing unit %d.", id);
				else if(unit != pc.unit)
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
				if(!f || f.ReadItemAndFind(item) < 0)
					Error("Update client: Broken CHANGE_EQUIPMENT.");
				else if(!IsValid(type))
					Error("Update client: CHANGE_EQUIPMENT, invalid slot %d.", type);
				else if(game->game_state == GS_LEVEL)
				{
					Unit* target = game_level->FindUnit(id);
					if(!target)
						Error("Update client: CHANGE_EQUIPMENT, missing unit %d.", id);
					else
					{
						if(item)
							game_res->PreloadItem(item);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: TAKE_WEAPON, missing unit %d.", id);
					else if(unit != pc.unit)
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

				if(game->game_state != GS_LEVEL)
					break;

				Unit* unit_ptr = game_level->FindUnit(id);
				if(!unit_ptr)
				{
					Error("Update client: ATTACK, missing unit %d.", id);
					break;
				}

				// don't start attack if this is local unit
				if(unit_ptr == pc.unit)
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
							game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_PrepareAttack:
					{
						if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
							game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.attack_id = ((typeflags & 0xF0) >> 4);
						unit.attack_power = 1.f;
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE, group);
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
						unit.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.action = A_SHOOT;
						unit.animation_state = (type == AID_Shoot ? 1 : 0);
						unit.hitted = false;
						if(!unit.bow_instance)
							unit.bow_instance = game_level->GetBowInstance(unit.GetBow().mesh);
						unit.bow_instance->Play(&unit.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
						unit.bow_instance->groups[0].speed = unit.mesh_inst->groups[group].speed;
					}
					break;
				case AID_Block:
					{
						unit.action = A_BLOCK;
						unit.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END, group);
						unit.mesh_inst->groups[group].blend_max = attack_speed;
						unit.animation_state = 0;
					}
					break;
				case AID_Bash:
					{
						unit.action = A_BASH;
						unit.animation_state = 0;
						unit.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.hitted = false;
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
						unit.mesh_inst->Play(NAMES::ani_attacks[unit.attack_id], PLAY_PRIO1 | PLAY_ONCE, group);
						unit.mesh_inst->groups[group].speed = attack_speed;
						unit.animation_state = 1;
						unit.hitted = false;
					}
					break;
				case AID_StopBlock:
					{
						unit.action = A_NONE;
						unit.mesh_inst->Deactivate(group);
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
					team->anyone_talking = IsSet(flags, 0x04);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: UPDATE_HP, missing unit %d.", id);
					else if(unit == pc.unit)
					{
						// handling of previous hp
						float hp_dif = hpp - unit->GetHpp();
						if(hp_dif < 0.f)
							pc.last_dmg += -hp_dif * unit->hpmax;
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(netid);
					if(!unit)
						Error("Update client: UPDATE_MP, missing unit %d.", netid);
					else if(unit == pc.unit)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(netid);
					if(!unit)
						Error("Update client: UPDATE_STAMINA, missing unit %d.", netid);
					else if(unit == pc.unit)
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
				else if(game->game_state == GS_LEVEL)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = game_res->tBlood[type];
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
				else if(game->game_state == GS_LEVEL)
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
						game->PlayAttachedSound(*unit, sound, dist);
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: DROP_ITEM, missing unit %d.", id);
					else if(unit != pc.unit)
					{
						unit->action = A_ANIMATION;
						unit->mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
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
				else if(game->game_state != GS_LEVEL)
					delete item;
				else
				{
					game_res->PreloadItem(item->item);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: PICKUP_ITEM, missing unit %d.", id);
					else if(unit != pc.unit)
					{
						unit->action = A_PICKUP;
						unit->animation = ANI_PLAY;
						unit->mesh_inst->Play(up_animation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
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
				else if(game->game_state == GS_LEVEL)
				{
					LevelArea* area;
					GroundItem* item = game_level->FindGroundItem(id, &area);
					if(!item)
						Error("Update client: REMOVE_ITEM, missing ground item %d.", id);
					else
					{
						RemoveElement(area->items, item);
						if(pc.data.before_player == BP_ITEM && pc.data.before_player_ptr.item == item)
							pc.data.before_player = BP_NONE;
						if(pc.data.picking_item_state == 1 && pc.data.picking_item == item)
							pc.data.picking_item_state = 2;
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
				bool ok = (f.ReadItemAndFind(item) > 0);
				f >> force;
				if(!f || !ok)
					Error("Update client: Broken CONSUME_ITEM.");
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CONSUME_ITEM, missing unit %d.", id);
					else if(item->type != IT_CONSUMABLE)
						Error("Update client: CONSUME_ITEM, %s is not consumable.", item->id.c_str());
					else if(unit != pc.unit || force)
					{
						game_res->PreloadItem(item);
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
				else if(game->game_state == GS_LEVEL)
					sound_mgr->PlaySound3d(game_res->GetMaterialSound(mat1, mat2), pos, HIT_SOUND_DIST);
			}
			break;
		// unit get stunned
		case NetChange::STUNNED:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken STUNNED.");
				else if(game->game_state == GS_LEVEL)
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
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						else
						{
							unit->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
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
				else if(game->game_state == GS_LEVEL)
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
					b.mesh = game_res->aArrow;
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

					sound_mgr->PlaySound3d(game_res->sBow[Rand() % 2], b.pos, ARROW_HIT_SOUND_DIST);
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
				else if(game->game_state != GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: IDLE, missing unit %d.", id);
					else if(animation_index >= unit->data->idles->anims.size())
						Error("Update client: IDLE, invalid animation index %u (count %u).", animation_index, unit->data->idles->anims.size());
					else
					{
						unit->mesh_inst->Play(unit->data->idles->anims[animation_index].c_str(), PLAY_ONCE, 0);
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
				else if(game->game_state == GS_LEVEL)
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
							unit->animation = ANI_PLAY;
							unit->action = A_ANIMATION;
						}

						if(game->dialog_context.dialog_mode && game->dialog_context.talker == unit)
						{
							game->dialog_context.dialog_s_text = text;
							game->dialog_context.dialog_text = game->dialog_context.dialog_s_text.c_str();
							game->dialog_context.dialog_wait = 1.f;
							game->dialog_context.skip_id = skip_id;
						}
						else if(pc.action == PlayerAction::Talk && pc.action_unit == unit)
						{
							game->predialog = text;
							game->dialog_context.skip_id = skip_id;
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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

				if(game->game_state != GS_LEVEL)
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
				if(unit == pc.unit)
				{
					if(game->fallback_type == FALLBACK::WAIT_FOR_WARP)
						game->fallback_type = FALLBACK::NONE;
					else if(game->fallback_type == FALLBACK::ARENA)
					{
						pc.unit->frozen = FROZEN::ROTATE;
						game->fallback_type = FALLBACK::NONE;
					}
					else if(game->fallback_type == FALLBACK::ARENA_EXIT)
					{
						pc.unit->frozen = FROZEN::NO;
						game->fallback_type = FALLBACK::NONE;

						if(pc.unit->hp <= 0.f)
						{
							pc.unit->HealPoison();
							pc.unit->live_state = Unit::ALIVE;
							pc.unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
							pc.unit->action = A_ANIMATION;
							pc.unit->animation = ANI_PLAY;
						}
					}
					PushChange(NetChange::WARP);
					interpolate_timer = 0.f;
					pc.data.rot_buf = 0.f;
					game_level->camera.Reset();
					pc.data.rot_buf = 0.f;
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
					PlayerInfo* info = TryGetPlayer(id);
					if(info)
					{
						team->leader_id = id;
						if(team->leader_id == team->my_id)
							game_gui->AddMsg(game->txYouAreLeader);
						else
							game_gui->AddMsg(Format(game->txPcIsLeader, info->name.c_str()));
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
					PlayerInfo* info = TryGetPlayer(player_id);
					if(info)
						game_gui->AddMsg(Format(game->txRolledNumber, info->name.c_str(), number));
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
					PlayerInfo* info = TryGetPlayer(player_id);
					if(!info)
						Error("Update client: REMOVE_PLAYER, missing player %u.", player_id);
					else if(player_id != team->my_id)
					{
						info->left = reason;
						net->RemovePlayer(*info);
						players.erase(info->GetIndex());
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

				if(game->game_state != GS_LEVEL)
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
							game_res->PreloadItem(unit->used_item);
							unit->weapon_taken = W_NONE;
							unit->weapon_state = WS_HIDDEN;
						}
					}
					else
						unit->action = A_NONE;

					unit->UseUsable(usable);
					if(pc.data.before_player == BP_USABLE && pc.data.before_player_ptr.usable == usable)
						pc.data.before_player = BP_NONE;
				}
				else
				{
					if(unit->player != game->pc && !IsSet(base.use_flags, BaseUsable::CONTAINER))
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
				else if(game->game_state == GS_LEVEL)
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
			game->SetMusic(MusicType::Death);
			game_gui->CloseAllPanels(true);
			++game->death_screen;
			game->death_fade = 0;
			game->death_solo = false;
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					LevelArea& area = game_level->GetArea(unit->pos);
					area.units.push_back(unit);
					unit->area = &area;
					if(unit->summoner)
						game_level->SpawnUnitEffect(*unit);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: CHANGE_ARENA_STATE, missing unit %d.", id);
					else
					{
						if(state < -1 || state > 1)
							state = -1;
						unit->in_arena = state;
						if(unit == pc.unit && state >= 0)
							pc.RefreshCooldown();
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
				else if(game->game_state == GS_LEVEL && game_level->city_ctx && IsSet(game_level->city_ctx->flags, City::HaveArena)
					&& game_level->GetArena() == pc.unit->area)
				{
					Sound* sound;
					if(type == 0)
						sound = game_res->sArenaFight;
					else if(type == 1)
						sound = game_res->sArenaWin;
					else
						sound = game_res->sArenaLost;
					sound_mgr->PlaySound2d(sound);
				}
			}
			break;
		// leaving notification
		case NetChange::LEAVE_LOCATION:
			if(game->game_state == GS_LEVEL)
			{
				game->fallback_type = FALLBACK::WAIT_FOR_WARP;
				game->fallback_t = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// exit to map
		case NetChange::EXIT_TO_MAP:
			if(game->game_state == GS_LEVEL)
				game->ExitToMap();
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					Error("Update client: Broken TRAVEL.");
				else if(game->game_state == GS_WORLDMAP)
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
				else if(game->game_state == GS_WORLDMAP)
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
				else if(game->game_state == GS_WORLDMAP)
					world->StopTravel(pos, false);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game->game_state == GS_WORLDMAP)
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
				else if(game->game_state == GS_LEVEL)
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
							}
							else if(door->state == Door::Opening)
							{
								door->state = Door::Closing2;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
							}
							else if(door->state == Door::Opening2)
							{
								door->state = Door::Closing;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
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
							}
							else if(door->state == Door::Closing)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening2;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
							}
							else if(door->state == Door::Closing2)
							{
								door->locked = LOCK_NONE;
								door->state = Door::Opening;
								door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
							}
							else
								ok = false;
						}

						if(ok && Rand() % 2 == 0)
						{
							Sound* sound;
							if(is_closing && Rand() % 2 == 0)
								sound = game_res->sDoorClose;
							else
								sound = game_res->sDoor[Rand() % 3];
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

				if(game->game_state != GS_LEVEL)
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
				else  if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
			sound_mgr->PlaySound2d(game_res->sEvil);
			break;
		// start encounter on world map
		case NetChange::ENCOUNTER:
			{
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken ENCOUNTER.");
				else if(game->game_state == GS_WORLDMAP)
				{
					DialogInfo info;
					info.event = [this](int)
					{
						game_gui->world_map->dialog_enc = nullptr;
						PushChange(NetChange::CLOSE_ENCOUNTER);
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
			if(game->game_state == GS_WORLDMAP && game_gui->world_map->dialog_enc)
			{
				gui->CloseDialog(game_gui->world_map->dialog_enc);
				game_gui->world_map->dialog_enc = nullptr;
			}
			break;
		// close portal in location
		case NetChange::CLOSE_PORTAL:
			if(game->game_state == GS_LEVEL)
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
			if(game->game_state == GS_LEVEL)
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
					if(pe->tex == game_res->tBlood[BLOOD_RED])
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
				if(type == L_DUNGEON)
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
				f.ReadCasted<byte>(loc->target);
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
						else
						{
							unit->animation = ANI_PLAY;
							unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
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

				if(game->game_state != GS_LEVEL)
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

				if(game->game_state != GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
					game->noai = state;
			}
			break;
		// end of game, time run out
		case NetChange::END_OF_GAME:
			Info("Update client: Game over - time run out.");
			game_gui->CloseAllPanels(true);
			game->end_of_game = true;
			game->death_fade = 0.f;
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
			ReadNetVars(f);
			if(!f)
				Error("Update client: Broken CHANGE_MP_VARS.");
			break;
		// game saved notification
		case NetChange::GAME_SAVED:
			game_gui->mp_box->Add(game->txGameSaved);
			game_gui->messages->AddGameMsg3(GMS_GAME_SAVED);
			break;
		// ai left team due too many team members
		case NetChange::HERO_LEAVE:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken HERO_LEAVE.");
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: HERO_LEAVE, missing unit %d.", id);
					else
						game_gui->mp_box->Add(Format(game->txMpNPCLeft, unit->GetName()));
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
					game->paused = is_paused;
					game_gui->mp_box->Add(is_paused ? game->txGamePaused : game->txGameResumed);
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
				else if(game->game_state == GS_WORLDMAP)
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
				else if(game->game_state == GS_WORLDMAP)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: USABLE_SOUND, missing unit %d.", id);
					else if(!unit->usable)
						Error("Update client: USABLE_SOUND, unit %d (%s) don't use usable.", id, unit->data->id.c_str());
					else if(unit != pc.unit)
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(unit && unit->player)
					{
						if(unit->player != game->pc)
							unit->player->UseAction(true);
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update client: USE_ITEM, missing unit %d.", id);
					else
					{
						unit->action = A_USE_ITEM;
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
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
					StartFastTravel(2);
					break;
				case FAST_TRAVEL_CANCEL:
				case FAST_TRAVEL_DENY:
				case FAST_TRAVEL_NOT_SAFE:
				case FAST_TRAVEL_IN_PROGRESS:
					CancelFastTravel(option, id);
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
				PlayerInfo* info = TryGetPlayer(id);
				if(!info)
				{
					Error("Update client: FAST_TRAVEL_VOTE invaid player %u.", id);
					break;
				}
				info->fast_travel = true;
			}
			break;
		// start of cutscene
		case NetChange::CUTSCENE_START:
			{
				bool instant;
				f >> instant;
				if(!f)
					Error("Update client: Broken CUTSCENE_START.");
				else
					game->CutsceneStart(instant);
			}
			break;
		// queue cutscene image to show
		case NetChange::CUTSCENE_IMAGE:
			{
				float time;
				const string& image = f.ReadString1();
				f >> time;
				if(!f)
					Error("Update client: Broken CUTSCENE_IMAGE.");
				else
					game->CutsceneImage(image, time);
			}
			break;
		// queue cutscene text to show
		case NetChange::CUTSCENE_TEXT:
			{
				float time;
				const string& text = f.ReadString1();
				f >> time;
				if(!f)
					Error("Update client: Broken CUTSCENE_TEXT.");
				else
					game->CutsceneText(text, time);
			}
			break;
		// skip current cutscene
		case NetChange::CUTSCENE_SKIP:
			game->CutsceneEnded(false);
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
bool Net::ProcessControlMessageClientForMe(BitStreamReader& f)
{
	PlayerController& pc = *game->pc;

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
		f >> pc.last_dmg_poison;
	if(IsSet(flags, PlayerInfo::UF_GOLD))
		f >> pc.unit->gold;
	if(IsSet(flags, PlayerInfo::UF_ALCOHOL))
		f >> pc.unit->alcohol;
	if(IsSet(flags, PlayerInfo::UF_LEARNING_POINTS))
		f >> pc.learning_points;
	if(IsSet(flags, PlayerInfo::UF_LEVEL))
		f >> pc.unit->level;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES(2).");
		return true;
	}

	// changes
	byte changes_count;
	f >> changes_count;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES at changes.");
		return true;
	}

	for(byte change_i = 0; change_i < changes_count; ++change_i)
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
				else if(game->game_state == GS_LEVEL)
				{
					pc.unit->AddItem2(pc.data.picking_item->item, (uint)count, (uint)team_count, false);
					if(pc.data.picking_item->item->type == IT_GOLD)
						sound_mgr->PlaySound2d(game_res->sCoins);
					if(pc.data.picking_item_state == 2)
						delete pc.data.picking_item;
					pc.data.picking_item_state = 0;
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

				if(game->game_state != GS_LEVEL)
					break;

				if(pc.unit->action == A_PREPARE)
					pc.unit->action = A_NONE;
				if(!can_loot)
				{
					game_gui->messages->AddGameMsg3(GMS_IS_LOOTED);
					pc.action = PlayerAction::None;
					break;
				}

				// read items
				if(!f.ReadItemListTeam(*pc.chest_trade))
				{
					Error("Update single client: Broken LOOT(2).");
					break;
				}

				// start trade
				if(pc.action == PlayerAction::LootUnit)
					game_gui->inventory->StartTrade(I_LOOT_BODY, *pc.action_unit);
				else if(pc.action == PlayerAction::LootContainer)
					game_gui->inventory->StartTrade(I_LOOT_CONTAINER, pc.action_usable->container->items);
				else
					game_gui->inventory->StartTrade(I_LOOT_CHEST, pc.action_chest->items);
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
					pc.action = PlayerAction::None;
					game_gui->messages->AddGameMsg3(GMS_UNIT_BUSY);
				}
				else if(id != -2 && game->game_state == GS_LEVEL) // not entered/left building
				{
					// start dialog
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: START_DIALOG, missing unit %d.", id);
					else
					{
						pc.action = PlayerAction::Talk;
						pc.action_unit = unit;
						game->dialog_context.StartDialog(unit);
						if(!game->predialog.empty())
						{
							game->dialog_context.dialog_s_text = game->predialog;
							game->dialog_context.dialog_text = game->dialog_context.dialog_s_text.c_str();
							game->dialog_context.dialog_wait = 1.f;
							game->predialog.clear();
						}
						else if(unit->bubble)
						{
							game->dialog_context.dialog_s_text = unit->bubble->text;
							game->dialog_context.dialog_text = game->dialog_context.dialog_s_text.c_str();
							game->dialog_context.dialog_wait = 1.f;
							game->dialog_context.skip_id = unit->bubble->skip_id;
						}
						pc.data.before_player = BP_NONE;
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
				else if(game->game_state != GS_LEVEL)
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
					game->dialog_context.choice_selected = 0;
					game->dialog_context.show_choices = true;
					game->dialog_context.dialog_esc = escape;
					game->dialog_choices.resize(count);
					for(byte i = 0; i < count; ++i)
					{
						f >> game->dialog_choices[i];
						if(!f)
						{
							Error("Update single client: Broken SHOW_DIALOG_CHOICES at %u index.", i);
							break;
						}
					}
					game_gui->level_gui->UpdateScrollbar(game->dialog_choices.size());
				}
			}
			break;
		// end of dialog
		case NetChangePlayer::END_DIALOG:
			if(game->game_state == GS_LEVEL)
			{
				game->dialog_context.dialog_mode = false;
				if(pc.action == PlayerAction::Talk)
					pc.action = PlayerAction::None;
				pc.unit->look_target = nullptr;
			}
			break;
		// start trade
		case NetChangePlayer::START_TRADE:
			{
				int id;
				f >> id;
				if(!f.ReadItemList(game->chest_trade))
				{
					Error("Update single client: Broken START_TRADE.");
					break;
				}

				if(game->game_state != GS_LEVEL)
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

				game_gui->inventory->StartTrade(I_TRADE, game->chest_trade, trader);
			}
			break;
		// add multiple same items to inventory
		case NetChangePlayer::ADD_ITEMS:
			{
				int team_count, count;
				const Item* item;
				f >> team_count;
				f >> count;
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS, invalid count %d or team count %d.", count, team_count);
				else if(game->game_state == GS_LEVEL)
					pc.unit->AddItem2(item, (uint)count, (uint)team_count, false);
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
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_TRADER.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS_TRADER, invalid count %d or team count %d.", count, team_count);
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: ADD_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc.IsTradingWith(unit))
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
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_CHEST.");
				else if(count <= 0 || team_count > count)
					Error("Update single client: ADD_ITEMS_CHEST, invalid count %d or team count %d.", count, team_count);
				else if(game->game_state == GS_LEVEL)
				{
					Chest* chest = game_level->FindChest(id);
					if(!chest)
						Error("Update single client: ADD_ITEMS_CHEST, missing chest %d.", id);
					else if(pc.action != PlayerAction::LootChest || pc.action_chest != chest)
						Error("Update single client: ADD_ITEMS_CHEST, chest %d is not opened by player.", id);
					else
					{
						game_res->PreloadItem(item);
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
				else if(game->game_state == GS_LEVEL)
					pc.unit->RemoveItem(i_index, (uint)count);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: REMOVE_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc.IsTradingWith(unit))
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
				else if(game->game_state == GS_LEVEL)
					pc.unit->frozen = frozen;
			}
			break;
		// remove quest item from inventory
		case NetChangePlayer::REMOVE_QUEST_ITEM:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken REMOVE_QUEST_ITEM.");
				else if(game->game_state == GS_LEVEL)
					pc.unit->RemoveQuestItem(id);
			}
			break;
		// someone else is using usable
		case NetChangePlayer::USE_USABLE:
			if(game->game_state == GS_LEVEL)
			{
				game_gui->messages->AddGameMsg3(GMS_USED);
				if(pc.action == PlayerAction::LootContainer)
					pc.action = PlayerAction::None;
				if(pc.unit->action == A_PREPARE)
					pc.unit->action = A_NONE;
			}
			break;
		// change development mode for player
		case NetChangePlayer::DEVMODE:
			{
				bool allowed;
				f >> allowed;
				if(!f)
					Error("Update single client: Broken CHEATS.");
				else if(game->devmode != allowed)
				{
					game_gui->AddMsg(allowed ? game->txDevmodeOn : game->txDevmodeOff);
					game->devmode = allowed;
				}
			}
			break;
		// start sharing/giving items
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				cstring name = (type == NetChangePlayer::START_SHARE ? "START_SHARE" : "START_GIVE");
				if(pc.action_unit && pc.action_unit->IsTeamMember() && game->game_state == GS_LEVEL)
				{
					Unit& unit = *pc.action_unit;
					f >> unit.weight;
					f >> unit.weight_max;
					f >> unit.gold;
					unit.stats = unit.data->GetStats(unit.level);
					if(!f.ReadItemListTeam(unit.items))
						Error("Update single client: Broken %s.", name);
					else
						game_gui->inventory->StartTrade(type == NetChangePlayer::START_SHARE ? I_SHARE : I_GIVE, unit);
				}
				else
				{
					if(game->game_state == GS_LEVEL)
						Error("Update single client: %s, player is not talking with team member.", name);
					// try to skip
					UnitStats stats;
					vector<ItemSlot> items;
					f.Skip(sizeof(int) * 3);
					stats.Read(f);
					if(!f.ReadItemListTeam(items, true))
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					PlayerInfo* info = TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: PVP, invalid player id %u.", player_id);
					else
						game->arena->ShowPvpRequest(info->u);
				}
			}
			break;
		// preparing to warp
		case NetChangePlayer::PREPARE_WARP:
			if(game->game_state == GS_LEVEL)
			{
				game->fallback_type = FALLBACK::WAIT_FOR_WARP;
				game->fallback_t = -1.f;
				pc.unit->frozen = (pc.unit->usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
			}
			break;
		// entering arena
		case NetChangePlayer::ENTER_ARENA:
			if(game->game_state == GS_LEVEL)
			{
				game->fallback_type = FALLBACK::ARENA;
				game->fallback_t = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// start of arena combat
		case NetChangePlayer::START_ARENA_COMBAT:
			if(game->game_state == GS_LEVEL)
				pc.unit->frozen = FROZEN::NO;
			break;
		// exit from arena
		case NetChangePlayer::EXIT_ARENA:
			if(game->game_state == GS_LEVEL)
			{
				game->fallback_type = FALLBACK::ARENA_EXIT;
				game->fallback_t = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// player refused to pvp
		case NetChangePlayer::NO_PVP:
			{
				byte player_id;
				f >> player_id;
				if(!f)
					Error("Update single client: Broken NO_PVP.");
				else if(game->game_state == GS_LEVEL)
				{
					PlayerInfo* info = TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: NO_PVP, invalid player id %u.", player_id);
					else
						game_gui->AddMsg(Format(game->txPvpRefuse, info->name.c_str()));
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
				else if(game->game_state == GS_LEVEL)
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
				else if(game->game_state == GS_LEVEL)
				{
					if(id == -1)
						pc.unit->look_target = nullptr;
					else
					{
						Unit* unit = game_level->FindUnit(id);
						if(!unit)
							Error("Update single client: LOOK_AT, missing unit %d.", id);
						else
							pc.unit->look_target = unit;
					}
				}
			}
			break;
		// end of fallback
		case NetChangePlayer::END_FALLBACK:
			if(game->game_state == GS_LEVEL && game->fallback_type == FALLBACK::CLIENT)
				game->fallback_type = FALLBACK::CLIENT2;
			break;
		// response to rest in inn
		case NetChangePlayer::REST:
			{
				byte days;
				f >> days;
				if(!f)
					Error("Update single client: Broken REST.");
				else if(game->game_state == GS_LEVEL)
				{
					game->fallback_type = FALLBACK::REST;
					game->fallback_t = -1.f;
					game->fallback_1 = days;
					pc.unit->frozen = FROZEN::YES;
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
				else if(game->game_state == GS_LEVEL)
				{
					game->fallback_type = FALLBACK::TRAIN;
					game->fallback_t = -1.f;
					game->fallback_1 = type;
					game->fallback_2 = stat_type;
					pc.unit->frozen = FROZEN::YES;
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
				else if(game->game_state == GS_LEVEL)
				{
					pc.unit->pos = new_pos;
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
					PlayerInfo* info = TryGetPlayer(player_id);
					if(!info)
						Error("Update single client: GOLD_RECEIVED, invalid player id %u.", player_id);
					else
					{
						game_gui->mp_box->Add(Format(game->txReceivedGold, count, info->name.c_str()));
						sound_mgr->PlaySound2d(game_res->sCoins);
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					if(!unit)
						Error("Update single client: UPDATE_TRADER_GOLD, missing unit %d.", id);
					else if(!pc.IsTradingWith(unit))
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
				else if(game->game_state == GS_LEVEL)
				{
					Unit* unit = game_level->FindUnit(id);
					bool ok = true;
					if(!unit)
					{
						Error("Update single client: UPDATE_TRADER_INVENTORY, missing unit %d.", id);
						ok = false;
					}
					else if(!pc.IsTradingWith(unit))
					{
						Error("Update single client: UPDATE_TRADER_INVENTORY, unit %d (%s) is not trading with player.",
							id, unit->data->id.c_str());
						ok = false;
					}
					if(ok)
					{
						if(!f.ReadItemListTeam(unit->items))
							Error("Update single client: Broken UPDATE_TRADER_INVENTORY(2).");
					}
					else
					{
						vector<ItemSlot> items;
						if(!f.ReadItemListTeam(items, true))
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
							pc.kills = *buf++;
						if(IsSet(flags, STAT_DMG_DONE))
							pc.dmg_done = *buf++;
						if(IsSet(flags, STAT_DMG_TAKEN))
							pc.dmg_taken = *buf++;
						if(IsSet(flags, STAT_KNOCKS))
							pc.knocks = *buf++;
						if(IsSet(flags, STAT_ARENA_FIGHTS))
							pc.arena_fights = *buf++;
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
							pc.unit->Set((AttributeId)what, value);
						break;
					case ChangedStatType::SKILL:
						if(what >= (byte)SkillId::MAX)
							Error("Update single client: STAT_CHANGED, invalid skill %u.", what);
						else
							pc.unit->Set((SkillId)what, value);
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
					pc.AddPerk(perk, value);
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
					pc.RemovePerk(perk, value);
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
					pc.unit->AddEffect(e);
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
					pc.unit->RemoveEffects(effect, source, source_id, value);
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
					pc.Rest(days, false, false);
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
					game_gui->messages->AddFormattedMessage(game->pc, id, subtype, value);
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
					sound_mgr->PlaySound2d(game_res->sCoins);
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
void Net::Client_Say(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_Say: Broken packet.");
	else
	{
		PlayerInfo* info = TryGetPlayer(id);
		if(!info)
			Error("Client_Say: Broken packet, missing player %d.", id);
		else
		{
			cstring s = Format("%s: %s", info->name.c_str(), text.c_str());
			game_gui->AddMsg(s);
			if(game->game_state == GS_LEVEL)
				game_gui->level_gui->AddSpeechBubble(info->u, text.c_str());
		}
	}
}

//=================================================================================================
void Net::Client_Whisper(BitStreamReader& f)
{
	byte id;

	f >> id;
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_Whisper: Broken packet.");
	else
	{
		PlayerInfo* info = TryGetPlayer(id);
		if(!info)
			Error("Client_Whisper: Broken packet, missing player %d.", id);
		else
		{
			cstring s = Format("%s@: %s", info->name.c_str(), text.c_str());
			game_gui->AddMsg(s);
		}
	}
}

//=================================================================================================
void Net::Client_ServerSay(BitStreamReader& f)
{
	const string& text = f.ReadString1();
	if(!f)
		Error("Client_ServerSay: Broken packet.");
	else
		net->AddServerMsg(text.c_str());
}

//=================================================================================================
void Net::FilterClientChanges()
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
}

//=================================================================================================
bool Net::FilterOut(NetChange& c)
{
	switch(c.type)
	{
	case NetChange::CHANGE_EQUIPMENT:
		return IsServer();
	case NetChange::CHANGE_FLAGS:
	case NetChange::UPDATE_CREDIT:
	case NetChange::ALL_QUESTS_COMPLETED:
	case NetChange::CHANGE_LOCATION_STATE:
	case NetChange::ADD_RUMOR:
	case NetChange::ADD_NOTE:
	case NetChange::REGISTER_ITEM:
	case NetChange::ADD_QUEST:
	case NetChange::UPDATE_QUEST:
	case NetChange::RENAME_ITEM:
	case NetChange::REMOVE_PLAYER:
	case NetChange::CHANGE_LEADER:
	case NetChange::RANDOM_NUMBER:
	case NetChange::CHEAT_SKIP_DAYS:
	case NetChange::CHEAT_NOCLIP:
	case NetChange::CHEAT_GODMODE:
	case NetChange::CHEAT_INVISIBLE:
	case NetChange::CHEAT_ADD_ITEM:
	case NetChange::CHEAT_ADD_GOLD:
	case NetChange::CHEAT_SET_STAT:
	case NetChange::CHEAT_MOD_STAT:
	case NetChange::CHEAT_REVEAL:
	case NetChange::GAME_OVER:
	case NetChange::CHEAT_CITIZEN:
	case NetChange::WORLD_TIME:
	case NetChange::ADD_LOCATION:
	case NetChange::REMOVE_CAMP:
	case NetChange::CHEAT_NOAI:
	case NetChange::END_OF_GAME:
	case NetChange::UPDATE_FREE_DAYS:
	case NetChange::CHANGE_MP_VARS:
	case NetChange::PAY_CREDIT:
	case NetChange::GIVE_GOLD:
	case NetChange::DROP_GOLD:
	case NetChange::HERO_LEAVE:
	case NetChange::PAUSED:
	case NetChange::CLOSE_ENCOUNTER:
	case NetChange::GAME_STATS:
	case NetChange::CHANGE_ALWAYS_RUN:
	case NetChange::SET_SHORTCUT:
		return false;
	case NetChange::TALK:
	case NetChange::TALK_POS:
		if(IsServer() && c.str)
		{
			StringPool.Free(c.str);
			RemoveElement(net_strs, c.str);
			c.str = nullptr;
		}
		return true;
	case NetChange::RUN_SCRIPT:
	case NetChange::CHEAT_ARENA:
	case NetChange::CUTSCENE_IMAGE:
	case NetChange::CUTSCENE_TEXT:
		StringPool.Free(c.str);
		return true;
	default:
		return true;
	}
}

//=================================================================================================
void Net::ReadNetVars(BitStreamReader& f)
{
	f >> mp_use_interp;
	f >> mp_interp;
}

//=================================================================================================
bool Net::ReadWorldData(BitStreamReader& f)
{
	// world
	if(!world->Read(f))
	{
		Error("Read world: Broken packet for world data.");
		return false;
	}

	// quests
	if(!quest_mgr->Read(f))
		return false;

	// rumors
	f.ReadStringArray<byte, word>(game_gui->journal->GetRumors());
	if(!f)
	{
		Error("Read world: Broken packet for rumors.");
		return false;
	}

	// game stats
	game_stats->Read(f);
	if(!f)
	{
		Error("Read world: Broken packet for game stats.");
		return false;
	}

	// mp vars
	ReadNetVars(f);
	if(!f)
	{
		Error("Read world: Broken packet for mp vars.");
		return false;
	}

	// secret note text
	f >> Quest_Secret::GetNote().desc;
	if(!f)
	{
		Error("Read world: Broken packet for secret note text.");
		return false;
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read world: Broken checksum.");
		return false;
	}

	// load music
	game_res->LoadCommonMusic();

	return true;
}

//=================================================================================================
bool Net::ReadPlayerStartData(BitStreamReader& f)
{
	game->pc = new PlayerController;

	f >> game->devmode;
	f >> game->noai;
	game->pc->ReadStart(f);
	f.ReadStringArray<word, word>(game_gui->journal->GetNotes());
	if(!f)
		return false;

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read player start data: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
bool Net::ReadLevelData(BitStreamReader& f)
{
	game_level->camera.Reset();
	game->pc->data.rot_buf = 0.f;
	world->RemoveBossLevel();

	bool loaded_resources;
	f >> mp_load;
	f >> loaded_resources;
	if(!f)
	{
		Error("Read level: Broken packet for loading info.");
		return false;
	}

	if(!game_level->Read(f, loaded_resources))
		return false;

	// items to preload
	uint items_load_count = f.Read<uint>();
	if(!f.Ensure(items_load_count * 2))
	{
		Error("Read level: Broken items preload count.");
		return false;
	}
	std::set<const Item*>& items_load = game->items_load;
	items_load.clear();
	for(uint i = 0; i < items_load_count; ++i)
	{
		const string& item_id = f.ReadString1();
		if(!f)
		{
			Error("Read level: Broken item preload '%u'.", i);
			return false;
		}
		if(item_id[0] != '$')
		{
			auto item = Item::TryGet(item_id);
			if(!item)
			{
				Error("Read level: Missing item preload '%s'.", item_id.c_str());
				return false;
			}
			items_load.insert(item);
		}
		else
		{
			int quest_id = f.Read<int>();
			if(!f)
			{
				Error("Read level: Broken quest item preload '%u'.", i);
				return false;
			}
			const Item* item = quest_mgr->FindQuestItemClient(item_id.c_str(), quest_id);
			if(!item)
			{
				Error("Read level: Missing quest item preload '%s' (%d).", item_id.c_str(), quest_id);
				return false;
			}
			const Item* base = Item::TryGet(item_id.c_str() + 1);
			if(!base)
			{
				Error("Read level: Missing quest item preload base '%s' (%d).", item_id.c_str(), quest_id);
				return false;
			}
			items_load.insert(base);
			items_load.insert(item);
		}
	}

	// checksum
	byte check;
	f >> check;
	if(!f || check != 0xFF)
	{
		Error("Read level: Broken checksum.");
		return false;
	}

	return true;
}

//=================================================================================================
bool Net::ReadPlayerData(BitStreamReader& f)
{
	PlayerInfo& info = GetMe();

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
	game->pc = unit->player;
	game->pc->player_info = &info;
	info.pc = game->pc;
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
	if(!f.ReadItemListTeam(unit->items))
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
	if(!f || !game->pc->Read(f))
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
	PlayerInfo* leader_info = TryGetPlayer(team->leader_id);
	if(!leader_info)
	{
		Error("Read player data: Missing player %d.", team->leader_id);
		return false;
	}
	team->leader = leader_info->u;

	game->dialog_context.pc = unit->player;

	// multiplayer load data
	if(mp_load)
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
void Net::SendClient(BitStreamWriter& f, PacketPriority priority, PacketReliability reliability)
{
	assert(IsClient());
	peer->Send(&f.GetBitStream(), priority, reliability, 0, server, false);
}
