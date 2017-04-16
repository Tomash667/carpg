#pragma once

typedef bool(*BoolFunc)();

struct Encounter
{
	VEC2 pos;
	int szansa;
	float zasieg;
	bool dont_attack, timed;
	GameDialog* dialog;
	SPAWN_GROUP grupa;
	cstring text;
	Quest_Encounter* quest; // tak naprawd� nie musi to by� Quest_Encounter, mo�e by� zwyk�y Quest, chyba �e jest to czasowy encounter!
	LocationEventHandler* location_event_handler;
	// nowe pola
	BoolFunc check_func;

	// dla kompatybilno�ci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(nullptr)
	{
	}
};
