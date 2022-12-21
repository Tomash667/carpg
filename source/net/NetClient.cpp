#include "Pch.h"
#include "Net.h"

#include "Ability.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "Cave.h"
#include "City.h"
#include "CommandParser.h"
#include "Console.h"
#include "CraftPanel.h"
#include "Door.h"
#include "Electro.h"
#include "EntityInterpolator.h"
#include "Explo.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "GroundItem.h"
#include "InfoBox.h"
#include "Inventory.h"
#include "Journal.h"
#include "Level.h"
#include "LevelGui.h"
#include "LevelPart.h"
#include "LoadScreen.h"
#include "MpBox.h"
#include "MultiInsideLocation.h"
#include "Object.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Team.h"
#include "TeamPanel.h"
#include "Trap.h"
#include "Unit.h"
#include "World.h"
#include "WorldMapGui.h"

#include <DialogBox.h>
#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <SoundManager.h>

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

	if(IsDebug())
		peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS);
}

//=================================================================================================
void Net::OnNewGameClient()
{
	DeleteElements(questMgr->questItems);
	game->devmode = game->defaultDevmode;
	game->trainMove = 0.f;
	team->anyoneTalking = false;
	gameLevel->canFastTravel = false;
	interpolateTimer = 0.f;
	changes.clear();
	if(!netStrs.empty())
		StringPool.Free(netStrs);
	game->paused = false;
	game->hardcoreMode = false;
	if(!mpQuickload)
	{
		gameGui->mpBox->Reset();
		gameGui->mpBox->visible = true;
	}
}
//=================================================================================================
void Net::UpdateClient(float dt)
{
	if(game->gameState == GS_LEVEL)
	{
		float gameDt = dt * game->gameSpeed;

		// interpolacja pozycji gracza
		if(interpolateTimer > 0.f)
		{
			interpolateTimer -= gameDt;
			if(interpolateTimer >= 0.f)
				game->pc->unit->visualPos = Vec3::Lerp(game->pc->unit->visualPos, game->pc->unit->pos, (0.1f - interpolateTimer) * 10);
			else
				game->pc->unit->visualPos = game->pc->unit->pos;
		}

		// interpolacja pozycji/obrotu postaci
		InterpolateUnits(dt);
	}

	Packet* packet;
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		BitStreamReader reader(packet);
		byte msgId;
		reader >> msgId;

		switch(msgId)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			{
				cstring text, textEng;
				if(packet->data[0] == ID_CONNECTION_LOST)
				{
					text = game->txLostConnection;
					textEng = "Update client: Lost connection with server.";
				}
				else
				{
					text = game->txYouDisconnected;
					textEng = "Update client: You have been disconnected.";
				}
				Info(textEng);
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
		case ID_SERVER_CLOSE:
			{
				byte reason = (packet->length == 2 ? packet->data[1] : 0);
				cstring reasonText, reasonTextInt;
				if(reason == ServerClose_Kicked)
				{
					reasonText = "You have been kicked out.";
					reasonTextInt = game->txYouKicked;
				}
				else
				{
					reasonText = "Server was closed.";
					reasonTextInt = game->txServerClosed;
				}
				Info("Update client: %s", reasonText);
				peer->DeallocatePacket(packet);
				game->ExitToMenu();
				gui->SimpleDialog(reasonTextInt, nullptr);
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
					gameLevel->dungeonLevel = level;
					Info("Update client: Level change to %s (%d, level %d).", gameLevel->location->name.c_str(), gameLevel->locationIndex, gameLevel->dungeonLevel);
					gameGui->infoBox->Show(game->txGeneratingLocation);
					game->LeaveLevel();
					game->netMode = Game::NM_TRANSFER;
					game->netState = NetState::Client_ChangingLevel;
					gameGui->loadScreen->visible = true;
					gameGui->levelGui->visible = false;
					gameGui->worldMap->Hide();
					game->arena->ClosePvpDialog();
					if(gameGui->worldMap->dialogEnc)
					{
						gui->CloseDialog(gameGui->worldMap->dialogEnc);
						gameGui->worldMap->dialogEnc = nullptr;
					}
					peer->DeallocatePacket(packet);
					FilterClientChanges();
					game->LoadingStart(4);
					return;
				}
			}
			break;
		case ID_CHANGES:
			if(!ProcessControlMessageClient(reader))
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
				mpQuickload = true;
				game->ClearGame();
				reader >> mpLoadWorldmap;
				game->LoadingStart(mpLoadWorldmap ? 4 : 9);
				gameGui->infoBox->Show(game->txLoadingSaveByServer);
				gameGui->worldMap->Hide();
				game->netMode = Game::NM_TRANSFER;
				game->netState = NetState::Client_BeforeTransfer;
				game->gameState = GS_LOAD;
				gameLevel->ready = false;
				game->itemsLoad.clear();
			}
			break;
		default:
			Warn("UpdateClient: Unknown packet from server: %u.", msgId);
			break;
		}
	}

	// send my position/action
	updateTimer += dt;
	if(updateTimer >= TICK)
	{
		updateTimer = 0;
		BitStreamWriter f;
		f << ID_CONTROL;
		if(game->gameState == GS_LEVEL)
		{
			f << true;
			f << game->pc->unit->pos;
			f << game->pc->unit->rot;
			f << game->pc->unit->meshInst->groups[0].speed;
			f.WriteCasted<byte>(game->pc->unit->animation);
		}
		else
			f << false;
		WriteClientChanges(f);
		changes.clear();
		SendClient(f, HIGH_PRIORITY, RELIABLE_ORDERED);
	}
}

//=================================================================================================
void Net::InterpolateUnits(float dt)
{
	for(LocationPart& locPart : gameLevel->ForEachPart())
	{
		for(Unit* unit : locPart.units)
		{
			if(!unit->IsLocalPlayer())
				unit->interp->Update(dt, unit->visualPos, unit->rot);
			if(unit->meshInst->mesh->head.nGroups == 1)
			{
				if(!unit->meshInst->groups[0].anim)
				{
					unit->action = A_NONE;
					unit->animation = ANI_STAND;
				}
			}
			else
			{
				if(!unit->meshInst->groups[0].anim && !unit->meshInst->groups[1].anim)
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
			f.WriteCasted<byte>(c.id == 0 ? pc.unit->weaponTaken : pc.unit->weaponHiding);
			break;
		case NetChange::ATTACK:
			{
				byte b = (byte)c.id;
				b |= ((pc.unit->act.attack.index & 0xF) << 4);
				f << b;
				f << c.f[1];
				if(c.id == AID_Shoot)
					f << pc.unit->targetPos;
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
		case NetChange::TRAVEL:
		case NetChange::CHEAT_TRAVEL:
		case NetChange::REST:
		case NetChange::FAST_TRAVEL:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CHEAT_WARP_TO_ENTRY:
		case NetChange::CHEAT_CHANGE_LEVEL:
		case NetChange::CHEAT_NOAI:
		case NetChange::PVP:
		case NetChange::CHANGE_ALWAYS_RUN:
			f << (c.id != 0);
			break;
		case NetChange::PICKUP_ITEM:
		case NetChange::LOOT_UNIT:
		case NetChange::TALK:
		case NetChange::USE_CHEST:
		case NetChange::SKIP_DIALOG:
		case NetChange::PAY_CREDIT:
		case NetChange::DROP_GOLD:
		case NetChange::TAKE_ITEM_CREDIT:
		case NetChange::CLEAN_LEVEL:
			f << c.id;
			break;
		case NetChange::STOP_TRADE:
		case NetChange::GET_ALL_ITEMS:
		case NetChange::EXIT_BUILDING:
		case NetChange::WARP:
		case NetChange::CHEAT_GOTO_MAP:
		case NetChange::CHEAT_REVEAL_MINIMAP:
		case NetChange::ENTER_LOCATION:
		case NetChange::CLOSE_ENCOUNTER:
		case NetChange::YELL:
		case NetChange::END_FALLBACK:
		case NetChange::END_TRAVEL:
		case NetChange::CUTSCENE_SKIP:
			break;
		case NetChange::ADD_NOTE:
			f << gameGui->journal->GetNotes().back();
			break;
		case NetChange::USE_USABLE:
			f << c.id;
			f.WriteCasted<byte>(c.count);
			break;
		case NetChange::CHEAT_ADD_ITEM:
			f << c.baseItem->id;
			f << c.count;
			f << (c.id != 0);
			break;
		case NetChange::CHEAT_SPAWN_UNIT:
			f << c.baseUnit->id;
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
			f << c.count;
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
		case NetChange::PLAYER_ABILITY:
			f << (c.unit ? c.unit->id : -1);
			f << c.pos;
			f << c.ability->hash;
			break;
		case NetChange::RUN_SCRIPT:
			f.WriteString4(*c.str);
			StringPool.Free(c.str);
			f << c.id;
			break;
		case NetChange::SET_NEXT_ACTION:
			f.WriteCasted<byte>(pc.nextAction);
			switch(pc.nextAction)
			{
			case NA_NONE:
				break;
			case NA_REMOVE:
			case NA_DROP:
				f.WriteCasted<byte>(pc.nextActionData.slot);
				break;
			case NA_EQUIP:
			case NA_CONSUME:
			case NA_EQUIP_DRAW:
				f << pc.nextActionData.index;
				f << pc.nextActionData.item->id;
				break;
			case NA_USE:
				f << pc.nextActionData.usable->id;
				break;
			default:
				assert(0);
				break;
			}
			break;
		case NetChange::GENERIC_CMD:
			f.WriteCasted<byte>(c.size);
			f.Write(&c.data, c.size);
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
				switch(shortcut.type)
				{
				case Shortcut::TYPE_SPECIAL:
					f.WriteCasted<byte>(shortcut.value);
					break;
				case Shortcut::TYPE_ITEM:
					f << shortcut.item->id;
					break;
				case Shortcut::TYPE_ABILITY:
					f << shortcut.ability->hash;
					break;
				}
			}
			break;
		case NetChange::CAST_SPELL:
			f << c.pos;
			break;
		case NetChange::CRAFT:
			f << c.recipe->hash;
			f << c.count;
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
bool Net::ProcessControlMessageClient(BitStreamReader& f)
{
	PlayerController& pc = *game->pc;

	// read count
	word changesCount;
	f >> changesCount;
	if(!f)
	{
		Error("Update client: Broken ID_CHANGES.");
		return true;
	}

	// read changes
	for(word changeI = 0; changeI < changesCount; ++changeI)
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
				float rot, aniSpeed;
				Animation ani;
				f >> id;
				f >> pos;
				f >> rot;
				f >> aniSpeed;
				f.ReadCasted<byte>(ani);
				if(!f)
				{
					Error("Update client: Broken UNIT_POS.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Unit* unit = gameLevel->FindUnit(id);
				if(!unit)
					Error("Update client: UNIT_POS, missing unit %d.", id);
				else if(unit != pc.unit)
				{
					unit->pos = pos;
					unit->meshInst->groups[0].speed = aniSpeed;
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
				ITEM_SLOT slot;
				const Item* item;
				f >> id;
				f.ReadCasted<byte>(slot);
				if(!f || f.ReadItemAndFind(item) < 0)
					Error("Update client: Broken CHANGE_EQUIPMENT.");
				else if(!IsValid(slot))
					Error("Update client: CHANGE_EQUIPMENT, invalid slot %d.", slot);
				else if(game->gameState == GS_LEVEL)
				{
					Unit* target = gameLevel->FindUnit(id);
					if(!target)
						Error("Update client: CHANGE_EQUIPMENT, missing unit %d.", id);
					else
						target->ReplaceItem(slot, item);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: TAKE_WEAPON, missing unit %d.", id);
					else if(unit != pc.unit)
						unit->SetWeaponState(!hide, type, false);
				}
			}
			break;
		// unit attack
		case NetChange::ATTACK:
			{
				int id;
				byte typeflags;
				float attackSpeed;
				f >> id;
				f >> typeflags;
				f >> attackSpeed;
				if(!f)
				{
					Error("Update client: Broken ATTACK.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Unit* unitPtr = gameLevel->FindUnit(id);
				if(!unitPtr)
				{
					Error("Update client: ATTACK, missing unit %d.", id);
					break;
				}

				// don't start attack if this is local unit
				if(unitPtr == pc.unit)
					break;

				Unit& unit = *unitPtr;
				byte type = (typeflags & 0xF);
				int group = unit.meshInst->mesh->head.nGroups - 1;

				bool isBow = false;
				switch(type)
				{
				case AID_Attack:
					if(unit.action == A_ATTACK && unit.animationState == AS_ATTACK_PREPARE)
					{
						unit.animationState = AS_ATTACK_CAN_HIT;
						unit.meshInst->groups[group].speed = attackSpeed;
					}
					else
					{
						if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
							game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
						unit.action = A_ATTACK;
						unit.animationState = AS_ATTACK_CAN_HIT;
						unit.act.attack.index = ((typeflags & 0xF0) >> 4);
						unit.act.attack.power = 1.f;
						unit.act.attack.run = false;
						unit.act.attack.hitted = false;
						unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, group);
						unit.meshInst->groups[group].speed = attackSpeed;
					}
					break;
				case AID_PrepareAttack:
					if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
						game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
					unit.action = A_ATTACK;
					unit.animationState = AS_ATTACK_PREPARE;
					unit.act.attack.index = ((typeflags & 0xF0) >> 4);
					unit.act.attack.power = 1.f;
					unit.act.attack.run = false;
					unit.act.attack.hitted = false;
					unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, group);
					unit.meshInst->groups[group].speed = attackSpeed;
					break;
				case AID_Shoot:
				case AID_StartShoot:
					isBow = true;
					if(unit.action == A_SHOOT && unit.animationState == AS_SHOOT_PREPARE)
						unit.animationState = AS_SHOOT_CAN;
					else
						unit.DoRangedAttack(type == AID_StartShoot, false, attackSpeed);
					break;
				case AID_Block:
					unit.action = A_BLOCK;
					unit.meshInst->Play(NAMES::aniBlock, PLAY_PRIO1 | PLAY_STOP_AT_END, group);
					unit.meshInst->groups[group].blendMax = attackSpeed;
					break;
				case AID_Bash:
					unit.action = A_BASH;
					unit.animationState = AS_BASH_ANIMATION;
					unit.meshInst->Play(NAMES::aniBash, PLAY_ONCE | PLAY_PRIO1, group);
					unit.meshInst->groups[group].speed = attackSpeed;
					break;
				case AID_RunningAttack:
					if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
						game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
					unit.action = A_ATTACK;
					unit.animationState = AS_ATTACK_CAN_HIT;
					unit.act.attack.index = ((typeflags & 0xF0) >> 4);
					unit.act.attack.power = 1.5f;
					unit.act.attack.run = true;
					unit.act.attack.hitted = false;
					unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, group);
					unit.meshInst->groups[group].speed = attackSpeed;
					break;
				case AID_Cancel:
					if(unit.action == A_SHOOT)
					{
						gameLevel->FreeBowInstance(unit.bowInstance);
						isBow = true;
					}
					unit.action = A_NONE;
					unit.meshInst->Deactivate(1);
					break;
				}

				unit.SetWeaponStateInstant(WeaponState::Taken, isBow ? W_BOW : W_ONE_HANDED);
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
					team->isBandit = IsSet(flags, F_IS_BANDIT);
					team->craziesAttack = IsSet(flags, F_CRAZIES_ATTACKING);
					team->anyoneTalking = IsSet(flags, F_ANYONE_TALKING);
					gameLevel->canFastTravel = IsSet(flags, F_CAN_FAST_TRAVEL);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: UPDATE_HP, missing unit %d.", id);
					else if(unit == pc.unit)
					{
						// handling of previous hp
						float hpDif = hpp - unit->GetHpp();
						if(hpDif < 0.f)
							pc.lastDmg += -hpDif * unit->hpmax;
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(netid);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(netid);
					if(!unit)
						Error("Update client: UPDATE_STAMINA, missing unit %d.", netid);
					else if(unit == pc.unit)
						unit->stamina = staminap * unit->staminaMax;
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
				else if(game->gameState == GS_LEVEL)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = gameRes->tBlood[type];
					pe->emissionInterval = 0.f;
					pe->life = 5.f;
					pe->particleLife = 0.5f;
					pe->emissions = 1;
					pe->spawn = Int2(10, 15);
					pe->maxParticles = 15;
					pe->pos = pos;
					pe->speedMin = Vec3(-1, 0, -1);
					pe->speedMax = Vec3(1, 1, 1);
					pe->posMin = Vec3(-0.1f, -0.1f, -0.1f);
					pe->posMax = Vec3(0.1f, 0.1f, 0.1f);
					pe->size = Vec2(0.3f, 0.f);
					pe->alpha = Vec2(0.9f, 0.f);
					pe->mode = 0;
					pe->Init();
					gameLevel->GetLocationPart(pos).lvlPart->pes.push_back(pe);
				}
			}
			break;
		// play unit sound
		case NetChange::UNIT_SOUND:
			{
				int id;
				SOUND_ID soundId;
				f >> id;
				f.ReadCasted<byte>(soundId);
				if(!f)
					Error("Update client: Broken UNIT_SOUND.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: UNIT_SOUND, missing unit %d.", id);
					else if(Sound* sound = unit->GetSound(soundId))
					{
						float dist;
						switch(soundId)
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
		// play unit misc sound
		case NetChange::UNIT_MISC_SOUND:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken UNIT_MISC_SOUND.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: UNIT_MISC_SOUND, missing unit %d.", id);
					else
						unit->PlaySound(gameRes->sCoughs, Unit::COUGHS_SOUND_DIST);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: DROP_ITEM, missing unit %d.", id);
					else if(unit != pc.unit)
					{
						unit->action = A_ANIMATION;
						unit->meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
					}
				}
			}
			break;
		// spawn item on ground
		case NetChange::SPAWN_ITEM:
			{
				GroundItem* groundItem = new GroundItem;
				if(!groundItem->Read(f))
				{
					Error("Update client: Broken SPAWN_ITEM.");
					delete groundItem;
				}
				else if(game->gameState != GS_LEVEL)
					delete groundItem;
				else
				{
					gameLevel->GetLocationPart(groundItem->pos)
						.AddGroundItem(groundItem, false);
				}
			}
			break;
		// unit picks up item
		case NetChange::PICKUP_ITEM:
			{
				int id;
				bool upAnimation;
				f >> id;
				f >> upAnimation;
				if(!f)
					Error("Update client: Broken PICKUP_ITEM.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: PICKUP_ITEM, missing unit %d.", id);
					else if(unit != pc.unit)
					{
						unit->action = A_PICKUP;
						unit->animation = ANI_PLAY;
						unit->meshInst->Play(upAnimation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
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
				else if(game->gameState == GS_LEVEL)
				{
					LocationPart* locPart;
					GroundItem* groundItem = gameLevel->FindGroundItem(id, &locPart);
					if(!groundItem)
						Error("Update client: REMOVE_ITEM, missing ground item %d.", id);
					else
						locPart->RemoveGroundItem(groundItem);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: CONSUME_ITEM, missing unit %d.", id);
					else if(item->type != IT_CONSUMABLE)
						Error("Update client: CONSUME_ITEM, %s is not consumable.", item->id.c_str());
					else if(unit != pc.unit || force)
					{
						gameRes->PreloadItem(item);
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
				else if(game->gameState == GS_LEVEL)
					soundMgr->PlaySound3d(gameRes->GetMaterialSound(mat1, mat2), pos, HIT_SOUND_DIST);
			}
			break;
		// unit get stunned
		case NetChange::STUNNED:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken STUNNED.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: STUNNED, missing unit %d.", id);
					else
					{
						unit->BreakAction();

						if(unit->action != A_POSITION)
							unit->action = A_PAIN;
						else
							unit->animationState = AS_POSITION_HURT;

						if(unit->meshInst->mesh->head.nGroups == 2)
							unit->meshInst->Play(NAMES::aniHurt, PLAY_PRIO1 | PLAY_ONCE, 1);
						else
						{
							unit->meshInst->Play(NAMES::aniHurt, PLAY_PRIO3 | PLAY_ONCE, 0);
							unit->animation = ANI_PLAY;
						}
					}
				}
			}
			break;
		// create shooted arrow
		case NetChange::SHOOT_ARROW:
			{
				int id, ownerId, abilityHash;
				Vec3 pos;
				float rotX, rotY, speed, speedY;
				f >> id;
				f >> ownerId;
				f >> pos;
				f >> rotX;
				f >> rotY;
				f >> speed;
				f >> speedY;
				f >> abilityHash;
				if(!f)
					Error("Update client: Broken SHOOT_ARROW.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* owner;
					if(ownerId == -1)
						owner = nullptr;
					else
					{
						owner = gameLevel->FindUnit(ownerId);
						if(!owner)
						{
							Error("Update client: SHOOT_ARROW, missing unit %d.", ownerId);
							break;
						}
					}

					Ability* ability = nullptr;
					if(abilityHash != 0)
					{
						ability = Ability::Get(abilityHash);
						if(!ability)
						{
							Error("Update client: SHOOT_ARROW, missing ability %d.", abilityHash);
							break;
						}
					}

					LocationPart& locPart = gameLevel->GetLocationPart(pos);

					Bullet* bullet = new Bullet;
					locPart.lvlPart->bullets.push_back(bullet);

					bullet->id = id;
					bullet->Register();
					bullet->isArrow = true;
					bullet->mesh = gameRes->aArrow;
					bullet->pos = pos;
					bullet->startPos = pos;
					bullet->rot = Vec3(rotX, rotY, 0);
					bullet->yspeed = speedY;
					bullet->owner = nullptr;
					bullet->pe = nullptr;
					bullet->speed = speed;
					bullet->ability = ability;
					bullet->tex = nullptr;
					bullet->texSize = 0.f;
					bullet->timer = ARROW_TIMER;
					bullet->owner = owner;

					if(bullet->ability && bullet->ability->mesh)
						bullet->mesh = bullet->ability->mesh;

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					if(bullet->ability)
					{
						tpe->color1 = bullet->ability->color;
						tpe->color2 = tpe->color1;
						tpe->color2.w = 0;
					}
					else
					{
						tpe->color1 = Vec4(1, 1, 1, 0.5f);
						tpe->color2 = Vec4(1, 1, 1, 0);
					}
					tpe->Init(50);
					locPart.lvlPart->tpes.push_back(tpe);
					bullet->trail = tpe;

					soundMgr->PlaySound3d(gameRes->sBow[Rand() % 2], bullet->pos, HIT_SOUND_DIST);
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
				else if(game->gameState != GS_LEVEL)
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
							Unit* unit = gameLevel->FindUnit(id);
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
				byte animationIndex;
				f >> id;
				f >> animationIndex;
				if(!f)
					Error("Update client: Broken IDLE.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: IDLE, missing unit %d.", id);
					else if(animationIndex >= unit->data->idles->anims.size())
						Error("Update client: IDLE, invalid animation index %u (count %u).", animationIndex, unit->data->idles->anims.size());
					else
					{
						unit->meshInst->Play(unit->data->idles->anims[animationIndex].c_str(), PLAY_ONCE, 0);
						unit->animation = ANI_IDLE;
					}
				}
			}
			break;
		// info about completing all unique quests
		case NetChange::ALL_QUESTS_COMPLETED:
			questMgr->uniqueCompletedShow = true;
			break;
		// unit talks
		case NetChange::TALK:
			{
				int id, skipId;
				byte animation;
				f >> id;
				f >> animation;
				f >> skipId;
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken TALK.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: TALK, missing unit %d.", id);
					else
						game->dialogContext.ClientTalk(unit, text, skipId, animation);
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
				else if(game->gameState == GS_LEVEL)
					gameGui->levelGui->AddSpeechBubble(pos, text.c_str());
			}
			break;
		// change location state
		case NetChange::CHANGE_LOCATION_STATE:
			{
				byte locationIndex;
				f >> locationIndex;
				if(!f)
					Error("Update client: Broken CHANGE_LOCATION_STATE.");
				else if(!world->VerifyLocation(locationIndex))
					Error("Update client: CHANGE_LOCATION_STATE, invalid location %u.", locationIndex);
				else
				{
					Location* loc = world->GetLocation(locationIndex);
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
					gameGui->messages->AddGameMsg3(GMS_ADDED_RUMOR);
					gameGui->journal->GetRumors().push_back(text);
					gameGui->journal->NeedUpdate(Journal::Rumors);
				}
			}
			break;
		// hero tells his name
		case NetChange::TELL_NAME:
			{
				int id;
				bool setName;
				f >> id;
				f >> setName;
				if(setName)
					f.ReadString1();
				if(!f)
					Error("Update client: Broken TELL_NAME.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: TELL_NAME, missing unit %d.", id);
					else if(!unit->IsHero())
						Error("Update client: TELL_NAME, unit %d (%s) is not a hero.", id, unit->data->id.c_str());
					else
					{
						unit->hero->knowName = true;
						if(setName)
							unit->hero->name = f.GetStringBuffer();
					}
				}
			}
			break;
		// change unit hair color
		case NetChange::HAIR_COLOR:
			{
				int id;
				Vec4 hairColor;
				f >> id;
				f >> hairColor;
				if(!f)
					Error("Update client: Broken HAIR_COLOR.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: HAIR_COLOR, missing unit %d.", id);
					else if(unit->data->type != UNIT_TYPE::HUMAN)
						Error("Update client: HAIR_COLOR, unit %d (%s) is not human.", id, unit->data->id.c_str());
					else
						unit->humanData->hairColor = hairColor;
				}
			}
			break;
		// warp unit
		case NetChange::WARP:
			{
				int id;
				char partId;
				Vec3 pos;
				float rot;
				f >> id;
				f >> partId;
				f >> pos;
				f >> rot;

				if(!f)
				{
					Error("Update client: Broken WARP.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Unit* unit = gameLevel->FindUnit(id);
				if(!unit)
				{
					Error("Update client: WARP, missing unit %d.", id);
					break;
				}

				LocationPart* locPart = gameLevel->GetLocationPartById(partId);
				if(!locPart)
				{
					Error("Update client: WARP, invalid location part %d.", partId);
					break;
				}

				unit->BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, false, true);

				LocationPart* oldLocPart = unit->locPart;
				unit->locPart = locPart;
				unit->pos = pos;
				unit->rot = rot;

				unit->visualPos = unit->pos;
				if(unit->interp)
					unit->interp->Reset(unit->pos, unit->rot);

				if(oldLocPart != unit->locPart)
				{
					RemoveElement(oldLocPart->units, unit);
					locPart->units.push_back(unit);
				}
				if(unit == pc.unit)
				{
					if(game->fallbackType == FALLBACK::WAIT_FOR_WARP)
						game->fallbackType = FALLBACK::NONE;
					else if(game->fallbackType == FALLBACK::ARENA)
					{
						pc.unit->frozen = FROZEN::ROTATE;
						game->fallbackType = FALLBACK::NONE;
					}
					else if(game->fallbackType == FALLBACK::ARENA_EXIT)
					{
						pc.unit->frozen = FROZEN::NO;
						game->fallbackType = FALLBACK::NONE;

						if(pc.unit->hp <= 0.f)
							pc.unit->Standup(false);
					}
					PushChange(NetChange::WARP);
					interpolateTimer = 0.f;
					pc.data.rotBuf = 0.f;
					gameLevel->camera.Reset();
					pc.data.rotBuf = 0.f;
				}
			}
			break;
		// register new item
		case NetChange::REGISTER_ITEM:
			{
				const string& itemId = f.ReadString1();
				if(!f)
					Error("Update client: Broken REGISTER_ITEM.");
				else
				{
					const Item* base;
					base = Item::TryGet(itemId);
					if(!base)
					{
						Error("Update client: REGISTER_ITEM, missing base item %s.", itemId.c_str());
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
						f >> item->questId;
						if(!f)
							Error("Update client: Broken REGISTER_ITEM(3).");
						else
							questMgr->questItems.push_back(item);
					}
				}
			}
			break;
		// added quest
		case NetChange::ADD_QUEST:
			{
				int id;
				f >> id;
				const string& questName = f.ReadString1();
				if(!f)
				{
					Error("Update client: Broken ADD_QUEST.");
					break;
				}

				PlaceholderQuest* quest = new PlaceholderQuest;
				quest->questIndex = questMgr->quests.size();
				quest->name = questName;
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
				gameGui->journal->NeedUpdate(Journal::Quests, quest->questIndex);
				gameGui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
				questMgr->quests.push_back(quest);
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

				Quest* quest = questMgr->FindQuest(id, false);
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
					gameGui->journal->NeedUpdate(Journal::Quests, quest->questIndex);
					gameGui->messages->AddGameMsg3(GMS_JOURNAL_UPDATED);
				}
			}
			break;
		// item rename
		case NetChange::RENAME_ITEM:
			{
				int questId;
				f >> questId;
				const string& itemId = f.ReadString1();
				if(!f)
					Error("Update client: Broken RENAME_ITEM.");
				else
				{
					bool found = false;
					for(Item* item : questMgr->questItems)
					{
						if(item->questId == questId && item->id == itemId)
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
						Error("Update client: RENAME_ITEM, missing quest item %d.", questId);
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
						team->leaderId = id;
						if(team->leaderId == team->myId)
							gameGui->AddMsg(game->txYouAreLeader);
						else
							gameGui->AddMsg(Format(game->txPcIsLeader, info->name.c_str()));
						team->leader = info->u;

						if(gameGui->worldMap->dialogEnc)
							gameGui->worldMap->dialogEnc->bts[0].state = (team->IsLeader() ? Button::NONE : Button::DISABLED);
					}
					else
						Error("Update client: CHANGE_LEADER, missing player %u.", id);
				}
			}
			break;
		// player get random number
		case NetChange::RANDOM_NUMBER:
			{
				byte playerId, number;
				f >> playerId;
				f >> number;
				if(!f)
					Error("Update client: Broken RANDOM_NUMBER.");
				else if(playerId != team->myId)
				{
					PlayerInfo* info = TryGetPlayer(playerId);
					if(info)
						gameGui->AddMsg(Format(game->txRolledNumber, info->name.c_str(), number));
					else
						Error("Update client: RANDOM_NUMBER, missing player %u.", playerId);
				}
			}
			break;
		// remove player from game
		case NetChange::REMOVE_PLAYER:
			{
				byte playerId;
				PlayerInfo::LeftReason reason;
				f >> playerId;
				f.ReadCasted<byte>(reason);
				if(!f)
					Error("Update client: Broken REMOVE_PLAYER.");
				else
				{
					PlayerInfo* info = TryGetPlayer(playerId);
					if(!info)
						Error("Update client: REMOVE_PLAYER, missing player %u.", playerId);
					else if(playerId != team->myId)
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
				int id, usableId;
				USE_USABLE_STATE state;
				f >> id;
				f >> usableId;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					Error("Update client: Broken USE_USABLE.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Unit* unit = gameLevel->FindUnit(id);
				if(!unit)
				{
					Error("Update client: USE_USABLE, missing unit %d.", id);
					break;
				}

				Usable* usable = gameLevel->FindUsable(usableId);
				if(!usable)
				{
					Error("Update client: USE_USABLE, missing usable %d.", usableId);
					break;
				}

				BaseUsable& base = *usable->base;
				if(state == USE_USABLE_START || state == USE_USABLE_START_SPECIAL)
				{
					if(!IsSet(base.useFlags, BaseUsable::CONTAINER))
					{
						unit->action = A_USE_USABLE;
						unit->animation = ANI_PLAY;
						unit->meshInst->Play(state == USE_USABLE_START_SPECIAL ? "czyta_papiery" : base.anim.c_str(), PLAY_PRIO1, 0);
						unit->targetPos = unit->pos;
						unit->targetPos2 = usable->pos;
						if(base.limitRot == 4)
							unit->targetPos2 -= Vec3(sin(usable->rot) * 1.5f, 0, cos(usable->rot) * 1.5f);
						unit->timer = 0.f;
						unit->animationState = AS_USE_USABLE_MOVE_TO_OBJECT;
						unit->act.useUsable.rot = Vec3::LookAtAngle(unit->pos, usable->pos);
						unit->usedItem = base.item;
						if(unit->usedItem)
						{
							gameRes->PreloadItem(unit->usedItem);
							unit->SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
						}
					}
					else
						unit->action = A_NONE;

					unit->UseUsable(usable);
					if(pc.data.beforePlayer == BP_USABLE && pc.data.beforePlayerPtr.usable == usable)
						pc.data.beforePlayer = BP_NONE;
				}
				else
				{
					if(unit->player != game->pc && !IsSet(base.useFlags, BaseUsable::CONTAINER))
					{
						unit->action = A_NONE;
						unit->animation = ANI_STAND;
						if(unit->liveState == Unit::ALIVE)
							unit->usedItem = nullptr;
					}

					if(usable->user != unit)
					{
						Error("Update client: USE_USABLE, unit %d not using %d.", id, usableId);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: STAND_UP, missing unit %d.", id);
					else
						unit->Standup(false);
				}
			}
			break;
		// game over
		case NetChange::GAME_OVER:
			Info("Update client: Game over - all players died.");
			game->SetMusic(MusicType::Death);
			gameGui->CloseAllPanels();
			++game->deathScreen;
			game->deathFade = 0;
			game->deathSolo = false;
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: RECRUIT_NPC, missing unit %d.", id);
					else if(!unit->IsHero())
						Error("Update client: RECRUIT_NPC, unit %d (%s) is not a hero.", id, unit->data->id.c_str());
					else
					{
						unit->hero->teamMember = true;
						unit->hero->type = free ? HeroType::Visitor : HeroType::Normal;
						unit->hero->credit = 0;
						team->members.push_back(unit);
						if(!free)
							team->activeMembers.push_back(unit);
						if(gameGui->team->visible)
							gameGui->team->Changed();
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: KICK_NPC, missing unit %d.", id);
					else if(!unit->IsHero() || !unit->hero->teamMember)
						Error("Update client: KICK_NPC, unit %d (%s) is not a team member.", id, unit->data->id.c_str());
					else
					{
						unit->hero->teamMember = false;
						RemoveElementOrder(team->members.ptrs, unit);
						if(unit->hero->type == HeroType::Normal)
							RemoveElementOrder(team->activeMembers.ptrs, unit);
						if(gameGui->team->visible)
							gameGui->team->Changed();
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: REMOVE_UNIT, missing unit %d.", id);
					else
						gameLevel->RemoveUnit(unit);
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
				else if(game->gameState == GS_LEVEL)
				{
					LocationPart& locPart = gameLevel->GetLocationPart(unit->pos);
					locPart.units.push_back(unit);
					unit->locPart = &locPart;
					if(unit->summoner)
						gameLevel->SpawnUnitEffect(*unit);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: CHANGE_ARENA_STATE, missing unit %d.", id);
					else
					{
						if(state < -1 || state > 1)
							state = -1;
						unit->inArena = state;
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
				else if(game->gameState == GS_LEVEL && gameLevel->cityCtx && IsSet(gameLevel->cityCtx->flags, City::HaveArena)
					&& gameLevel->GetArena() == pc.unit->locPart)
				{
					Sound* sound;
					if(type == 0)
						sound = gameRes->sArenaFight;
					else if(type == 1)
						sound = gameRes->sArenaWin;
					else
						sound = gameRes->sArenaLost;
					soundMgr->PlaySound2d(sound);
				}
			}
			break;
		// leaving notification
		case NetChange::LEAVE_LOCATION:
			if(game->gameState == GS_LEVEL)
			{
				game->fallbackType = FALLBACK::WAIT_FOR_WARP;
				game->fallbackTimer = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// exit to map
		case NetChange::EXIT_TO_MAP:
			if(game->gameState == GS_LEVEL)
				game->ExitToMap();
			break;
		// leader wants to travel to location
		case NetChange::TRAVEL:
			{
				byte loc;
				f >> loc;
				if(!f)
					Error("Update client: Broken TRAVEL.");
				else if(game->gameState == GS_WORLDMAP)
				{
					world->Travel(loc, false);
					gameGui->worldMap->StartTravel();
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
				else if(game->gameState == GS_WORLDMAP)
				{
					world->TravelPos(pos, false);
					gameGui->worldMap->StartTravel();
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
				else if(game->gameState == GS_WORLDMAP)
					world->StopTravel(pos, false);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game->gameState == GS_WORLDMAP)
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
				bool isClosing;
				f >> id;
				f >> isClosing;
				if(!f)
					Error("Update client: Broken USE_DOOR.");
				else if(game->gameState == GS_LEVEL)
				{
					Door* door = gameLevel->FindDoor(id);
					if(!door)
						Error("Update client: USE_DOOR, missing door %d.", id);
					else
						door->SetState(isClosing);
				}
			}
			break;
		// create explosion effect
		case NetChange::CREATE_EXPLOSION:
			{
				Vec3 pos;
				int abilityHash;
				f >> abilityHash;
				f >> pos;
				if(!f)
				{
					Error("Update client: Broken CREATE_EXPLOSION.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
					Error("Update client: CREATE_EXPLOSION, missing ability %u.", abilityHash);
				else if(!IsSet(ability->flags, Ability::Explode))
					Error("Update client: CREATE_EXPLOSION, ability '%s' is not explosion.", ability->id.c_str());
				else
					gameLevel->GetLocationPart(pos).CreateExplo(ability, pos);
			}
			break;
		// create trap
		case NetChange::CREATE_TRAP:
			{
				int id;
				TRAP_TYPE type;
				Vec3 pos;
				f >> id;
				f.ReadCasted<byte>(type);
				f >> pos;
				if(!f)
					Error("Update client: Broken CREATE_TRAP.");
				else
					gameLevel->CreateTrap(pos, type, id);
			}
			break;
		// remove trap
		case NetChange::REMOVE_TRAP:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken REMOVE_TRAP.");
				else  if(game->gameState == GS_LEVEL)
				{
					if(!gameLevel->RemoveTrap(id))
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
				else if(game->gameState == GS_LEVEL)
				{
					Trap* trap = gameLevel->FindTrap(id);
					if(trap)
						trap->mpTrigger = true;
					else
						Error("Update client: TRIGGER_TRAP, missing trap %d.", id);
				}
			}
			break;
		// play evil sound
		case NetChange::EVIL_SOUND:
			soundMgr->PlaySound2d(gameRes->sEvil);
			break;
		// start encounter on world map
		case NetChange::ENCOUNTER:
			{
				const string& text = f.ReadString1();
				if(!f)
					Error("Update client: Broken ENCOUNTER.");
				else if(game->gameState == GS_WORLDMAP)
				{
					DialogInfo info;
					info.event = [this](int)
					{
						gameGui->worldMap->dialogEnc = nullptr;
						PushChange(NetChange::CLOSE_ENCOUNTER);
					};
					info.name = "encounter";
					info.order = DialogOrder::Top;
					info.parent = nullptr;
					info.pause = true;
					info.type = DIALOG_OK;
					info.text = text;

					gameGui->worldMap->dialogEnc = gui->ShowDialog(info);
					if(!team->IsLeader())
						gameGui->worldMap->dialogEnc->bts[0].state = Button::DISABLED;
					assert(world->GetState() == World::State::TRAVEL);
					world->SetState(World::State::ENCOUNTER);
				}
			}
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(game->gameState == GS_WORLDMAP && gameGui->worldMap->dialogEnc)
			{
				gui->CloseDialog(gameGui->worldMap->dialogEnc);
				gameGui->worldMap->dialogEnc = nullptr;
			}
			break;
		// close portal in location
		case NetChange::CLOSE_PORTAL:
			if(game->gameState == GS_LEVEL)
			{
				if(gameLevel->location && gameLevel->location->portal)
				{
					delete gameLevel->location->portal;
					gameLevel->location->portal = nullptr;
				}
				else
					Error("Update client: CLOSE_PORTAL, missing portal.");
			}
			break;
		// clean altar in evil quest
		case NetChange::CLEAN_ALTAR:
			if(game->gameState == GS_LEVEL)
			{
				// change object
				BaseObject* baseObj = BaseObject::Get("bloody_altar");
				Object* obj = gameLevel->localPart->FindObject(baseObj);
				obj->base = BaseObject::Get("altar");
				obj->mesh = obj->base->mesh;
				resMgr->Load(obj->mesh);

				// remove particles
				float bestDist = 999.f;
				ParticleEmitter* bestPe = nullptr;
				for(ParticleEmitter* pe : gameLevel->localPart->lvlPart->pes)
				{
					if(pe->tex == gameRes->tBlood[BLOOD_RED])
					{
						float dist = Vec3::Distance(pe->pos, obj->pos);
						if(dist < bestDist)
						{
							bestDist = dist;
							bestPe = pe;
						}
					}
				}
				assert(bestPe);
				bestPe->destroy = true;
			}
			break;
		// add new location
		case NetChange::ADD_LOCATION:
			{
				byte locationIndex;
				LOCATION type;
				f >> locationIndex;
				f.ReadCasted<byte>(type);

				Location* loc;
				switch(type)
				{
				case L_CITY:
					{
						City* city = new City;
						city->citizens = 0;
						city->citizensWorld = 0;
						loc = city;
					}
					break;
				case L_DUNGEON:
					{
						byte levels;
						f >> levels;
						if(levels == 1)
							loc = new SingleInsideLocation;
						else
							loc = new MultiInsideLocation(levels);
					}
					break;
				case L_CAVE:
					loc = new Cave;
					break;
				default:
					loc = new OutsideLocation;
					break;
				}
				loc->type = type;
				loc->index = locationIndex;

				f.ReadCasted<byte>(loc->state);
				f.ReadCasted<byte>(loc->target);
				f >> loc->pos;
				f >> loc->name;
				f.ReadCasted<byte>(loc->image);
				if(!f)
				{
					Error("Update client: Broken ADD_LOCATION.");
					delete loc;
					break;
				}

				world->AddLocationAtIndex(loc);
			}
			break;
		// remove camp
		case NetChange::REMOVE_CAMP:
			{
				byte campIndex;
				f >> campIndex;
				if(!f)
					Error("Update client: Broken REMOVE_CAMP.");
				else if(!world->VerifyLocation(campIndex) || world->GetLocation(campIndex)->type != L_CAMP)
					Error("Update client: REMOVE_CAMP, invalid location %u.", campIndex);
				else
					world->RemoveLocation(campIndex);
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
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: CHANGE_AI_MODE, missing unit %d.", id);
					else
						unit->aiMode = mode;
				}
			}
			break;
		// change unit base type
		case NetChange::CHANGE_UNIT_BASE:
			{
				int id;
				f >> id;
				const string& unitId = f.ReadString1();
				if(!f)
					Error("Update client: Broken CHANGE_UNIT_BASE.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					UnitData* ud = UnitData::TryGet(unitId);
					if(unit && ud)
						unit->data = ud;
					else if(!unit)
						Error("Update client: CHANGE_UNIT_BASE, missing unit %d.", id);
					else
						Error("Update client: CHANGE_UNIT_BASE, missing base unit '%s'.", unitId.c_str());
				}
			}
			break;
		// unit cast spell animation
		case NetChange::CAST_SPELL:
			{
				int id, hash;
				f >> id;
				f >> hash;
				if(!f)
					Error("Update client: Broken CAST_SPELL.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: CAST_SPELL, missing unit %d.", id);
					else
					{
						Ability* ability = Ability::Get(hash);
						if(ability)
						{
							unit->action = A_CAST;
							unit->animationState = AS_CAST_ANIMATION;
							if(ability->animation.empty())
							{
								if(unit->meshInst->mesh->head.nGroups == 2)
									unit->meshInst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
								else
								{
									unit->animation = ANI_PLAY;
									unit->meshInst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 0);
								}
							}
							else
							{
								unit->meshInst->Play(ability->animation.c_str(), PLAY_ONCE | PLAY_PRIO1, 0);
								unit->animation = ANI_PLAY;
							}
						}
						else
							Error("Update client: CAST_SPELL, missing ability %d.", hash);
					}
				}
			}
			break;
		// create ball - spell effect
		case NetChange::CREATE_SPELL_BALL:
			{
				int abilityHash;
				int id, ownerId;
				Vec3 pos;
				float rotY, speedY;
				f >> abilityHash;
				f >> id;
				f >> ownerId;
				f >> pos;
				f >> rotY;
				f >> speedY;
				if(!f)
				{
					Error("Update client: Broken CREATE_SPELL_BALL.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Ability* abilityPtr = Ability::Get(abilityHash);
				if(!abilityPtr)
				{
					Error("Update client: CREATE_SPELL_BALL, missing ability %u.", abilityHash);
					break;
				}

				Unit* unit = nullptr;
				if(ownerId != -1)
				{
					unit = gameLevel->FindUnit(ownerId);
					if(!unit)
						Error("Update client: CREATE_SPELL_BALL, missing unit %d.", ownerId);
				}

				Ability& ability = *abilityPtr;
				LocationPart& locPart = gameLevel->GetLocationPart(pos);

				Bullet* bullet = new Bullet;
				locPart.lvlPart->bullets.push_back(bullet);

				bullet->id = id;
				bullet->Register();
				bullet->isArrow = false;
				bullet->pos = pos;
				bullet->rot = Vec3(0, rotY, 0);
				bullet->mesh = ability.mesh;
				bullet->tex = ability.tex;
				bullet->texSize = ability.size;
				bullet->speed = ability.speed;
				bullet->timer = ability.range / (ability.speed - 1);
				bullet->trail = nullptr;
				bullet->pe = nullptr;
				bullet->ability = &ability;
				bullet->startPos = bullet->pos;
				bullet->owner = unit;
				bullet->yspeed = speedY;

				if(ability.texParticle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = ability.texParticle;
					pe->emissionInterval = 0.1f;
					pe->life = -1;
					pe->particleLife = 0.5f;
					pe->emissions = -1;
					pe->spawn = Int2(3, 4);
					pe->maxParticles = 50;
					pe->pos = bullet->pos;
					pe->speedMin = Vec3(-1, -1, -1);
					pe->speedMax = Vec3(1, 1, 1);
					pe->posMin = Vec3(-ability.size, -ability.size, -ability.size);
					pe->posMax = Vec3(ability.size, ability.size, ability.size);
					pe->size = Vec2(ability.sizeParticle, 0.f);
					pe->alpha = Vec2(1.f, 0.f);
					pe->mode = 1;
					pe->Init();
					locPart.lvlPart->pes.push_back(pe);
					bullet->pe = pe;
				}
			}
			break;
		// play spell sound
		case NetChange::SPELL_SOUND:
			{
				int type, abilityHash;
				Vec3 pos;
				f >> type;
				f >> abilityHash;
				f >> pos;
				if(!f)
				{
					Error("Update client: Broken SPELL_SOUND.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
					Error("Update client: SPELL_SOUND, missing ability %u.", abilityHash);
				else if(type == 0)
					soundMgr->PlaySound3d(ability->soundCast, pos, ability->soundCastDist);
				else
					soundMgr->PlaySound3d(ability->soundHit, pos, ability->soundHitDist);
			}
			break;
		// drain blood effect
		case NetChange::CREATE_DRAIN:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken CREATE_DRAIN.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: CREATE_DRAIN, missing unit %d.", id);
					else
					{
						LevelPart& lvlPart = *unit->locPart->lvlPart;
						if(lvlPart.pes.empty())
							Error("Update client: CREATE_DRAIN, missing blood.");
						else
						{
							Drain& drain = Add1(lvlPart.drains);
							drain.target = unit;
							drain.pe = lvlPart.pes.back();
							drain.t = 0.f;
							drain.pe->manualDelete = 1;
							drain.pe->speedMin = Vec3(-3, 0, -3);
							drain.pe->speedMax = Vec3(3, 3, 3);
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
				else if(game->gameState == GS_LEVEL)
				{
					LocationPart& locPart = gameLevel->GetLocationPart(p1);
					Electro* e = new Electro;
					e->id = id;
					e->locPart = &locPart;
					e->Register();
					e->ability = Ability::Get("thunder_bolt");
					e->startPos = p1;
					e->AddLine(p1, p2);
					e->valid = true;
					locPart.lvlPart->electros.push_back(e);
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
				else if(game->gameState == GS_LEVEL)
				{
					Electro* e = gameLevel->FindElectro(id);
					if(!e)
						Error("Update client: UPDATE_ELECTRO, missing electro %d.", id);
					else
					{
						Vec3 from = e->lines.back().to;
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
				else if(game->gameState == GS_LEVEL)
				{
					Ability* ability = Ability::Get("thunder_bolt");

					// sound
					if(ability->soundHit)
						soundMgr->PlaySound3d(ability->soundHit, pos, ability->soundHitDist);

					// particles
					if(ability->texParticle)
					{
						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = ability->texParticle;
						pe->emissionInterval = 0.f;
						pe->life = 0.f;
						pe->particleLife = 0.5f;
						pe->emissions = 1;
						pe->spawn = Int2(8, 12);
						pe->maxParticles = 12;
						pe->pos = pos;
						pe->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
						pe->speedMax = Vec3(1.5f, 1.5f, 1.5f);
						pe->posMin = Vec3(-ability->size, -ability->size, -ability->size);
						pe->posMax = Vec3(ability->size, ability->size, ability->size);
						pe->size = Vec2(ability->sizeParticle, 0.f);
						pe->alpha = Vec2(1.f, 0.f);
						pe->mode = 1;
						pe->Init();

						gameLevel->GetLocationPart(pos).lvlPart->pes.push_back(pe);
					}
				}
			}
			break;
		// create particle effect
		case NetChange::PARTICLE_EFFECT:
			{
				int abilityHash;
				Vec3 pos;
				Vec2 bounds;
				f >> abilityHash;
				f >> pos;
				f >> bounds;
				if(!f)
					Error("Update client: Broken PARTICLE_EFFECT.");
				else if(Ability* ability = Ability::Get(abilityHash))
					gameLevel->CreateSpellParticleEffect(nullptr, ability, pos, bounds);
				else
					Error("Update client: PARTICLE_EFFECT, missing ability %d.", abilityHash);
			}
			break;
		// someone used cheat 'revealMinimap'
		case NetChange::CHEAT_REVEAL_MINIMAP:
			gameLevel->RevealMinimap();
			break;
		// revealing minimap
		case NetChange::REVEAL_MINIMAP:
			{
				word count;
				f >> count;
				if(!f.Ensure(count * sizeof(byte) * 2))
					Error("Update client: Broken REVEAL_MINIMAP.");
				else if(game->gameState == GS_LEVEL)
				{
					for(word i = 0; i < count; ++i)
					{
						byte x, y;
						f.Read(x);
						f.Read(y);
						gameLevel->minimapReveal.push_back(Int2(x, y));
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
			gameGui->CloseAllPanels();
			game->endOfGame = true;
			game->deathFade = 0.f;
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

						Unit* unit = gameLevel->FindUnit(id);
						if(!unit || !unit->IsPlayer())
						{
							Error("Update client: UPDATE_FREE_DAYS, missing unit %d or is not a player (0x%p).", id, unit);
							f.Skip(sizeof(int) * 2 * (count - i - 1));
							break;
						}

						unit->player->freeDays = days;
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
			gameGui->mpBox->Add(game->txGameSaved);
			gameGui->messages->AddGameMsg3(GMS_GAME_SAVED);
			break;
		// ai left team due too many team members
		case NetChange::HERO_LEAVE:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken HERO_LEAVE.");
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: HERO_LEAVE, missing unit %d.", id);
					else
						gameGui->mpBox->Add(Format(game->txMpNPCLeft, unit->GetName()));
				}
			}
			break;
		// game paused/resumed
		case NetChange::PAUSED:
			{
				bool isPaused;
				f >> isPaused;
				if(!f)
					Error("Update client: Broken PAUSED.");
				else
				{
					game->paused = isPaused;
					gameGui->mpBox->Add(isPaused ? game->txGamePaused : game->txGameResumed);
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
				byte locationIndex;
				f >> locationIndex;
				if(!f)
					Error("Update client: Broken CHEAT_TRAVEL.");
				else if(!world->VerifyLocation(locationIndex))
					Error("Update client: CHEAT_TRAVEL, invalid location index %u.", locationIndex);
				else if(game->gameState == GS_WORLDMAP)
				{
					world->Warp(locationIndex, false);
					gameGui->worldMap->StartTravel(true);
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
				else if(game->gameState == GS_WORLDMAP)
				{
					world->WarpPos(pos, false);
					gameGui->worldMap->StartTravel(true);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: REMOVE_USED_ITEM, missing unit %d.", id);
					else
						unit->usedItem = nullptr;
				}
			}
			break;
		// game stats showed at end of game
		case NetChange::GAME_STATS:
			f >> gameStats->totalKills;
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: USABLE_SOUND, missing unit %d.", id);
					else if(!unit->usable)
						Error("Update client: USABLE_SOUND, unit %d (%s) don't use usable.", id, unit->data->id.c_str());
					else if(unit != pc.unit)
						soundMgr->PlaySound3d(unit->usable->base->sound, unit->GetCenter(), Usable::SOUND_DIST);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(unit)
						unit->BreakAction();
					else
						Error("Update client: BREAK_ACTION, missing unit %d.", id);
				}
			}
			break;
		// player used ability
		case NetChange::PLAYER_ABILITY:
			{
				int id;
				int abilityHash;
				float speed;
				f >> id;
				f >> abilityHash;
				f >> speed;
				if(!f)
				{
					Error("Update client: Broken PLAYER_ABILITY.");
					break;
				}
				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
				{
					Error("Update client: PLAYER_ABILITY, invalid ability %u.", abilityHash);
					break;
				}
				if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(unit && unit->player)
					{
						if(unit->player != game->pc)
						{
							Vec3 speedV(speed, 0, 0);
							unit->player->UseAbility(ability, true, &speedV);
						}
					}
					else
						Error("Update client: PLAYER_ABILITY, invalid player unit %d.", id);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: USE_ITEM, missing unit %d.", id);
					else
					{
						unit->action = A_USE_ITEM;
						unit->meshInst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update client: MARK_UNIT, missing unit %d.", id);
					else
						unit->mark = mark;
				}
			}
			break;
		// someone used cheat 'cleanLevel'
		case NetChange::CLEAN_LEVEL:
			{
				int buildingId;
				f >> buildingId;
				if(!f)
					Error("Update client: Broken CLEAN_LEVEL.");
				else if(game->gameState == GS_LEVEL)
					gameLevel->CleanLevel(buildingId);
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
				int chestId, unitId;
				f >> chestId;
				f >> unitId;
				if(!f)
				{
					Error("Update client: Broken USE_CHEST.");
					break;
				}
				Chest* chest = gameLevel->FindChest(chestId);
				if(!chest)
					Error("Update client: USE_CHEST, missing chest %d.", chestId);
				else if(unitId == -1)
					chest->OpenClose(nullptr);
				else
				{
					Unit* unit = gameLevel->FindUnit(unitId);
					if(!unit)
						Error("Update client: USE_CHEST, missing unit %d.", unitId);
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
				info->fastTravel = true;
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
		// remove bullet
		case NetChange::REMOVE_BULLET:
			{
				int id;
				bool hitUnit;
				f >> id;
				f >> hitUnit;
				if(!f)
					Error("Update client: Broken REMOVE_BULLET.");
				else
				{
					Bullet* bullet = Bullet::GetById(id);
					if(bullet)
						bullet->timer = (hitUnit ? 0.f : 0.1f);
				}
			}
			break;
		// start boss fight
		case NetChange::BOSS_START:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update client: Broken BOSS_START.");
				else
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(unit)
						gameLevel->StartBossFight(*unit);
					else
						Error("Update client: BOSS_START, missing unit %d.", id);
				}
			}
			break;
		// end boss fight
		case NetChange::BOSS_END:
			gameLevel->EndBossFight();
			break;
		// add investment
		case NetChange::ADD_INVESTMENT:
			{
				int questId, gold;
				f >> questId;
				f >> gold;
				const string& name = f.ReadString1();
				if(!f)
					Error("Update client: Broken ADD_INVESTMENT.");
				else
					team->AddInvestment(name.c_str(), questId, gold);
			}
			break;
		// update investment
		case NetChange::UPDATE_INVESTMENT:
			{
				int questId, gold;
				f >> questId;
				f >> gold;
				if(!f)
					Error("Update client: Broken UPDATE_INVESTMENT.");
				else
					team->UpdateInvestment(questId, gold);
			}
			break;
		// add visible effect to unit
		case NetChange::ADD_UNIT_EFFECT:
			{
				int unitId;
				EffectId effect;
				float time;
				f >> unitId;
				f.ReadCasted<byte>(effect);
				f >> time;
				if(!f)
					Error("Update client: Broken ADD_UNIT_EFFECT.");
				else
				{
					Unit* unit = gameLevel->FindUnit(unitId);
					if(!unit)
						Error("Update client: ADD_UNIT_EFFECT, missing unit %d.", unitId);
					else if(unit != pc.unit)
					{
						Effect e;
						e.effect = effect;
						e.source = EffectSource::Temporary;
						e.sourceId = -1;
						e.value = (effect == EffectId::Rooted ? EffectValue_Rooted_Vines : EffectValue_Generic);
						e.power = 0;
						e.time = time;
						unit->AddEffect(e);
					}
				}
			}
			break;
		// remove visible effect from unit
		case NetChange::REMOVE_UNIT_EFFECT:
			{
				int unitId;
				EffectId effect;
				f >> unitId;
				f.ReadCasted<byte>(effect);
				if(!f)
					Error("Update client: Broken REMOVE_UNIT_EFFECT.");
				else
				{
					Unit* unit = gameLevel->FindUnit(unitId);
					if(!unit)
						Error("Update client: REMOVE_UNIT_EFFECT, missing unit %d.", unitId);
					else if(unit != pc.unit)
						unit->RemoveEffect(effect);
				}
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
			Error("Update client: Invalid checksum (%u).", changeI);
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
		f >> pc.lastDmgPoison;
	if(IsSet(flags, PlayerInfo::UF_GOLD))
		f >> pc.unit->gold;
	if(IsSet(flags, PlayerInfo::UF_ALCOHOL))
		f >> pc.unit->alcohol;
	if(IsSet(flags, PlayerInfo::UF_LEARNING_POINTS))
		f >> pc.learningPoints;
	if(IsSet(flags, PlayerInfo::UF_LEVEL))
		f >> pc.unit->level;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES(2).");
		return true;
	}

	// changes
	byte changesCount;
	f >> changesCount;
	if(!f)
	{
		Error("Update single client: Broken ID_PLAYER_CHANGES at changes.");
		return true;
	}

	for(byte changeI = 0; changeI < changesCount; ++changeI)
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
		// response to looting
		case NetChangePlayer::LOOT:
			{
				bool canLoot;
				f >> canLoot;
				if(!f)
				{
					Error("Update single client: Broken LOOT.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				if(pc.unit->action == A_PREPARE)
					pc.unit->action = A_NONE;
				if(!canLoot)
				{
					gameGui->messages->AddGameMsg3(GMS_IS_LOOTED);
					pc.action = PlayerAction::None;
					break;
				}

				// read items
				if(pc.action == PlayerAction::LootUnit)
				{
					array<const Item*, SLOT_MAX>& equipped = pc.actionUnit->GetEquippedItems();
					for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
					{
						const string& id = f.ReadString1();
						if(id.empty())
							equipped[i] = nullptr;
						else
						{
							equipped[i] = Item::TryGet(id);
							if(equipped[i])
								gameRes->PreloadItem(equipped[i]);
						}
					}
				}
				if(!f.ReadItemListTeam(*pc.chestTrade))
				{
					Error("Update single client: Broken LOOT(2).");
					break;
				}

				// start trade
				if(pc.action == PlayerAction::LootUnit)
					gameGui->inventory->StartTrade(I_LOOT_BODY, *pc.actionUnit);
				else if(pc.action == PlayerAction::LootContainer)
					gameGui->inventory->StartTrade(I_LOOT_CONTAINER, pc.actionUsable->container->items);
				else
					gameGui->inventory->StartTrade(I_LOOT_CHEST, pc.actionChest->items);
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
					gameGui->messages->AddGameMsg3(GMS_UNIT_BUSY);
				}
				else if(id != -2 && game->gameState == GS_LEVEL) // not entered/left building
				{
					// start dialog
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update single client: START_DIALOG, missing unit %d.", id);
					else
					{
						pc.action = PlayerAction::Talk;
						pc.actionUnit = unit;
						pc.data.beforePlayer = BP_NONE;
						game->dialogContext.StartDialog(unit);
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
				else if(game->gameState != GS_LEVEL)
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
					DialogContext& ctx = game->dialogContext;
					ctx.choiceSelected = 0;
					ctx.mode = DialogContext::WAIT_CHOICES;
					ctx.dialogEsc = escape;
					ctx.choices.resize(count);
					for(byte i = 0; i < count; ++i)
					{
						string* str = StringPool.Get();
						f >> *str;
						ctx.choices[i] = DialogChoice(str);
						if(!f)
						{
							Error("Update single client: Broken SHOW_DIALOG_CHOICES at %u index.", i);
							break;
						}
					}
					gameGui->levelGui->UpdateScrollbar(ctx.choices.size());
				}
			}
			break;
		// end of dialog
		case NetChangePlayer::END_DIALOG:
			if(game->gameState == GS_LEVEL)
			{
				game->dialogContext.dialogMode = false;
				if(pc.action == PlayerAction::Talk)
					pc.action = PlayerAction::None;
				pc.unit->lookTarget = nullptr;
			}
			break;
		// start trade
		case NetChangePlayer::START_TRADE:
			{
				int id;
				f >> id;
				if(!f.ReadItemList(game->chestTrade))
				{
					Error("Update single client: Broken START_TRADE.");
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Unit* trader = gameLevel->FindUnit(id);
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

				gameGui->inventory->StartTrade(I_TRADE, game->chestTrade, trader);
			}
			break;
		// add multiple same items to inventory
		case NetChangePlayer::ADD_ITEMS:
			{
				int teamCount, count;
				const Item* item;
				f >> teamCount;
				f >> count;
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS.");
				else if(count <= 0 || teamCount > count)
					Error("Update single client: ADD_ITEMS, invalid count %d or team count %d.", count, teamCount);
				else if(game->gameState == GS_LEVEL)
					pc.unit->AddItem2(item, (uint)count, (uint)teamCount, false);
			}
			break;
		// add items to trader which is trading with player
		case NetChangePlayer::ADD_ITEMS_TRADER:
			{
				int id, count, teamCount;
				const Item* item;
				f >> id;
				f >> count;
				f >> teamCount;
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_TRADER.");
				else if(count <= 0 || teamCount > count)
					Error("Update single client: ADD_ITEMS_TRADER, invalid count %d or team count %d.", count, teamCount);
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update single client: ADD_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc.IsTradingWith(unit))
						Error("Update single client: ADD_ITEMS_TRADER, unit %d (%s) is not trading with player.", id, unit->data->id.c_str());
					else
						unit->AddItem2(item, (uint)count, (uint)teamCount, false);
				}
			}
			break;
		// add items to chest which is open by player
		case NetChangePlayer::ADD_ITEMS_CHEST:
			{
				int id, count, teamCount;
				const Item* item;
				f >> id;
				f >> count;
				f >> teamCount;
				if(f.ReadItemAndFind(item) <= 0)
					Error("Update single client: Broken ADD_ITEMS_CHEST.");
				else if(count <= 0 || teamCount > count)
					Error("Update single client: ADD_ITEMS_CHEST, invalid count %d or team count %d.", count, teamCount);
				else if(game->gameState == GS_LEVEL)
				{
					Chest* chest = gameLevel->FindChest(id);
					if(!chest)
						Error("Update single client: ADD_ITEMS_CHEST, missing chest %d.", id);
					else if(pc.action != PlayerAction::LootChest || pc.actionChest != chest)
						Error("Update single client: ADD_ITEMS_CHEST, chest %d is not opened by player.", id);
					else
					{
						gameRes->PreloadItem(item);
						chest->AddItem(item, (uint)count, (uint)teamCount, false);
					}
				}
			}
			break;
		// remove items from inventory
		case NetChangePlayer::REMOVE_ITEMS:
			{
				int iIndex, count;
				f >> iIndex;
				f >> count;
				if(!f)
					Error("Update single client: Broken REMOVE_ITEMS.");
				else if(count <= 0)
					Error("Update single client: REMOVE_ITEMS, invalid count %d.", count);
				else if(game->gameState == GS_LEVEL)
					pc.unit->RemoveItem(iIndex, (uint)count);
			}
			break;
		// remove items from traded inventory which is trading with player
		case NetChangePlayer::REMOVE_ITEMS_TRADER:
			{
				int id, iIndex, count;
				f >> id;
				f >> count;
				f >> iIndex;
				if(!f)
					Error("Update single client: Broken REMOVE_ITEMS_TRADER.");
				else if(count <= 0)
					Error("Update single client: REMOVE_ITEMS_TRADE, invalid count %d.", count);
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
					if(!unit)
						Error("Update single client: REMOVE_ITEMS_TRADER, missing unit %d.", id);
					else if(!pc.IsTradingWith(unit))
						Error("Update single client: REMOVE_ITEMS_TRADER, unit %d (%s) is not trading with player.", id, unit->data->id.c_str());
					else
						unit->RemoveItem(iIndex, (uint)count);
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
				else if(game->gameState == GS_LEVEL)
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
				else if(game->gameState == GS_LEVEL)
					pc.unit->RemoveQuestItem(id);
			}
			break;
		// someone else is using usable
		case NetChangePlayer::USE_USABLE:
			if(game->gameState == GS_LEVEL)
			{
				gameGui->messages->AddGameMsg3(GMS_USED);
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
					gameGui->AddMsg(allowed ? game->txDevmodeOn : game->txDevmodeOff);
					game->devmode = allowed;
				}
			}
			break;
		// start sharing/giving items
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				cstring name = (type == NetChangePlayer::START_SHARE ? "START_SHARE" : "START_GIVE");
				if(pc.actionUnit && pc.actionUnit->IsTeamMember() && game->gameState == GS_LEVEL)
				{
					Unit& unit = *pc.actionUnit;
					SubprofileInfo sub;
					f >> unit.weight;
					f >> unit.weightMax;
					f >> unit.gold;
					f >> sub;
					f >> unit.effects;
					array<const Item*, SLOT_MAX>& equipped = unit.GetEquippedItems();
					for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
					{
						const string& id = f.ReadString1();
						if(id.empty())
							equipped[i] = nullptr;
						else
						{
							equipped[i] = Item::TryGet(id);
							if(equipped[i])
								gameRes->PreloadItem(equipped[i]);
						}
					}
					unit.stats = unit.data->GetStats(sub);
					if(!f.ReadItemListTeam(unit.items))
						Error("Update single client: Broken %s.", name);
					else
						gameGui->inventory->StartTrade(type == NetChangePlayer::START_SHARE ? I_SHARE : I_GIVE, unit);
				}
				else
				{
					if(game->gameState == GS_LEVEL)
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
				bool isBetter;
				f >> isBetter;
				if(!f)
					Error("Update single client: Broken IS_BETTER_ITEM.");
				else if(game->gameState == GS_LEVEL)
					gameGui->inventory->invTradeMine->IsBetterItemResponse(isBetter);
			}
			break;
		// question about pvp
		case NetChangePlayer::PVP:
			{
				byte playerId;
				f >> playerId;
				if(!f)
					Error("Update single client: Broken PVP.");
				else if(game->gameState == GS_LEVEL)
				{
					PlayerInfo* info = TryGetPlayer(playerId);
					if(!info)
						Error("Update single client: PVP, invalid player id %u.", playerId);
					else
						game->arena->ShowPvpRequest(info->u);
				}
			}
			break;
		// preparing to warp
		case NetChangePlayer::PREPARE_WARP:
			if(game->gameState == GS_LEVEL)
			{
				game->fallbackType = FALLBACK::WAIT_FOR_WARP;
				game->fallbackTimer = -1.f;
				pc.unit->frozen = (pc.unit->usable ? FROZEN::YES_NO_ANIM : FROZEN::YES);
			}
			break;
		// entering arena
		case NetChangePlayer::ENTER_ARENA:
			if(game->gameState == GS_LEVEL)
			{
				game->fallbackType = FALLBACK::ARENA;
				game->fallbackTimer = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// start of arena combat
		case NetChangePlayer::START_ARENA_COMBAT:
			if(game->gameState == GS_LEVEL)
				pc.unit->frozen = FROZEN::NO;
			break;
		// exit from arena
		case NetChangePlayer::EXIT_ARENA:
			if(game->gameState == GS_LEVEL)
			{
				game->fallbackType = FALLBACK::ARENA_EXIT;
				game->fallbackTimer = -1.f;
				pc.unit->frozen = FROZEN::YES;
			}
			break;
		// player refused to pvp
		case NetChangePlayer::NO_PVP:
			{
				byte playerId;
				f >> playerId;
				if(!f)
					Error("Update single client: Broken NO_PVP.");
				else if(game->gameState == GS_LEVEL)
				{
					PlayerInfo* info = TryGetPlayer(playerId);
					if(!info)
						Error("Update single client: NO_PVP, invalid player id %u.", playerId);
					else
						gameGui->AddMsg(Format(game->txPvpRefuse, info->name.c_str()));
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
				else if(game->gameState == GS_LEVEL)
					gameGui->messages->AddGameMsg3(reason == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
			}
			break;
		// force player to look at unit
		case NetChangePlayer::LOOK_AT:
			{
				int id;
				f >> id;
				if(!f)
					Error("Update single client: Broken LOOK_AT.");
				else if(game->gameState == GS_LEVEL)
				{
					if(id == -1)
						pc.unit->lookTarget = nullptr;
					else
					{
						Unit* unit = gameLevel->FindUnit(id);
						if(!unit)
							Error("Update single client: LOOK_AT, missing unit %d.", id);
						else
							pc.unit->lookTarget = unit;
					}
				}
			}
			break;
		// end of fallback
		case NetChangePlayer::END_FALLBACK:
			if(game->gameState == GS_LEVEL && game->fallbackType == FALLBACK::CLIENT)
				game->fallbackType = FALLBACK::CLIENT2;
			break;
		// response to rest in inn
		case NetChangePlayer::REST:
			{
				byte days;
				f >> days;
				if(!f)
					Error("Update single client: Broken REST.");
				else if(game->gameState == GS_LEVEL)
				{
					game->fallbackType = FALLBACK::REST;
					game->fallbackTimer = -1.f;
					game->fallbackValue = days;
					pc.unit->frozen = FROZEN::YES;
				}
			}
			break;
		// response to training
		case NetChangePlayer::TRAIN:
			{
				byte type;
				int value;
				f >> type;
				f >> value;
				if(!f)
					Error("Update single client: Broken TRAIN.");
				else if(game->gameState == GS_LEVEL)
				{
					game->fallbackType = FALLBACK::TRAIN;
					game->fallbackTimer = -1.f;
					game->fallbackValue = type;
					game->fallbackValue2 = value;
					pc.unit->frozen = FROZEN::YES;
				}
			}
			break;
		// warped player to not stuck position
		case NetChangePlayer::UNSTUCK:
			{
				Vec3 newPos;
				f >> newPos;
				if(!f)
					Error("Update single client: Broken UNSTUCK.");
				else if(game->gameState == GS_LEVEL)
				{
					pc.unit->pos = newPos;
					interpolateTimer = 0.1f;
				}
			}
			break;
		// message about receiving gold from another player
		case NetChangePlayer::GOLD_RECEIVED:
			{
				byte playerId;
				int count;
				f >> playerId;
				f >> count;
				if(!f)
					Error("Update single client: Broken GOLD_RECEIVED.");
				else if(count <= 0)
					Error("Update single client: GOLD_RECEIVED, invalid count %d.", count);
				else
				{
					PlayerInfo* info = TryGetPlayer(playerId);
					if(!info)
						Error("Update single client: GOLD_RECEIVED, invalid player id %u.", playerId);
					else
					{
						gameGui->mpBox->Add(Format(game->txReceivedGold, count, info->name.c_str()));
						soundMgr->PlaySound2d(gameRes->sCoins);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
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
				else if(game->gameState == GS_LEVEL)
				{
					Unit* unit = gameLevel->FindUnit(id);
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
						array<const Item*, SLOT_MAX>& equipped = unit->GetEquippedItems();
						for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
						{
							const string& id = f.ReadString1();
							if(id.empty())
								equipped[i] = nullptr;
							else
							{
								equipped[i] = Item::TryGet(id);
								if(equipped[i])
									gameRes->PreloadItem(equipped[i]);
							}
						}
						if(!f.ReadItemListTeam(unit->items))
							Error("Update single client: Broken UPDATE_TRADER_INVENTORY(2).");
						gameGui->inventory->BuildTmpInventory(1);
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
					int setFlags = CountBits(flags);
					// read to buffer
					f.Read(BUF, sizeof(int) * setFlags);
					if(!f)
						Error("Update single client: Broken PLAYER_STATS(2).");
					else
					{
						int* buf = (int*)BUF;
						if(IsSet(flags, STAT_KILLS))
							pc.kills = *buf++;
						if(IsSet(flags, STAT_DMG_DONE))
							pc.dmgDone = *buf++;
						if(IsSet(flags, STAT_DMG_TAKEN))
							pc.dmgTaken = *buf++;
						if(IsSet(flags, STAT_KNOCKS))
							pc.knocks = *buf++;
						if(IsSet(flags, STAT_ARENA_FIGHTS))
							pc.arenaFights = *buf++;
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
				int perkHash, value;
				f >> perkHash;
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken ADD_PERK.");
				else if(Perk* perk = Perk::Get(perkHash))
					pc.AddPerk(perk, value);
				else
					Error("Update single client: ADD_PERK, invalid perk %d.", perkHash);
			}
			break;
		// remove perk from player
		case NetChangePlayer::REMOVE_PERK:
			{
				int perkHash, value;
				f >> perkHash;
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken REMOVE_PERK.");
				else if(Perk* perk = Perk::Get(perkHash))
					pc.RemovePerk(perk, value);
				else
					Error("Update single client: ADD_PERK, invalid perk %d.", perkHash);
			}
			break;
		// show game message
		case NetChangePlayer::GAME_MESSAGE:
			{
				int gmId;
				f >> gmId;
				if(!f)
					Error("Update single client: Broken GAME_MESSAGE.");
				else
					gameGui->messages->AddGameMsg3((GMS)gmId);
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
					gameGui->console->AddMsg(output->c_str());
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
				f.ReadCasted<char>(e.sourceId);
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
				int sourceId, value;
				f.ReadCasted<char>(effect);
				f.ReadCasted<char>(source);
				f.ReadCasted<char>(sourceId);
				f.ReadCasted<char>(value);
				if(!f)
					Error("Update single client: Broken REMOVE_EFFECT.");
				else
					pc.unit->RemoveEffects(effect, source, sourceId, value);
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
					gameGui->messages->AddFormattedMessage(game->pc, id, subtype, value);
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
					soundMgr->PlaySound2d(gameRes->sCoins);
			}
			break;
		// add ability to player
		case NetChangePlayer::ADD_ABILITY:
			{
				int abilityHash;
				f >> abilityHash;
				if(!f)
				{
					Error("Update single client: Broken ADD_ABILITY.");
					break;
				}
				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
					Error("Update single client: ADD_ABILITY, invalid ability %u.", abilityHash);
				else
					pc.AddAbility(ability);
			}
			break;
		// remove ability from player
		case NetChangePlayer::REMOVE_ABILITY:
			{
				int abilityHash;
				f >> abilityHash;
				if(!f)
				{
					Error("Update single client: Broken REMOVE_ABILITY.");
					break;
				}
				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
					Error("Update single client: REMOVE_ABILITY, invalid ability %u.", abilityHash);
				else
					pc.RemoveAbility(ability);
			}
			break;
		// after crafting - update ingredients, play sound
		case NetChangePlayer::AFTER_CRAFT:
			gameGui->craft->AfterCraft();
			break;
		// add recipe to player
		case NetChangePlayer::ADD_RECIPE:
			{
				int recipeHash;
				f >> recipeHash;
				if(!f)
				{
					Error("Update single client: Broken ADD_RECIPE.");
					break;
				}
				Recipe* recipe = Recipe::TryGet(recipeHash);
				if(!recipe)
					Error("Update single client: ADD_RECIPE, invalid recipe %d.", recipeHash);
				else
					pc.AddRecipe(recipe);
			}
			break;
		// end prepare action
		case NetChangePlayer::END_PREPARE:
			if(pc.unit->action == A_PREPARE)
				pc.unit->action = A_NONE;
			break;
		default:
			Warn("Update single client: Unknown player change type %d.", type);
			break;
		}

		byte checksum;
		f >> checksum;
		if(!f || checksum != 0xFF)
		{
			Error("Update single client: Invalid checksum (%u).", changeI);
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
			gameGui->AddMsg(s);
			if(game->gameState == GS_LEVEL)
				gameGui->levelGui->AddSpeechBubble(info->u, text.c_str());
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
			gameGui->AddMsg(s);
		}
	}
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
	case NetChange::CHEAT_ADD_ITEM:
	case NetChange::CHEAT_SET_STAT:
	case NetChange::CHEAT_MOD_STAT:
	case NetChange::GAME_OVER:
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
	case NetChange::CUTSCENE_IMAGE:
	case NetChange::CUTSCENE_TEXT:
		if(IsServer() && c.str)
		{
			StringPool.Free(c.str);
			RemoveElement(netStrs, c.str);
			c.str = nullptr;
		}
		return true;
	case NetChange::RUN_SCRIPT:
	case NetChange::CHEAT_ARENA:
		StringPool.Free(c.str);
		return true;
	default:
		return true;
	}
}

//=================================================================================================
void Net::ReadNetVars(BitStreamReader& f)
{
	f >> mpUseInterp;
	f >> mpInterp;
	f >> game->gameSpeed;
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
	if(!questMgr->Read(f))
		return false;

	// rumors
	f.ReadStringArray<byte, word>(gameGui->journal->GetRumors());
	if(!f)
	{
		Error("Read world: Broken packet for rumors.");
		return false;
	}

	// game stats
	gameStats->Read(f);
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
	gameRes->LoadCommonMusic();

	return true;
}

//=================================================================================================
bool Net::ReadPlayerStartData(BitStreamReader& f)
{
	game->pc = new PlayerController;

	f >> game->devmode;
	f >> game->noai;
	game->pc->ReadStart(f);
	f.ReadStringArray<word, word>(gameGui->journal->GetNotes());
	team->ReadInvestments(f);
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
	gameLevel->camera.Reset();
	game->pc->data.rotBuf = 0.f;

	bool loadedResources;
	f >> mpLoad;
	f >> loadedResources;
	if(!f)
	{
		Error("Read level: Broken packet for loading info.");
		return false;
	}

	if(!gameLevel->Read(f, loadedResources))
		return false;

	// items to preload
	uint itemsLoadCount = f.Read<uint>();
	if(!f.Ensure(itemsLoadCount * 2))
	{
		Error("Read level: Broken items preload count.");
		return false;
	}
	std::set<const Item*>& itemsLoad = game->itemsLoad;
	itemsLoad.clear();
	for(uint i = 0; i < itemsLoadCount; ++i)
	{
		const string& itemId = f.ReadString1();
		if(!f)
		{
			Error("Read level: Broken item preload '%u'.", i);
			return false;
		}
		if(itemId[0] != '$')
		{
			auto item = Item::TryGet(itemId);
			if(!item)
			{
				Error("Read level: Missing item preload '%s'.", itemId.c_str());
				return false;
			}
			itemsLoad.insert(item);
		}
		else
		{
			int questId = f.Read<int>();
			if(!f)
			{
				Error("Read level: Broken quest item preload '%u'.", i);
				return false;
			}
			const Item* item = questMgr->FindQuestItemClient(itemId.c_str(), questId);
			if(!item)
			{
				Error("Read level: Missing quest item preload '%s' (%d).", itemId.c_str(), questId);
				return false;
			}
			const Item* base = Item::TryGet(itemId.c_str() + 1);
			if(!base)
			{
				Error("Read level: Missing quest item preload base '%s' (%d).", itemId.c_str(), questId);
				return false;
			}
			itemsLoad.insert(base);
			itemsLoad.insert(item);
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

	Unit* unit = gameLevel->FindUnit(id);
	if(!unit)
	{
		Error("Read player data: Missing unit %d.", id);
		return false;
	}
	info.u = unit;
	game->pc = unit->player;
	game->pc->playerInfo = &info;
	info.pc = game->pc;
	gameLevel->camera.target = unit;

	// items
	array<const Item*, SLOT_MAX>& equipped = unit->GetEquippedItems();
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		const string& itemId = f.ReadString1();
		if(!f)
		{
			Error("Read player data: Broken item %d.", i);
			return false;
		}
		if(!itemId.empty())
		{
			equipped[i] = Item::TryGet(itemId);
			if(!equipped[i])
				return false;
		}
		else
			equipped[i] = nullptr;
	}
	if(!f.ReadItemListTeam(unit->items))
	{
		Error("Read player data: Broken item list.");
		return false;
	}

	int credit = unit->player->credit,
		freeDays = unit->player->freeDays;

	unit->player->Init(*unit, nullptr);

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

	unit->prevSpeed = 0.f;
	unit->weight = 0;
	unit->CalculateLoad();
	unit->RecalculateWeight();
	unit->hpmax = unit->CalculateMaxHp();
	unit->hp *= unit->hpmax;
	unit->mpmax = unit->CalculateMaxMp();
	unit->mp *= unit->mpmax;
	unit->staminaMax = unit->CalculateMaxStamina();
	unit->stamina *= unit->staminaMax;

	unit->player->credit = credit;
	unit->player->freeDays = freeDays;
	unit->player->isLocal = true;

	// other team members
	team->members.clear();
	team->activeMembers.clear();
	team->members.push_back(unit);
	team->activeMembers.push_back(unit);
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
		Unit* teamMember = gameLevel->FindUnit(id);
		if(!teamMember)
		{
			Error("Read player data: Missing team member %d.", id);
			return false;
		}
		team->members.push_back(teamMember);
		if(teamMember->IsPlayer() || teamMember->hero->type == HeroType::Normal)
			team->activeMembers.push_back(teamMember);
	}
	f.ReadCasted<byte>(team->leaderId);
	if(!f)
	{
		Error("Read player data: Broken team leader.");
		return false;
	}
	PlayerInfo* leaderInfo = TryGetPlayer(team->leaderId);
	if(!leaderInfo)
	{
		Error("Read player data: Missing player %d.", team->leaderId);
		return false;
	}
	team->leader = leaderInfo->u;

	game->dialogContext.pc = unit->player;

	// multiplayer load data
	if(mpLoad)
	{
		f >> unit->usedItemIsTeam;
		f >> unit->raiseTimer;
		if(unit->action == A_ATTACK)
		{
			f >> unit->act.attack.power;
			f >> unit->act.attack.run;
		}
		else if(unit->action == A_CAST)
		{
			unit->act.cast.ability = Ability::Get(f.Read<int>());
			f >> unit->act.cast.target;
		}
		if(!f)
		{
			Error("Read player data: Broken multiplayer data.");
			return false;
		}

		if(unit->usable && unit->action == A_USE_USABLE && Any(unit->animationState, AS_USE_USABLE_USING, AS_USE_USABLE_USING_SOUND)
			&& IsSet(unit->usable->base->useFlags, BaseUsable::ALCHEMY))
			gameGui->craft->Show();
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
