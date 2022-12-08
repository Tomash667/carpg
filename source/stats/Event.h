#pragma once

//-----------------------------------------------------------------------------
enum EventType
{
	EVENT_ANY = -1,
	EVENT_ENTER,
	EVENT_PICKUP,
	EVENT_UPDATE,
	EVENT_TIMEOUT,
	EVENT_ENCOUNTER,
	EVENT_DIE,
	EVENT_CLEARED,
	EVENT_GENERATE
};

//-----------------------------------------------------------------------------
struct Event
{
	EventType type;
	Quest2* quest;
};

//-----------------------------------------------------------------------------
// Used to keep info about all events associated with quest
struct EventPtr
{
	enum Source
	{
		LOCATION,
		UNIT
	};

	Source source;
	EventType type;
	union
	{
		Location* location;
		Unit* unit;
	};

	bool operator == (const EventPtr& e) const
	{
		return source == e.source
			&& location == e.location
			&& (type == e.type || e.type == EVENT_ANY || type == EVENT_ANY);
	}
};

//-----------------------------------------------------------------------------
struct ScriptEvent
{
	EventType type;
	union
	{
		struct OnCleared
		{
			Location* location;
		} onCleared;
		struct OnDie
		{
			Unit* unit;
		} onDie;
		struct OnEncounter
		{
		} onEncounter;
		struct OnEnter
		{
			Location* location;
		} onEnter;
		struct OnGenerate
		{
			Location* location;
			MapSettings* mapSettings;
			int stage;
		} onGenerate;
		struct OnPickup
		{
			Unit* unit;
			GroundItem* item;
		} onPickup;
		struct OnTimeout
		{
		} onTimeout;
		struct OnUpdate
		{
			Unit* unit;
		} onUpdate;
	};
	bool cancel;

	ScriptEvent(EventType type) : type(type), cancel(false) {}
};
