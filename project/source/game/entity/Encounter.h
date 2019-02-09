#pragma once

//-----------------------------------------------------------------------------
typedef bool(*BoolFunc)();

//-----------------------------------------------------------------------------
enum EncounterMode
{
	ENCOUNTER_COMBAT,
	ENCOUNTER_SPECIAL,
	ENCOUNTER_QUEST
};

//-----------------------------------------------------------------------------
enum SpecialEncounter
{
	SE_CRAZY_MAGE,
	SE_CRAZY_HEROES,
	SE_MERCHANT,
	SE_HEROES,
	SE_BANDITS_VS_TRAVELERS,
	SE_HEROES_VS_ENEMIES,
	SE_ENEMIES_COMBAT,
	SE_MAX_NORMAL,
	SE_GOLEM = SE_MAX_NORMAL,
	SE_CRAZY,
	SE_UNK,
	SE_CRAZY_COOK,
	SE_TOMIR
};

//-----------------------------------------------------------------------------
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
	int st; // when -1 use world st

	// dla kompatybilnoœci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(nullptr), st(-1)
	{
	}
};
