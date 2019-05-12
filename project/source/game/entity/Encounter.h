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
	int index, chance;
	float range;
	Quest* quest;
	GameDialog* dialog;
	UnitGroup* group;
	cstring text;
	string* pooled_string;
	LocationEventHandler* location_event_handler;
	BoolFunc check_func;
	int st; // when -1 use world st
	bool dont_attack, timed, scripted;

	// dla kompatybilnoœci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(nullptr), st(-1), pooled_string(nullptr), scripted(false) {}
	Encounter(Quest* quest) : pos(Vec2::Zero), chance(100), range(64.f), dont_attack(false), timed(false), dialog(nullptr), group(nullptr), text(nullptr),
		pooled_string(nullptr), quest(quest), location_event_handler(nullptr), check_func(nullptr), st(-1), scripted(true) {}
	~Encounter();
	const string& GetTextS();
	void SetTextS(const string& str);
};
