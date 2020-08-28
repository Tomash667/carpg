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
