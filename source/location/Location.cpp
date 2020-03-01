#include "Pch.h"
#include "Game.h"
#include "SaveState.h"
#include "City.h"
#include "Quest.h"
#include "BitStreamFunc.h"
#include "Portal.h"
#include "GameFile.h"
#include "Quest_Scripted.h"
#include "QuestManager.h"
#include "UnitGroup.h"

//=================================================================================================
Location::~Location()
{
	Portal* p = portal;
	while(p)
	{
		Portal* next_portal = p->next_portal;
		delete p;
		p = next_portal;
	}
}

//=================================================================================================
bool Location::CheckUpdate(int& days_passed, int worldtime)
{
	bool need_reset = reset;
	reset = false;
	if(last_visit == -1)
		days_passed = -1;
	else
		days_passed = worldtime - last_visit;
	last_visit = worldtime;
	if(dont_clean)
		days_passed = 0;
	return need_reset;
}

//=================================================================================================
void Location::Save(GameWriter& f, bool)
{
	f << pos;
	f << name;
	f << state;
	f << target;
	int quest_id;
	if(active_quest)
	{
		if(active_quest == ACTIVE_QUEST_HOLDER)
			quest_id = (int)ACTIVE_QUEST_HOLDER;
		else
			quest_id = active_quest->id;
	}
	else
		quest_id = -1;
	f << quest_id;
	f << last_visit;
	f << st;
	f << outside;
	f << reset;
	f << group;
	f << dont_clean;
	f << seed;
	f << image;

	// portals
	Portal* p = portal;
	while(p)
	{
		f.Write1();
		p->Save(f);
		p = p->next_portal;
	}
	f.Write0();

	// events
	f << events.size();
	for(Event& e : events)
	{
		f << e.type;
		f << e.quest->id;
	}
}

//=================================================================================================
void Location::Load(GameReader& f, bool)
{
	if(LOAD_VERSION < V_0_12)
	{
		old::LOCATION old_type;
		f >> old_type;
		switch(old_type)
		{
		case old::L_CITY:
			type = L_CITY;
			break;
		case old::L_CAVE:
			type = L_CAVE;
			break;
		case old::L_CAMP:
			type = L_CAMP;
			target = 0;
			break;
		case old::L_DUNGEON:
		case old::L_CRYPT:
			type = L_DUNGEON;
			break;
		case old::L_FOREST:
			type = L_OUTSIDE;
			target = FOREST;
			break;
		case old::L_MOONWELL:
			type = L_OUTSIDE;
			target = MOONWELL;
			break;
		case old::L_ENCOUNTER:
			type = L_ENCOUNTER;
			target = 0;
			break;
		case old::L_ACADEMY:
			type = L_NULL;
			break;
		}
	}
	f >> pos;
	f >> name;
	f >> state;
	if(LOAD_VERSION >= V_0_12)
		f >> target;
	int quest_id = f.Read<int>();
	if(quest_id == -1)
		active_quest = nullptr;
	else if(quest_id == (int)ACTIVE_QUEST_HOLDER)
		active_quest = ACTIVE_QUEST_HOLDER;
	else
	{
		game->load_location_quest.push_back(this);
		active_quest = (Quest_Dungeon*)quest_id;
	}
	f >> last_visit;
	f >> st;
	f >> outside;
	f >> reset;
	f >> group;
	f >> dont_clean;
	f >> seed;
	f >> image;

	// portals
	if(f.Read1())
	{
		Portal* p = new Portal;
		portal = p;
		while(true)
		{
			p->Load(f);
			if(f.Read1())
			{
				Portal* np = new Portal;
				p->next_portal = np;
				p = np;
			}
			else
			{
				p->next_portal = nullptr;
				break;
			}
		}
	}
	else
		portal = nullptr;

	// events
	if(LOAD_VERSION >= V_0_9)
	{
		events.resize(f.Read<uint>());
		for(Event& e : events)
		{
			int quest_id;
			f >> e.type;
			f >> quest_id;
			quest_mgr->AddQuestRequest(quest_id, (Quest**)&e.quest, [&]()
			{
				EventPtr event;
				event.source = EventPtr::LOCATION;
				event.type = e.type;
				event.location = this;
				e.quest->AddEventPtr(event);
			});
		}
	}
}

//=================================================================================================
Portal* Location::GetPortal(int index)
{
	assert(portal && index >= 0);

	Portal* cportal = portal;
	int cindex = 0;

	while(cportal)
	{
		if(index == cindex)
			return cportal;

		++cindex;
		cportal = cportal->next_portal;
	}

	assert(0);
	return nullptr;
}

//=================================================================================================
Portal* Location::TryGetPortal(int index) const
{
	assert(index >= 0);

	Portal* cportal = portal;
	int cindex = 0;

	while(cportal)
	{
		if(index == cindex)
			return cportal;

		++cindex;
		cportal = cportal->next_portal;
	}

	return nullptr;
}

//=================================================================================================
void Location::WritePortals(BitStreamWriter& f) const
{
	// count
	uint count = 0;
	Portal* cportal = portal;
	while(cportal)
	{
		++count;
		cportal = cportal->next_portal;
	}
	f.WriteCasted<byte>(count);

	// portals
	cportal = portal;
	while(cportal)
	{
		f << cportal->pos;
		f << cportal->rot;
		f << (cportal->target_loc != -1);
		cportal = cportal->next_portal;
	}
}

//=================================================================================================
bool Location::ReadPortals(BitStreamReader& f, int at_level)
{
	byte count = f.Read<byte>();
	if(!f.Ensure(count * Portal::MIN_SIZE))
		return false;

	if(portal)
	{
		Portal* p = portal;
		while(p)
		{
			Portal* next = p->next_portal;
			delete p;
			p = next;
		}
		portal = nullptr;
	}

	Portal* cportal = nullptr;
	for(byte i = 0; i < count; ++i)
	{
		Portal* p = new Portal;
		bool active;
		f >> p->pos;
		f >> p->rot;
		f >> active;
		if(!f)
		{
			delete p;
			return false;
		}

		p->target_loc = (active ? 0 : -1);
		p->at_level = at_level;
		p->next_portal = nullptr;

		if(cportal)
		{
			cportal->next_portal = p;
			cportal = p;
		}
		else
			cportal = portal = p;
	}

	return true;
}

//=================================================================================================
void Location::SetKnown()
{
	if(state == LS_UNKNOWN || state == LS_HIDDEN)
	{
		state = LS_KNOWN;
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_LOCATION_STATE;
			c.id = index;
		}
	}
}

//=================================================================================================
void Location::AddEventHandler(Quest_Scripted* quest, EventType type)
{
	assert(Any(type, EVENT_ENTER, EVENT_PICKUP, EVENT_CLEARED, EVENT_GENERATE));

	Event e;
	e.quest = quest;
	e.type = type;
	events.push_back(e);

	EventPtr event;
	event.source = EventPtr::LOCATION;
	event.type = type;
	event.location = this;
	quest->AddEventPtr(event);
}

//=================================================================================================
void Location::RemoveEventHandler(Quest_Scripted* quest, EventType type, bool cleanup)
{
	LoopAndRemove(events, [=](Event& e)
	{
		return e.quest == quest && (type == EVENT_ANY || e.type == type);
	});

	if(!cleanup)
	{
		EventPtr event;
		event.source = EventPtr::LOCATION;
		event.type = type;
		event.location = this;
		quest->RemoveEventPtr(event);
	}
}

//=================================================================================================
// set if passed, returns prev value
bool Location::RequireLoadingResources(bool* to_set)
{
	if(to_set)
	{
		bool result = loaded_resources;
		loaded_resources = *to_set;
		return result;
	}
	else
		return loaded_resources;
}

//=================================================================================================
void Location::SetImage(LOCATION_IMAGE image)
{
	if(this->image != image)
	{
		this->image = image;
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_LOCATION_IMAGE;
			c.id = index;
		}
	}
}

//=================================================================================================
void Location::SetName(cstring name)
{
	if(this->name != name)
	{
		this->name = name;
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_LOCATION_NAME;
			c.id = index;
		}
	}
}

//=================================================================================================
void Location::SetNamePrefix(cstring prefix)
{
	vector<string> strs = Split(name.c_str());
	if(strs[0] == prefix)
		return;
	name = Format("%s %s", prefix, strs[1].c_str());
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_LOCATION_NAME;
		c.id = index;
	}
}
