#include "Pch.h"
#include "GameCore.h"
#include "Quest_CampNearCity.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "GameFile.h"
#include "World.h"
#include "Team.h"
#include "ItemHelper.h"
#include "SaveState.h"

//=================================================================================================
void Quest_CampNearCity::Start()
{
	quest_id = Q_CAMP_NEAR_CITY;
	type = QuestType::Captain;
	start_loc = W.GetCurrentLocationIndex();
	switch(Rand() % 3)
	{
	case 0:
		group = SG_BANDITS;
		break;
	case 1:
		group = SG_GOBLINS;
		break;
	case 2:
		group = SG_ORCS;
		break;
	}
}

//=================================================================================================
GameDialog* Quest_CampNearCity::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_camp_near_city_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_camp_near_city_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_camp_near_city_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_CampNearCity::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// player accepted quest
		{
			Location& sl = GetStartLocation();
			bool is_city = LocationHelper::IsCity(sl);

			OnStart(game->txQuest[is_city ? 57 : 58]);
			quest_manager.quests_timeout.push_back(this);

			// event
			target_loc = W.CreateCamp(sl.pos, group);
			location_event_handler = this;

			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.SetKnown();
			st = tl.st;

			cstring gn;
			switch(group)
			{
			case SG_BANDITS:
			default:
				gn = game->txQuest[59];
				break;
			case SG_ORCS:
				gn = game->txQuest[60];
				break;
			case SG_GOBLINS:
				gn = game->txQuest[61];
				break;
			}

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[62], gn, GetLocationDirName(sl.pos, tl.pos), sl.name.c_str(),
				is_city ? game->txQuest[63] : game->txQuest[64]));
		}
		break;
	case Progress::ClearedLocation:
		// player cleared location
		{
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			OnUpdate(game->txQuest[65]);
		}
		break;
	case Progress::Finished:
		// player talked with captain, end of quest
		{
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			int reward = GetReward();
			game->AddReward(reward);
			Team.AddExp(reward * 3);
			OnUpdate(game->txQuest[66]);
		}
		break;
	case Progress::Timeout:
		// player failed to clear camp on time
		{
			state = Quest::Failed;
			City& city = (City&)GetStartLocation();
			city.quest_captain = CityQuestState::Failed;
			OnUpdate(Format(game->txQuest[67], LocationHelper::IsCity(city) ? game->txQuest[63] : game->txQuest[64]));
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_CampNearCity::FormatString(const string& str)
{
	if(str == "Bandyci_zalozyli")
	{
		switch(group)
		{
		case SG_BANDITS:
			return game->txQuest[68];
		case SG_ORCS:
			return game->txQuest[69];
		case SG_GOBLINS:
			return game->txQuest[70];
		default:
			assert(0);
			return game->txQuest[71];
		}
	}
	else if(str == "naszego_miasta")
	{
		if(LocationHelper::IsCity(GetStartLocation()))
			return game->txQuest[72];
		else
			return game->txQuest[73];
	}
	else if(str == "miasto")
	{
		if(LocationHelper::IsCity(GetStartLocation()))
			return game->txQuest[63];
		else
			return game->txQuest[64];
	}
	else if(str == "dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	else if(str == "bandyci_zaatakowali")
	{
		switch(group)
		{
		case SG_BANDITS:
			return game->txQuest[74];
		case SG_ORCS:
			return game->txQuest[75];
		case SG_GOBLINS:
			return game->txQuest[266];
		default:
			assert(0);
			return nullptr;
		}
	}
	else if(str == "reward")
		return Format("%d", GetReward());
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_CampNearCity::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_CampNearCity::OnTimeout(TimeoutType ttype)
{
	if(prog == Progress::Started)
	{
		OnUpdate(game->txQuest[277]);
		if(ttype == TIMEOUT_CAMP)
			W.AbadonLocation(&GetTargetLocation());
	}

	return true;
}

//=================================================================================================
bool Quest_CampNearCity::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started && !timeout)
		SetProgress(Progress::ClearedLocation);
	return false;
}

//=================================================================================================
bool Quest_CampNearCity::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "camp") == 0 && prog == Progress::ClearedLocation);
}

//=================================================================================================
void Quest_CampNearCity::Save(GameWriter& f)
{
	Quest_Dungeon::Save(f);

	f << group;
	f << st;
}

//=================================================================================================
bool Quest_CampNearCity::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	f >> group;
	if(LOAD_VERSION >= V_DEV)
		f >> st;
	else if(target_loc != -1)
		st = GetTargetLocation().st;
	else
		st = 10;

	location_event_handler = this;

	return true;
}

//=================================================================================================
int Quest_CampNearCity::GetReward() const
{
	return ItemHelper::CalculateReward(st, Int2(5, 15), Int2(2500, 5000));
}
