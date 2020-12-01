#pragma once

//-----------------------------------------------------------------------------
enum EventType
{
	EVENT_ANY = -1,
	EVENT_ENTER = 1 << 0,
	EVENT_PICKUP = 1 << 1,
	EVENT_UPDATE = 1 << 2,
	EVENT_TIMEOUT = 1 << 3,
	EVENT_ENCOUNTER = 1 << 4,
	EVENT_DIE = 1 << 5,
	EVENT_CLEARED = 1 << 6,
	EVENT_GENERATE = 1 << 7,
	EVENT_RECRUIT = 1 << 8,
	EVENT_KICK = 1 << 9,
	EVENT_LEAVE = 1 << 10
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
		} on_cleared;
		struct OnDie
		{
			Unit* unit;
		} on_die;
		struct OnEncounter
		{
		} on_encounter;
		struct OnEnter
		{
			Location* location;
		} on_enter;
		struct OnGenerate
		{
			Location* location;
			MapSettings* map_settings;
			int stage;
		} on_generate;
		struct OnPickup
		{
			Unit* unit;
			GroundItem* item;
		} on_pickup;
		struct OnTimeout
		{
		} on_timeout;
		struct OnUpdate
		{
			Unit* unit;
		} on_update;
	};
	bool cancel;

	ScriptEvent(EventType type) : type(type), cancel(false) {}
};
