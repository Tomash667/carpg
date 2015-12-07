#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Quest_Evil.h"

void Game::WorldProgress(int days, WorldProgressMode mode)
{
	if(mode == WPM_TRAVEL)
		assert(days == 1);

	// zmiana dnia/miesi¹ca/roku
	worldtime += days;
	day += days;
	if(day >= 30)
	{
		int ile = day/30;
		month += ile;
		day -= ile*30;
		if(month >= 12)
		{
			ile = month/12;
			year += ile;
			month -= ile*12;
			// nowe miejsce na chlanie
			contest_where = GetRandomCityLocation(contest_where);
			if(year >= 160)
			{
				// koniec gry
				LOG("Game over: you are too old.");
				CloseAllPanels(true);
				koniec_gry = true;
				death_fade = 0.f;
				if(IsOnline())
				{
					PushNetChange(NetChange::GAME_STATS);
					PushNetChange(NetChange::END_OF_GAME);
				}
			}
		}
	}

	if(mode == WPM_TRAVEL)
	{
		bool autoheal = (quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss);

		// regeneracja hp / trenowanie
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			if(autoheal)
				(*it)->hp = (*it)->hpmax;
			if((*it)->IsPlayer())
				(*it)->player->TravelTick();
			else
				(*it)->hero->PassTime(1, true);
		}

		// ubywanie wolnych dni
		if(IsOnline())
		{
			int maks = 0;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->free_days > maks)
					maks = (*it)->player->free_days;
			}

			if(maks > 0)
			{
				for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
				{
					if((*it)->IsPlayer() && (*it)->player->free_days == maks)
						--(*it)->player->free_days;
				}

				PushNetChange(NetChange::UPDATE_FREE_DAYS);
			}
		}
	}
	else if(mode == WPM_NORMAL)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			if((*it)->IsHero())
				(*it)->hero->PassTime(days);
		}
	}

	// aktualizacja mapy œwiata
	DoWorldProgress(days);

	if(IsOnline())
		PushNetChange(NetChange::WORLD_TIME);
}
