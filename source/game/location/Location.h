#pragma once

//-----------------------------------------------------------------------------
#include "SpawnGroup.h"
#include "LevelContext.h"

//-----------------------------------------------------------------------------
// rodzaj lokacji
//-----------------------------------------------------------------------------
// po dodaniu nowej lokacji trzeba w wielu miejscach pozmieniaæ coœ, tu jest ich lista:
// RebuildMinimap
// CreateLocation
// WorldMapGui::LoadData
// EnterLocation
// Location::GenerateName
// DrawMinimap
// LevelAreaContext* Game::ForLevel(int loc, int level)
// -- niekompletna --
enum LOCATION
{
	L_CITY, // miasto otoczone kamiennym murem i wie¿yczki, przez œrodek biegnie kamienna droga, 
	L_VILLAGE, // wioska otoczona palisad¹, drewniane wie¿e, udeptana droga
	L_CAVE, // jaskinia, jakieœ zwierzêta/potwory/ewentualnie bandyci
	L_CAMP, // obóz bandytów/poszukiwaczy przygód/wojska (tymczasowa lokacja)
	L_DUNGEON, // podziemia, ró¿nej g³êbokoœci, posiadaj¹ pomieszczenia o okreœlonym celu (skarbiec, sypialnie itp), zazwyczaj bandyci lub opuszczone
	L_CRYPT, // krypta, ma dwa cele: skarbiec lub wiêzienie dla nieumar³ych, zazwyczaj s¹ tu nieumarli, czasami przy wejœciu bandyci lub potwory
	L_FOREST, // las, zazwyczaj pusty, czasem potwory lub bandyci maj¹ tu ma³y obóz
	L_MOONWELL, // jak las ale ze wzgórzem na œrodku i fontann¹
	L_ENCOUNTER, // losowe spotkanie na drodze
	L_ACADEMY,
	L_MAX,
	L_NULL
};

//-----------------------------------------------------------------------------
enum LOCATION_TOKEN : byte
{
	LT_NULL,
	LT_OUTSIDE,
	LT_CITY,
	LT_VILLAGE,
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
struct Quest_Dungeon;
struct Location;

//-----------------------------------------------------------------------------
struct Portal
{
	VEC3 pos;
	float rot;
	int at_level;
	int target; // portal wyjœciowy
	int target_loc; // docelowa lokacja
	Portal* next_portal;

	static const int MIN_SIZE = 17;

	void Save(HANDLE file);
	void Load(Location* loc, HANDLE file);

	inline VEC3 GetSpawnPos() const
	{
		return pos + VEC3(sin(rot)*2, 0, cos(rot)*2);
	}
};

//-----------------------------------------------------------------------------
// przypisanie takiego quest do lokacji spowoduje ¿e nie zostanie zajêta przez inny quest
#define ACTIVE_QUEST_HOLDER 0xFFFFFFFE

//-----------------------------------------------------------------------------
// struktura opisuj¹ca lokacje na mapie œwiata
struct Location : public ILevel
{
	LOCATION type; // typ lokacji
	LOCATION_STATE state; // stan lokacji
	VEC2 pos; // pozycja na mapie œwiata
	string name; // nazwa lokacji
	Quest_Dungeon* active_quest; // aktywne zadanie zwi¹zane z t¹ lokacj¹
	int last_visit; // czas ostatniej wizyty
	int st; // poziom trudnoœci
	uint seed;
	SPAWN_GROUP spawn; // rodzaj wrogów w tej lokacji
	Portal* portal;
	bool reset; // resetowanie lokacji po wejœciu
	bool outside; // czy poziom jest otwarty
	bool dont_clean;

	explicit Location(bool outside) : active_quest(nullptr), last_visit(-1), reset(false), state(LS_UNKNOWN), outside(outside), st(0), spawn(SG_BRAK), portal(nullptr), dont_clean(false)
	{

	}

	virtual ~Location()
	{
		Portal* p = portal;
		while(p)
		{
			Portal* next_portal = p->next_portal;
			delete p;
			p = next_portal;
		}
	}

	void GenerateName();
	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);

	virtual bool CheckUpdate(int& days_passed, int worldtime)
	{
		bool need_reset = reset;
		reset = false;
		if(last_visit == -1)
			days_passed = -1;
		else
			days_passed = worldtime - last_visit;
		last_visit = worldtime;
		if(dont_clean)
			days_passed = 0;
		return need_reset;
	}

	virtual int GetRandomLevel() const
	{
		return -1;
	}
	virtual int GetLastLevel() const
	{
		return 0;
	}

	virtual void BuildRefidTable() = 0;
	virtual bool FindUnit(Unit* unit, int* level = nullptr) = 0;
	virtual Unit* FindUnit(UnitData* data, int& at_level) = 0;

	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_NULL;
	}
	Portal* GetPortal(int index);
	Portal* TryGetPortal(int index) const;
	void WritePortals(BitStream& stream) const;
	bool ReadPortals(BitStream& stream, int at_level);

	inline bool IsSingleLevel() const
	{
		return type != L_DUNGEON && type != L_CRYPT;
	}
	inline bool IsCityVillage() const
	{
		return type == L_CITY || type == L_VILLAGE;
	}
};

//-----------------------------------------------------------------------------
struct LocationEventHandler
{
	enum Event
	{
		CLEARED
	};

	virtual void HandleLocationEvent(Event event) = 0;
	virtual int GetLocationEventHandlerQuestRefid() = 0;
};

//-----------------------------------------------------------------------------
void SetLocationNames();
