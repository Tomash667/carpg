#include "Pch.h"
#include "GameCore.h"
#include "Quest_Tournament.h"
#include "GameFile.h"
#include "World.h"

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

bool Quest_Tournament::HaveJoined(Unit* unit)
{
	for(Unit* u : units)
	{
		if(unit == u)
			return true;
	}
	return false;
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
