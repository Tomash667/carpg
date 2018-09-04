#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Quest_Evil.h"
#include "Team.h"
#include "World.h"
#include "QuestManager.h"
#include "Quest_Contest.h"

void Game::WorldProgress(int days, WorldProgressMode mode)
{
	if(mode == WPM_TRAVEL)
		assert(days == 1);

	// zmiana dnia/miesi¹ca/roku
	int prev_year = W.GetYear();
	W.Update(days);
	if(prev_year != W.GetYear())
	{
		// new contest city
		Quest_Contest* contest = QM.quest_contest;
		contest->where = W.GetRandomSettlementIndex(contest->where);

		// end of game
		if(W.GetYear() >= 160)
		{
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

	if(mode == WPM_TRAVEL)
	{
		bool autoheal = (quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss);

		// regeneracja hp / trenowanie
		for(Unit* unit : Team.members)
		{
			if(autoheal)
				unit->hp = unit->hpmax;
			if(unit->IsPlayer())
				unit->player->TravelTick();
			else
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
}
