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
	EVENT_GENERATE,
	EVENT_USE,
	EVENT_TIMER
};

//-----------------------------------------------------------------------------
struct Event
{
	Event() {}
	Event(nullptr_t) : quest(nullptr) {}
	operator bool() const { return quest != nullptr; }

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
	Location* location; // EVENT_CLEARED, EVENT_ENTER, EVENT_GENERATE
	Unit* unit; // EVENT_DIE, EVENT_ENTER, EVENT_PICKUP, EVENT_UPDATE, EVENT_USE
	GroundItem* groundItem; // EVENT_PICKUP
	const Item* item; // EVENT_PICKUP, EVENT_USE
	MapSettings* mapSettings; // EVENT_GENERATE
	int stage; // EVENT_GENERATE
	bool cancel;

	ScriptEvent(EventType type) : type(type), cancel(false) {}
};
