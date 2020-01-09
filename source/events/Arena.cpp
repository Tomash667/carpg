#include "Pch.h"
#include "GameCore.h"
#include "Arena.h"
#include "Team.h"
#include "Unit.h"
#include "AIController.h"
#include "Level.h"
#include "World.h"
#include "City.h"
#include "Language.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "QuestManager.h"
#include "Quest_Tournament.h"
#include "DialogBox.h"
#include "UnitGroup.h"
#include "SoundManager.h"
#include "PlayerInfo.h"
#include "GameResources.h"

//=================================================================================================
void Arena::Init()
{
	quest_mgr->RegisterSpecialHandler(this, "use_arena");
	quest_mgr->RegisterSpecialHandler(this, "arena_combat");
	quest_mgr->RegisterSpecialHandler(this, "pvp_gather");
	quest_mgr->RegisterSpecialHandler(this, "start_pvp");
	quest_mgr->RegisterSpecialHandler(this, "pvp");
	quest_mgr->RegisterSpecialIfHandler(this, "arena_can_combat");
	quest_mgr->RegisterSpecialIfHandler(this, "is_arena_free");
	quest_mgr->RegisterSpecialIfHandler(this, "have_player");
	quest_mgr->RegisterSpecialIfHandler(this, "waiting_for_pvp");
}

//=================================================================================================
void Arena::LoadLanguage()
{
	txPvp = Str("pvp");
	txPvpWith = Str("pvpWith");
	txPvpTooFar = Str("pvpTooFar");
	LoadArray(txArenaText, "arenaText");
	LoadArray(txArenaTextU, "arenaTextU");
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
	dialog_pvp = nullptr;
	fighter = nullptr;
	mode = NONE;
	free = true;
	pvp_response.ok = false;
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
		near_players.clear();
		for(Unit& unit : team->active_members)
		{
			if(unit.IsPlayer() && unit.player != ctx.pc && Vec3::Distance2d(unit.pos, game_level->city_ctx->arena_pos) < 5.f)
				near_players.push_back(&unit);
		}
		near_players_str.resize(near_players.size());
		for(uint i = 0, size = near_players.size(); i != size; ++i)
			near_players_str[i] = Format(txPvpWith, near_players[i]->player->name.c_str());
	}
	else if(strcmp(msg, "start_pvp") == 0)
	{
		// walka z towarzyszem
		StartPvp(ctx.pc, ctx.talker);
	}
	else if(strncmp(msg, "pvp/", 4) == 0)
	{
		int id = int(msg[4] - '1');
		Unit* u = near_players[id];
		if(Vec3::Distance2d(u->pos, game_level->city_ctx->arena_pos) > 5.f)
		{
			ctx.dialog_s_text = Format(txPvpTooFar, u->player->name.c_str());
			ctx.DialogTalk(ctx.dialog_s_text.c_str());
			++ctx.dialog_pos;
			return true;
		}
		else
		{
			if(u->player->is_local)
			{
				DialogInfo info;
				info.event = DialogEvent(this, &Arena::PvpEvent);
				info.name = "pvp";
				info.order = ORDER_TOP;
				info.parent = nullptr;
				info.pause = false;
				info.text = Format(txPvp, ctx.pc->name.c_str());
				info.type = DIALOG_YESNO;
				dialog_pvp = gui->ShowDialog(info);
				pvp_unit = near_players[id];
			}
			else
			{
				NetChangePlayer& c = Add1(near_players[id]->player->player_info->changes);
				c.type = NetChangePlayer::PVP;
				c.id = ctx.pc->id;
			}

			pvp_response.ok = true;
			pvp_response.from = ctx.pc->unit;
			pvp_response.to = u;
			pvp_response.timer = 0.f;
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
		return !world->IsSameWeek(game_level->city_ctx->arena_time);
	if(strcmp(msg, "is_arena_free") == 0)
		return free;
	else if(strncmp(msg, "have_player/", 12) == 0)
	{
		int id = int(msg[12] - '1');
		return id < (int)near_players.size();
	}
	else if(strcmp(msg, "waiting_for_pvp") == 0)
		return pvp_response.ok;
	assert(0);
	return false;
}

//=================================================================================================
void Arena::SpawnArenaViewers(int count)
{
	assert(InRange(count, 1, 9));

	vector<Mesh::Point*> points;
	UnitData& ud = *UnitData::Get("viewer");
	InsideBuilding* arena = game_level->GetArena();
	Mesh* mesh = arena->building->inside_mesh;

	for(vector<Mesh::Point>::iterator it = mesh->attach_points.begin(), end = mesh->attach_points.end(); it != end; ++it)
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
		Vec3 look_at(arena->offset.x, 0, arena->offset.y);
		Unit* u = game_level->SpawnUnitNearLocation(*arena, pos, ud, &look_at, -1, 2.f);
		if(u)
		{
			u->ai->loc_timer = Random(6.f, 12.f);
			u->temporary = true;
			viewers.push_back(u);
		}
		--count;
	}
}

//=================================================================================================
void Arena::Clean()
{
	InsideBuilding* arena = game_level->city_ctx->FindInsideBuilding(BuildingGroup::BG_ARENA);

	// wyrzuæ ludzi z areny
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.frozen = FROZEN::NO;
		u.in_arena = -1;
		u.area = game_level->local_area;
		u.busy = Unit::Busy_No;
		if(u.hp <= 0.f)
		{
			u.HealPoison();
			u.live_state = Unit::ALIVE;
		}
		if(u.IsAI())
			u.ai->Reset();
		game_level->WarpUnit(u, arena->outside_spawn);
		u.rot = arena->outside_rot;
	}
	RemoveArenaViewers();
	free = true;
	mode = NONE;
}

//=================================================================================================
void Arena::RemoveArenaViewers()
{
	UnitData* ud = UnitData::Get("viewer");
	LevelArea& area = *game_level->GetArena();

	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		if((*it)->data == ud)
			game_level->RemoveUnit(*it);
	}

	viewers.clear();
}

//=================================================================================================
void Arena::UpdatePvpRequest(float dt)
{
	if(game->paused)
		return;
	if(game->game_state == GS_LEVEL && Net::IsOnline() && pvp_response.ok)
	{
		pvp_response.timer += dt;
		if(pvp_response.timer >= 5.f)
		{
			pvp_response.ok = false;
			if(pvp_response.to == game->pc->unit)
			{
				dialog_pvp->CloseDialog();
				dialog_pvp = nullptr;
			}
			if(Net::IsServer())
			{
				if(pvp_response.from == game->pc->unit)
					game_gui->AddMsg(Format(game->txPvpRefuse, pvp_response.to->player->name.c_str()));
				else
				{
					NetChangePlayer& c = Add1(pvp_response.from->player->player_info->changes);
					c.type = NetChangePlayer::NO_PVP;
					c.id = pvp_response.to->player->id;
				}
			}
		}
	}
	else
		pvp_response.ok = false;
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
	game_level->city_ctx->arena_time = world->GetWorldtime();
	units.clear();

	// dodaj gracza na arenê
	if(ctx.is_local)
	{
		game->fallback_type = FALLBACK::ARENA;
		game->fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
		c.type = NetChangePlayer::ENTER_ARENA;
		ctx.pc->arena_fights++;
	}

	ctx.pc->unit->frozen = FROZEN::YES;
	ctx.pc->unit->in_arena = 0;
	units.push_back(ctx.pc->unit);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_ARENA_STATE;
		c.unit = ctx.pc->unit;
	}

	for(Unit& unit : team->members)
	{
		if(unit.frozen != FROZEN::NO || Vec3::Distance2d(unit.pos, game_level->city_ctx->arena_pos) > 5.f)
			continue;
		if(unit.IsPlayer())
		{
			unit.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true, true);

			unit.frozen = FROZEN::YES;
			unit.in_arena = 0;
			units.push_back(&unit);

			unit.player->arena_fights++;
			unit.player->stat_flags |= STAT_ARENA_FIGHTS;

			if(unit.player == game->pc)
			{
				game->fallback_type = FALLBACK::ARENA;
				game->fallback_t = -1.f;
			}
			else
			{
				NetChangePlayer& c = Add1(unit.player->player_info->changes);
				c.type = NetChangePlayer::ENTER_ARENA;
			}

			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_ARENA_STATE;
			c.unit = &unit;
		}
		else if(unit.IsHero() && unit.CanFollowWarp() && !unit.dont_attack)
		{
			unit.frozen = FROZEN::YES;
			unit.in_arena = 0;
			units.push_back(&unit);

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = &unit;
			}
		}
	}

	// select enemy list
	cstring list_id;
	switch(level)
	{
	default:
	case 1:
		list_id = "arena_easy";
		SpawnArenaViewers(1);
		break;
	case 2:
		list_id = "arena_medium";
		SpawnArenaViewers(3);
		break;
	case 3:
		list_id = "arena_hard";
		SpawnArenaViewers(5);
		break;
	}
	UnitGroup* group = UnitGroup::Get(list_id)->GetRandomGroup();

	// prepare list of units that can be spawned
	int lvl = level * 5 + Random(-1, +1) + 3;
	int min_level = Max(lvl - 5, lvl / 2);
	int max_level = lvl + 1;
	Pooled<TmpUnitGroup> part;
	part->Fill(group, min_level, max_level);

	// spawn enemies
	InsideBuilding* arena = game_level->GetArena();
	for(TmpUnitGroup::Spawn& spawn : part->Roll(lvl, units.size()))
	{
		Unit* u = game_level->SpawnUnitInsideRegion(*arena, arena->region2, *spawn.first, spawn.second);
		if(u)
		{
			u->rot = 0.f;
			u->in_arena = 1;
			u->frozen = FROZEN::YES;
			units.push_back(u);
		}
	}
}

//=================================================================================================
void Arena::HandlePvpResponse(PlayerInfo& info, bool accepted)
{
	if(pvp_response.ok && pvp_response.to == info.u)
	{
		if(accepted)
			StartPvp(pvp_response.from->player, pvp_response.to);
		else
		{
			if(pvp_response.from->player == game->pc)
				game_gui->AddMsg(Format(game->txPvpRefuse, info.name.c_str()));
			else
			{
				NetChangePlayer& c = Add1(pvp_response.from->player->player_info->changes);
				c.type = NetChangePlayer::NO_PVP;
				c.id = pvp_response.to->player->id;
			}
		}

		if(pvp_response.ok && pvp_response.to == game->pc->unit && dialog_pvp)
		{
			gui->CloseDialog(dialog_pvp);
			dialog_pvp = nullptr;
		}

		pvp_response.ok = false;
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
		game->fallback_type = FALLBACK::ARENA;
		game->fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::ENTER_ARENA;
	}

	// fallback postaci
	if(unit->IsPlayer())
	{
		if(unit->player == game->pc)
		{
			game->fallback_type = FALLBACK::ARENA;
			game->fallback_t = -1.f;
		}
		else
		{
			NetChangePlayer& c = Add1(unit->player->player_info->changes);
			c.type = NetChangePlayer::ENTER_ARENA;
		}
	}

	// dodaj do areny
	player->unit->frozen = FROZEN::YES;
	player->arena_fights++;
	player->stat_flags |= STAT_ARENA_FIGHTS;
	units.push_back(player->unit);
	unit->frozen = FROZEN::YES;
	units.push_back(unit);
	if(unit->IsPlayer())
	{
		unit->player->arena_fights++;
		unit->player->stat_flags |= STAT_ARENA_FIGHTS;
	}
	pvp_player = player;
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
		if((*it)->in_arena == 0)
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
					if((*it)->in_arena == 0)
						game_level->WarpUnit(*it, WARP_ARENA);
				}
			}
			else
			{
				for(auto unit : units)
					game_level->WarpUnit(unit, WARP_ARENA);

				if(!units.empty())
				{
					units[0]->in_arena = 0;
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = units[0];
					}
					if(units.size() >= 2)
					{
						units[1]->in_arena = 1;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = units[1];
						}
					}
				}
			}

			// reset cooldowns
			for(auto unit : units)
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
			if(game_level->GetArena() == game->pc->unit->area)
				sound_mgr->PlaySound2d(game_res->sArenaFight);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::ARENA_SOUND;
				c.id = 0;
			}
			state = IN_PROGRESS;
			for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
			{
				(*it)->frozen = FROZEN::NO;
				if((*it)->IsPlayer() && (*it)->player != game->pc)
				{
					NetChangePlayer& c = Add1((*it)->player->player_info->changes);
					c.type = NetChangePlayer::START_ARENA_COMBAT;
				}
			}
		}
	}
	else if(state == IN_PROGRESS)
	{
		// talking by observers
		for(vector<Unit*>::iterator it = viewers.begin(), end = viewers.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.ai->loc_timer -= dt;
			if(u.ai->loc_timer <= 0.f)
			{
				u.ai->loc_timer = Random(6.f, 12.f);

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
			++count[unit->in_arena];
			if(unit->live_state != Unit::DEAD)
				++alive[unit->in_arena];
		}

		if(alive[0] == 0 || alive[1] == 0)
		{
			state = WAITING_TO_END;
			timer = 0.f;
			bool victory_sound;
			if(alive[0] == 0)
			{
				result = 1;
				victory_sound = false;
			}
			else
			{
				result = 0;
				victory_sound = true;
			}
			if(mode != FIGHT)
			{
				if(count[0] == 0 || count[1] == 0)
					victory_sound = false; // someone quit
				else
					victory_sound = true;
			}

			if(game_level->GetArena() == game->pc->unit->area)
				sound_mgr->PlaySound2d(victory_sound ? game_res->sArenaWin : game_res->sArenaLost);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::ARENA_SOUND;
				c.id = victory_sound ? 1 : 2;
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
						game->fallback_type = FALLBACK::ARENA_EXIT;
						game->fallback_t = -1.f;
					}
					else
					{
						NetChangePlayer& c = Add1((*it)->player->player_info->changes);
						c.type = NetChangePlayer::EXIT_ARENA;
					}
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
					if(unit->in_arena != 0)
					{
						game_level->RemoveUnit(unit);
						continue;
					}

					unit->frozen = FROZEN::NO;
					unit->in_arena = -1;
					if(unit->hp <= 0.f)
					{
						unit->HealPoison();
						unit->live_state = Unit::ALIVE;
						unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
						unit->action = A_ANIMATION;
						unit->animation = ANI_PLAY;
						if(unit->IsAI())
							unit->ai->Reset();
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::STAND_UP;
							c.unit = unit;
						}
					}

					game_level->WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = unit;
					}
				}
			}
			else
			{
				for(Unit* unit : units)
				{
					unit->frozen = FROZEN::NO;
					unit->in_arena = -1;
					if(unit->hp <= 0.f)
					{
						unit->HealPoison();
						unit->live_state = Unit::ALIVE;
						unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
						unit->action = A_ANIMATION;
						unit->animation = ANI_PLAY;
						if(unit->IsAI())
							unit->ai->Reset();
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::STAND_UP;
							c.unit = unit;
						}
					}

					game_level->WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = unit;
					}
				}

				if(mode == PVP && fighter && fighter->IsHero())
				{
					fighter->hero->lost_pvp = (result == 0);
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
				quest_mgr->quest_tournament->FinishCombat();
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
			pvp_player->StartDialog(fighter, GameDialog::TryGet(IsSet(fighter->data->flags, F_CRAZY) ? "crazy_pvp" : "hero_pvp"));
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
	dialog_pvp = nullptr;
	if(!pvp_response.ok)
		return;

	if(Net::IsServer())
	{
		if(id == BUTTON_YES)
		{
			// zaakceptuj pvp
			StartPvp(pvp_response.from->player, pvp_response.to);
		}
		else
		{
			// nie akceptuj pvp
			NetChangePlayer& c = Add1(pvp_response.from->player->player_info->changes);
			c.type = NetChangePlayer::NO_PVP;
			c.id = pvp_response.to->player->id;
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PVP;
		c.unit = pvp_unit;
		if(id == BUTTON_YES)
			c.id = 1;
		else
			c.id = 0;
	}

	pvp_response.ok = false;
}

//=================================================================================================
void Arena::ClosePvpDialog()
{
	if(pvp_response.ok && pvp_response.to == game->pc->unit)
	{
		if(dialog_pvp)
		{
			gui->CloseDialog(dialog_pvp);
			dialog_pvp = nullptr;
		}
		pvp_response.ok = false;
	}
}

//=================================================================================================
void Arena::ShowPvpRequest(Unit* unit)
{
	pvp_unit = unit;

	DialogInfo info;
	info.event = DialogEvent(this, &Arena::PvpEvent);
	info.name = "pvp";
	info.order = ORDER_TOP;
	info.parent = nullptr;
	info.pause = false;
	info.text = Format(txPvp, unit->player->name.c_str());
	info.type = DIALOG_YESNO;
	dialog_pvp = gui->ShowDialog(info);

	pvp_response.ok = true;
	pvp_response.timer = 0.f;
	pvp_response.to = game->pc->unit;
}

//=================================================================================================
void Arena::RewardExp(Unit* dead_unit)
{
	if(mode == PVP)
	{
		Unit* winner = units[dead_unit->in_arena == 0 ? 1 : 0];
		if(winner->IsPlayer())
		{
			int exp = 50 * dead_unit->level;
			if(dead_unit->IsPlayer())
				exp /= 2;
			winner->player->AddExp(exp);
		}
	}
	else if(!dead_unit->IsTeamMember())
	{
		rvector<Unit> to_reward;
		for(Unit* unit : units)
		{
			if(unit->in_arena != dead_unit->in_arena)
				to_reward.push_back(unit);
		}
		team->AddExp(50 * dead_unit->level, &to_reward);
	}
}

//=================================================================================================
void Arena::SpawnUnit(const vector<Enemy>& units)
{
	InsideBuilding* arena = game_level->GetArena();

	game_level->CleanLevel(arena->area_id);

	for(const Enemy& unit : units)
	{
		for(uint i = 0; i < unit.count; ++i)
		{
			if(unit.side)
			{
				Unit* u = game_level->SpawnUnitInsideRegion(*arena, arena->region2, *unit.unit, unit.level);
				if(u)
				{
					u->rot = 0.f;
					u->in_arena = 1;
				}
			}
			else
			{
				Unit* u = game_level->SpawnUnitInsideRegion(*arena, arena->region1, *unit.unit, unit.level);
				if(u)
				{
					u->rot = PI;
					u->in_arena = 0;
				}
			}
		}
	}
}
