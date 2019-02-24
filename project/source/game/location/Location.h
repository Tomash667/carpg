#pragma once

//-----------------------------------------------------------------------------
#include "SpawnGroup.h"
#include "LevelContext.h"

//-----------------------------------------------------------------------------
// Location types
//-----------------------------------------------------------------------------
// Changes required after adding new location:
// + World
// + LocationGeneratorFactory
// + Location::GenerateName
// + WorldMapGui::LoadData
// + LevelArea
enum LOCATION
{
	L_CITY, // miasto otoczone kamiennym murem i wie¿yczki, przez œrodek biegnie kamienna droga
	L_VILLAGE_OLD, // removed (left for backward compability)
	L_CAVE, // jaskinia, jakieœ zwierzêta/potwory/ewentualnie bandyci
	L_CAMP, // obóz bandytów/poszukiwaczy przygód/wojska (tymczasowa lokacja)
	L_DUNGEON, // podziemia, ró¿nej g³êbokoœci, posiadaj¹ pomieszczenia o okreœlonym celu (skarbiec, sypialnie itp), zazwyczaj bandyci lub opuszczone
	L_CRYPT, // krypta, ma dwa cele: skarbiec lub wiêzienie dla nieumar³ych, zazwyczaj s¹ tu nieumarli, czasami przy wejœciu bandyci lub potwory
	L_FOREST, // las, zazwyczaj pusty, czasem potwory lub bandyci maj¹ tu ma³y obóz
	L_MOONWELL, // jak las ale ze wzgórzem na œrodku i fontann¹
	L_ENCOUNTER, // losowe spotkanie na drodze
	L_MAX,
	L_NULL
};

//-----------------------------------------------------------------------------
enum LOCATION_IMAGE
{
	LI_CITY,
	LI_VILLAGE,
	LI_CAVE,
	LI_CAMP,
	LI_DUNGEON,
	LI_CRYPT,
	LI_FOREST,
	LI_MOONWELL,
	LI_TOWER,
	LI_LABYRINTH,
	LI_MINE,
	LI_SAWMILL,
	LI_DUNGEON2,
	LI_MAX
};

//-----------------------------------------------------------------------------
enum LOCATION_TOKEN : byte
{
	LT_NULL,
	LT_OUTSIDE,
	LT_CITY,
	LT_VILLAGE_OLD,
	LT_CAVE,
	LT_SINGLE_DUNGEON,
	LT_MULTI_DUNGEON,
	LT_CAMP
};

//-----------------------------------------------------------------------------
// stan lokacji
enum LOCATION_STATE
{
	LS_HIDDEN,
	LS_UNKNOWN,
	LS_KNOWN,
	LS_VISITED,
	LS_ENTERED,
	LS_CLEARED
};

//-----------------------------------------------------------------------------
// przypisanie takiego quest do lokacji spowoduje ¿e nie zostanie zajêta przez inny quest
#define ACTIVE_QUEST_HOLDER 0xFFFFFFFE

//-----------------------------------------------------------------------------
// struktura opisuj¹ca lokacje na mapie œwiata
struct Location : public ILevel
{
	int index;
	LOCATION type; // typ lokacji
	LOCATION_STATE state; // stan lokacji
	Vec2 pos; // pozycja na mapie œwiata
	string name; // nazwa lokacji
	Quest_Dungeon* active_quest; // aktywne zadanie zwi¹zane z t¹ lokacj¹
	int last_visit; // worldtime from last time when team entered location or -1
	int st; // poziom trudnoœci
	uint seed;
	SPAWN_GROUP spawn; // rodzaj wrogów w tej lokacji
	Portal* portal;
	LOCATION_IMAGE image;
	bool reset; // resetowanie lokacji po wejœciu
	bool outside; // czy poziom jest otwarty
	bool dont_clean;
	bool loaded_resources;

	Location(bool outside) : active_quest(nullptr), last_visit(-1), reset(false), state(LS_UNKNOWN), outside(outside), st(0), spawn(SG_NONE),
		portal(nullptr), dont_clean(false), loaded_resources(false)
	{
	}

	virtual ~Location();

	// virtual functions to implement
	virtual void Save(GameWriter& f, bool local);
	virtual void Load(GameReader& f, bool local, LOCATION_TOKEN token);
	virtual void Write(BitStreamWriter& f) = 0;
	virtual bool Read(BitStreamReader& f) = 0;
	virtual void BuildRefidTables() = 0;
	virtual bool FindUnit(Unit* unit, int* level = nullptr) = 0;
	virtual Unit* FindUnit(UnitData* data, int& at_level) = 0;
	virtual bool CheckUpdate(int& days_passed, int worldtime);
	virtual LOCATION_TOKEN GetToken() const { return LT_NULL; }
	virtual int GetRandomLevel() const { return -1; }
	virtual int GetLastLevel() const { return 0; }
	virtual bool RequireLoadingResources(bool* to_set);

	Portal* GetPortal(int index);
	Portal* TryGetPortal(int index) const;
	void WritePortals(BitStreamWriter& f) const;
	bool ReadPortals(BitStreamReader& f, int at_level);
	void SetVisited()
	{
		if(state == LS_KNOWN)
			state = LS_VISITED;
	}
	void SetKnown();
	void SetImage(LOCATION_IMAGE image);
	void SetName(cstring name);
	void SetNamePrefix(cstring prefix);
};

//-----------------------------------------------------------------------------
struct LocationEventHandler
{
	enum Event
	{
		CLEARED
	};

	// return true to prevent adding default news about clearing location
	virtual bool HandleLocationEvent(Event event) = 0;
	virtual int GetLocationEventHandlerQuestRefid() = 0;
};
