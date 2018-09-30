#include "Pch.h"
#include "GameCore.h"
#include "Quest_SpreadNews.h"
#include "Dialog.h"
#include "Game.h"
#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
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
	start_loc = W.GetCurrentLocationIndex();
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
			OnStart(game->txQuest[213]);
			quest_manager.quests_timeout2.push_back(this);

			prog = Progress::Started;

			Location& loc = GetStartLocation();
			bool is_city = LocationHelper::IsCity(loc);
			msgs.push_back(Format(game->txQuest[3], is_city ? game->txForMayor : game->txForSoltys, loc.name.c_str(), W.GetDate()));
			msgs.push_back(Format(game->txQuest[17], Upper(is_city ? game->txForMayor : game->txForSoltys), loc.name.c_str(), FormatString("targets")));
		}
		break;
	case Progress::Deliver:
		// player told news to mayor
		{
			uint ile = 0;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(W.GetCurrentLocationIndex() == it->location)
				{
					it->given = true;
					++ile;
				}
				else if(it->given)
					++ile;
			}

			Location& loc = *W.GetCurrentLocation();
			cstring msg = Format(game->txQuest[18], LocationHelper::IsCity(loc) ? game->txForMayor : game->txForSoltys, loc.name.c_str());

			if(ile == entries.size())
			{
				prog = Progress::Deliver;
				OnUpdate({ msg, Format(game->txQuest[19], GetStartLocationName()) });
			}
			else
				OnUpdate(msg);

			RemoveElementTry(quest_manager.quests_timeout2, (Quest*)this);
		}
		break;
	case Progress::Timeout:
		// player failed to spread news in time
		{
			prog = Progress::Timeout;
			state = Quest::Failed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::Failed;

			OnUpdate(game->txQuest[20]);
		}
		break;
	case Progress::Finished:
		// player spread news to all mayors, end of quest
		{
			prog = Progress::Finished;
			state = Quest::Completed;
			((City&)GetStartLocation()).quest_mayor = CityQuestState::None;
			game->AddReward(200);

			OnUpdate(game->txQuest[21]);
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
	OnUpdate(game->txQuest[277]);
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
				if(it->location == W.GetCurrentLocationIndex())
					return !it->given;
			}
		}
	}
	else if(strcmp(topic, "tell_news_end") == 0)
		return prog == Progress::Deliver && W.GetCurrentLocationIndex() == start_loc;
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
