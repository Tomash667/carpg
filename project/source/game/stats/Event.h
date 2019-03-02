#pragma once

enum EventType
{
	EVENT_ENTER,
	EVENT_PICKUP,
	EVENT_UPDATE,
	EVENT_TIMEOUT,
	EVENT_ENCOUNTER
};

struct Event
{
	EventType type;
	Quest_Scripted* quest;
};
