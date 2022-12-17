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
	EVENT_USE
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
	union
	{
		Location* location; // EVENT_CLEARED, EVENT_ENTER, EVENT_GENERATE
		Unit* unit; // EVENT_DIE, EVENT_PICKUP, EVENT_UPDATE, EVENT_USE
	};
	union
	{
		GroundItem* groundItem; // EVENT_PICKUP
		MapSettings* mapSettings; // EVENT_GENERATE
	};
	const Item* item; // EVENT_PICKUP, EVENT_USE
	int stage; // EVENT_GENERATE
	bool cancel;

	ScriptEvent(EventType type) : type(type), cancel(false) {}
};
