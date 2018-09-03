#include "Pch.h"
#include "GameCore.h"
#include "Quest_SpreadNews.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "GameGui.h"
#include "GameFile.h"
#include "World.h"

//-----------------------------------------------------------------------------
bool SortEntries(const Quest_SpreadNews::Entry& e1, const Quest_SpreadNews::Entry& e2)
{
	return e1.dist < e2.dist;
}

//=================================================================================================
void Quest_SpreadNews::Start()
{
	type = QuestType::Mayor;
	quest_id = Q_SPREAD_NEWS;
	start_loc = W.current_location_index;
	Vec2 pos = GetStartLocation().pos;
	bool sorted = false;
	const vector<Location*>& locations = W.GetLocations();
	for(uint i = 0, count = locations.size(); i < count; ++i)
	{
		if(!locations[i] || locations[i]->type != L_CITY)
			break;
		if(i == start_loc)
			continue;
		Location& loc = *locations[i];
		float dist = Vec2::Distance(pos, loc.pos);
		bool ok = false;
		if(entries.size() < 5)
			ok = true;
		else
		{
			if(!sorted)
			{
				std::sort(entries.begin(), entries.end(), SortEntries);
				sorted = true;
			}
			if(entries.back().dist > dist)
			{
				ok = true;
				sorted = false;
				entries.pop_back();
			}
		}
		if(ok)
		{
			Entry& e = Add1(entries);
			e.location = i;
			e.given = false;
			e.dist = dist;
		}
	}
}

//=================================================================================================
GameDialog* Quest_SpreadNews::GetDialog(int type2)
{
	switch(type2)
	{
	case QUEST_DIALOG_START:
		return FindDialog("q_spread_news_start");
	case QUEST_DIALOG_FAIL:
		return FindDialog("q_spread_news_timeout");
	case QUEST_DIALOG_NEXT:
		if(prog == Progress::Started)
			return FindDialog("q_spread_news_tell");
		else
			return FindDialog("q_spread_news_end");
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
void Quest_SpreadNews::SetProgress(int prog2)
{
	switch(prog2)
	{
	case Progress::Started:
		// told info to spread by player
		{
			prog = Progress::Started;
			start_time = W.GetWorldtime();
			state = Quest::Started;

			quest_index = quest_manager.quests.size();
			quest_manager.quests.push_back(this);
			RemoveElement<Quest*>(quest_manager.unaccepted_quests, this);
			quest_manager.quests_timeout2.push_back(this);

			Location& loc = GetStartLocation();
			bool is_city = LocationHelper::IsCity(loc);
			name = game->txQuest[213];
			msgs.push_back(Format(game->txQuest[3], is_city ? game->txForMayor : game->txForSoltys, loc.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[17], Upper(is_city ? game->txForMayor : game->txForSoltys), loc.name.c_str(), FormatString("targets")));
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_AddQuest(refid);
		}
		break;
	case Progress::Deliver:
		// player told news to mayor
		{
			uint ile = 0;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(W.current_location_index == it->location)
				{
					it->given = true;
					++ile;
				}
				else if(it->given)
					++ile;
			}

			Location& loc = *W.current_location;
			msgs.push_back(Format(game->txQuest[18], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str()));

			if(ile == entries.size())
			{
				prog = Progress::Deliver;
				msgs.push_back(Format(game->txQuest[19], GetStartLocationName()));
			}

			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);
			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);

			if(Net::IsOnline())
			{
				if(prog == Progress::Deliver)
					game->Net_UpdateQuestMulti(refid, 2);
				else
					game->Net_UpdateQuest(refid);
			}
		}
		break;
	case Progress::Timeout:
		// player failed to spread news in time
		{
			prog = Progress::Timeout;
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::Failed;

			msgs.push_back(game->txQuest[20]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	case Progress::Finished:
		// player spread news to all mayors, end of quest
		{
			prog = Progress::Finished;
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::None;
			game->AddReward(200);

			msgs.push_back(game->txQuest[21]);
			game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
			game->AddGameMsg3(GMS_JOURNAL_UPDATED);

			if(Net::IsOnline())
				game->Net_UpdateQuest(refid);
		}
		break;
	}
}

//=================================================================================================
cstring Quest_SpreadNews::FormatString(const string& str)
{
	if(str == "targets")
	{
		static string s;
		s.clear();
		for(uint i = 0, count = entries.size(); i < count; ++i)
		{
			s += W.GetLocation(entries[i].location)->name;
			if(i == count - 2)
				s += game->txQuest[264];
			else if(i != count - 1)
				s += ", ";
		}
		return s.c_str();
	}
	else if(str == "start_loc")
		return GetStartLocationName();
	else
	{
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
bool Quest_SpreadNews::IsTimedout() const
{
	return W.GetWorldtime() - start_time > 60 && prog < Progress::Deliver;
}

//=================================================================================================
bool Quest_SpreadNews::OnTimeout(TimeoutType ttype)
{
	msgs.push_back(game->txQuest[277]);
	game->game_gui->journal->NeedUpdate(Journal::Quests, quest_index);
	game->AddGameMsg3(GMS_JOURNAL_UPDATED);

	return true;
}

//=================================================================================================
bool Quest_SpreadNews::IfNeedTalk(cstring topic) const
{
	if(strcmp(topic, "tell_news") == 0)
	{
		if(prog == Progress::Started && !timeout)
		{
			for(vector<Entry>::const_iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(it->location == W.current_location_index)
					return !it->given;
			}
		}
	}
	else if(strcmp(topic, "tell_news_end") == 0)
		return prog == Progress::Deliver && W.current_location_index == start_loc;
	return false;
}

//=================================================================================================
void Quest_SpreadNews::Save(GameWriter& f)
{
	Quest::Save(f);

	if(IsActive())
		f << entries;
}

//=================================================================================================
bool Quest_SpreadNews::Load(GameReader& f)
{
	Quest::Load(f);

	if(IsActive())
		f >> entries;

	return true;
}
