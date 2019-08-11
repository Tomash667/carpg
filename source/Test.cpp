#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GlobalGui.h"
#include "World.h"
#include "Level.h"

static int state, steps;
static Timer timer;
static float tdt;
static Location* loc;

void UpdateTest()
{
	Game& game = Game::Get();
	float dt = timer.Tick();

	switch(state)
	{
	case 0:
		if(game.game_state == GS_LEVEL && game.input->Pressed(Key::N8))
		{
			Info("TEST: Started.");
			state = 1;
			tdt = 5;
		}
		break;
	case 1:
		tdt -= dt;
		if(tdt <= 0.f)
		{
			++steps;
			if(steps >= 10 && Rand() % 3 == 0)
			{
				game.Quickload(false);
				steps = 0;
				tdt = 10.f;
			}
			else if(Rand() % 2 == 0)
			{
				game.Quicksave(false);
				tdt = 3.f;
			}
			state = 2;
		}
		break;
	case 2:
		tdt -= dt;
		if(tdt <= 0.f)
		{
			Location* cl = W.GetCurrentLocation();
			if(cl->GetLastLevel() == L.dungeon_level)
			{
				loc = W.GetRandomLocation([](Location* l)
				{
					return l->state < LS_VISITED && l->type != L_ENCOUNTER;
				});
				if(!loc)
				{
					state = 0;
					Info("TEST: Finished!");
					global::gui->SimpleDialog("Test finished!", nullptr);
				}
				else
				{
					game.ExitToMap();
					tdt = 2.f;
					state = 3;
				}
			}
			else
			{
				game.ChangeLevel(+1);
				tdt = 5.f;
				state = 1;
			}
		}
		break;
	case 3:
		tdt -= dt;
		if(tdt <= 0.f)
		{
			W.Warp(loc->index);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHEAT_TRAVEL;
				c.id = loc->index;
			}
			tdt = 2.f;
			state = 4;
		}
		break;
	case 4:
		tdt -= dt;
		if(tdt <= 0.f)
		{
			game.EnterLocation();
			tdt = 5.f;
			state = 1;
		}
		break;
	}
}
