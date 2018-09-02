#pragma once

typedef bool(*BoolFunc)();

struct Encounter
{
	Vec2 pos;
	int chance;
	float range;
	bool dont_attack, timed;
	GameDialog* dialog;
	SPAWN_GROUP group;
	cstring text;
	Quest_Encounter* quest; // tak naprawdê nie musi to byæ Quest_Encounter, mo¿e byæ zwyk³y Quest, chyba ¿e jest to czasowy encounter!
	LocationEventHandler* location_event_handler;
	// nowe pola
	BoolFunc check_func;

	// dla kompatybilnoœci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(nullptr)
	{
	}
};
