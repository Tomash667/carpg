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
	EVENT_TIMER,
	EVENT_DESTROY,
	EVENT_RECRUIT,
	EVENT_KICK
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
		UNIT,
		USABLE
	};

	Source source;
	EventType type;
	union
	{
		Location* location;
		Unit* unit;
		Usable* usable;
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
		struct
		{
			Location* location;
		} onCleared;
		struct
		{
			Usable* usable;
		} onDestroy;
		struct
		{
			Unit* unit;
		} onDie;
		struct
		{
			Location* location;
			Unit* unit;
		} onEnter;
		struct
		{
			Location* location;
			MapSettings* mapSettings;
			int stage;
			bool cancel;
		} onGenerate;
		struct
		{
			Unit* unit;
		} onKick;
		struct
		{
			Unit* unit;
			GroundItem* groundItem;
			const Item* item;
		} onPickup;
		struct
		{
			Unit* unit;
		} onRecruit;
		struct
		{
			Unit* unit;
		} onUpdate;
		struct
		{
			Unit* unit;
			const Item* item;
		} onUse;
	};

	ScriptEvent(EventType type) : type(type) {}
};
