#include "Pch.h"
#include "Arena.h"

#include "AIController.h"
#include "City.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "Language.h"
#include "Level.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"
#include "Team.h"
#include "Unit.h"
#include "UnitGroup.h"
#include "World.h"

#include <DialogBox.h>
#include <SoundManager.h>

//=================================================================================================
void Arena::Init()
{
	questMgr->RegisterSpecialHandler(this, "use_arena");
	questMgr->RegisterSpecialHandler(this, "arena_combat");
	questMgr->RegisterSpecialHandler(this, "pvp_gather");
	questMgr->RegisterSpecialHandler(this, "start_pvp");
	questMgr->RegisterSpecialHandler(this, "pvp");
	questMgr->RegisterSpecialIfHandler(this, "arena_can_combat");
	questMgr->RegisterSpecialIfHandler(this, "is_arena_free");
	questMgr->RegisterSpecialIfHandler(this, "have_player");
	questMgr->RegisterSpecialIfHandler(this, "waiting_for_pvp");
}

//=================================================================================================
void Arena::LoadLanguage()
{
	txPvp = Str("pvp");
	txPvpWith = Str("pvpWith");
	txPvpTooFar = Str("pvpTooFar");
	StrArray(txArenaText, "arenaText");
	StrArray(txArenaTextU, "arenaTextU");
}

//=================================================================================================
void Arena::Start(Mode mode)
{
	this->mode = mode;
	free = false;
	state = WAITING_TO_WARP;
	timer = 0.f;
	units.clear();
}

//=================================================================================================
void Arena::Reset()
{
	viewers.clear();
	dialogPvp = nullptr;
	fighter = nullptr;
	mode = NONE;
	free = true;
	pvpResponse.ok = false;
}

//=================================================================================================
bool Arena::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "use_arena") == 0)
		free = false;
	else if(strncmp(msg, "arena_combat/", 13) == 0)
		StartArenaCombat(msg[13] - '0');
	else if(strcmp(msg, "pvp_gather") == 0)
	{
		nearPlayers.clear();
		for(Unit& unit : team->activeMembers)
		{
			if(unit.IsPlayer() && unit.player != ctx.pc && Vec3::Distance2d(unit.pos, gameLevel->cityCtx->arenaPos) < 5.f)
				nearPlayers.push_back(&unit);
		}
		nearPlayersStr.resize(nearPlayers.size());
		for(uint i = 0, size = nearPlayers.size(); i != size; ++i)
			nearPlayersStr[i] = Format(txPvpWith, nearPlayers[i]->player->name.c_str());
	}
	else if(strcmp(msg, "start_pvp") == 0)
	{
		// walka z towarzyszem
		StartPvp(ctx.pc, ctx.talker);
	}
	else if(strncmp(msg, "pvp/", 4) == 0)
	{
		int id = int(msg[4] - '1');
		Unit* u = nearPlayers[id];
		if(Vec3::Distance2d(u->pos, gameLevel->cityCtx->arenaPos) > 5.f)
		{
			ctx.dialogString = Format(txPvpTooFar, u->player->name.c_str());
			ctx.Talk(ctx.dialogString.c_str());
			++ctx.dialogPos;
			return true;
		}
		else
		{
			if(u->player->isLocal)
			{
				DialogInfo info;
				info.event = DialogEvent(this, &Arena::PvpEvent);
				info.name = "pvp";
				info.order = DialogOrder::Top;
				info.parent = nullptr;
				info.pause = false;
				info.text = Format(txPvp, ctx.pc->name.c_str());
				info.type = DIALOG_YESNO;
				dialogPvp = gui->ShowDialog(info);
				pvpUnit = nearPlayers[id];
			}
			else
			{
				NetChangePlayer& c = nearPlayers[id]->player->playerInfo->PushChange(NetChangePlayer::PVP);
				c.id = ctx.pc->id;
			}

			pvpResponse.ok = true;
			pvpResponse.from = ctx.pc->unit;
			pvpResponse.to = u;
			pvpResponse.timer = 0.f;
		}
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Arena::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "arena_can_combat") == 0)
		return !world->IsSameWeek(gameLevel->cityCtx->arenaTime);
	if(strcmp(msg, "is_arena_free") == 0)
		return free;
	else if(strncmp(msg, "have_player/", 12) == 0)
	{
		int id = int(msg[12] - '1');
		return id < (int)nearPlayers.size();
	}
	else if(strcmp(msg, "waiting_for_pvp") == 0)
		return pvpResponse.ok;
	assert(0);
	return false;
}

//=================================================================================================
void Arena::SpawnArenaViewers(int count)
{
	assert(InRange(count, 1, 9));

	vector<Mesh::Point*> points;
	UnitData& ud = *UnitData::Get("viewer");
	InsideBuilding* arena = gameLevel->GetArena();
	Mesh* mesh = arena->building->insideMesh;

	for(vector<Mesh::Point>::iterator it = mesh->attachPoints.begin(), end = mesh->attachPoints.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "o_s_viewer_", 11) == 0)
			points.push_back(&*it);
	}

	while(count > 0)
	{
		int id = Rand() % points.size();
		Mesh::Point* pt = points[id];
		points.erase(points.begin() + id);
		Vec3 pos(pt->mat._41 + arena->offset.x, pt->mat._42, pt->mat._43 + arena->offset.y);
		Vec3 lookAt(arena->offset.x, 0, arena->offset.y);
		Unit* u = gameLevel->SpawnUnitNearLocation(*arena, pos, ud, &lookAt, -1, 2.f);
		if(u)
		{
			u->ai->locTimer = Random(6.f, 12.f);
			u->temporary = true;
			viewers.push_back(u);
		}
		--count;
	}
}

//=================================================================================================
void Arena::Clean()
{
	InsideBuilding* arena = gameLevel->cityCtx->FindInsideBuilding(BuildingGroup::BG_ARENA);

	// wyrzuæ ludzi z areny
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.frozen = FROZEN::NO;
		u.inArena = -1;
		u.locPart = gameLevel->localPart;
		u.busy = Unit::Busy_No;
		if(u.hp <= 0.f)
			u.Standup(false, true);
		gameLevel->WarpUnit(u, arena->outsideSpawn);
		u.rot = arena->outsideRot;
	}
	RemoveArenaViewers();
	free = true;
	mode = NONE;
}

//=================================================================================================
void Arena::RemoveArenaViewers()
{
	UnitData* ud = UnitData::Get("viewer");
	LocationPart& locPart = *gameLevel->GetArena();

	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		if((*it)->data == ud)
			gameLevel->RemoveUnit(*it);
	}

	viewers.clear();
}

//=================================================================================================
void Arena::UpdatePvpRequest(float dt)
{
	if(game->paused)
		return;
	if(game->gameState == GS_LEVEL && Net::IsOnline() && pvpResponse.ok)
	{
		pvpResponse.timer += dt;
		if(pvpResponse.timer >= 5.f)
		{
			pvpResponse.ok = false;
			if(pvpResponse.to == game->pc->unit)
			{
				dialogPvp->CloseDialog();
				dialogPvp = nullptr;
			}
			if(Net::IsServer())
			{
				if(pvpResponse.from == game->pc->unit)
					gameGui->AddMsg(Format(game->txPvpRefuse, pvpResponse.to->player->name.c_str()));
				else
				{
					NetChangePlayer& c = pvpResponse.from->player->playerInfo->PushChange(NetChangePlayer::NO_PVP);
					c.id = pvpResponse.to->player->id;
				}
			}
		}
	}
	else
		pvpResponse.ok = false;
}

//=================================================================================================
void Arena::StartArenaCombat(int level)
{
	assert(InRange(level, 1, 3));

	DialogContext& ctx = *DialogContext::current;

	free = false;
	mode = FIGHT;
	state = WAITING_TO_WARP;
	timer = 0.f;
	difficulty = level;
	gameLevel->cityCtx->arenaTime = world->GetWorldtime();
	units.clear();

	// dodaj gracza na arenê
	if(ctx.isLocal)
	{
		game->fallbackType = FALLBACK::ARENA;
		game->fallbackTimer = -1.f;
	}
	else
	{
		ctx.pc->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);
		ctx.pc->arenaFights++;
	}

	ctx.pc->unit->frozen = FROZEN::YES;
	ctx.pc->unit->inArena = 0;
	units.push_back(ctx.pc->unit);

	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
		c.unit = ctx.pc->unit;
	}

	for(Unit& unit : team->members)
	{
		if(unit.frozen != FROZEN::NO || Vec3::Distance2d(unit.pos, gameLevel->cityCtx->arenaPos) > 5.f)
			continue;
		if(unit.IsPlayer())
		{
			unit.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true, true);

			unit.frozen = FROZEN::YES;
			unit.inArena = 0;
			units.push_back(&unit);

			unit.player->arenaFights++;
			unit.player->statFlags |= STAT_ARENA_FIGHTS;

			if(unit.player == game->pc)
			{
				game->fallbackType = FALLBACK::ARENA;
				game->fallbackTimer = -1.f;
			}
			else
				unit.player->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);

			NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
			c.unit = &unit;
		}
		else if(unit.IsHero() && unit.CanFollowWarp() && !unit.dontAttack)
		{
			unit.frozen = FROZEN::YES;
			unit.inArena = 0;
			units.push_back(&unit);

			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
				c.unit = &unit;
			}
		}
	}

	// select enemy list
	cstring listId;
	switch(level)
	{
	default:
	case 1:
		listId = "arena_easy";
		SpawnArenaViewers(1);
		break;
	case 2:
		listId = "arena_medium";
		SpawnArenaViewers(3);
		break;
	case 3:
		listId = "arena_hard";
		SpawnArenaViewers(5);
		break;
	}
	UnitGroup* group = UnitGroup::Get(listId)->GetRandomGroup();

	// prepare list of units that can be spawned
	int lvl = level * 5 + Random(-1, +1) + 3;
	int minLevel = Max(lvl - 5, lvl / 2);
	int maxLevel = lvl + 1;
	Pooled<TmpUnitGroup> part;
	part->Fill(group, minLevel, maxLevel);

	// spawn enemies
	InsideBuilding* arena = gameLevel->GetArena();
	for(TmpUnitGroup::Spawn& spawn : part->Roll(lvl, units.size()))
	{
		Unit* u = gameLevel->SpawnUnitInsideRegion(*arena, arena->region2, *spawn.first, spawn.second);
		if(u)
		{
			u->rot = 0.f;
			u->inArena = 1;
			u->frozen = FROZEN::YES;
			units.push_back(u);
		}
	}
}

//=================================================================================================
void Arena::HandlePvpResponse(PlayerInfo& info, bool accepted)
{
	if(pvpResponse.ok && pvpResponse.to == info.u)
	{
		if(accepted)
			StartPvp(pvpResponse.from->player, pvpResponse.to);
		else
		{
			if(pvpResponse.from->player == game->pc)
				gameGui->AddMsg(Format(game->txPvpRefuse, info.name.c_str()));
			else
			{
				NetChangePlayer& c = pvpResponse.from->player->playerInfo->PushChange(NetChangePlayer::NO_PVP);
				c.id = pvpResponse.to->player->id;
			}
		}

		if(pvpResponse.ok && pvpResponse.to == game->pc->unit && dialogPvp)
		{
			gui->CloseDialog(dialogPvp);
			dialogPvp = nullptr;
		}

		pvpResponse.ok = false;
	}
}

//=================================================================================================
void Arena::StartPvp(PlayerController* player, Unit* unit)
{
	free = false;
	mode = PVP;
	state = WAITING_TO_WARP;
	timer = 0.f;
	units.clear();

	// fallback gracza
	if(player == game->pc)
	{
		game->fallbackType = FALLBACK::ARENA;
		game->fallbackTimer = -1.f;
	}
	else
		player->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);

	// fallback postaci
	if(unit->IsPlayer())
	{
		if(unit->player == game->pc)
		{
			game->fallbackType = FALLBACK::ARENA;
			game->fallbackTimer = -1.f;
		}
		else
			unit->player->playerInfo->PushChange(NetChangePlayer::ENTER_ARENA);
	}

	// dodaj do areny
	player->unit->frozen = FROZEN::YES;
	player->arenaFights++;
	player->statFlags |= STAT_ARENA_FIGHTS;
	units.push_back(player->unit);
	unit->frozen = FROZEN::YES;
	units.push_back(unit);
	if(unit->IsPlayer())
	{
		unit->player->arenaFights++;
		unit->player->statFlags |= STAT_ARENA_FIGHTS;
	}
	pvpPlayer = player;
	fighter = unit;

	// stwórz obserwatorów na arenie na podstawie poziomu postaci
	int level = max(player->unit->level, unit->level);

	if(level < 10)
		SpawnArenaViewers(1);
	else if(level < 15)
		SpawnArenaViewers(2);
	else
		SpawnArenaViewers(3);
}

//=================================================================================================
void Arena::AddReward(int gold, int exp)
{
	rvector<Unit> v;
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->inArena == 0)
			v.push_back(*it);
	}

	team->AddGold(gold * v.size(), &v, true);
	team->AddExp(-exp, &v);
}

//=================================================================================================
void Arena::Update(float dt)
{
	if(state == WAITING_TO_WARP)
	{
		timer += dt * 2;
		if(timer >= 1.f)
		{
			if(mode == FIGHT)
			{
				for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
				{
					if((*it)->inArena == 0)
						gameLevel->WarpUnit(*it, WARP_ARENA);
				}
			}
			else
			{
				for(Unit* unit : units)
					gameLevel->WarpUnit(unit, WARP_ARENA);

				if(!units.empty())
				{
					units[0]->inArena = 0;
					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
						c.unit = units[0];
					}
					if(units.size() >= 2)
					{
						units[1]->inArena = 1;
						if(Net::IsOnline())
						{
							NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
							c.unit = units[1];
						}
					}
				}
			}

			// reset cooldowns
			for(Unit* unit : units)
			{
				if(unit->IsPlayer())
					unit->player->RefreshCooldown();
			}

			state = WAITING_TO_START;
			timer = 0.f;
		}
	}
	else if(state == WAITING_TO_START)
	{
		timer += dt;
		if(timer >= 2.f)
		{
			if(gameLevel->GetArena() == game->pc->unit->locPart)
				soundMgr->PlaySound2d(gameRes->sArenaFight);
			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::ARENA_SOUND);
				c.id = 0;
			}
			state = IN_PROGRESS;
			for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
			{
				(*it)->frozen = FROZEN::NO;
				if((*it)->IsPlayer() && (*it)->player != game->pc)
					(*it)->player->playerInfo->PushChange(NetChangePlayer::START_ARENA_COMBAT);
			}
		}
	}
	else if(state == IN_PROGRESS)
	{
		// talking by observers
		for(vector<Unit*>::iterator it = viewers.begin(), end = viewers.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.ai->locTimer -= dt;
			if(u.ai->locTimer <= 0.f)
			{
				u.ai->locTimer = Random(6.f, 12.f);

				cstring text;
				if(Rand() % 2 == 0)
					text = RandomString(txArenaText);
				else
					text = Format(RandomString(txArenaTextU), GetRandomArenaHero()->GetRealName());

				u.Talk(text);
			}
		}

		// count how many are still alive
		int count[2] = { 0 }, alive[2] = { 0 };
		for(Unit* unit : units)
		{
			++count[unit->inArena];
			if(unit->liveState != Unit::DEAD)
				++alive[unit->inArena];
		}

		if(alive[0] == 0 || alive[1] == 0)
		{
			state = WAITING_TO_END;
			timer = 0.f;
			bool victorySound;
			if(alive[0] == 0)
			{
				result = 1;
				victorySound = false;
			}
			else
			{
				result = 0;
				victorySound = true;
			}
			if(mode != FIGHT)
			{
				if(count[0] == 0 || count[1] == 0)
					victorySound = false; // someone quit
				else
					victorySound = true;
			}

			if(gameLevel->GetArena() == game->pc->unit->locPart)
				soundMgr->PlaySound2d(victorySound ? gameRes->sArenaWin : gameRes->sArenaLost);
			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::ARENA_SOUND);
				c.id = victorySound ? 1 : 2;
			}
		}
	}
	else if(state == WAITING_TO_END)
	{
		timer += dt;
		if(timer >= 2.f)
		{
			for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
			{
				(*it)->frozen = FROZEN::YES;
				if((*it)->IsPlayer())
				{
					if((*it)->player == game->pc)
					{
						game->fallbackType = FALLBACK::ARENA_EXIT;
						game->fallbackTimer = -1.f;
					}
					else
						(*it)->player->playerInfo->PushChange(NetChangePlayer::EXIT_ARENA);
				}
			}

			state = WAITING_TO_EXIT;
			timer = 0.f;
		}
	}
	else if(state == WAITING_TO_EXIT)
	{
		timer += dt * 2;
		if(timer >= 1.f)
		{
			if(mode == FIGHT)
			{
				if(result == 0)
				{
					int gold, exp;
					switch(difficulty)
					{
					default:
					case 1:
						gold = 500;
						exp = 1000;
						break;
					case 2:
						gold = 1000;
						exp = 2000;
						break;
					case 3:
						gold = 2500;
						exp = 5000;
						break;
					}

					AddReward(gold, exp);
				}

				for(Unit* unit : units)
				{
					if(unit->inArena != 0)
					{
						gameLevel->RemoveUnit(unit);
						continue;
					}

					unit->frozen = FROZEN::NO;
					unit->inArena = -1;
					if(unit->hp <= 0.f)
						unit->Standup(false);

					gameLevel->WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
						c.unit = unit;
					}
				}
			}
			else
			{
				for(Unit* unit : units)
				{
					unit->frozen = FROZEN::NO;
					unit->inArena = -1;
					if(unit->hp <= 0.f)
						unit->Standup(false);

					gameLevel->WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Net::PushChange(NetChange::CHANGE_ARENA_STATE);
						c.unit = unit;
					}
				}

				if(mode == PVP && fighter && fighter->IsHero())
				{
					fighter->hero->lostPvp = (result == 0);
					state = WAITING_TO_EXIT_TALK;
					timer = 0.f;
				}
			}

			if(mode != TOURNAMENT)
			{
				RemoveArenaViewers();
				units.clear();
			}
			else
				questMgr->questTournament->FinishCombat();
			if(state != WAITING_TO_EXIT_TALK)
			{
				mode = NONE;
				free = true;
			}
		}
	}
	else
	{
		timer += dt;
		if(timer >= 0.5f)
		{
			pvpPlayer->StartDialog(fighter, GameDialog::TryGet(IsSet(fighter->data->flags, F_CRAZY) ? "crazy_pvp" : "hero_pvp"));
			mode = NONE;
			free = true;
		}
	}
}

//=================================================================================================
Unit* Arena::GetRandomArenaHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || (*it)->IsHero())
			v->push_back(*it);
	}

	return v->at(Rand() % v->size());
}

//=================================================================================================
void Arena::PvpEvent(int id)
{
	dialogPvp = nullptr;
	if(!pvpResponse.ok)
		return;

	if(Net::IsServer())
	{
		if(id == BUTTON_YES)
		{
			// zaakceptuj pvp
			StartPvp(pvpResponse.from->player, pvpResponse.to);
		}
		else
		{
			// nie akceptuj pvp
			NetChangePlayer& c = pvpResponse.from->player->playerInfo->PushChange(NetChangePlayer::NO_PVP);
			c.id = pvpResponse.to->player->id;
		}
	}
	else
	{
		NetChange& c = Net::PushChange(NetChange::PVP);
		c.unit = pvpUnit;
		if(id == BUTTON_YES)
			c.id = 1;
		else
			c.id = 0;
	}

	pvpResponse.ok = false;
}

//=================================================================================================
void Arena::ClosePvpDialog()
{
	if(pvpResponse.ok && pvpResponse.to == game->pc->unit)
	{
		if(dialogPvp)
		{
			gui->CloseDialog(dialogPvp);
			dialogPvp = nullptr;
		}
		pvpResponse.ok = false;
	}
}

//=================================================================================================
void Arena::ShowPvpRequest(Unit* unit)
{
	pvpUnit = unit;

	DialogInfo info;
	info.event = DialogEvent(this, &Arena::PvpEvent);
	info.name = "pvp";
	info.order = DialogOrder::Top;
	info.parent = nullptr;
	info.pause = false;
	info.text = Format(txPvp, unit->player->name.c_str());
	info.type = DIALOG_YESNO;
	dialogPvp = gui->ShowDialog(info);

	pvpResponse.ok = true;
	pvpResponse.timer = 0.f;
	pvpResponse.to = game->pc->unit;
}

//=================================================================================================
void Arena::RewardExp(Unit* deadUnit)
{
	if(mode == PVP)
	{
		Unit* winner = units[deadUnit->inArena == 0 ? 1 : 0];
		if(winner->IsPlayer())
		{
			int exp = 50 * deadUnit->level;
			if(deadUnit->IsPlayer())
				exp /= 2;
			winner->player->AddExp(exp);
		}
	}
	else if(!deadUnit->IsTeamMember())
	{
		rvector<Unit> toReward;
		for(Unit* unit : units)
		{
			if(unit->inArena != deadUnit->inArena)
				toReward.push_back(unit);
		}
		team->AddExp(50 * deadUnit->level, &toReward);
	}
}

//=================================================================================================
void Arena::SpawnUnit(const vector<Enemy>& units)
{
	InsideBuilding* arena = gameLevel->GetArena();

	gameLevel->CleanLevel(arena->partId);

	for(const Enemy& unit : units)
	{
		for(uint i = 0; i < unit.count; ++i)
		{
			if(unit.side)
			{
				Unit* u = gameLevel->SpawnUnitInsideRegion(*arena, arena->region2, *unit.unit, unit.level);
				if(u)
				{
					u->rot = 0.f;
					u->inArena = 1;
				}
			}
			else
			{
				Unit* u = gameLevel->SpawnUnitInsideRegion(*arena, arena->region1, *unit.unit, unit.level);
				if(u)
				{
					u->rot = PI;
					u->inArena = 0;
				}
			}
		}
	}
}
