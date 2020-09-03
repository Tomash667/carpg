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
	quest_mgr->RegisterSpecialHandler(this, "contest_start");
	quest_mgr->RegisterSpecialHandler(this, "contest_join");
	quest_mgr->RegisterSpecialHandler(this, "contest_reward");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_done");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_here");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_today");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_in_progress");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_started");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_joined");
	quest_mgr->RegisterSpecialIfHandler(this, "contest_winner");
	quest_mgr->RegisterFormatString(this, "contest_loc");
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
	rumor = quest_mgr->AddQuestRumor(quest_mgr->txRumorQ[2]);

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
	if(LOAD_VERSION >= V_0_10)
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
		if(!generated && game->game_state == GS_LEVEL && game_level->location_index == where)
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

	InsideBuilding* inn = game_level->city_ctx->FindInn();
	Unit& innkeeper = *inn->FindUnit(UnitData::Get("innkeeper"));

	if(!innkeeper.IsAlive())
	{
		for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.busy = Unit::Busy_No;
			u.look_target = nullptr;
			u.event_handler = nullptr;
			if(u.IsPlayer())
			{
				u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
				if(u.player != game->pc)
				{
					NetChangePlayer& c = Add1(u.player->player_info->changes);
					c.type = NetChangePlayer::LOOK_AT;
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
			bool leaving_event = (dist > 10.f || unit->area != inn);
			if(leaving_event != unit->player->leaving_event)
			{
				unit->player->leaving_event = leaving_event;
				if(leaving_event)
					game_gui->messages->AddGameMsg3(unit->player, GMS_GETTING_OUT_OF_RANGE);
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
				if(u.IsStanding() && u.IsAI() && !u.event_handler && u.frozen == FROZEN::NO && u.busy == Unit::Busy_No)
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
						if(quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::MageCured)
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
					kick = (dist > 10.f || u.area != inn);
				}
				if(kick || u.area != inn || u.frozen != FROZEN::NO || !u.IsStanding())
				{
					if(u.IsPlayer())
						game_gui->messages->AddGameMsg3(u.player, GMS_LEFT_EVENT);
					*it = nullptr;
					removed = true;
				}
				else
				{
					u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
					if(u.IsPlayer() && u.player != game->pc)
					{
						NetChangePlayer& c = Add1(u.player->player_info->changes);
						c.type = NetChangePlayer::LOOK_AT;
						c.id = innkeeper.id;
					}
					u.busy = Unit::Busy_Yes;
					u.look_target = innkeeper;
					u.event_handler = this;
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
				innkeeper.ai->st.idle.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				innkeeper.Talk(txContestNoPeople);
			}
			else
			{
				innkeeper.ai->st.idle.action = AIController::Idle_Rot;
				innkeeper.ai->st.idle.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				innkeeper.Talk(txContestTalk[0]);
			}
		}
	}
	else if(state == CONTEST_IN_PROGRESS)
	{
		bool talking = true;
		cstring next_text = nullptr, next_drink = nullptr;

		switch(state2)
		{
		case 0:
			next_text = txContestTalk[1];
			break;
		case 1:
			next_text = txContestTalk[2];
			break;
		case 2:
			next_text = txContestTalk[3];
			break;
		case 3:
			next_drink = "beer";
			break;
		case 4:
			talking = false;
			next_text = txContestTalk[4];
			break;
		case 5:
			next_drink = "beer";
			break;
		case 6:
			talking = false;
			next_text = txContestTalk[5];
			break;
		case 7:
			next_drink = "beer";
			break;
		case 8:
			talking = false;
			next_text = txContestTalk[6];
			break;
		case 9:
			next_drink = "vodka";
			break;
		case 10:
			talking = false;
			next_text = txContestTalk[7];
			break;
		case 11:
			next_drink = "vodka";
			break;
		case 12:
			talking = false;
			next_text = txContestTalk[8];
			break;
		case 13:
			next_drink = "vodka";
			break;
		case 14:
			talking = false;
			next_text = txContestTalk[9];
			break;
		case 15:
			next_text = txContestTalk[10];
			break;
		case 16:
			next_drink = "spirit";
			break;
		case 17:
			talking = false;
			next_text = txContestTalk[11];
			break;
		case 18:
			next_drink = "spirit";
			break;
		case 19:
			talking = false;
			next_text = txContestTalk[12];
			break;
		default:
			if((state2 - 20) % 2 == 0)
			{
				if(state2 != 20)
					talking = false;
				next_text = txContestTalk[13];
			}
			else
				next_drink = "spirit";
			break;
		}

		if(talking)
		{
			if(innkeeper.CanAct())
			{
				if(next_text)
					innkeeper.Talk(next_text);
				else
				{
					assert(next_drink);
					time = 0.f;
					const Consumable& drink = Item::Get(next_drink)->ToConsumable();
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
					assert(next_text);
					innkeeper.Talk(next_text);
					++state2;
				}
				else if(units.size() == 1)
				{
					state = CONTEST_FINISH;
					state2 = 0;
					innkeeper.look_target = units.back();
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
				innkeeper.look_target = nullptr;
				state = CONTEST_DONE;
				generated = false;
				winner = nullptr;
				break;
			case 2: // win2
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = nullptr;
				winner = units.back();
				units.clear();
				state = CONTEST_DONE;
				generated = false;
				winner->look_target = nullptr;
				winner->busy = Unit::Busy_No;
				winner->event_handler = nullptr;
				break;
			case 3: // no people
				for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
				{
					Unit& u = **it;
					u.busy = Unit::Busy_No;
					u.look_target = nullptr;
					u.event_handler = nullptr;
					if(u.IsPlayer())
					{
						u.BreakAction(Unit::BREAK_ACTION_MODE::NORMAL, true);
						if(u.player != game->pc)
						{
							NetChangePlayer& c = Add1(u.player->player_info->changes);
							c.type = NetChangePlayer::LOOK_AT;
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
		unit->look_target = nullptr;
		unit->busy = Unit::Busy_No;
		unit->event_handler = nullptr;
		RemoveElement(units, unit);

		if(Net::IsOnline() && unit->IsPlayer() && unit->player != game->pc)
		{
			NetChangePlayer& c = Add1(unit->player->player_info->changes);
			c.type = NetChangePlayer::LOOK_AT;
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
		u.look_target = nullptr;
		u.event_handler = nullptr;
	}

	InsideBuilding* inn = game_level->city_ctx->FindInn();
	Unit* innkeeper = inn->FindUnit(UnitData::Get("innkeeper"));

	innkeeper->talking = false;
	innkeeper->node->mesh_inst->need_update = true;
	innkeeper->busy = Unit::Busy_No;
	state = CONTEST_DONE;
	units.clear();
	world->AddNews(txContestNoWinner);
}

//=================================================================================================
void Quest_Contest::SpawnDrunkmans()
{
	InsideBuilding* inn = game_level->city_ctx->FindInn();
	generated = true;
	UnitData& pijak = *UnitData::Get("pijak");
	int count = Random(4, 6);
	for(int i = 0; i < count; ++i)
		game_level->SpawnUnitInsideInn(pijak, -2, inn, Level::SU_TEMPORARY | Level::SU_MAIN_ROOM);
}
