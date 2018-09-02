#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"

//-----------------------------------------------------------------------------
class World
{
	friend class WorldMapGui;
public:
	enum class State
	{
		ON_MAP, // on map (current_location set)
		INSIDE_LOCATION, // inside location (current_location set)
		INSIDE_ENCOUNTER, // inside encounter location (current_location set)
		TRAVEL, // traveling on map (current_location is nullptr)
		ENCOUNTER // shown encounter message, waiting to close & load level (current_location is nullptr)
	};

	static const float TRAVEL_SPEED;

	void Init();
	void Cleanup();
	void OnNewGame();
	void Reset();
	void Update(int days);
	void DoWorldProgress(int days);
	void GenerateWorld(int start_location_type = -1, int start_location_target = -1);
	void ExitToMap();
	bool ChangeLevel(int index, bool encounter);
	void StartInLocation(Location* loc);
	void StartEncounter();
	void Travel(int index);
	void EndTravel();
	void Warp(int index);

	// encounters
	Encounter* AddEncounter(int& index);
	void RemoveEncounter(int index);
	Encounter* GetEncounter(int index);
	Encounter* RecreateEncounter(int index);
	void RemoveTimedEncounters();
	const vector<Encounter*>& GetEncounters() const { return encounters; }

	int CreateCamp(const Vec2& pos, SPAWN_GROUP group, float range = 64.f, bool allow_exact = true);
	Location* CreateLocation(LOCATION type, int levels = -1, bool is_village = false);
	typedef std::pair<LOCATION, bool>(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist, bool unique_name);
	int AddLocation(Location* loc);
	void RemoveLocation(Location* loc);

	void Save(GameWriter& f);
	void Load(GameReader& f, LoadingHandler& loading);
	void LoadOld(GameReader& f, LoadingHandler& loading, int part);
	void Write(BitStreamWriter& f);
	void WriteTime(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void ReadTime(BitStreamReader& f);

	bool IsSameWeek(int worldtime2) const;
	cstring GetDate() const;
	int GetDay() const { return day; }
	int GetMonth() const { return month; }
	int GetRandomSettlementIndex(int excluded = -1) const;
	// get random 0-settlement, 1-city, 2-village excluded from used list
	int GetRandomSettlementIndex(const vector<int>& used, int type = 0) const;
	Location* GetRandomSettlement(int excluded = -1) const  { return locations[GetRandomSettlementIndex(excluded)]; }
	int GetRandomFreeSettlementIndex(int excluded = -1) const;
	int GetRandomCityIndex(int excluded = -1) const;
	int GetClosestLocation(LOCATION type, const Vec2& pos, int target = -1);
	int GetClosestLocationNotTarget(LOCATION type, const Vec2& pos, int not_target);
	bool FindPlaceForLocation(Vec2& pos, float range = 64.f, bool allow_exact = true);
	int GetEncounterLocationIndex() const { return encounter_loc; } // FIXME remove?
	int GetWorldtime() const { return worldtime; }
	int GetYear() const { return year; }
	State GetState() const { return state; }
	const Vec2& GetWorldPos() const { return world_pos; }
	float GetEncounterChance() const { return encounter_chance; }
	float GetTravelDir() const { return travel_dir; }
	void GetOutsideSpawnPoint(Vec3& pos, float& dir) const;
	void SetTravelDir(const Vec3& pos);

	void SetState(State state) { this->state = state; }
	void SetWorldPos(const Vec2& world_pos) { this->world_pos = world_pos; }

private:
	State state;
public: // FIXME
	Location* current_location; // wskaŸnik na aktualn¹ lokacjê [odtwarzany]
	int current_location_index, // current location index or -1
		travel_location_index; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (-1 otherwise)
	vector<Location*> locations; // can be nullptr
private:
	vector<Encounter*> encounters;
public:
	vector<Int2> boss_levels; // levels with boss music (x-location index, y-dungeon level)
	uint settlements; // count and index below this value is city/village
	private:
	uint
		empty_locations; // counter
public:
	uint	encounter_loc, // encounter location index
		create_camp, // counter to create new random camps
		travel_day;
private:
	Vec2 world_pos,
		travel_start_pos;
	float travel_timer,
		encounter_timer, // increase chance for encounter every 0.25 sec
		encounter_chance,
		travel_dir; // from which direction team will enter level after travel
public:
	bool first_city; // spawn more low level heroes in first city
private:
	int year; // in game year, starts at 100
	int month; // in game month, 0 to 11
	int day; // in game day, 0 to 29
	int worldtime; // number of passed game days, starts at 0
	cstring txDate, txRandomEncounter;

	void LoadLocations(GameReader& f, LoadingHandler& loading);
};
extern World W;
