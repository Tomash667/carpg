#include "Pch.h"
#include "Base.h"
#include "Quest_KillAnimals.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"

//=================================================================================================
void Quest_KillAnimals::Start()
{
	quest_id = Q_KILL_ANIMALS;
	type = Type::Captain;
	start_loc = game->current_location;
}

//=================================================================================================
GameDialog* Quest_KillAnimals::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_kill_animals_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_kill_animals_timeout");
	case QUEST_DIALOG_NEXT:
		return FindDialog("q_kill_animals_end");
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
			name = game->txQuest[76];

			Location& sl = *game->locations[start_loc];

			// event
			target_loc = game->GetClosestLocation(rand2()%2 == 0 ? L_FOREST : L_CAVE, sl.pos);
			location_event_handler = this;
			
			Location& tl = *game->locations[target_loc];
			tl.active_quest = this;
			bool now_known = false;
			if(tl.state == LS_UNKNOWN)
			{
				tl.state = LS_KNOWN;
				now_known = true;
			}

			QM.AcceptQuest(this, 1);

			msgs.push_back(Format(game->txQuest[29], sl.name.c_str(), game->day+1, game->month+1, game->year));
			msgs.push_back(Format(game->txQuest[77], sl.name.c_str(), tl.name.c_str(), GetLocationDirName(sl.pos, tl.pos)));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
			{
				game->Net_AddQuest(refid);
				if(now_known)
					game->Net_ChangeLocationState(target_loc, false);
			}
		}
		break;
	case Progress::ClearedLocation:
		// player cleared location from animals
		{
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			QM.RemoveTimeout(this, 1);
			msgs.push_back(Format(game->txQuest[78], game->locations[target_loc]->name.c_str()));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// player talked with captain, end of quest
		{
			state = Quest::Completed;
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::None;
			game->AddReward(1200);
			msgs.push_back(game->txQuest[79]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Timeout:
		// player failed to clear location in time
		{
			state = Quest::Failed;
			((City*)game->locations[start_loc])->quest_captain = CityQuestState::Failed;
			msgs.push_back(game->txQuest[80]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			if(target_loc != -1)
			{
				Location& loc = *game->locations[target_loc];
				if(loc.active_quest == this)
					loc.active_quest = nullptr;
			}
			QM.RemoveTimeout(this, 1);

			if(game->IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_KillAnimals::FormatString(const string& str)
{
	if(str == "target_loc")
		return game->locations[target_loc]->name.c_str();
	else if(str == "target_dir")
		return GetLocationDirName(game->locations[start_loc]->pos, game->locations[target_loc]->pos);
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_KillAnimals::IsTimedout() const
{
	return game->worldtime - start_time > 30;
}

//=================================================================================================
bool Quest_KillAnimals::OnTimeout(TimeoutType ttype)
{
	if(prog == Progress::Started)
	{
		msgs.push_back(game->txQuest[277]);
		game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
		game->AddGameMsg3(GMS_JOURNAL_UPDATED);

		game->AbadonLocation(game->locations[target_loc]);
	}

	return true;
}

//=================================================================================================
void Quest_KillAnimals::HandleLocationEvent(LocationEventHandler::Event event)
{
	if(event == LocationEventHandler::CLEARED && prog == Progress::Started && !timeout)
		SetProgress(Progress::ClearedLocation);
}

//=================================================================================================
bool Quest_KillAnimals::IfNeedTalk(cstring topic) const
{
	return (strcmp(topic, "animals") == 0 && prog == Progress::ClearedLocation);
}

//=================================================================================================
void Quest_KillAnimals::Load(HANDLE file)
{
	Quest_Dungeon::Load(file);

	location_event_handler = this;
}
