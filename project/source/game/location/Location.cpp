#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "SaveState.h"
#include "City.h"
#include "Quest.h"
#include "BitStreamFunc.h"
#include "Portal.h"
#include "GameFile.h"
#include "Quest_Scripted.h"
#include "QuestManager.h"

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
	f << type;
	f << pos;
	f << name;
	f << state;
	int refid;
	if(active_quest)
	{
		if(active_quest == (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
			refid = ACTIVE_QUEST_HOLDER;
		else
			refid = active_quest->refid;
	}
	else
		refid = -1;
	f << refid;
	f << last_visit;
	f << st;
	f << outside;
	f << reset;
	f << spawn;
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
		f << e.quest->refid;
	}
}

//=================================================================================================
void Location::Load(GameReader& f, bool, LOCATION_TOKEN token)
{
	f >> type;
	if(LOAD_VERSION < V_0_5 && type == L_VILLAGE_OLD)
		type = L_CITY;
	f >> pos;
	f >> name;
	f >> state;
	int refid = f.Read<int>();
	if(refid == -1)
		active_quest = nullptr;
	else if(refid == ACTIVE_QUEST_HOLDER)
		active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
	else
	{
		Game::Get().load_location_quest.push_back(this);
		active_quest = (Quest_Dungeon*)refid;
	}
	f >> last_visit;
	f >> st;
	f >> outside;
	f >> reset;
	f >> spawn;
	f >> dont_clean;
	if(LOAD_VERSION >= V_0_3)
		f >> seed;
	else
		seed = 0;
	if(LOAD_VERSION >= V_0_5)
		f >> image;
	else
	{
		switch(type)
		{
		case L_CITY:
			image = LI_CITY;
			break;
		case L_CAVE:
			image = LI_CAVE;
			break;
		case L_CAMP:
			image = LI_CAMP;
			break;
		default:
		case L_DUNGEON:
			image = LI_DUNGEON;
			break;
		case L_CRYPT:
			image = LI_CRYPT;
			break;
		case L_FOREST:
			image = LI_FOREST;
			break;
		case L_MOONWELL:
			image = LI_MOONWELL;
			break;
		}
	}

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
			int refid;
			f >> e.type;
			f >> refid;
			QM.AddQuestRequest(refid, (Quest**)&e.quest, [&]()
			{
				EventPtr event;
				event.source = EventPtr::LOCATION;
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
	Event e;
	e.quest = quest;
	e.type = type;
	events.push_back(e);

	EventPtr event;
	event.source = EventPtr::LOCATION;
	event.location = this;
	quest->AddEventPtr(event);
}

//=================================================================================================
void Location::RemoveEventHandler(Quest_Scripted* quest, bool cleanup)
{
	LoopAndRemove(events, [quest](Event& e)
	{
		return e.quest == quest;
	});

	if(!cleanup)
	{
		EventPtr event;
		event.source = EventPtr::LOCATION;
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
