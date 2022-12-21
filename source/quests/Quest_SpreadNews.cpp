#include "Pch.h"
#include "Quest_SpreadNews.h"

#include "Journal.h"
#include "LocationHelper.h"
#include "QuestManager.h"
#include "Team.h"
#include "World.h"

//-----------------------------------------------------------------------------
bool SortEntries(const Quest_SpreadNews::Entry& e1, const Quest_SpreadNews::Entry& e2)
{
	return e1.dist < e2.dist;
}

//=================================================================================================
void Quest_SpreadNews::Start()
{
	category = QuestCategory::Mayor;
	type = Q_SPREAD_NEWS;
	startLoc = world->GetCurrentLocation();
	Vec2 pos = startLoc->pos;
	bool sorted = false;
	const vector<Location*>& locations = world->GetLocations();
	for(uint i = 0, count = locations.size(); i < count; ++i)
	{
		if(!locations[i] || locations[i]->type != L_CITY)
			break;
		if(i == startLoc->index)
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
		return GameDialog::TryGet("q_spread_news_start");
	case QUEST_DIALOG_FAIL:
		return GameDialog::TryGet("q_spread_news_timeout");
	case QUEST_DIALOG_NEXT:
		if(prog == Progress::Started)
			return GameDialog::TryGet("q_spread_news_tell");
		else
			return GameDialog::TryGet("q_spread_news_end");
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
			OnStart(questMgr->txQuest[213]);
			questMgr->questTimeouts2.push_back(this);

			prog = Progress::Started;

			bool isCity = LocationHelper::IsCity(startLoc);
			msgs.push_back(Format(questMgr->txQuest[3], isCity ? questMgr->txForMayor : questMgr->txForSoltys, startLoc->name.c_str(), world->GetDate()));
			msgs.push_back(Format(questMgr->txQuest[17], Upper(isCity ? questMgr->txForMayor : questMgr->txForSoltys), startLoc->name.c_str(), FormatString("targets")));
		}
		break;
	case Progress::Deliver:
		// player told news to mayor
		{
			uint count = 0;
			for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
			{
				if(world->GetCurrentLocationIndex() == it->location)
				{
					it->given = true;
					++count;
				}
				else if(it->given)
					++count;
			}

			Location& loc = *world->GetCurrentLocation();
			cstring msg = Format(questMgr->txQuest[18], LocationHelper::IsCity(loc) ? questMgr->txForMayor : questMgr->txForSoltys, loc.name.c_str());

			if(count == entries.size())
			{
				prog = Progress::Deliver;
				OnUpdate({ msg, Format(questMgr->txQuest[19], GetStartLocationName()) });
			}
			else
				OnUpdate(msg);

			RemoveElementTry(questMgr->questTimeouts2, static_cast<Quest*>(this));
		}
		break;
	case Progress::Timeout:
		// player failed to spread news in time
		{
			prog = Progress::Timeout;
			state = Quest::Failed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::Failed;

			OnUpdate(questMgr->txQuest[20]);
		}
		break;
	case Progress::Finished:
		// player spread news to all mayors, end of quest
		{
			prog = Progress::Finished;
			state = Quest::Completed;
			static_cast<City*>(startLoc)->questMayor = CityQuestState::None;
			team->AddReward(500, 2000);

			OnUpdate(questMgr->txQuest[21]);
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
			s += world->GetLocation(entries[i].location)->name;
			if(i == count - 2)
				s += questMgr->txQuest[264];
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
	return world->GetWorldtime() - startTime >= 60 && prog < Progress::Deliver;
}

//=================================================================================================
bool Quest_SpreadNews::OnTimeout(TimeoutType ttype)
{
	OnUpdate(questMgr->txQuest[267]);
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
				if(it->location == world->GetCurrentLocationIndex())
					return !it->given;
			}
		}
	}
	else if(strcmp(topic, "tell_news_end") == 0)
		return prog == Progress::Deliver && world->GetCurrentLocation() == startLoc;
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
Quest::LoadResult Quest_SpreadNews::Load(GameReader& f)
{
	Quest::Load(f);

	if(IsActive())
		f >> entries;

	return LoadResult::Ok;
}
