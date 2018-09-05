#include "Pch.h"
#include "GameCore.h"
#include "Quest_Tournament.h"
#include "GameFile.h"
#include "World.h"
#include "Dialog.h"
#include "QuestManager.h"
#include "Game.h"

void Quest_Tournament::InitOnce()
{
	QM.RegisterSpecialHandler(this, "ironfist_start");
	QM.RegisterSpecialHandler(this, "ironfist_join");
	QM.RegisterSpecialHandler(this, "ironfist_train");
	QM.RegisterSpecialIfHandler(this, "ironfist_can_start");
	QM.RegisterSpecialIfHandler(this, "ironfist_done");
	QM.RegisterSpecialIfHandler(this, "ironfist_here");
	QM.RegisterSpecialIfHandler(this, "ironfist_preparing");
	QM.RegisterSpecialIfHandler(this, "ironfist_started");
	QM.RegisterSpecialIfHandler(this, "ironfist_joined");
	QM.RegisterSpecialIfHandler(this, "ironfist_winner");
}

void Quest_Tournament::Init()
{
	year = 0;
	city_year = W.GetYear();
	city = W.GetRandomCityIndex();
	state = TOURNAMENT_NOT_DONE;
	units.clear();
	winner = nullptr;
	generated = false;
}

void Quest_Tournament::Save(GameWriter& f)
{
	f << year;
	f << city;
	f << city_year;
	f << winner;
	f << generated;
}

void Quest_Tournament::Load(GameReader& f)
{
	f >> year;
	f >> city;
	f >> city_year;
	f >> winner;
	f >> generated;
	state = TOURNAMENT_NOT_DONE;
	units.clear();
}

void Quest_Tournament::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "ironfist_start") == 0)
	{
		Game::Get().StartTournament(ctx.talker);
		units.push_back(ctx.pc->unit);
		ctx.pc->unit->ModGold(-100);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "ironfist_join") == 0)
	{
		units.push_back(ctx.pc->unit);
		ctx.pc->unit->ModGold(-100);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "ironfist_train") == 0)
	{
		winner = nullptr;
		ctx.pc->unit->frozen = FROZEN::YES;
		if(ctx.is_local)
		{
			// local fallback
			Game& game = Game::Get();
			game.fallback_type = FALLBACK::TRAIN;
			game.fallback_t = -1.f;
			game.fallback_1 = 2;
			game.fallback_2 = 0;
		}
		else
		{
			// send info about training
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = 2;
			c.ile = 0;
		}
	}
}

bool Quest_Tournament::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "ironfist_can_start") == 0)
	{
		return state == TOURNAMENT_NOT_DONE
			&& city == W.GetCurrentLocationIndex()
			&& W.GetDay() == 6
			&& W.GetMonth() == 2
			&& year != W.GetYear();
	}
	else if(strcmp(msg, "ironfist_done") == 0)
		return year == W.GetYear();
	else if(strcmp(msg, "ironfist_here") == 0)
		return city == W.GetCurrentLocationIndex();
	else if(strcmp(msg, "ironfist_preparing") == 0)
		return state == TOURNAMENT_STARTING;
	else if(strcmp(msg, "ironfist_started") == 0)
	{
		if(state == TOURNAMENT_IN_PROGRESS)
		{
			// winner can stop dialog and talk
			if(winner == ctx.pc->unit && state2 == 2 && state3 == 1)
			{
				master->look_target = nullptr;
				state = TOURNAMENT_NOT_DONE;
			}
			else
				return true;
		}
	}
	else if(strcmp(msg, "ironfist_joined") == 0)
	{
		for(Unit* u : units)
		{
			if(ctx.pc->unit == u)
				return true;
		}
	}
	else if(strcmp(msg, "ironfist_winner") == 0)
		return winner == ctx.pc->unit;
	return false;
}
