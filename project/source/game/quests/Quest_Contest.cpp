#include "Pch.h"
#include "GameCore.h"
#include "Quest_Contest.h"
#include "World.h"
#include "PlayerController.h"
#include "GameFile.h"
#include "QuestManager.h"
#include "Dialog.h"
#include "Game.h"

//=================================================================================================
void Quest_Contest::InitOnce()
{
	QM.RegisterSpecialHandler(this, "contest_start");
	QM.RegisterSpecialHandler(this, "contest_join");
	QM.RegisterSpecialHandler(this, "contest_reward");
	QM.RegisterSpecialIfHandler(this, "contest_done");
	QM.RegisterSpecialIfHandler(this, "contest_here");
	QM.RegisterSpecialIfHandler(this, "contest_today");
	QM.RegisterSpecialIfHandler(this, "contest_in_progress");
	QM.RegisterSpecialIfHandler(this, "contest_started");
	QM.RegisterSpecialIfHandler(this, "contest_joined");
	QM.RegisterSpecialIfHandler(this, "contest_winner");
	QM.RegisterFormatString(this, "contest_loc");
}

//=================================================================================================
void Quest_Contest::Init()
{
	state = CONTEST_NOT_DONE;
	where = W.GetRandomSettlementIndex();
	units.clear();
	winner = nullptr;
	generated = false;
	year = W.GetYear();
}

//=================================================================================================
void Quest_Contest::Save(GameWriter& f)
{
	f << where;
	f << state;
	f << generated;
	f << winner;
	if(state >= CONTEST_STARTING)
	{
		f << state2;
		f << time;
		f << units.size();
		for(Unit* unit : units)
			f << unit->refid;
	}
}

//=================================================================================================
void Quest_Contest::Load(GameReader& f)
{
	f >> where;
	f >> state;
	f >> generated;
	f >> winner;
	if(state >= CONTEST_STARTING)
	{
		f >> state2;
		f >> time;
		units.resize(f.Read<uint>());
		for(Unit*& unit : units)
			f >> unit;
	}
	year = W.GetYear();
}

//=================================================================================================
void Quest_Contest::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "contest_start") == 0)
	{
		state = CONTEST_STARTING;
		time = 0;
		units.clear();
		units.push_back(ctx.pc->unit);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "contest_join") == 0)
	{
		units.push_back(ctx.pc->unit);
		ctx.pc->leaving_event = false;
	}
	else if(strcmp(msg, "contest_reward") == 0)
	{
		winner = nullptr;
		Game& game = Game::Get();
		game.AddItem(*ctx.pc->unit, ItemList::GetItem("contest_reward"), 1, false);
		if(ctx.is_local)
			game.AddGameMsg3(GMS_ADDED_ITEM);
		else
			game.Net_AddedItemMsg(ctx.pc);
	}
}

//=================================================================================================
bool Quest_Contest::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "contest_done") == 0)
		return state == CONTEST_DONE;
	else if(strcmp(msg, "contest_here") == 0)
		return where == W.GetCurrentLocationIndex();
	else if(strcmp(msg, "contest_today") == 0)
		return state == Quest_Contest::CONTEST_TODAY;
	else if(strcmp(msg, "chlanie_trwa") == 0)
		return state == Quest_Contest::CONTEST_IN_PROGRESS;
	else if(strcmp(msg, "contest_started") == 0)
		return state == Quest_Contest::CONTEST_STARTING;
	else if(strcmp(msg, "contest_joined") == 0)
	{
		for(Unit* u : units)
		{
			if(ctx.pc->unit == u)
				return true;
		}
		return false;
	}
	else if(strcmp(msg, "contest_winner") == 0)
		return winner == ctx.pc->unit;
	return false;
}

//=================================================================================================
cstring Quest_Contest::FormatString(const string& str)
{
	if(str == "contest_loc")
		return W.GetLocation(where)->name.c_str();
	return nullptr;
}
