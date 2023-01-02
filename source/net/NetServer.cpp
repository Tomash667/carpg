#include "Pch.h"
#include "Net.h"
#include "BitStreamFunc.h"
#include "Team.h"
#include "World.h"
#include "City.h"
#include "InsideLocation.h"
#include "InsideLocationLevel.h"
#include "Level.h"
#include "GroundItem.h"
#include "Ability.h"
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
#include "GameResources.h"
#include "CraftPanel.h"

const float CHANGE_LEVEL_TIMER = 5.f;

//=================================================================================================
void Net::InitServer()
{
	Info("Creating server (port %d)...", port);

	OpenPeer();

	uint maxConnections = maxPlayers - 1;
	if(!serverLan)
		++maxConnections;

	SocketDescriptor sd(port, nullptr);
	sd.socketFamily = AF_INET;
	StartupResult r = peer->Startup(maxConnections, &sd, 1);
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

	peer->SetMaximumIncomingConnections((word)maxPlayers - 1);
	if(IsDebug())
		peer->SetTimeoutTime(60 * 60 * 1000, UNASSIGNED_SYSTEM_ADDRESS);

	if(!serverLan)
	{
		ConnectionAttemptResult result = peer->Connect(api->GetApiUrl(), api->GetProxyPort(), nullptr, 0, nullptr, 0, 6);
		if(result == CONNECTION_ATTEMPT_STARTED)
			masterServerState = MasterServerState::Connecting;
		else
		{
			masterServerState = MasterServerState::NotConnected;
			Error("Failed to connect to master server: SLikeNet error %d.", result);
		}
	}
	masterServerAdr = UNASSIGNED_SYSTEM_ADDRESS;

	Info("Server created. Waiting for connection.");

	SetMode(Mode::Server);
}

//=================================================================================================
void Net::OnNewGameServer()
{
	activePlayers = 1;
	DeleteElements(players);
	team->myId = 0;
	team->leaderId = 0;
	lastId = 0;
	game->paused = false;
	game->hardcoreMode = false;

	PlayerInfo* info = new PlayerInfo;
	players.push_back(info);

	PlayerInfo& sp = *info;
	sp.name = game->playerName;
	sp.id = 0;
	sp.state = PlayerInfo::IN_LOBBY;
	sp.left = PlayerInfo::LEFT_NO;

	game->skipIdCounter = 0;
	updateTimer = 0.f;
	game->arena->Reset();
	team->anyoneTalking = false;
	gameLevel->canFastTravel = false;
	warps.clear();
	if(!mpLoad)
		changes.clear(); // was clear before loading and now contains registered questItems
	if(!netStrs.empty())
		StringPool.Free(netStrs);
	gameGui->server->maxPlayers = maxPlayers;
	gameGui->server->serverName = serverName;
	gameGui->server->UpdateServerInfo();
	gameGui->server->Show();
	if(!mpQuickload)
	{
		gameGui->mpBox->Reset();
		gameGui->mpBox->visible = true;
	}

	if(!mpLoad)
		gameGui->server->CheckAutopick();
	else
	{
		// search for saved character
		PlayerInfo* old = FindOldPlayer(game->playerName.c_str());
		if(old)
		{
			sp.devmode = old->devmode;
			sp.clas = old->clas;
			sp.hd.CopyFrom(old->hd);
			sp.loaded = true;
			gameGui->server->UseLoadedCharacter(true);
		}
		else
		{
			gameGui->server->UseLoadedCharacter(false);
			gameGui->server->CheckAutopick();
		}
	}

	game->SetTitle("SERVER");
}

//=================================================================================================
void Net::UpdateServer(float dt)
{
	if(game->gameState == GS_LEVEL)
	{
		float gameDt = dt * game->gameSpeed;

		for(PlayerInfo& info : players)
		{
			if(info.left == PlayerInfo::LEFT_NO && !info.pc->isLocal)
			{
				info.pc->UpdateCooldown(gameDt);
				if(!info.u->IsStanding())
					info.u->TryStandup(gameDt);
			}
		}

		InterpolatePlayers(dt);
		UpdateFastTravel(gameDt);
		game->pc->unit->changed = true;
	}

	Packet* packet;
	for(packet = peer->Receive(); packet; peer->DeallocatePacket(packet), packet = peer->Receive())
	{
		BitStreamReader reader(packet);
		PlayerInfo* ptrInfo = FindPlayer(packet->systemAddress);
		if(!ptrInfo || ptrInfo->left != PlayerInfo::LEFT_NO)
		{
			if(packet->data[0] != ID_UNCONNECTED_PING)
				Info("Ignoring packet from %s.", packet->systemAddress.ToString());
			continue;
		}

		PlayerInfo& info = *ptrInfo;
		byte msgId;
		reader >> msgId;

		switch(msgId)
		{
		case ID_CONNECTION_LOST:
		case ID_DISCONNECTION_NOTIFICATION:
			Info(msgId == ID_CONNECTION_LOST ? "Lost connection with player %s." : "Player %s has disconnected.", info.name.c_str());
			--activePlayers;
			playersLeft = true;
			info.left = (msgId == ID_CONNECTION_LOST ? PlayerInfo::LEFT_DISCONNECTED : PlayerInfo::LEFT_QUIT);
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
			Warn("UpdateServer: Unknown packet from %s: %u.", info.name.c_str(), msgId);
			break;
		}
	}

	ProcessLeftPlayers();

	updateTimer += dt;
	if(updateTimer >= TICK && activePlayers > 1)
	{
		if(game->gameState == GS_LEVEL)
		{
			bool lastAnyoneTalking = team->anyoneTalking;
			bool lastCanFastTravel = gameLevel->canFastTravel;
			team->anyoneTalking = team->IsAnyoneTalking();
			gameLevel->canFastTravel = gameLevel->CanFastTravel();
			if(lastAnyoneTalking != team->anyoneTalking || lastCanFastTravel != gameLevel->canFastTravel)
				PushChange(NetChange::CHANGE_FLAGS);
		}

		updateTimer = 0;
		BitStreamWriter f;
		f << ID_CHANGES;

		// add changed units positions and ai modes
		if(game->gameState == GS_LEVEL)
		{
			for(LocationPart& locPart : gameLevel->ForEachPart())
				ServerProcessUnits(locPart.units);
		}

		// send revealed minimap tiles
		if(!gameLevel->minimapRevealMp.empty())
		{
			if(game->gameState == GS_LEVEL)
				PushChange(NetChange::REVEAL_MINIMAP);
			else
				gameLevel->minimapRevealMp.clear();
		}

		// changes
		WriteServerChanges(f);
		changes.clear();
		assert(netStrs.empty());
		SendAll(f, HIGH_PRIORITY, RELIABLE_ORDERED);

		for(PlayerInfo& info : players)
		{
			if(info.id == team->myId || info.left != PlayerInfo::LEFT_NO)
				continue;

			// update stats
			if(info.u->player->statFlags != 0)
			{
				NetChangePlayer& c = Add1(info.changes);
				c.id = info.u->player->statFlags;
				c.type = NetChangePlayer::PLAYER_STATS;
				info.u->player->statFlags = 0;
			}

			// write & send updates
			if(!info.changes.empty() || info.updateFlags)
			{
				BitStreamWriter f;
				WriteServerChangesForPlayer(f, info);
				SendServer(f, HIGH_PRIORITY, RELIABLE_ORDERED, info.adr);
				info.updateFlags = 0;
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
		if(!info.pc->isLocal && info.left == PlayerInfo::LEFT_NO)
			info.u->interp->Update(dt, info.u->visualPos, info.u->rot);
	}
}

//=================================================================================================
void Net::UpdateFastTravel(float dt)
{
	if(!fastTravel)
		return;

	if(!gameLevel->CanFastTravel())
	{
		CancelFastTravel(FAST_TRAVEL_NOT_SAFE, 0);
		return;
	}

	int playerId = -1;
	bool ok = true;
	for(PlayerInfo& info : players)
	{
		if(info.left == PlayerInfo::LEFT_NO && !info.fastTravel)
		{
			playerId = info.id;
			ok = false;
			break;
		}
	}

	if(ok)
	{
		NetChange& c = Add1(changes);
		c.type = NetChange::FAST_TRAVEL;
		c.id = FAST_TRAVEL_IN_PROGRESS;

		if(fastTravelNotification)
		{
			fastTravelNotification->Close();
			fastTravelNotification = nullptr;
		}

		game->ExitToMap();
	}
	else
	{
		fastTravelTimer += dt;
		if(fastTravelTimer >= 5.f)
			CancelFastTravel(FAST_TRAVEL_DENY, playerId);
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
		if((*it)->IsAI() && (*it)->ai->changeAiMode)
		{
			NetChange& c = Add1(changes);
			c.type = NetChange::CHANGE_AI_MODE;
			c.unit = *it;
			(*it)->ai->changeAiMode = false;
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

		gameLevel->WarpUnit(warp.u, warp.where, warp.building);
		warp.u->frozen = FROZEN::NO;

		NetChangePlayer& c = Add1(warp.u->player->playerInfo->changes);
		c.type = NetChangePlayer::SET_FROZEN;
		c.id = 0;

		return true;
	});
}

//=================================================================================================
void Net::ProcessLeftPlayers()
{
	if(!playersLeft)
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

		if(team->leaderId == c.id)
		{
			team->leaderId = team->myId;
			team->leader = game->pc->unit;
			NetChange& c2 = Add1(changes);
			c2.type = NetChange::CHANGE_LEADER;
			c2.id = team->myId;

			if(gameGui->worldMap->dialogEnc)
				gameGui->worldMap->dialogEnc->bts[0].state = Button::NONE;

			gameGui->AddMsg(game->txYouAreLeader);
		}

		team->CheckCredit();
		team->CalculatePlayersLevel();
		delete& info;

		return true;
	});

	playersLeft = false;
}

//=================================================================================================
void Net::RemovePlayer(PlayerInfo& info)
{
	switch(info.left)
	{
	case PlayerInfo::LEFT_TIMEOUT:
		{
			Info("Player %s kicked due to timeout.", info.name.c_str());
			gameGui->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_KICK:
		{
			Info("Player %s kicked from server.", info.name.c_str());
			gameGui->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_DISCONNECTED:
		{
			Info("Player %s disconnected from server.", info.name.c_str());
			gameGui->AddMsg(Format(game->txPlayerDisconnected, info.name.c_str()));
		}
		break;
	case PlayerInfo::LEFT_QUIT:
		{
			Info("Player %s quit game.", info.name.c_str());
			gameGui->AddMsg(Format(game->txPlayerQuit, info.name.c_str()));
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
	RemoveElementOrder(team->activeMembers.ptrs, unit);
	if(game->gameState == GS_WORLDMAP)
	{
		if(IsLocal() && !gameLevel->isOpen)
			game->DeleteUnit(unit);
	}
	else
	{
		gameLevel->toRemove.push_back(unit);
		unit->toRemove = true;
	}
	info.u = nullptr;
}

//=================================================================================================
bool Net::ProcessControlMessageServer(BitStreamReader& f, PlayerInfo& info)
{
	bool moveInfo;
	f >> moveInfo;
	if(!f)
	{
		Error("UpdateServer: Broken packet ID_CONTROL from %s.", info.name.c_str());
		return true;
	}

	Unit& unit = *info.u;
	PlayerController& player = *info.u->player;

	// movement/animation info
	if(moveInfo)
	{
		if(!info.warping && game->gameState == GS_LEVEL)
		{
			Vec3 newPos;
			float rot;
			Animation ani;

			f >> newPos;
			f >> rot;
			f >> unit.meshInst->groups[0].speed;
			f.ReadCasted<byte>(ani);
			if(!f)
			{
				Error("UpdateServer: Broken packet ID_CONTROL(2) from %s.", info.name.c_str());
				return true;
			}

			float dist = Vec3::Distance(unit.pos, newPos);
			if(dist >= 10.f)
			{
				// too big change in distance, warp unit to old position
				Warn("UpdateServer: Invalid unit movement from %s ((%g,%g,%g) -> (%g,%g,%g)).", info.name.c_str(), unit.pos.x, unit.pos.y, unit.pos.z,
					newPos.x, newPos.y, newPos.z);
				gameLevel->WarpUnit(unit, unit.pos);
				unit.interp->Add(unit.pos, rot);
			}
			else
			{
				unit.player->TrainMove(dist);
				if(player.noclip || unit.usable || CheckMove(unit, newPos))
				{
					// update position
					if(!unit.pos.Equal(newPos) && !gameLevel->location->outside)
					{
						// reveal minimap
						Int2 newTile(int(newPos.x / 2), int(newPos.z / 2));
						if(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)) != newTile)
							FOV::DungeonReveal(newTile, gameLevel->minimapReveal);
					}
					unit.pos = newPos;
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
	byte changesCount;
	f >> changesCount;
	if(!f)
	{
		Error("UpdateServer: Broken packet ID_CONTROL(4) from %s.", info.name.c_str());
		return true;
	}

	// process changes
	for(byte changeI = 0; changeI < changesCount; ++changeI)
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
				int iIndex;
				f >> iIndex;
				if(!f)
					Error("Update server: Broken CHANGE_EQUIPMENT from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					if(iIndex >= 0)
					{
						// equipping item
						if(uint(iIndex) >= unit.items.size())
						{
							Error("Update server: CHANGE_EQUIPMENT from %s, invalid index %d.", info.name.c_str(), iIndex);
							break;
						}

						ItemSlot& slot = unit.items[iIndex];
						if(!unit.CanWear(slot.item))
						{
							Error("Update server: CHANGE_EQUIPMENT from %s, item at index %d (%s) is not wearable.",
								info.name.c_str(), iIndex, slot.item->id.c_str());
							break;
						}

						unit.EquipItem(iIndex);
					}
					else
					{
						// removing item
						ITEM_SLOT slot = IIndexToSlot(iIndex);

						if(!IsValid(slot))
							Error("Update server: CHANGE_EQUIPMENT from %s, invalid slot type %d.", info.name.c_str(), slot);
						else if(!unit.HaveEquippedItem(slot))
							Error("Update server: CHANGE_EQUIPMENT from %s, empty slot type %d.", info.name.c_str(), slot);
						else
						{
							const Item* item = unit.GetEquippedItem(slot);
							unit.RemoveEquippedItem(slot);
							unit.AddItem(item, 1u, false);
						}
					}
				}
			}
			break;
		// player take/hide weapon
		case NetChange::TAKE_WEAPON:
			{
				bool hide;
				WeaponType weaponType;
				f >> hide;
				f.ReadCasted<byte>(weaponType);
				if(!f)
					Error("Update server: Broken TAKE_WEAPON from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
					unit.SetWeaponState(!hide, weaponType, true);
			}
			break;
		// player attacks
		case NetChange::ATTACK:
			{
				byte typeflags;
				float attackSpeed;
				Vec3 targetPos;
				f >> typeflags;
				f >> attackSpeed;
				if((typeflags & 0xF) == AID_Shoot)
					f >> targetPos;
				if(!f)
					Error("Update server: Broken ATTACK from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					const byte type = (typeflags & 0xF);
					bool isBow = false;
					switch(type)
					{
					case AID_Attack:
						if(unit.action == A_ATTACK && unit.animationState == AS_ATTACK_PREPARE)
						{
							const float ratio = unit.meshInst->groups[1].time / unit.GetAttackFrame(0);
							unit.animationState = AS_ATTACK_CAN_HIT;
							unit.meshInst->groups[1].speed = (ratio + unit.GetAttackSpeed()) * unit.GetStaminaAttackSpeedMod();
							unit.act.attack.power = ratio + 1.f;
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
							unit.act.attack.hitted = 0;
							unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
							unit.meshInst->groups[1].speed = attackSpeed;
						}
						unit.player->Train(TrainWhat::AttackStart, 0.f, 0);
						break;
					case AID_PrepareAttack:
						{
							if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
								game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
							unit.action = A_ATTACK;
							unit.animationState = AS_ATTACK_PREPARE;
							unit.act.attack.index = ((typeflags & 0xF0) >> 4);
							unit.act.attack.power = 1.f;
							unit.act.attack.run = false;
							unit.act.attack.hitted = 0;
							unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
							unit.meshInst->groups[1].speed = attackSpeed;
							const Weapon& weapon = unit.GetWeapon();
							unit.RemoveStamina(weapon.GetInfo().stamina * unit.GetStaminaMod(weapon));
							unit.timer = 0.f;
						}
						break;
					case AID_Shoot:
					case AID_StartShoot:
						isBow = true;
						if(unit.action == A_SHOOT && unit.animationState == AS_SHOOT_PREPARE)
							unit.animationState = AS_SHOOT_CAN;
						else
							unit.DoRangedAttack(type == AID_StartShoot, false);
						if(type == AID_Shoot)
							unit.targetPos = targetPos;
						break;
					case AID_Block:
						unit.action = A_BLOCK;
						unit.meshInst->Play(NAMES::aniBlock, PLAY_PRIO1 | PLAY_STOP_AT_END, 1);
						unit.meshInst->groups[1].speed = 1.f;
						unit.meshInst->groups[1].blendMax = attackSpeed;
						break;
					case AID_Bash:
						unit.action = A_BASH;
						unit.animationState = AS_BASH_ANIMATION;
						unit.meshInst->Play(NAMES::aniBash, PLAY_ONCE | PLAY_PRIO1, 1);
						unit.meshInst->groups[1].speed = attackSpeed;
						unit.player->Train(TrainWhat::BashStart, 0.f, 0);
						unit.RemoveStamina(Unit::STAMINA_BASH_ATTACK * unit.GetStaminaMod(unit.GetShield()));
						break;
					case AID_RunningAttack:
						{
							if(unit.data->sounds->Have(SOUND_ATTACK) && Rand() % 4 == 0)
								game->PlayAttachedSound(unit, unit.data->sounds->Random(SOUND_ATTACK), Unit::ATTACK_SOUND_DIST);
							unit.action = A_ATTACK;
							unit.animationState = AS_ATTACK_CAN_HIT;
							unit.act.attack.index = ((typeflags & 0xF0) >> 4);
							unit.act.attack.power = 1.5f;
							unit.act.attack.run = true;
							unit.act.attack.hitted = 0;
							unit.meshInst->Play(NAMES::aniAttacks[unit.act.attack.index], PLAY_PRIO1 | PLAY_ONCE, 1);
							unit.meshInst->groups[1].speed = attackSpeed;
							const Weapon& weapon = unit.GetWeapon();
							unit.RemoveStamina(weapon.GetInfo().stamina * 1.5f * unit.GetStaminaMod(weapon));
						}
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

					// send to other players
					if(activePlayers > 2)
					{
						NetChange& c = Add1(changes);
						c.unit = &unit;
						c.type = NetChange::ATTACK;
						c.id = typeflags;
						c.f[1] = attackSpeed;
					}
				}
			}
			break;
		// player drops item
		case NetChange::DROP_ITEM:
			{
				int iIndex, count;
				f >> iIndex;
				f >> count;
				if(!f)
					Error("Update server: Broken DROP_ITEM from %s.", info.name.c_str());
				else if(count == 0)
					Error("Update server: DROP_ITEM from %s, count %d.", info.name.c_str(), count);
				else if(game->gameState == GS_LEVEL)
				{
					GroundItem* item;

					if(iIndex >= 0)
					{
						// dropping unequipped item
						if(iIndex >= (int)unit.items.size())
						{
							Error("Update server: DROP_ITEM from %s, invalid index %d (count %d).", info.name.c_str(), iIndex, count);
							break;
						}

						ItemSlot& sl = unit.items[iIndex];
						if(count > (int)sl.count)
						{
							Error("Update server: DROP_ITEM from %s, index %d (count %d) have only %d count.", info.name.c_str(), iIndex,
								count, sl.count);
							count = sl.count;
						}
						sl.count -= count;
						unit.weight -= sl.item->weight * count;
						item = new GroundItem;
						item->Register();
						item->item = sl.item;
						item->count = count;
						item->teamCount = min(count, (int)sl.teamCount);
						sl.teamCount -= item->teamCount;
						if(sl.count == 0)
							unit.items.erase(unit.items.begin() + iIndex);
					}
					else
					{
						// dropping equipped item
						ITEM_SLOT slotType = IIndexToSlot(iIndex);
						if(!IsValid(slotType))
						{
							Error("Update server: DROP_ITEM from %s, invalid slot %d.", info.name.c_str(), slotType);
							break;
						}

						if(!unit.HaveEquippedItem(slotType))
						{
							Error("Update server: DROP_ITEM from %s, empty slot %d.", info.name.c_str(), slotType);
							break;
						}

						item = new GroundItem;
						item->Register();
						item->item = unit.GetEquippedItem(slotType);
						item->count = 1;
						item->teamCount = 0;
						unit.RemoveEquippedItem(slotType);
					}

					unit.action = A_ANIMATION;
					unit.meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
					item->pos = unit.pos;
					item->pos.x -= sin(unit.rot) * 0.25f;
					item->pos.z -= cos(unit.rot) * 0.25f;
					item->rot = Quat::RotY(Random(MAX_ANGLE));
					if(!questMgr->questSecret->CheckMoonStone(item, unit))
						unit.locPart->AddGroundItem(item);

					// send to other players
					if(activePlayers > 2)
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

				if(game->gameState != GS_LEVEL)
					break;

				LocationPart* locPart;
				GroundItem* groundItem = gameLevel->FindGroundItem(id, &locPart);
				if(!groundItem)
				{
					Error("Update server: PICKUP_ITEM from %s, missing item %d.", info.name.c_str(), id);
					break;
				}

				// add item
				unit.AddItem2(groundItem->item, groundItem->count, groundItem->teamCount, false);

				// start animation
				bool upAnimation = (groundItem->pos.y > unit.pos.y + 0.5f);
				unit.action = A_PICKUP;
				unit.animation = ANI_PLAY;
				unit.meshInst->Play(upAnimation ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);

				// pick gold sound
				if(groundItem->item->type == IT_GOLD)
				{
					NetChangePlayer& c = Add1(info.changes);
					c.type = NetChangePlayer::SOUND;
					c.id = 0;
				}

				// send info to other players about picking item
				if(activePlayers > 2)
				{
					NetChange& c3 = Add1(changes);
					c3.type = NetChange::PICKUP_ITEM;
					c3.unit = &unit;
					c3.count = (upAnimation ? 1 : 0);
				}

				// event
				ScriptEvent event(EVENT_PICKUP);
				event.onPickup.unit = &unit;
				event.onPickup.groundItem = groundItem;
				event.onPickup.item = groundItem->item;
				gameLevel->location->FireEvent(event);

				locPart->RemoveGroundItem(groundItem);
			}
			break;
		// player consume item
		case NetChange::CONSUME_ITEM:
			{
				int index;
				f >> index;
				if(!f)
					Error("Update server: Broken CONSUME_ITEM from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
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

				if(game->gameState != GS_LEVEL)
					break;

				Unit* lootedUnit = gameLevel->FindUnit(id);
				if(!lootedUnit)
				{
					Error("Update server: LOOT_UNIT from %s, missing unit %d.", info.name.c_str(), id);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::LOOT;
				if(lootedUnit->busy == Unit::Busy_Looted)
				{
					// someone else is already looting unit
					c.id = 0;
				}
				else
				{
					// start looting unit
					c.id = 1;
					lootedUnit->busy = Unit::Busy_Looted;
					player.action = PlayerAction::LootUnit;
					player.actionUnit = lootedUnit;
					player.chestTrade = &lootedUnit->items;
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

				if(game->gameState != GS_LEVEL)
					break;

				Chest* chest = gameLevel->FindChest(id);
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
					player.actionChest = chest;
					player.chestTrade = &chest->items;
					chest->OpenClose(player.unit);
				}
			}
			break;
		// player gets item from unit or container
		case NetChange::GET_ITEM:
			{
				int iIndex, count;
				f >> iIndex;
				f >> count;
				if(!f)
				{
					Error("Update server: Broken GET_ITEM from %s.", info.name.c_str());
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				if(!player.IsTrading())
				{
					Error("Update server: GET_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(iIndex >= 0)
				{
					// getting not equipped item
					if(iIndex >= (int)player.chestTrade->size())
					{
						Error("Update server: GET_ITEM from %s, invalid index %d.", info.name.c_str(), iIndex);
						break;
					}

					ItemSlot& slot = player.chestTrade->at(iIndex);
					if(count < 1 || count >(int)slot.count)
					{
						Error("Update server: GET_ITEM from %s, invalid item count %d (have %d).", info.name.c_str(), count, slot.count);
						break;
					}

					if(slot.item->type == IT_GOLD)
					{
						// special handling of gold
						uint teamCount = min(slot.teamCount, (uint)count);
						if(teamCount == 0)
						{
							unit.gold += slot.count;
							info.UpdateGold();
						}
						else
						{
							team->AddGold(teamCount);
							uint count = slot.count - teamCount;
							if(count)
							{
								unit.gold += count;
								info.UpdateGold();
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chestTrade->erase(player.chestTrade->begin() + iIndex);
						else
							slot.teamCount -= teamCount;
					}
					else
					{
						// player get item from corpse/chest/npc or bought from trader
						uint teamCount = (player.action == PlayerAction::Trade ? 0 : min((uint)count, slot.teamCount));
						unit.AddItem2(slot.item, (uint)count, teamCount, false, false);
						if(player.action == PlayerAction::Trade)
						{
							int price = ItemHelper::GetItemPrice(slot.item, unit, true) * count;
							unit.gold -= price;
							player.Train(TrainWhat::Trade, (float)price, 0);
						}
						else if(player.action == PlayerAction::ShareItems && slot.item->type == IT_CONSUMABLE)
						{
							const Consumable& pot = slot.item->ToConsumable();
							if(pot.aiType == Consumable::AiType::Healing)
								player.actionUnit->ai->havePotion = HavePotion::Check;
							else if(pot.aiType == Consumable::AiType::Mana)
								player.actionUnit->ai->haveMpPotion = HavePotion::Check;
						}
						if(player.action != PlayerAction::LootChest && player.action != PlayerAction::LootContainer)
						{
							player.actionUnit->weight -= slot.item->weight * count;
							if(player.action == PlayerAction::LootUnit)
							{
								if(slot.item == player.actionUnit->usedItem)
								{
									player.actionUnit->usedItem = nullptr;
									// removed item from hand, send info to other players
									if(activePlayers > 2)
									{
										NetChange& c = Add1(changes);
										c.type = NetChange::REMOVE_USED_ITEM;
										c.unit = player.actionUnit;
									}
								}

								if(IsSet(slot.item->flags, ITEM_IMPORTANT))
								{
									player.actionUnit->mark = false;
									NetChange& c = Add1(changes);
									c.type = NetChange::MARK_UNIT;
									c.unit = player.actionUnit;
									c.id = 0;
								}

								ScriptEvent e(EVENT_PICKUP);
								e.onPickup.unit = &unit;
								e.onPickup.groundItem = nullptr;
								e.onPickup.item = slot.item;
								player.actionUnit->FireEvent(e);
							}
						}
						slot.count -= count;
						if(slot.count == 0)
							player.chestTrade->erase(player.chestTrade->begin() + iIndex);
						else
							slot.teamCount -= teamCount;
					}
				}
				else
				{
					// getting equipped item
					ITEM_SLOT type = IIndexToSlot(iIndex);
					if(Any(player.action, PlayerAction::LootChest, PlayerAction::LootContainer)
						|| !IsValid(type)
						|| !player.actionUnit->HaveEquippedItem(type))
					{
						Error("Update server: GET_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
						break;
					}

					// get equipped item from unit
					const Item* item = player.actionUnit->GetEquippedItem(type);
					unit.AddItem2(item, 1u, 1u, false, false);
					player.actionUnit->RemoveEquippedItem(type);

					ScriptEvent e(EVENT_PICKUP);
					e.onPickup.unit = &unit;
					e.onPickup.groundItem = nullptr;
					e.onPickup.item = item;
					player.actionUnit->FireEvent(e);
				}
			}
			break;
		// player puts item into unit or container
		case NetChange::PUT_ITEM:
			{
				int iIndex;
				uint count;
				f >> iIndex;
				f >> count;
				if(!f)
				{
					Error("Update server: Broken PUT_ITEM from %s.", info.name.c_str());
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				if(!player.IsTrading())
				{
					Error("Update server: PUT_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				if(iIndex >= 0)
				{
					// put not equipped item
					if(iIndex >= (int)unit.items.size())
					{
						Error("Update server: PUT_ITEM from %s, invalid index %d.", info.name.c_str(), iIndex);
						break;
					}

					ItemSlot& slot = unit.items[iIndex];
					if(count < 1 || count > slot.count)
					{
						Error("Update server: PUT_ITEM from %s, invalid count %u (have %u).", info.name.c_str(), count, slot.count);
						break;
					}

					uint teamCount = min(count, slot.teamCount);

					// add item
					if(player.action == PlayerAction::LootChest)
						player.actionChest->AddItem(slot.item, count, teamCount, false);
					else if(player.action == PlayerAction::LootContainer)
						player.actionUsable->container->AddItem(slot.item, count, teamCount);
					else if(player.action == PlayerAction::Trade)
					{
						InsertItem(*player.chestTrade, slot.item, count, teamCount);
						int price = ItemHelper::GetItemPrice(slot.item, unit, false);
						player.Train(TrainWhat::Trade, (float)price * count, 0);
						if(teamCount)
							team->AddGold(price * teamCount);
						uint normalCount = count - teamCount;
						if(normalCount)
						{
							unit.gold += price * normalCount;
							info.UpdateGold();
						}
					}
					else
					{
						Unit* t = player.actionUnit;
						uint addAsTeam = teamCount;
						if(player.action == PlayerAction::GiveItems && slot.item->type != IT_CONSUMABLE)
						{
							addAsTeam = 0;
							int price = slot.item->value / 2;
							if(slot.teamCount > 0)
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
						t->AddItem2(slot.item, count, addAsTeam, false, false);
						if(player.action == PlayerAction::ShareItems || player.action == PlayerAction::GiveItems)
						{
							if(slot.item->type == IT_CONSUMABLE)
							{
								const Consumable& pot = slot.item->ToConsumable();
								if(pot.aiType == Consumable::AiType::Healing)
									t->ai->havePotion = HavePotion::Yes;
								else if(pot.aiType == Consumable::AiType::Mana)
									t->ai->haveMpPotion = HavePotion::Yes;
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
						unit.items.erase(unit.items.begin() + iIndex);
					else
						slot.teamCount -= teamCount;
				}
				else
				{
					// put equipped item
					ITEM_SLOT type = IIndexToSlot(iIndex);
					if(!IsValid(type) || !unit.HaveEquippedItem(type))
					{
						Error("Update server: PUT_ITEM from %s, invalid or empty slot %d.", info.name.c_str(), type);
						break;
					}

					const Item* item = unit.GetEquippedItem(type);
					int price = ItemHelper::GetItemPrice(item, unit, false);
					// add new item
					if(player.action == PlayerAction::LootChest)
						player.actionChest->AddItem(item, 1u, 0u, false);
					else if(player.action == PlayerAction::LootContainer)
						player.actionUsable->container->AddItem(item, 1u, 0u);
					else if(player.action == PlayerAction::Trade)
					{
						InsertItem(*player.chestTrade, item, 1u, 0u);
						unit.gold += price;
						player.Train(TrainWhat::Trade, (float)price, 0);
					}
					else
					{
						player.actionUnit->AddItem2(item, 1u, 0u, false, false);
						if(player.action == PlayerAction::GiveItems)
						{
							price = item->value / 2;
							if(player.actionUnit->gold >= price)
							{
								// sold for gold
								player.actionUnit->gold -= price;
								unit.gold += price;
							}
							player.actionUnit->UpdateInventory();
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::UPDATE_TRADER_INVENTORY;
							c.unit = player.actionUnit;
						}
					}
					// remove equipped
					unit.RemoveEquippedItem(type);
				}
			}
			break;
		// player picks up all items from corpse/chest
		case NetChange::GET_ALL_ITEMS:
			{
				if(game->gameState != GS_LEVEL)
					break;
				if(!player.IsTrading())
				{
					Error("Update server: GET_ALL_ITEM, player %s is not trading.", info.name.c_str());
					break;
				}

				Unit* lootedUnit = nullptr;
				bool any = false, pickupEvent = false;

				// slots
				if(player.action != PlayerAction::LootChest && player.action != PlayerAction::LootContainer)
				{
					lootedUnit = player.actionUnit;
					pickupEvent = lootedUnit->HaveEventHandler(EVENT_PICKUP);

					array<const Item*, SLOT_MAX>& equipped = lootedUnit->GetEquippedItems();
					for(int i = 0; i < SLOT_MAX; ++i)
					{
						if(const Item* item = equipped[i])
						{
							InsertItemBare(unit.items, item);
							unit.weight += item->weight;
							any = true;

							if(pickupEvent)
							{
								ScriptEvent e(EVENT_PICKUP);
								e.onPickup.unit = &unit;
								e.onPickup.groundItem = nullptr;
								e.onPickup.item = item;
								lootedUnit->FireEvent(e);
							}
						}
					}

					// reset weight
					lootedUnit->weight = 0;
					lootedUnit->RemoveAllEquippedItems();
				}

				// not equipped items
				for(ItemSlot& slot : *player.chestTrade)
				{
					if(!slot.item)
						continue;

					if(slot.item->type == IT_GOLD)
						unit.AddItem(Item::gold, slot.count, slot.teamCount);
					else
					{
						InsertItemBare(unit.items, slot.item, slot.count, slot.teamCount);
						unit.weight += slot.item->weight * slot.count;
						any = true;

						if(pickupEvent)
						{
							ScriptEvent e(EVENT_PICKUP);
							e.onPickup.unit = &unit;
							e.onPickup.groundItem = nullptr;
							e.onPickup.item = slot.item;
							lootedUnit->FireEvent(e);
						}
					}
				}
				player.chestTrade->clear();

				if(any)
					SortItems(unit.items);
			}
			break;
		// player ends trade
		case NetChange::STOP_TRADE:
			if(game->gameState != GS_LEVEL)
				break;
			if(!player.IsTrading())
			{
				Error("Update server: STOP_TRADE, player %s is not trading.", info.name.c_str());
				break;
			}

			if(player.action == PlayerAction::LootChest)
				player.actionChest->OpenClose(nullptr);
			else if(player.action == PlayerAction::LootContainer)
			{
				unit.UseUsable(nullptr);

				NetChange& c = Add1(changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = player.actionUsable->id;
				c.count = USE_USABLE_END;
			}
			else if(player.actionUnit)
			{
				player.actionUnit->busy = Unit::Busy_No;
				player.actionUnit->lookTarget = nullptr;
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

				if(game->gameState != GS_LEVEL)
					break;

				if(index < 0 || index >= (int)unit.items.size())
				{
					Error("Update server: TAKE_ITEM_CREDIT from %s, invalid index %d.", info.name.c_str(), index);
					break;
				}

				ItemSlot& slot = unit.items[index];
				if(unit.CanWear(slot.item) && slot.teamCount != 0)
				{
					slot.teamCount = 0;
					player.credit += slot.item->value / 2;
					team->CheckCredit(true);
				}
				else
				{
					Error("Update server: TAKE_ITEM_CREDIT from %s, item %s (count %u, team count %u).", info.name.c_str(), slot.item->id.c_str(),
						slot.count, slot.teamCount);
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
				else if(game->gameState == GS_LEVEL)
				{
					unit.meshInst->Play(unit.data->idles->anims[index].c_str(), PLAY_ONCE, 0);
					unit.animation = ANI_IDLE;
					// send info to other players
					if(activePlayers > 2)
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

				if(game->gameState != GS_LEVEL)
					break;

				Unit* talkTo = gameLevel->FindUnit(id);
				if(!talkTo)
				{
					Error("Update server: TALK from %s, missing unit %d.", info.name.c_str(), id);
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::START_DIALOG;
				if(talkTo->busy != Unit::Busy_No || !talkTo->CanTalk(unit))
				{
					// can't talk to unit
					c.id = -1;
				}
				else if(talkTo->locPart != unit.locPart)
				{
					// unit left/entered building
					c.id = -2;
				}
				else
				{
					// start dialog
					c.id = talkTo->id;
					player.dialogCtx->StartDialog(talkTo);
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
				else if(game->gameState == GS_LEVEL)
				{
					if(player.dialogCtx->mode == DialogContext::WAIT_CHOICES && choice < player.dialogCtx->choices.size())
						player.dialogCtx->choiceSelected = choice;
					else
						Error("Update server: CHOICE from %s, not in dialog or invalid choice %u.", info.name.c_str(), choice);
				}
			}
			break;
		// pomijanie dialogu przez gracza
		case NetChange::SKIP_DIALOG:
			{
				int skipId;
				f >> skipId;
				if(!f)
					Error("Update server: Broken SKIP_DIALOG from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					DialogContext& ctx = *player.dialogCtx;
					if(ctx.dialogMode && ctx.mode == DialogContext::WAIT_TALK && ctx.skipId == skipId)
						ctx.choiceSelected = 1;
				}
			}
			break;
		// player wants to enter building
		case NetChange::ENTER_BUILDING:
			{
				byte buildingIndex;
				f >> buildingIndex;
				if(!f)
					Error("Update server: Broken ENTER_BUILDING from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					if(gameLevel->cityCtx && buildingIndex < gameLevel->cityCtx->insideBuildings.size())
					{
						WarpData& warp = Add1(warps);
						warp.u = &unit;
						warp.where = buildingIndex;
						warp.building = -1;
						warp.timer = 1.f;
						unit.frozen = FROZEN::YES;
					}
					else
						Error("Update server: ENTER_BUILDING from %s, invalid building index %u.", info.name.c_str(), buildingIndex);
				}
			}
			break;
		// player wants to exit building
		case NetChange::EXIT_BUILDING:
			if(game->gameState == GS_LEVEL)
			{
				if(unit.locPart->partType == LocationPart::Type::Building)
				{
					WarpData& warp = Add1(warps);
					warp.u = &unit;
					warp.where = LocationPart::OUTSIDE_ID;
					warp.building = -1;
					warp.timer = 1.f;
					unit.frozen = FROZEN::YES;
				}
				else
					Error("Update server: EXIT_BUILDING from %s, unit not in building.", info.name.c_str());
			}
			break;
		// notify about warping
		case NetChange::WARP:
			if(game->gameState == GS_LEVEL)
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
					gameGui->AddMsg(Format(game->txRolledNumber, info.name.c_str(), number));
					// send to other players
					if(activePlayers > 2)
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
				int usableId;
				USE_USABLE_STATE state;
				f >> usableId;
				f.ReadCasted<byte>(state);
				if(!f)
				{
					Error("Update server: Broken USE_USABLE from %s.", info.name.c_str());
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Usable* usable = gameLevel->FindUsable(usableId);
				if(!usable)
				{
					Error("Update server: USE_USABLE from %s, missing usable %d.", info.name.c_str(), usableId);
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
						if(!IsSet(base.useFlags, BaseUsable::CONTAINER))
						{
							unit.action = A_USE_USABLE;
							unit.animation = ANI_PLAY;
							unit.meshInst->Play(base.anim.c_str(), PLAY_PRIO1, 0);
							unit.targetPos = unit.pos;
							unit.targetPos2 = usable->pos;
							if(usable->base->limitRot == 4)
								unit.targetPos2 -= Vec3(sin(usable->rot) * 1.5f, 0, cos(usable->rot) * 1.5f);
							unit.timer = 0.f;
							unit.animationState = AS_USE_USABLE_MOVE_TO_OBJECT;
							unit.act.useUsable.rot = Vec3::LookAtAngle(unit.pos, usable->pos);
							unit.usedItem = base.item;
							if(unit.usedItem)
								unit.SetWeaponStateInstant(WeaponState::Hidden, W_NONE);
						}
						else
						{
							// start looting container
							NetChangePlayer& c = Add1(info.changes);
							c.type = NetChangePlayer::LOOT;
							c.id = 1;

							player.action = PlayerAction::LootContainer;
							player.actionUsable = usable;
							player.chestTrade = &usable->container->items;
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
							if(!IsSet(base.useFlags, BaseUsable::CONTAINER))
							{
								unit.action = A_USE_USABLE;
								unit.animation = ANI_STAND;
								unit.animationState = AS_USE_USABLE_MOVE_TO_ENDPOINT;
								unit.timer = 0;
								if(unit.liveState == Unit::ALIVE)
									unit.usedItem = nullptr;
							}
						}
						else if(state == USE_USABLE_END)
						{
							if(unit.action == A_USE_USABLE)
								unit.action = A_NONE;
							unit.UseUsable(nullptr);
						}
					}
				}

				// send info to players
				NetChange& c = Add1(changes);
				c.type = NetChange::USE_USABLE;
				c.unit = info.u;
				c.id = usableId;
				c.count = state;
			}
			break;
		// client checks if item is better for npc
		case NetChange::IS_BETTER_ITEM:
			{
				int iIndex;
				f >> iIndex;
				if(!f)
				{
					Error("Update server: Broken IS_BETTER_ITEM from %s.", info.name.c_str());
					break;
				}

				NetChangePlayer& c = Add1(info.changes);
				c.type = NetChangePlayer::IS_BETTER_ITEM;
				c.id = 0;

				if(game->gameState != GS_LEVEL)
					break;

				if(player.action == PlayerAction::GiveItems)
				{
					const Item* item = unit.GetIIndexItem(iIndex);
					if(item)
					{
						if(player.actionUnit->IsBetterItem(item))
							c.id = 1;
					}
					else
						Error("Update server: IS_BETTER_ITEM from %s, invalid iIndex %d.", info.name.c_str(), iIndex);
				}
				else
					Error("Update server: IS_BETTER_ITEM from %s, player is not giving items.", info.name.c_str());
			}
			break;
		// player used cheat 'gotoMap'
		case NetChange::CHEAT_GOTO_MAP:
			if(info.devmode)
			{
				if(game->gameState != GS_LEVEL)
					break;
				game->ExitToMap();
				return false;
			}
			else
				Error("Update server: Player %s used CHEAT_GOTO_MAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'revealMinimap'
		case NetChange::CHEAT_REVEAL_MINIMAP:
			if(info.devmode)
			{
				if(game->gameState == GS_LEVEL)
					gameLevel->RevealMinimap();
			}
			else
				Error("Update server: Player %s used CHEAT_REVEAL_MINIMAP without devmode.", info.name.c_str());
			break;
		// player used cheat 'addItem' or 'addTeamItem'
		case NetChange::CHEAT_ADD_ITEM:
			{
				int count;
				bool isTeam;
				const string& itemId = f.ReadString1();
				f >> count;
				f >> isTeam;
				if(!f)
					Error("Update server: Broken CHEAT_ADD_ITEM from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_ADD_ITEM without devmode.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					const Item* item = Item::TryGet(itemId);
					if(item && count)
						info.u->AddItem2(item, count, isTeam ? count : 0u, false);
					else
						Error("Update server: CHEAT_ADD_ITEM from %s, missing item %s or invalid count %u.", info.name.c_str(), itemId.c_str(), count);
				}
			}
			break;
		// player used cheat 'spawnUnit'
		case NetChange::CHEAT_SPAWN_UNIT:
			{
				byte count;
				char level, inArena;
				const string& unitId = f.ReadString1();
				f >> count;
				f >> level;
				f >> inArena;
				if(!f)
					Error("Update server: Broken CHEAT_SPAWN_UNIT from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_SPAWN_UNIT without devmode.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					UnitData* data = UnitData::TryGet(unitId);
					if(!data)
						Error("Update server: CHEAT_SPAWN_UNIT from %s, invalid unit id %s.", info.name.c_str(), unitId.c_str());
					else
					{
						if(inArena < -1 || inArena > 1)
							inArena = -1;

						LocationPart& locPart = *info.u->locPart;
						Vec3 pos = info.u->GetFrontPos();

						for(byte i = 0; i < count; ++i)
						{
							Unit* spawned = gameLevel->SpawnUnitNearLocation(locPart, pos, *data, &unit.pos, level);
							if(!spawned)
							{
								Warn("Update server: CHEAT_SPAWN_UNIT from %s, no free space for unit.", info.name.c_str());
								break;
							}
							else if(inArena != -1)
							{
								spawned->inArena = inArena;
								game->arena->units.push_back(spawned);
							}
						}
					}
				}
			}
			break;
		// player used cheat 'setStat' or 'modStat'
		case NetChange::CHEAT_SET_STAT:
		case NetChange::CHEAT_MOD_STAT:
			{
				cstring name = (type == NetChange::CHEAT_SET_STAT ? "CHEAT_SET_STAT" : "CHEAT_MOD_STAT");

				byte what;
				bool isSkill;
				char value;
				f >> what;
				f >> isSkill;
				f >> value;
				if(!f)
					Error("Update server: Broken %s from %s.", name, info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used %s without devmode.", info.name.c_str(), name);
				else if(game->gameState == GS_LEVEL)
				{
					if(isSkill)
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
								NetChangePlayer& c = Add1(player.playerInfo->changes);
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
								NetChangePlayer& c = Add1(player.playerInfo->changes);
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
				else if(game->gameState == GS_LEVEL)
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
				else if(game->gameState == GS_LEVEL)
				{
					CanLeaveLocationResult result = gameLevel->CanLeaveLocation(*info.u);
					if(result == CanLeaveLocationResult::Yes)
					{
						PushChange(NetChange::LEAVE_LOCATION);
						if(type == ENTER_FROM_OUTSIDE)
							game->fallbackType = FALLBACK::EXIT;
						else if(type == ENTER_FROM_PREV_LEVEL)
						{
							game->fallbackType = FALLBACK::CHANGE_LEVEL;
							game->fallbackValue = -1;
						}
						else if(type == ENTER_FROM_NEXT_LEVEL)
						{
							game->fallbackType = FALLBACK::CHANGE_LEVEL;
							game->fallbackValue = +1;
						}
						else
						{
							if(gameLevel->location->TryGetPortal(type))
							{
								game->fallbackType = FALLBACK::USE_PORTAL;
								game->fallbackValue = type;
							}
							else
							{
								Error("Update server: LEAVE_LOCATION from %s, invalid type %d.", type);
								break;
							}
						}

						game->fallbackTimer = -1.f;
						for(Unit& teamMember : team->members)
							teamMember.frozen = FROZEN::YES;
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
				bool isClosing;
				f >> id;
				f >> isClosing;
				if(!f)
				{
					Error("Update server: Broken USE_DOOR from %s.", info.name.c_str());
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				Door* door = gameLevel->FindDoor(id);
				if(door)
					door->SetState(isClosing);
				else
					Error("Update server: USE_DOOR from %s, missing door %d.", info.name.c_str(), id);
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
					Error("Update server: Broken TRAVEL_POS from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: TRAVEL_POS from %s, player is not leader.", info.name.c_str());
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
					Error("Update server: Broken STOP_TRAVEL from %s.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: STOP_TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(game->gameState == GS_WORLDMAP)
					world->StopTravel(pos, true);
			}
			break;
		// leader finished travel
		case NetChange::END_TRAVEL:
			if(game->gameState == GS_WORLDMAP)
				world->EndTravel();
			break;
		// enter current location
		case NetChange::ENTER_LOCATION:
			if(game->gameState == GS_WORLDMAP && world->GetState() == World::State::ON_MAP && team->IsLeader(info.u))
			{
				game->EnterLocation();
				return false;
			}
			else
				Error("Update server: ENTER_LOCATION from %s, not leader or not on map.", info.name.c_str());
			break;
		// close encounter message box
		case NetChange::CLOSE_ENCOUNTER:
			if(game->gameState == GS_WORLDMAP)
			{
				if(gameGui->worldMap->dialogEnc)
				{
					gui->CloseDialog(gameGui->worldMap->dialogEnc);
					gameGui->worldMap->dialogEnc = nullptr;
				}
				PushChange(NetChange::CLOSE_ENCOUNTER);
				game->Event_RandomEncounter(0);
				return false;
			}
			break;
		// player used cheat to change level (<>+shift+ctrl)
		case NetChange::CHEAT_CHANGE_LEVEL:
			{
				bool isDown;
				f >> isDown;
				if(!f)
					Error("Update server: Broken CHEAT_CHANGE_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_CHANGE_LEVEL without devmode.", info.name.c_str());
				else if(gameLevel->location->outside)
					Error("Update server:CHEAT_CHANGE_LEVEL from %s, outside location.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					game->ChangeLevel(isDown ? +1 : -1);
					return false;
				}
			}
			break;
		// player used cheat to warp to entry (<>+shift)
		case NetChange::CHEAT_WARP_TO_ENTRY:
			{
				bool isDown;
				f >> isDown;
				if(!f)
					Error("Update server: Broken CHEAT_WARP_TO_ENTRY from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_WARP_TO_ENTRY without devmode.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					InsideLocation* inside = (InsideLocation*)gameLevel->location;
					InsideLocationLevel& lvl = inside->GetLevelData();

					if(!isDown)
					{
						Int2 tile = lvl.GetPrevEntryFrontTile();
						unit.rot = DirToRot(lvl.prevEntryDir);
						gameLevel->WarpUnit(unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
					}
					else
					{
						Int2 tile = lvl.GetNextEntryFrontTile();
						unit.rot = DirToRot(lvl.nextEntryDir);
						gameLevel->WarpUnit(unit, Vec3(2.f * tile.x + 1.f, 0.f, 2.f * tile.y + 1.f));
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
				else if(game->gameState == GS_LEVEL)
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
				byte type;
				int value;
				f >> type;
				f >> value;
				if(!f)
					Error("Update server: Broken TRAIN from %s.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
				{
					if(type == 2)
						questMgr->questTournament->Train(player);
					else
					{
						cstring error = nullptr;
						if(type == 0)
						{
							if(value >= (byte)AttributeId::MAX)
								error = "attribute";
						}
						else if(type == 1)
						{
							if(value >= (byte)SkillId::MAX)
								error = "skill";
						}
						else if(type == 3)
						{
							Perk* perk = Perk::Get(value);
							if(!perk)
								error = "perk";
							else
							{
								player.AddPerk(perk, -1);
								NetChangePlayer& c = Add1(info.changes);
								c.type = NetChangePlayer::GAME_MESSAGE;
								c.id = GMS_LEARNED_PERK;
							}
						}
						else
						{
							Ability* ability = Ability::Get(value);
							if(!ability)
								error = "ability";
							else
							{
								player.AddAbility(ability);
								NetChangePlayer& c = Add1(info.changes);
								c.type = NetChangePlayer::GAME_MESSAGE;
								c.id = GMS_LEARNED_ABILITY;
							}
						}

						if(error)
							Error("Update server: TRAIN from %s, invalid %d %u.", info.name.c_str(), error, value);
						else if(type == 0 || type == 1)
							player.Train(type == 1, value);
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
				else if(team->leaderId != info.id)
					Error("Update server: CHANGE_LEADER from %s, player is not leader.", info.name.c_str());
				else
				{
					PlayerInfo* newLeader = TryGetPlayer(id);
					if(!newLeader)
					{
						Error("Update server: CHANGE_LEADER from %s, invalid player id %u.", id);
						break;
					}

					team->leaderId = id;
					team->leader = newLeader->u;

					if(team->leaderId == team->myId)
						gameGui->AddMsg(game->txYouAreLeader);
					else
						gameGui->AddMsg(Format(game->txPcIsLeader, team->leader->player->name.c_str()));

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

				if(game->gameState != GS_LEVEL)
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
							NetChangePlayer& c = Add1(target->player->playerInfo->changes);
							c.type = NetChangePlayer::GOLD_RECEIVED;
							c.id = info.id;
							c.count = count;
							target->player->playerInfo->UpdateGold();
						}
						else
						{
							gameGui->mpBox->Add(Format(game->txReceivedGold, count, info.name.c_str()));
							soundMgr->PlaySound2d(gameRes->sCoins);
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
				else if(game->gameState == GS_LEVEL)
				{
					if(count > 0 && count <= unit.gold && unit.IsStanding() && unit.action == A_NONE)
					{
						unit.gold -= count;

						// animation
						unit.action = A_ANIMATION;
						unit.meshInst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);

						// create item
						GroundItem* groundItem = new GroundItem;
						groundItem->Register();
						groundItem->item = Item::gold;
						groundItem->count = count;
						groundItem->teamCount = 0;
						groundItem->pos = unit.pos;
						groundItem->pos.x -= sin(unit.rot) * 0.25f;
						groundItem->pos.z -= cos(unit.rot) * 0.25f;
						groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
						info.u->locPart->AddGroundItem(groundItem);

						// send info to other players
						if(activePlayers > 2)
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
				else if(game->gameState == GS_LEVEL)
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
						InsertItem(*player.chestTrade, Item::gold, count, 0);
						unit.gold -= count;
					}
				}
			}
			break;
		// player used cheat for fast travel on map
		case NetChange::CHEAT_TRAVEL:
			{
				byte locationIndex;
				f >> locationIndex;
				if(!f)
					Error("Update server: Broken CHEAT_TRAVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CHEAT_TRAVEL without devmode.", info.name.c_str());
				else if(!team->IsLeader(unit))
					Error("Update server: CHEAT_TRAVEL from %s, player is not leader.", info.name.c_str());
				else if(!world->VerifyLocation(locationIndex))
					Error("Update server: CHEAT_TRAVEL from %s, invalid location index %u.", info.name.c_str(), locationIndex);
				else if(game->gameState == GS_WORLDMAP)
				{
					world->Warp(locationIndex, false);
					gameGui->worldMap->StartTravel(true);
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
				else if(game->gameState == GS_WORLDMAP)
				{
					world->WarpPos(pos, false);
					gameGui->worldMap->StartTravel(true);
				}
			}
			break;
		// player yell to move ai
		case NetChange::YELL:
			if(game->gameState == GS_LEVEL)
				player.Yell();
			break;
		// player used ability
		case NetChange::PLAYER_ABILITY:
			{
				int netid;
				Vec3 pos;
				int abilityHash;
				f >> netid;
				f >> pos;
				f >> abilityHash;
				if(!f)
				{
					Error("Update server: Broken PLAYER_ABILITY from %s.", info.name.c_str());
					break;
				}
				Ability* ability = Ability::Get(abilityHash);
				if(!ability)
				{
					Error("Update server: PLAYER_ABILITY, invalid ability %u.", abilityHash);
					break;
				}
				if(game->gameState == GS_LEVEL)
				{
					Unit* target = gameLevel->FindUnit(netid);
					if(!target && netid != -1)
						Error("Update server: PLAYER_ABILITY, invalid target %d from %s.", netid, info.name.c_str());
					else
						info.pc->UseAbility(ability, false, &pos, target);
				}
			}
			break;
		// client fallback ended
		case NetChange::END_FALLBACK:
			info.u->frozen = FROZEN::NO;
			break;
		// run script
		case NetChange::RUN_SCRIPT:
			{
				LocalString code;
				int targetId;
				f.ReadString4(*code);
				f >> targetId;
				if(!f)
				{
					Error("Update server: Broken RUN_SCRIPT from %s.", info.name.c_str());
					break;
				}

				if(game->gameState != GS_LEVEL)
					break;

				if(!info.devmode)
				{
					Error("Update server: Player %s used RUN_SCRIPT without devmode.", info.name.c_str());
					break;
				}

				Unit* target = gameLevel->FindUnit(targetId);
				if(!target && targetId != -1)
				{
					Error("Update server: RUN_SCRIPT, invalid target %d from %s.", targetId, info.name.c_str());
					break;
				}

				string& output = scriptMgr->OpenOutput();
				ScriptContext& ctx = scriptMgr->GetContext();
				ctx.pc = info.pc;
				ctx.target = target;
				scriptMgr->RunScript(code->c_str());

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
				scriptMgr->CloseOutput();
			}
			break;
		// player set next action
		case NetChange::SET_NEXT_ACTION:
			{
				f.ReadCasted<byte>(player.nextAction);
				if(!f)
				{
					Error("Update server: Broken SET_NEXT_ACTION from '%s'.", info.name.c_str());
					player.nextAction = NA_NONE;
					break;
				}
				switch(player.nextAction)
				{
				case NA_NONE:
					break;
				case NA_REMOVE:
				case NA_DROP:
					f.ReadCasted<byte>(player.nextActionData.slot);
					if(!f)
					{
						Error("Update server: Broken SET_NEXT_ACTION(2) from '%s'.", info.name.c_str());
						player.nextAction = NA_NONE;
					}
					else if(game->gameState != GS_LEVEL)
						player.nextAction = NA_NONE;
					else if(!IsValid(player.nextActionData.slot) || !unit.HaveEquippedItem(player.nextActionData.slot))
					{
						Error("Update server: SET_NEXT_ACTION, invalid slot %d from '%s'.", player.nextActionData.slot, info.name.c_str());
						player.nextAction = NA_NONE;
					}
					break;
				case NA_EQUIP:
				case NA_CONSUME:
				case NA_EQUIP_DRAW:
					{
						f >> player.nextActionData.index;
						const string& itemId = f.ReadString1();
						if(!f)
						{
							Error("Update server: Broken SET_NEXT_ACTION(3) from '%s'.", info.name.c_str());
							player.nextAction = NA_NONE;
						}
						else if(game->gameState != GS_LEVEL)
							player.nextAction = NA_NONE;
						else if(player.nextActionData.index < 0 || (uint)player.nextActionData.index >= info.u->items.size())
						{
							Error("Update server: SET_NEXT_ACTION, invalid index %d from '%s'.", player.nextActionData.index, info.name.c_str());
							player.nextAction = NA_NONE;
						}
						else
						{
							player.nextActionData.item = Item::TryGet(itemId);
							if(!player.nextActionData.item)
							{
								Error("Update server: SET_NEXT_ACTION, invalid item '%s' from '%s'.", itemId.c_str(), info.name.c_str());
								player.nextAction = NA_NONE;
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
							player.nextAction = NA_NONE;
						}
						else if(game->gameState == GS_LEVEL)
						{
							player.nextActionData.usable = gameLevel->FindUsable(id);
							if(!player.nextActionData.usable)
							{
								Error("Update server: SET_NEXT_ACTION, invalid usable %d from '%s'.", id, info.name.c_str());
								player.nextAction = NA_NONE;
							}
						}
					}
					break;
				default:
					Error("Update server: SET_NEXT_ACTION, invalid action %d from '%s'.", player.nextAction, info.name.c_str());
					player.nextAction = NA_NONE;
					break;
				}
			}
			break;
		// player toggle always run - notify to save it
		case NetChange::CHANGE_ALWAYS_RUN:
			{
				bool alwaysRun;
				f >> alwaysRun;
				if(!f)
					Error("Update server: Broken CHANGE_ALWAYS_RUN from %s.", info.name.c_str());
				else
					player.alwaysRun = alwaysRun;
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
				else if(game->gameState != GS_LEVEL)
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
				if(game->gameState != GS_LEVEL)
					break;
				if(index < 0 || index >= (int)unit.items.size())
				{
					Error("Update server: USE_ITEM, invalid index %d from %s.", index, info.name.c_str());
					break;
				}
				ItemSlot& slot = unit.items[index];
				if(!slot.item || slot.item->type != IT_BOOK)
				{
					Error("Update server: USE_ITEM, invalid item '%s' at index %d from %s.",
						slot.item ? slot.item->id.c_str() : "null", index, info.name.c_str());
					break;
				}

				const Book& book = slot.item->ToBook();
				if(IsSet(book.flags, ITEM_MAGIC_SCROLL))
				{
					unit.action = A_USE_ITEM;
					unit.usedItem = slot.item;
					unit.meshInst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);

					NetChange& c = Add1(changes);
					c.type = NetChange::USE_ITEM;
					c.unit = &unit;
				}
				else
				{
					if(!book.recipes.empty())
					{
						int skill = unit.GetBase(SkillId::ALCHEMY);
						bool anythingTooHard = false;
						for(Recipe* recipe : book.recipes)
						{
							if(!player.HaveRecipe(recipe))
							{
								if(skill >= recipe->skill)
									player.AddRecipe(recipe);
								else
									anythingTooHard = true;
							}
						}
						if(IsSet(book.flags, ITEM_SINGLE_USE))
							unit.RemoveItem(index, 1u);
						if(anythingTooHard)
							gameGui->messages->AddGameMsg3(&player, GMS_TOO_COMPLICATED);

						NetChangePlayer& c = Add1(info.changes);
						c.type = NetChangePlayer::END_PREPARE;
					}

					questMgr->CheckItemEventHandler(&unit, &book);
				}
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
				else if(game->gameState == GS_LEVEL)
					cmdp->ParseStringCommand(CMD_ARENA, str, info);
			}
			break;
		// clean level from blood and corpses
		case NetChange::CLEAN_LEVEL:
			{
				int buildingId;
				f >> buildingId;
				if(!f)
					Error("Update server: Broken CLEAN_LEVEL from %s.", info.name.c_str());
				else if(!info.devmode)
					Error("Update server: Player %s used CLEAN_LEVEL without devmode.", info.name.c_str());
				else if(game->gameState == GS_LEVEL)
					gameLevel->CleanLevel(buildingId);
			}
			break;
		// player set shortcut
		case NetChange::SET_SHORTCUT:
			{
				int index, value;
				Shortcut::Type type;
				f.ReadCasted<byte>(index);
				f.ReadCasted<byte>(type);
				bool ok = true;
				switch(type)
				{
				case Shortcut::TYPE_NONE:
					value = 0;
					break;
				case Shortcut::TYPE_SPECIAL:
					f.ReadCasted<byte>(value);
					break;
				case Shortcut::TYPE_ITEM:
					{
						const string& itemId = f.ReadString1();
						if(!f)
							break;
						value = (int)Item::TryGet(itemId);
						if(value == 0)
						{
							Error("Update server: SET_SHORTCUT invalid item '%s' from %s.", itemId.c_str(), info.name.c_str());
							ok = false;
						}
					}
					break;
				case Shortcut::TYPE_ABILITY:
					{
						int abilityHash;
						f >> abilityHash;
						if(!f)
							break;
						value = (int)Ability::Get(abilityHash);
						if(value == 0)
						{
							Error("Update server: SET_SHORTCUT invalid ability %u from %s.", abilityHash, info.name.c_str());
							ok = false;
						}
					}
					break;
				default:
					ok = false;
					break;
				}
				if(!ok)
					break;
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
					else if(!gameLevel->CanFastTravel())
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
					if(info.fastTravel)
						Error("Update server: FAST_TRAVEL already accepted from %s.", info.name.c_str());
					else
					{
						info.fastTravel = true;

						NetChange& c = Add1(changes);
						c.type = NetChange::FAST_TRAVEL_VOTE;
						c.id = info.id;
					}
					break;
				case FAST_TRAVEL_DENY:
					if(info.fastTravel)
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
		// unit cast spell
		case NetChange::CAST_SPELL:
			f >> info.u->targetPos;
			if(!f)
				Error("Update server: Broken CAST_SPELL from %s.", info.name.c_str());
			info.u->animationState = AS_CAST_TRIGGER;
			break;
		// craft item
		case NetChange::CRAFT:
			{
				int recipeHash = f.Read<int>();
				uint count = f.Read<uint>();
				if(!f)
				{
					Error("Update server: Broken CRAFT from %s.", info.name.c_str());
					break;
				}
				Recipe* recipe = Recipe::TryGet(recipeHash);
				if(!recipe)
				{
					Error("Update server: CRAFT, invalid recipe %d.", recipeHash);
					break;
				}
				if(count > 0)
				{
					if(!gameGui->craft->DoPlayerCraft(player, recipe, count))
						Error("Update server: CRAFT, missing ingredients to craft %s.", recipe->id.c_str());
				}
			}
			break;
		// response to pick rest days dialog
		case NetChange::PICK_REST:
			{
				int days;
				f >> days;
				if(!f)
				{
					Error("Update server: Broken PICK_REST from %s.", info.name.c_str());
					break;
				}
				player.dialogCtx->OnPickRestDays(days);
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
			Error("Update server: Invalid checksum from %s (%u).", info.name.c_str(), changeI);
			return true;
		}
	}

	return true;
}

//=================================================================================================
bool Net::CheckMove(Unit& unit, const Vec3& pos)
{
	gameLevel->globalCol.clear();

	const float radius = unit.GetUnitRadius();
	Level::IgnoreObjects ignore{};
	const Unit* ignoredUnits[2]{ &unit };
	const void* ignoredObjects[2]{};
	if(unit.usable)
		ignoredObjects[0] = unit.usable;
	ignore.ignoredUnits = ignoredUnits;
	ignore.ignoredObjects = ignoredObjects;

	gameLevel->GatherCollisionObjects(*unit.locPart, gameLevel->globalCol, pos, radius, &ignore);

	if(gameLevel->globalCol.empty())
		return true;

	return !gameLevel->Collide(gameLevel->globalCol, pos, radius);
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
				f << unit.meshInst->groups[0].speed;
				f.WriteCasted<byte>(unit.animation);
			}
			break;
		case NetChange::CHANGE_EQUIPMENT:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			f << c.unit->GetEquippedItem((ITEM_SLOT)c.id);
			break;
		case NetChange::TAKE_WEAPON:
			{
				Unit& u = *c.unit;
				f << u.id;
				f << (c.id != 0);
				f.WriteCasted<byte>(c.id == 0 ? u.weaponTaken : u.weaponHiding);
			}
			break;
		case NetChange::ATTACK:
			{
				Unit& u = *c.unit;
				f << u.id;
				byte b = (byte)c.id;
				b |= ((u.act.attack.index & 0xF) << 4);
				f << b;
				f << c.f[1];
			}
			break;
		case NetChange::CHANGE_FLAGS:
			{
				byte b = 0;
				if(team->isBandit)
					b |= F_IS_BANDIT;
				if(team->craziesAttack)
					b |= F_CRAZIES_ATTACKING;
				if(team->anyoneTalking)
					b |= F_ANYONE_TALKING;
				if(gameLevel->canFastTravel)
					b |= F_CAN_FAST_TRAVEL;
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
		case NetChange::USE_ITEM:
		case NetChange::BOSS_START:
		case NetChange::UNIT_MISC_SOUND:
			f << c.unit->id;
			break;
		case NetChange::TELL_NAME:
			f << c.unit->id;
			f << (c.id == 1);
			if(c.id == 1)
				f << c.unit->hero->name;
			break;
		case NetChange::UNIT_SOUND:
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
		case NetChange::CREATE_SPELL_BALL:
			f.Write(&c.data, c.size);
			break;
		case NetChange::UPDATE_CREDIT:
			{
				f << (byte)team->GetActiveTeamSize();
				for(Unit& unit : team->activeMembers)
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
						f << info.u->player->freeDays;
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
		case NetChange::BOSS_END:
			break;
		case NetChange::KICK_NPC:
		case NetChange::REMOVE_UNIT:
		case NetChange::REMOVE_TRAP:
		case NetChange::TRIGGER_TRAP:
		case NetChange::CLEAN_LEVEL:
		case NetChange::REMOVE_ITEM:
		case NetChange::DESTROY_USABLE:
			f << c.id;
			break;
		case NetChange::TALK:
			f << c.unit->id;
			f << (byte)c.id;
			f << c.count;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(netStrs, c.str);
			break;
		case NetChange::TALK_POS:
			f << c.pos;
			f << *c.str;
			StringPool.Free(c.str);
			RemoveElement(netStrs, c.str);
			break;
		case NetChange::CHANGE_LOCATION_STATE:
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::ADD_RUMOR:
			f << gameGui->journal->GetRumors()[c.id];
			break;
		case NetChange::HAIR_COLOR:
			f << c.unit->id;
			f << c.unit->humanData->hairColor;
			break;
		case NetChange::WARP:
			f << c.unit->id;
			f.WriteCasted<char>(c.unit->locPart->partId);
			f << c.unit->pos;
			f << c.unit->rot;
			break;
		case NetChange::REGISTER_ITEM:
			{
				f << c.baseItem->id;
				f << c.item2->id;
				f << c.item2->name;
				f << c.item2->desc;
				f << c.item2->questId;
			}
			break;
		case NetChange::ADD_QUEST:
			{
				Quest* q = questMgr->FindQuest(c.id, false);
				f << q->id;
				f << q->name;
				f.WriteString2(q->msgs[0]);
				f.WriteString2(q->msgs[1]);
			}
			break;
		case NetChange::UPDATE_QUEST:
			{
				Quest* q = questMgr->FindQuest(c.id, false);
				f << q->id;
				f.WriteCasted<byte>(q->state);
				f.WriteCasted<byte>(c.count);
				for(int i = 0; i < c.count; ++i)
					f.WriteString2(q->msgs[q->msgs.size() - c.count + i]);
			}
			break;
		case NetChange::RENAME_ITEM:
			{
				const Item* item = c.baseItem;
				f << item->questId;
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
			f.WriteCasted<char>(c.unit->inArena);
			break;
		case NetChange::WORLD_TIME:
			world->WriteTime(f);
			break;
		case NetChange::USE_DOOR:
			f << c.id;
			f << (c.count != 0);
			break;
		case NetChange::CREATE_EXPLOSION:
			f << c.ability->hash;
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
		case NetChange::SPELL_SOUND:
			f << c.extraId;
			f << c.ability->hash;
			f << c.pos;
			break;
		case NetChange::CREATE_ELECTRO:
			f << c.extraId;
			f << c.pos;
			f << c.vec3;
			break;
		case NetChange::UPDATE_ELECTRO:
			f << c.extraId;
			f << c.pos;
			break;
		case NetChange::ELECTRO_HIT:
			f << c.pos;
			break;
		case NetChange::PARTICLE_EFFECT:
			f << c.ability->hash;
			f << c.pos;
			f << c.extraFloats[0];
			f << c.extraFloats[1];
			break;
		case NetChange::REVEAL_MINIMAP:
			f.WriteCasted<word>(gameLevel->minimapRevealMp.size());
			for(vector<Int2>::iterator it2 = gameLevel->minimapRevealMp.begin(), end2 = gameLevel->minimapRevealMp.end(); it2 != end2; ++it2)
			{
				f.WriteCasted<byte>(it2->x);
				f.WriteCasted<byte>(it2->y);
			}
			gameLevel->minimapRevealMp.clear();
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
			f << gameStats->totalKills;
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
		case NetChange::UPDATE_INVESTMENT:
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
			RemoveElement(netStrs, c.str);
			break;
		case NetChange::PLAYER_ABILITY:
			f << c.unit->id;
			f << c.ability->hash;
			f << c.extraFloat;
			break;
		case NetChange::CAST_SPELL:
			f << c.unit->id;
			f << c.ability->hash;
			break;
		case NetChange::REMOVE_BULLET:
			f << c.id;
			f << (c.extraId == 1);
			break;
		case NetChange::ADD_INVESTMENT:
			{
				const Team::Investment& investment = team->GetInvestments().back();
				f << investment.questId;
				f << investment.gold;
				f << investment.name;
			}
			break;
		case NetChange::ADD_UNIT_EFFECT:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			f << c.extraFloat;
			break;
		case NetChange::REMOVE_UNIT_EFFECT:
			f << c.unit->id;
			f.WriteCasted<byte>(c.id);
			break;
		case NetChange::CREATE_TRAP:
			f << c.extraId;
			f.WriteCasted<byte>(c.id);
			f << c.pos;
			break;
		case NetChange::SET_CAN_ENTER:
			f << c.id;
			f << (c.count == 1);
			break;
		case NetChange::HIT_OBJECT:
			f << c.id;
			f << c.pos;
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
	f.WriteCasted<byte>(info.updateFlags);

	// variables
	if(IsSet(info.updateFlags, PlayerInfo::UF_POISON_DAMAGE))
		f << player.lastDmgPoison;
	if(IsSet(info.updateFlags, PlayerInfo::UF_GOLD))
		f << info.u->gold;
	if(IsSet(info.updateFlags, PlayerInfo::UF_ALCOHOL))
		f << info.u->alcohol;
	if(IsSet(info.updateFlags, PlayerInfo::UF_LEARNING_POINTS))
		f << info.pc->learningPoints;
	if(IsSet(info.updateFlags, PlayerInfo::UF_LEVEL))
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
		case NetChangePlayer::LOOT:
			f << (c.id != 0);
			if(c.id != 0)
			{
				if(player.action == PlayerAction::LootUnit)
				{
					array<const Item*, SLOT_MAX>& equipped = player.actionUnit->GetEquippedItems();
					for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
					{
						if(equipped[i])
							f << equipped[i]->id;
						else
							f.Write0();
					}
				}
				f.WriteItemListTeam(*player.chestTrade);
			}
			break;
		case NetChangePlayer::START_SHARE:
		case NetChangePlayer::START_GIVE:
			{
				Unit& u = *player.actionUnit;
				f << u.weight;
				f << u.weightMax;
				f << u.gold;
				f << u.stats->subprofile;
				f << u.effects;
				array<const Item*, SLOT_MAX>& equipped = u.GetEquippedItems();
				for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
				{
					if(equipped[i])
						f << equipped[i]->id;
					else
						f.Write0();
				}
				f.WriteItemListTeam(u.items);
			}
			break;
		case NetChangePlayer::START_DIALOG:
			f << c.id;
			break;
		case NetChangePlayer::SHOW_DIALOG_CHOICES:
			{
				DialogContext& ctx = *player.dialogCtx;
				f.WriteCasted<byte>(ctx.choices.size());
				f.WriteCasted<char>(ctx.dialogEsc);
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
		case NetChangePlayer::AFTER_CRAFT:
		case NetChangePlayer::END_PREPARE:
		case NetChangePlayer::PICK_REST:
			break;
		case NetChangePlayer::START_TRADE:
			f << c.id;
			f.WriteItemList(*player.chestTrade);
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
					f << c.item->questId;
			}
			break;
		case NetChangePlayer::TRAIN:
			f.WriteCasted<byte>(c.id);
			f << c.count;
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
			{
				f << c.unit->id;
				array<const Item*, SLOT_MAX>& equipped = c.unit->GetEquippedItems();
				for(int i = SLOT_MAX_VISIBLE; i < SLOT_MAX; ++i)
				{
					if(equipped[i])
						f << equipped[i]->id;
					else
						f.Write0();
				}
				f.WriteItemListTeam(c.unit->items);
			}
			break;
		case NetChangePlayer::PLAYER_STATS:
			f.WriteCasted<byte>(c.id);
			if(IsSet(c.id, STAT_KILLS))
				f << player.kills;
			if(IsSet(c.id, STAT_DMG_DONE))
				f << player.dmgDone;
			if(IsSet(c.id, STAT_DMG_TAKEN))
				f << player.dmgTaken;
			if(IsSet(c.id, STAT_KNOCKS))
				f << player.knocks;
			if(IsSet(c.id, STAT_ARENA_FIGHTS))
				f << player.arenaFights;
			break;
		case NetChangePlayer::STAT_CHANGED:
			f.WriteCasted<byte>(c.id);
			f.WriteCasted<byte>(c.a);
			f << c.count;
			break;
		case NetChangePlayer::ADD_PERK:
		case NetChangePlayer::REMOVE_PERK:
			f << c.id;
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
		case NetChangePlayer::ADD_ABILITY:
		case NetChangePlayer::REMOVE_ABILITY:
			f << c.ability->hash;
			break;
		case NetChangePlayer::ADD_RECIPE:
			f << c.recipe->hash;
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
		// player id is ignored by server but whe can check it anyway
		assert(id == info.id);

		cstring str = Format("%s: %s", info.name.c_str(), text.c_str());
		gameGui->AddMsg(str);

		// send to other players
		if(activePlayers > 2)
			peer->Send(&f.GetBitStream(), MEDIUM_PRIORITY, RELIABLE, 0, info.adr, true);

		if(game->gameState == GS_LEVEL)
			gameGui->levelGui->AddSpeechBubble(info.u, text.c_str());
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
		if(id == team->myId)
		{
			// message to me
			cstring str = Format("%s@: %s", info.name.c_str(), text.c_str());
			gameGui->AddMsg(str);
		}
		else
		{
			// relay message to other player
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
		case NetChange::CUTSCENE_TEXT:
		case NetChange::CUTSCENE_IMAGE:
			if(IsServer() && c.str)
			{
				StringPool.Free(c.str);
				RemoveElement(netStrs, c.str);
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
	case NetChangePlayer::ADD_ABILITY:
	case NetChangePlayer::REMOVE_ABILITY:
	case NetChangePlayer::ADD_RECIPE:
		return false;
	default:
		return true;
	}
}

//=================================================================================================
void Net::WriteNetVars(BitStreamWriter& f)
{
	f << mpUseInterp;
	f << mpInterp;
	f << game->gameSpeed;
}

//=================================================================================================
void Net::WriteWorldData(BitStreamWriter& f)
{
	Info("Preparing world data.");

	f << ID_WORLD_DATA;

	// world
	world->Write(f);

	// quests
	questMgr->Write(f);

	// rumors
	f.WriteStringArray<byte, word>(gameGui->journal->GetRumors());

	// stats
	gameStats->Write(f);

	// mp vars
	WriteNetVars(f);
	if(!netStrs.empty())
		StringPool.Free(netStrs);

	// secret note text
	f << Quest_Secret::GetNote().desc;

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WritePlayerStartData(BitStreamWriter& f, PlayerInfo& info)
{
	f << ID_PLAYER_START_DATA;

	f << info.devmode;
	f << game->noai;
	info.u->player->WriteStart(f);
	f.WriteStringArray<word, word>(info.notes);
	team->WriteInvestments(f);

	f.WriteCasted<byte>(0xFF);
}

//=================================================================================================
void Net::WriteLevelData(BitStreamWriter& f, bool loadedResources)
{
	f << ID_LEVEL_DATA;
	f << mpLoad;
	f << loadedResources;

	// level
	gameLevel->Write(f);

	// items preload
	std::set<const Item*>& itemsLoad = game->itemsLoad;
	f << itemsLoad.size();
	for(const Item* item : itemsLoad)
	{
		f << item->id;
		if(item->IsQuest())
			f << item->questId;
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
	array<const Item*, SLOT_MAX>& equipped = unit.GetEquippedItems();
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(equipped[i])
			f << equipped[i]->id;
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
	for(Unit& otherUnit : team->members)
	{
		if(&otherUnit != &unit)
			f << otherUnit.id;
	}
	f.WriteCasted<byte>(team->leaderId);

	// multiplayer load data
	if(mpLoad)
	{
		f << unit.usedItemIsTeam;
		f << unit.raiseTimer;
		if(unit.action == A_ATTACK)
		{
			f << unit.act.attack.power;
			f << unit.act.attack.run;
		}
		else if(unit.action == A_CAST)
		{
			f << unit.act.cast.ability->hash;
			f << unit.act.cast.target;
		}
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
	if(activePlayers <= 1)
		return 0;
	uint ack = peer->Send(&f.GetBitStream(), priority, reliability, 0, masterServerAdr, true);
	return ack;
}

//=================================================================================================
void Net::Save(GameWriter& f)
{
	f << serverName;
	f << password;
	f << activePlayers;
	f << maxPlayers;
	f << lastId;
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
	f << mpUseInterp;
	f << mpInterp;
}

//=================================================================================================
void Net::Load(GameReader& f)
{
	f >> serverName;
	f >> password;
	f >> activePlayers;
	f >> maxPlayers;
	f >> lastId;
	uint count;
	f >> count;
	DeleteElements(oldPlayers);
	oldPlayers.ptrs.resize(count);
	for(uint i = 0; i < count; ++i)
	{
		oldPlayers.ptrs[i] = new PlayerInfo;
		oldPlayers.ptrs[i]->Load(f);
	}
	if(LOAD_VERSION < V_0_12)
		f.Skip(sizeof(int) * 7); // old netid_counters
	f >> mpUseInterp;
	f >> mpInterp;
}

//=================================================================================================
int Net::GetServerFlags()
{
	int flags = 0;
	if(!password.empty())
		flags |= SERVER_PASSWORD;
	if(mpLoad)
		flags |= SERVER_SAVED;
	return flags;
}

//=================================================================================================
int Net::GetNewPlayerId()
{
	while(true)
	{
		lastId = (lastId + 1) % 256;
		bool ok = true;
		for(PlayerInfo& info : players)
		{
			if(info.id == lastId)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return lastId;
	}
}

//=================================================================================================
PlayerInfo* Net::FindOldPlayer(cstring nick)
{
	assert(nick);

	for(PlayerInfo& info : oldPlayers)
	{
		if(info.name == nick)
			return &info;
	}

	return nullptr;
}

//=================================================================================================
void Net::DeleteOldPlayers()
{
	const bool inLevel = gameLevel->isOpen;
	for(PlayerInfo& info : oldPlayers)
	{
		if(!info.loaded && info.u)
		{
			if(inLevel)
				RemoveElement(info.u->locPart->units, info.u);
			if(info.u->cobj)
			{
				delete info.u->cobj->getCollisionShape();
				phyWorld->removeCollisionObject(info.u->cobj);
				delete info.u->cobj;
			}
			delete info.u;
		}
	}
	DeleteElements(oldPlayers);
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

	ServerPanel* serverPanel = gameGui->server;
	if(serverPanel->visible)
	{
		serverPanel->AddMsg(Format(game->txPlayerKicked, info.name.c_str()));
		Info("Player %s was kicked.", info.name.c_str());

		if(activePlayers > 2)
			serverPanel->AddLobbyUpdate(Int2(Lobby_KickPlayer, info.id));

		serverPanel->CheckReady();
		serverPanel->UpdateServerInfo();
	}
	else
	{
		info.left = PlayerInfo::LEFT_KICK;
		playersLeft = true;
	}
}

//=================================================================================================
void Net::OnChangeLevel(int level)
{
	BitStreamWriter f;
	f << ID_CHANGE_LEVEL;
	f << (byte)gameLevel->locationIndex;
	f << (byte)level;
	f << false;

	uint ack = SendAll(f, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT);
	for(PlayerInfo& info : players)
	{
		if(info.id == team->myId)
			info.state = PlayerInfo::IN_GAME;
		else
		{
			info.state = PlayerInfo::WAITING_FOR_RESPONSE;
			info.ack = ack;
			info.timer = CHANGE_LEVEL_TIMER;
		}
	}
}
