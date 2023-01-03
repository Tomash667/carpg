#include "Pch.h"
#include "Quest_Contest.h"

#include "AIController.h"
#include "City.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "InsideBuilding.h"
#include "Language.h"
#include "Level.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "QuestManager.h"
#include "Quest_Mages.h"
#include "World.h"

//=================================================================================================
void Quest_Contest::InitOnce()
{
	questMgr->RegisterSpecialHandler(this, "contest_start");
	questMgr->RegisterSpecialHandler(this, "contest_join");
	questMgr->RegisterSpecialHandler(this, "contest_reward");
	questMgr->RegisterSpecialIfHandler(this, "contest_done");
	questMgr->RegisterSpecialIfHandler(this, "contest_here");
	questMgr->RegisterSpecialIfHandler(this, "contest_today");
	questMgr->RegisterSpecialIfHandler(this, "contest_in_progress");
	questMgr->RegisterSpecialIfHandler(this, "contest_started");
	questMgr->RegisterSpecialIfHandler(this, "contest_joined");
	questMgr->RegisterSpecialIfHandler(this, "contest_winner");
	questMgr->RegisterFormatString(this, "contest_loc");
}

//=================================================================================================
void Quest_Contest::LoadLanguage()
{
	txContestNoWinner = Str("contestNoWinner");
	txContestStart = Str("contestStart");
	StrArray(txContestTalk, "contestTalk");
	txContestWin = Str("contestWin");
	txContestWinNews = Str("contestWinNews");
	txContestDraw = Str("contestDraw");
	txContestPrize = Str("contestPrize");
	txContestNoPeople = Str("contestNoPeople");
}

//=================================================================================================
void Quest_Contest::Init()
{
	state = CONTEST_NOT_DONE;
	where = world->GetRandomSettlement()->index;
	units.clear();
	winner = nullptr;
	generated = false;
	year = world->GetDateValue().year;
	rumor = questMgr->AddQuestRumor(questMgr->txRumorQ[2]);

	if(game->devmode)
		Info("Contest - %s.", world->GetLocation(where)->name.c_str());
}

//=================================================================================================
void Quest_Contest::Save(GameWriter& f)
{
	f << where;
	f << state;
	f << generated;
	f << winner;
	f << rumor;
	if(state >= CONTEST_STARTING)
	{
		f << state2;
		f << time;
		f << units.size();
		for(Unit* unit : units)
			f << unit->id;
	}
}

//=================================================================================================
void Quest_Contest::Load(GameReader& f)
{
	f >> where;
	f >> state;
	f >> generated;
	f >> winner;
	f >> rumor;
	if(state >= CONTEST_STARTING)
	{
		f >> state2;
		f >> time;
		units.resize(f.Read<uint>());
		for(Unit*& unit : units)
			f >> unit;
	}
	year = world->GetDateValue().year;
}

//=================================================================================================
bool Quest_Contest::Special(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "contest_start") == 0)
	{
		state = CONTEST_STARTING;
		time = 0;
		units.clear();
		units.push_back(ctx.pc->unit);
		ctx.pc->leavingEvent = false;
	}
	else if(strcmp(msg, "contest_join") == 0)
	{
		units.push_back(ctx.pc->unit);
		ctx.pc->leavingEvent = false;
	}
	else if(strcmp(msg, "contest_reward") == 0)
	{
		winner = nullptr;
		ctx.pc->unit->AddItem2(ItemList::GetItem("contest_reward"), 1u, 0u);
	}
	else
		assert(0);
	return false;
}

//=================================================================================================
bool Quest_Contest::SpecialIf(DialogContext& ctx, cstring msg)
{
	if(strcmp(msg, "contest_done") == 0)
		return state == CONTEST_DONE;
	else if(strcmp(msg, "contest_here") == 0)
		return where == world->GetCurrentLocationIndex();
	else if(strcmp(msg, "contest_today") == 0)
		return state == CONTEST_TODAY;
	else if(strcmp(msg, "contest_in_progress") == 0)
		return state == CONTEST_IN_PROGRESS;
	else if(strcmp(msg, "contest_started") == 0)
		return state == CONTEST_STARTING;
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
	assert(0);
	return false;
}

//=================================================================================================
cstring Quest_Contest::FormatString(const string& str)
{
	if(str == "contest_loc")
		return world->GetLocation(where)->name.c_str();
	return nullptr;
}

//=================================================================================================
void Quest_Contest::OnProgress()
{
	int step; // 0 - before contest, 1 - time for contest, 2 - after contest
	const Date& date = world->GetDateValue();

	if(date.month < 8)
		step = 0;
	else if(date.month == 8)
	{
		if(date.day < 20)
			step = 0;
		else if(date.day == 20)
			step = 1;
		else
			step = 2;
	}
	else
		step = 2;

	switch(step)
	{
	case 0:
		if(state != CONTEST_NOT_DONE)
		{
			state = CONTEST_NOT_DONE;
			where = world->GetRandomSettlement(world->GetLocation(where))->index;
		}
		generated = false;
		units.clear();
		break;
	case 1:
		state = CONTEST_TODAY;
		if(!generated && game->gameState == GS_LEVEL && gameLevel->locationIndex == where)
			SpawnDrunkmans();
		break;
	case 2:
		state = CONTEST_DONE;
		generated = false;
		units.clear();
		break;
	}
}

//=================================================================================================
void Quest_Contest::Update(float dt)
{
	if(Any(state, CONTEST_NOT_DONE, CONTEST_DONE, CONTEST_TODAY))
		return;

	InsideBuilding* inn = gameLevel->cityCtx->FindInn();
	Unit& innkeeper = *inn->FindUnit(UnitData::Get("innkeeper"));

	if(!innkeeper.IsAlive())
	{
		for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.busy = Unit::Busy_No;
			u.lookTarget = nullptr;
			u.eventHandler = nullptr;
			if(u.IsPlayer())
			{
				u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
				if(u.player != game->pc)
				{
					NetChangePlayer& c = u.player->playerInfo->PushChange(NetChangePlayer::LOOK_AT);
					c.id = -1;
				}
			}
		}
		state = CONTEST_DONE;
		innkeeper.busy = Unit::Busy_No;
		return;
	}

	if(state == CONTEST_STARTING)
	{
		// info about getting out of range
		for(Unit* unit : units)
		{
			if(!unit->IsPlayer())
				continue;
			float dist = Vec3::Distance2d(unit->pos, innkeeper.pos);
			bool leavingEvent = (dist > 10.f || unit->locPart != inn);
			if(leavingEvent != unit->player->leavingEvent)
			{
				unit->player->leavingEvent = leavingEvent;
				if(leavingEvent)
					gameGui->messages->AddGameMsg3(unit->player, GMS_GETTING_OUT_OF_RANGE);
			}
		}

		// update timer
		if(innkeeper.busy == Unit::Busy_No && innkeeper.IsStanding() && innkeeper.ai->state == AIController::Idle)
		{
			float prev = time;
			time += dt;
			if(prev < 5.f && time >= 5.f)
				innkeeper.Talk(txContestStart);
		}

		if(time >= 15.f && innkeeper.busy != Unit::Busy_Talking)
		{
			// start contest
			state = CONTEST_IN_PROGRESS;

			// gather units
			for(vector<Unit*>::iterator it = inn->units.begin(), end = inn->units.end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsStanding() && u.IsAI() && !u.eventHandler && u.frozen == FROZEN::NO && u.busy == Unit::Busy_No)
				{
					bool ok = false;
					if(IsSet(u.data->flags2, F2_CONTEST))
						ok = true;
					else if(IsSet(u.data->flags2, F2_CONTEST_50))
					{
						if(Rand() % 2 == 0)
							ok = true;
					}
					else if(IsSet(u.data->flags3, F3_CONTEST_25))
					{
						if(Rand() % 4 == 0)
							ok = true;
					}
					else if(IsSet(u.data->flags3, F3_DRUNK_MAGE))
					{
						if(questMgr->questMages2->magesState < Quest_Mages2::State::MageCured)
							ok = true;
					}

					if(ok)
						units.push_back(*it);
				}
			}
			state2 = 0;

			// start looking at innkeeper, remove busy units/players out of range
			bool removed = false;
			for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
			{
				Unit& u = **it;
				bool kick = false;
				if(u.IsPlayer())
				{
					float dist = Vec3::Distance2d(u.pos, innkeeper.pos);
					kick = (dist > 10.f || u.locPart != inn);
				}
				if(kick || u.locPart != inn || u.frozen != FROZEN::NO || !u.IsStanding())
				{
					if(u.IsPlayer())
						gameGui->messages->AddGameMsg3(u.player, GMS_LEFT_EVENT);
					*it = nullptr;
					removed = true;
				}
				else
				{
					u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
					if(u.IsPlayer() && u.player != game->pc)
					{
						NetChangePlayer& c = u.player->playerInfo->PushChange(NetChangePlayer::LOOK_AT);
						c.id = innkeeper.id;
					}
					u.busy = Unit::Busy_Yes;
					u.lookTarget = innkeeper;
					u.eventHandler = this;
				}
			}
			if(removed)
				RemoveNullElements(units);

			// jeœli jest za ma³o ludzi
			if(units.size() <= 1u)
			{
				state = CONTEST_FINISH;
				state2 = 3;
				innkeeper.ai->st.idle.action = AIController::Idle_Rot;
				innkeeper.ai->st.idle.rot = innkeeper.ai->startRot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				innkeeper.Talk(txContestNoPeople);
			}
			else
			{
				innkeeper.ai->st.idle.action = AIController::Idle_Rot;
				innkeeper.ai->st.idle.rot = innkeeper.ai->startRot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				innkeeper.Talk(txContestTalk[0]);
			}
		}
	}
	else if(state == CONTEST_IN_PROGRESS)
	{
		bool talking = true;
		cstring nextText = nullptr, nextDrink = nullptr;

		switch(state2)
		{
		case 0:
			nextText = txContestTalk[1];
			break;
		case 1:
			nextText = txContestTalk[2];
			break;
		case 2:
			nextText = txContestTalk[3];
			break;
		case 3:
			nextDrink = "beer";
			break;
		case 4:
			talking = false;
			nextText = txContestTalk[4];
			break;
		case 5:
			nextDrink = "beer";
			break;
		case 6:
			talking = false;
			nextText = txContestTalk[5];
			break;
		case 7:
			nextDrink = "beer";
			break;
		case 8:
			talking = false;
			nextText = txContestTalk[6];
			break;
		case 9:
			nextDrink = "vodka";
			break;
		case 10:
			talking = false;
			nextText = txContestTalk[7];
			break;
		case 11:
			nextDrink = "vodka";
			break;
		case 12:
			talking = false;
			nextText = txContestTalk[8];
			break;
		case 13:
			nextDrink = "vodka";
			break;
		case 14:
			talking = false;
			nextText = txContestTalk[9];
			break;
		case 15:
			nextText = txContestTalk[10];
			break;
		case 16:
			nextDrink = "spirit";
			break;
		case 17:
			talking = false;
			nextText = txContestTalk[11];
			break;
		case 18:
			nextDrink = "spirit";
			break;
		case 19:
			talking = false;
			nextText = txContestTalk[12];
			break;
		default:
			if((state2 - 20) % 2 == 0)
			{
				if(state2 != 20)
					talking = false;
				nextText = txContestTalk[13];
			}
			else
				nextDrink = "spirit";
			break;
		}

		if(talking)
		{
			if(innkeeper.CanAct())
			{
				if(nextText)
					innkeeper.Talk(nextText);
				else
				{
					assert(nextDrink);
					time = 0.f;
					const Consumable& drink = Item::Get(nextDrink)->ToConsumable();
					for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
						(*it)->ConsumeItem(drink, true);
				}

				++state2;
			}
		}
		else
		{
			time += dt;
			if(time >= 5.f)
			{
				if(units.size() >= 2)
				{
					assert(nextText);
					innkeeper.Talk(nextText);
					++state2;
				}
				else if(units.size() == 1)
				{
					state = CONTEST_FINISH;
					state2 = 0;
					innkeeper.lookTarget = units.back();
					world->AddNews(Format(txContestWinNews, units.back()->GetName()));
					innkeeper.Talk(txContestWin);
				}
				else
				{
					state = CONTEST_FINISH;
					state2 = 1;
					world->AddNews(txContestNoWinner);
					innkeeper.Talk(txContestNoWinner);
				}
			}
		}
	}
	else if(state == CONTEST_FINISH)
	{
		if(innkeeper.CanAct())
		{
			switch(state2)
			{
			case 0: // win
				state2 = 2;
				innkeeper.Talk(txContestPrize);
				break;
			case 1: // draw
				innkeeper.busy = Unit::Busy_No;
				innkeeper.lookTarget = nullptr;
				state = CONTEST_DONE;
				generated = false;
				winner = nullptr;
				break;
			case 2: // win2
				innkeeper.busy = Unit::Busy_No;
				innkeeper.lookTarget = nullptr;
				winner = units.back();
				units.clear();
				state = CONTEST_DONE;
				generated = false;
				winner->lookTarget = nullptr;
				winner->busy = Unit::Busy_No;
				winner->eventHandler = nullptr;
				break;
			case 3: // no people
				for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
				{
					Unit& u = **it;
					u.busy = Unit::Busy_No;
					u.lookTarget = nullptr;
					u.eventHandler = nullptr;
					if(u.IsPlayer())
					{
						u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
						if(u.player != game->pc)
						{
							NetChangePlayer& c = u.player->playerInfo->PushChange(NetChangePlayer::LOOK_AT);
							c.id = -1;
						}
					}
				}
				state = CONTEST_DONE;
				innkeeper.busy = Unit::Busy_No;
				break;
			}
		}
	}
}

//=================================================================================================
void Quest_Contest::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	if(event == UnitEventHandler::FALL)
	{
		// unit fallen from drinking...
		unit->lookTarget = nullptr;
		unit->busy = Unit::Busy_No;
		unit->eventHandler = nullptr;
		RemoveElement(units, unit);

		if(Net::IsOnline() && unit->IsPlayer() && unit->player != game->pc)
		{
			NetChangePlayer& c = unit->player->playerInfo->PushChange(NetChangePlayer::LOOK_AT);
			c.id = -1;
		}
	}
}

//=================================================================================================
void Quest_Contest::Cleanup()
{
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.busy = Unit::Busy_No;
		u.lookTarget = nullptr;
		u.eventHandler = nullptr;
	}

	InsideBuilding* inn = gameLevel->cityCtx->FindInn();
	Unit* innkeeper = inn->FindUnit(UnitData::Get("innkeeper"));

	innkeeper->talking = false;
	innkeeper->meshInst->needUpdate = true;
	innkeeper->busy = Unit::Busy_No;
	state = CONTEST_DONE;
	units.clear();
	world->AddNews(txContestNoWinner);
}

//=================================================================================================
void Quest_Contest::SpawnDrunkmans()
{
	InsideBuilding* inn = gameLevel->cityCtx->FindInn();
	generated = true;
	UnitData& pijak = *UnitData::Get("pijak");
	int count = Random(4, 6);
	for(int i = 0; i < count; ++i)
		gameLevel->SpawnUnitInsideInn(pijak, -2, inn, Level::SU_TEMPORARY | Level::SU_MAIN_ROOM);
}
