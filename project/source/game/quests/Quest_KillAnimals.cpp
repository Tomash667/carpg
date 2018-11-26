#include "Pch.h"
#include "GameCore.h"
#include "Quest_KillAnimals.h"
#include "Game.h"
#include "Journal.h"
#include "QuestManager.h"
#include "City.h"
#include "World.h"
#include "Team.h"

//=================================================================================================
void Quest_KillAnimals::Start()
{
	quest_id = Q_KILL_ANIMALS;
	type = QuestType::Captain;
	start_loc = W.GetCurrentLocationIndex();
}

//=================================================================================================
GameDialog* Quest_KillAnimals::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return GameDialog::TryGet("q_kill_animals_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_kill_animals_timeout");
	case QUEST_DIALOG_NEXT:
		return GameDialog::TryGet("q_kill_animals_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_KillAnimals::SetProgress(int prog2)
{
	prog = prog2;
	switch(prog2)
	{
	case Progress::Started:
		// player accepted quest
		{
			OnStart(game->txQuest[76]);
			quest_manager.quests_timeout.push_back(this);

			// event
			Location& sl = GetStartLocation();
			target_loc = W.GetClosestLocation(Rand() % 2 == 0 ? L_FOREST : L_CAVE, sl.pos);
			location_event_handler = this;

			Location& tl = GetTargetLocation();
			tl.active_quest = this;
			tl.SetKnown();

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[77], sl.name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
		}
		break;
	case Progress::ClearedLocation:
		// player cleared location from animals
		{
			if(target_loc != -1)
			{
				Location& loc = GetTargetLocation();
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			RemoveElementTry<Quest_Dungeon*>(quest_manager.quests_timeout, this);
			OnUpdate(Format(game->txQuest[78], GetTargetLocationName()));
		}
		break;
	case Progress::Finished:
		// player talked with captain, end of quest
		{
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_captain = CityQuestState::None;
			game->AddReward(1200);
			Team.AddExp(9000);
			OnUpdate(game->txQuest[79]);
		}
		break;
	case Progress::Timeout:
		// player failed to clear location in time
		{
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_captain = CityQuestState::Failed;
			OnUpdate(game->txQuest[80]);
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
cstring Quest_KillAnimals::FormatString(const string& str)
{
	if(str == "target_loc")
		return GetTargetLocationName();
	else if(str == "target_dir")
		return GetLocationDirName(GetStartLocation().pos, GetTargetLocation().pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_KillAnimals::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 30;
}

//=================================================================================================
bool Quest_KillAnimals::OnTimeout(TimeoutType ttype)
{
	if(prog == Progress::Started)
	{
		OnUpdate(game->txQuest[277]);
		W.AbadonLocation(&GetTargetLocation());
	}

	return true;
}

//=================================================================================================
bool Quest_KillAnimals::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started && !timeout)
		SetProgress(Progress::ClearedLocation);
	return false;
}

//=================================================================================================
bool Quest_KillAnimals::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "animals") == 0 && prog == Progress::ClearedLocation);
}

//=================================================================================================
bool Quest_KillAnimals::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	location_event_handler = this;

	return true;
}
