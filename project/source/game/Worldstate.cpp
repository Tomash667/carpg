#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "Quest_Evil.h"
#include "Team.h"

void Game::WorldProgress(int days, WorldProgressMode mode)
{
	if(mode == WPM_TRAVEL)
		assert(days == 1);

	// zmiana dnia/miesi¹ca/roku
	worldtime += days;
	day += days;
	if(day >= 30)
	{
		int ile = day / 30;
		month += ile;
		day -= ile * 30;
		if(month >= 12)
		{
			ile = month / 12;
			year += ile;
			month -= ile * 12;
			// nowe miejsce na chlanie
			contest_where = GetRandomSettlement(contest_where);
			if(year >= 160)
			{
				// koniec gry
				Info("Game over: you are too old.");
				CloseAllPanels(true);
				koniec_gry = true;
				death_fade = 0.f;
				if(Net::IsOnline())
				{
					Net::PushChange(NetChange::GAME_STATS);
					Net::PushChange(NetChange::END_OF_GAME);
				}
			}
		}
	}

	if(mode == WPM_TRAVEL)
	{
		bool autoheal = (quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss);

		// regeneracja hp / trenowanie
		for(Unit* unit : Team.members)
		{
			if(autoheal)
				unit->hp = unit->hpmax;
			if(unit->IsHero())
				unit->hero->PassTime(1, true);
		}

		// ubywanie wolnych dni
		if(Net::IsOnline())
		{
			int maks = 0;
			for(Unit* unit : Team.active_members)
			{
				if(unit->IsPlayer() && unit->player->free_days > maks)
					maks = unit->player->free_days;
			}

			if(maks > 0)
			{
				for(Unit* unit : Team.active_members)
				{
					if(unit->IsPlayer() && unit->player->free_days == maks)
						--unit->player->free_days;
				}

				Net::PushChange(NetChange::UPDATE_FREE_DAYS);
			}
		}
	}
	else if(mode == WPM_NORMAL)
	{
		for(Unit* unit : Team.members)
		{
			if(unit->IsHero())
				unit->hero->PassTime(days);
		}
	}

	// aktualizacja mapy œwiata
	DoWorldProgress(days);

	if(Net::IsOnline())
		Net::PushChange(NetChange::WORLD_TIME);
}

void Game::OnTravel(float dist)
{
	for(auto unit : Team.members)
	{
		if(unit->IsPlayer())
			unit->player->TrainMove(dist / 100);
	}
}
