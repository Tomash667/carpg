#include "Pch.h"
#include "GameCore.h"
#include "Quest_Contest.h"
#include "World.h"
#include "PlayerController.h"
#include "GameFile.h"

void Quest_Contest::Init()
{
	state = CONTEST_NOT_DONE;
	where = W.GetRandomSettlementIndex();
	units.clear();
	winner = nullptr;
	generated = false;
	year = W.GetYear();
}

void Quest_Contest::Start(PlayerController* pc)
{
	state = CONTEST_STARTING;
	time = 0;
	units.clear();
	units.push_back(pc->unit);
	pc->leaving_event = false;
}

void Quest_Contest::AddPlayer(PlayerController* pc)
{
	units.push_back(pc->unit);
	pc->leaving_event = false;
}

bool Quest_Contest::HaveJoined(Unit* unit)
{
	for(Unit* u : units)
	{
		if(unit == u)
			return true;
	}
	return false;
}

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
