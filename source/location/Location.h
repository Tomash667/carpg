#pragma once

//-----------------------------------------------------------------------------
#include "Event.h"

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
	L_NULL,
	L_CITY,
	L_CAVE,
	L_CAMP,
	L_DUNGEON,
	L_OUTSIDE,
	L_ENCOUNTER,
	L_OFFSCREEN
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
	LI_ACADEMY,
	LI_CAPITAL,
	LI_HUNTERS_CAMP,
	LI_HILLS,
	LI_MAX
};

//-----------------------------------------------------------------------------
namespace old
{
	enum LOCATION
	{
		L_CITY,
		L_VILLAGE_OLD,
		L_CAVE,
		L_CAMP,
		L_DUNGEON,
		L_CRYPT,
		L_FOREST,
		L_MOONWELL,
		L_ENCOUNTER,
		L_ACADEMY,
		L_NULL
	};

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
};

//-----------------------------------------------------------------------------
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
// used to prevent other quest from using this location
static Quest* const ACTIVE_QUEST_HOLDER = (Quest*)0xFFFFFFFE;
static constexpr int ANY_TARGET = -1;

//-----------------------------------------------------------------------------
struct Location
{
	int index;
	LOCATION type;
	LOCATION_STATE state;
	int target;
	Vec2 pos;
	string name;
	Quest* active_quest;
	int last_visit; // worldtime from last time when team entered location or -1
	int st;
	uint seed;
	UnitGroup* group; // cannot be null (use UnitGroup::empty)
	Portal* portal;
	LOCATION_IMAGE image;
	vector<Event> events;
	bool reset; // force reset location when entering
	bool outside; // czy poziom jest otwarty
	bool dont_clean;
	bool loaded_resources;

	Location(bool outside) : active_quest(nullptr), last_visit(-1), reset(false), state(LS_UNKNOWN), outside(outside), st(0), group(nullptr),
		portal(nullptr), dont_clean(false), loaded_resources(false), target(0)
	{
	}

	virtual ~Location();

	// virtual functions to implement
	virtual void Apply(vector<std::reference_wrapper<LocationPart>>& parts) = 0;
	virtual void Save(GameWriter& f);
	virtual void Load(GameReader& f);
	virtual void Write(BitStreamWriter& f) = 0;
	virtual bool Read(BitStreamReader& f) = 0;
	virtual bool CheckUpdate(int& days_passed, int worldtime);
	virtual int GetRandomLevel() const { return -1; }
	virtual int GetLastLevel() const { return 0; }
	virtual bool RequireLoadingResources(bool* to_set);

	Portal* GetPortal(int index);
	Portal* TryGetPortal(int index) const;
	void WritePortals(BitStreamWriter& f) const;
	bool ReadPortals(BitStreamReader& f, int at_level);
	LOCATION_IMAGE GetImage() const { return image; }
	const string& GetName() const { return name; }
	void SetVisited()
	{
		if(state == LS_KNOWN)
			state = LS_VISITED;
	}
	void SetKnown();
	void SetImage(LOCATION_IMAGE image);
	void SetName(cstring name);
	void SetNameS(const string& name) { SetName(name.c_str()); }
	void SetNamePrefix(cstring prefix);
	void AddEventHandler(Quest2* quest, EventType type);
	void RemoveEventHandler(Quest2* quest, EventType type, bool cleanup = false);
	void RemoveEventHandlerS(Quest2* quest, EventType type) { RemoveEventHandler(quest, type, false); }
	bool IsVisited() const { return last_visit != -1; }
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
	virtual int GetLocationEventHandlerQuestId() = 0;
};

//-----------------------------------------------------------------------------
inline void operator << (GameWriter& f, Location* loc)
{
	int index = loc ? loc->index : -1;
	f << index;
}
void operator >> (GameReader& f, Location*& loc);
