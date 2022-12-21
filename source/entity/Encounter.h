#pragma once

//-----------------------------------------------------------------------------
typedef bool(*BoolFunc)();

//-----------------------------------------------------------------------------
enum EncounterMode
{
	ENCOUNTER_COMBAT,
	ENCOUNTER_SPECIAL,
	ENCOUNTER_QUEST,
	ENCOUNTER_GLOBAL
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
	SE_CRAZY_COOK = SE_MAX_NORMAL,
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
	string* pooledString;
	LocationEventHandler* locationEventHandler;
	BoolFunc checkFunc;
	int st; // when -1 use world st
	bool dontAttack, timed, scripted;

	// dla kompatybilnoœci ze starym kodem, ustawia tylko nowe pola
	Encounter() : checkFunc(nullptr), st(-1), pooledString(nullptr), scripted(false) {}
	Encounter(Quest* quest) : pos(Vec2::Zero), chance(100), range(64.f), dontAttack(false), timed(false), dialog(nullptr), group(nullptr), text(nullptr),
		pooledString(nullptr), quest(quest), locationEventHandler(nullptr), checkFunc(nullptr), st(-1), scripted(true) {}
	~Encounter();
	const string& GetTextS();
	void SetTextS(const string& str);
};

//-----------------------------------------------------------------------------
struct GlobalEncounter
{
	typedef delegate<void(EncounterSpawn&)> Callback;

	Quest* quest;
	Callback callback;
	int chance;
	cstring text;
};

//-----------------------------------------------------------------------------
struct EncounterSpawn
{
	cstring groupName, groupName2;
	UnitData* essential;
	GameDialog* dialog;
	int count, count2, level, level2;
	bool dontAttack, backAttack, farEncounter, isTeam, isTeam2;

	EncounterSpawn(int st) : groupName(nullptr), groupName2(nullptr), essential(nullptr), dialog(nullptr), count(0), count2(0), level(st), level2(st),
		dontAttack(false), backAttack(false), farEncounter(false), isTeam(false), isTeam2(false) {}
};
