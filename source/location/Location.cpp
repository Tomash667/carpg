#include "Pch.h"
#include "Game.h"

#include "BitStreamFunc.h"
#include "City.h"
#include "Portal.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Scripted.h"
#include "UnitGroup.h"
#include "World.h"

//=================================================================================================
Location::~Location()
{
	Portal* p = portal;
	while(p)
	{
		Portal* nextPortal = p->nextPortal;
		delete p;
		p = nextPortal;
	}
}

//=================================================================================================
bool Location::CheckUpdate(int& daysPassed, int worldtime)
{
	bool needReset = reset;
	reset = false;
	if(lastVisit == -1)
		daysPassed = -1;
	else
		daysPassed = worldtime - lastVisit;
	lastVisit = worldtime;
	if(dontClean)
		daysPassed = 0;
	return needReset;
}

//=================================================================================================
void Location::Save(GameWriter& f)
{
	f << pos;
	f << name;
	f << state;
	f << target;
	int questId;
	if(activeQuest)
	{
		if(activeQuest == ACTIVE_QUEST_HOLDER)
			questId = (int)ACTIVE_QUEST_HOLDER;
		else
			questId = activeQuest->id;
	}
	else
		questId = -1;
	f << questId;
	f << lastVisit;
	f << st;
	f << outside;
	f << reset;
	f << group;
	f << dontClean;
	f << seed;
	f << image;

	// portals
	Portal* p = portal;
	while(p)
	{
		f.Write1();
		p->Save(f);
		p = p->nextPortal;
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
void Location::Load(GameReader& f)
{
	if(LOAD_VERSION < V_0_12)
	{
		old::LOCATION oldType;
		f >> oldType;
		switch(oldType)
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
	int questId = f.Read<int>();
	if(questId == -1)
		activeQuest = nullptr;
	else if(questId == (int)ACTIVE_QUEST_HOLDER)
		activeQuest = ACTIVE_QUEST_HOLDER;
	else
	{
		game->loadLocationQuest.push_back(this);
		activeQuest = (Quest_Dungeon*)questId;
	}
	f >> lastVisit;
	f >> st;
	f >> outside;
	f >> reset;
	f >> group;
	f >> dontClean;
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
				p->nextPortal = np;
				p = np;
			}
			else
			{
				p->nextPortal = nullptr;
				break;
			}
		}
	}
	else
		portal = nullptr;

	// events
	events.resize(f.Read<uint>());
	for(Event& e : events)
	{
		int questId;
		f >> e.type;
		f >> questId;
		questMgr->AddQuestRequest(questId, (Quest**)&e.quest, [&]()
		{
			EventPtr event;
			event.source = EventPtr::LOCATION;
			event.type = e.type;
			event.location = this;
			e.quest->AddEventPtr(event);
		});
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
		cportal = cportal->nextPortal;
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
		cportal = cportal->nextPortal;
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
		cportal = cportal->nextPortal;
	}
	f.WriteCasted<byte>(count);

	// portals
	cportal = portal;
	while(cportal)
	{
		f << cportal->pos;
		f << cportal->rot;
		f << (cportal->targetLoc != -1);
		cportal = cportal->nextPortal;
	}
}

//=================================================================================================
bool Location::ReadPortals(BitStreamReader& f, int atLevel)
{
	byte count = f.Read<byte>();
	if(!f.Ensure(count * Portal::MIN_SIZE))
		return false;

	if(portal)
	{
		Portal* p = portal;
		while(p)
		{
			Portal* next = p->nextPortal;
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

		p->targetLoc = (active ? 0 : -1);
		p->atLevel = atLevel;
		p->nextPortal = nullptr;

		if(cportal)
		{
			cportal->nextPortal = p;
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
void Location::AddEventHandler(Quest2* quest, EventType type)
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
void Location::RemoveEventHandler(Quest2* quest, EventType type, bool cleanup)
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
void Location::FireEvent(ScriptEvent& e)
{
	if(events.empty())
		return;

	for(Event& event : events)
	{
		if(event.type == e.type)
			event.quest->FireEvent(e);
	}
}

//=================================================================================================
// set if passed, returns prev value
bool Location::RequireLoadingResources(bool* toSet)
{
	if(toSet)
	{
		bool result = loadedResources;
		loadedResources = *toSet;
		return result;
	}
	else
		return loadedResources;
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

//=================================================================================================
void operator >> (GameReader& f, Location*& loc)
{
	int index = f.Read<int>();
	if(index == -1)
		loc = nullptr;
	else
		loc = world->GetLocation(index);
}
