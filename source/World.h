#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "Location.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
struct EncounterData
{
	EncounterMode mode;
	union
	{
		Encounter* encounter;
		UnitGroup* group;
		SpecialEncounter special;
	};
	int st;
};

//-----------------------------------------------------------------------------
// World handling
// General notes:
// + don't create settlements after GenerateWorld (currently it is hardcoded that they are before other locations - here and in Quest_SpreadNews)
// + currently only camps can be removed
class World : public GameComponent
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

	enum UpdateMode
	{
		UM_NORMAL,
		UM_TRAVEL,
		UM_SKIP
	};

	static const float TRAVEL_SPEED;
	static const float MAP_KM_RATIO;
	static const int TILE_SIZE = 30;

	// general
	void LoadLanguage() override;
	void PostInit() override;
	void Cleanup() override;
	void OnNewGame();
	void Reset();
	void Update(int days, UpdateMode mode);
	void ExitToMap();
	void ChangeLevel(int index, bool encounter);
	void StartInLocation(Location* loc);
	void Warp(int index);
	void WarpPos(const Vec2& pos);
	void Reveal();

	// save/load
	void Save(GameWriter& f);
	void Load(GameReader& f, LoadingHandler& loading);
	void LoadOld(GameReader& f, LoadingHandler& loading, int part, bool inside);
	void Write(BitStreamWriter& f);
	void WriteTime(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void ReadTime(BitStreamReader& f);

	// world generation
	void GenerateWorld(int start_location_type = -1, int start_location_target = -1);
	void CalculateTiles();
	void SmoothTiles();
	void CreateCity(const Vec2& pos, bool village);
	void SetLocationImageAndName(Location* l);
	int CreateCamp(const Vec2& pos, UnitGroup* group, float range = 64.f, bool allow_exact = true);
private:
	typedef pair<LOCATION, bool>(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist);
	Location* CreateLocation(LOCATION type, int levels = -1, bool is_village = false);
public:
	Location* CreateLocation(LOCATION type, const Vec2& pos, float range = 64.f, int target = -1, UnitGroup* group = UnitGroup::random,
		bool allow_exact = true, int dungeon_levels = -1);
	int AddLocation(Location* loc);
	void AddLocationAtIndex(Location* loc);
	void RemoveLocation(int index);
	bool VerifyLocation(int index) const { return index >= 0 && index < (int)locations.size() && locations[index]; }
	void DeleteCamp(Camp* camp, bool remove = true);
	void AbadonLocation(Location* loc);

	// datetime
	bool IsSameWeek(int worldtime2) const;
	cstring GetDate() const;
	cstring GetDate(int year, int month, int day) const;
	int GetDay() const { return day; }
	int GetMonth() const { return month; }
	int GetYear() const { return year; }
	int GetWorldtime() const { return worldtime; }

	// world state
	State GetState() const { return state; }
	void SetState(State state) { this->state = state; }
	Location* GetCurrentLocation() const { return current_location; }
	int GetCurrentLocationIndex() const { return current_location_index; }
	const vector<Location*>& GetLocations() const { return locations; }
	Location* GetLocation(int index) const { assert(index >= 0 && index < (int)locations.size()); return locations[index]; }
	const Vec2& GetWorldPos() const { return world_pos; }
	const Vec2& GetTargetPos() const { return travel_target_pos; }
	void SetWorldPos(const Vec2& world_pos) { this->world_pos = world_pos; }
	uint GetSettlements() { return settlements; }
	int GetRandomSettlementIndex(int excluded = -1) const;
	int GetRandomSettlementIndex(const vector<int>& used, int type = 0) const;
	Location* GetRandomSettlement(int excluded = -1) const { return locations[GetRandomSettlementIndex(excluded)]; }
	int GetRandomFreeSettlementIndex(int excluded = -1) const;
	int GetRandomCityIndex(int excluded = -1) const;
	int GetClosestLocation(LOCATION type, const Vec2& pos, int target = -1);
	Location* GetClosestLocationS(LOCATION type, const Vec2& pos, int target = -1) { return locations[GetClosestLocation(type, pos, target)]; }
	int GetClosestLocationNotTarget(LOCATION type, const Vec2& pos, int not_target);
	bool FindPlaceForLocation(Vec2& pos, float range = 64.f, bool allow_exact = true);
	int GetRandomSpawnLocation(const Vec2& pos, UnitGroup* group, float range = 160.f);
	int GetNearestLocation(const Vec2& pos, int flags, bool not_quest, int target_flags = -1);
	int GetNearestSettlement(const Vec2& pos) { return GetNearestLocation(pos, (1 << L_CITY), false); }
	const Vec2& GetWorldBounds() const { return world_bounds; }
	City* GetRandomSettlement(delegate<bool(City*)> pred);
	Location* GetRandomSettlement(Location* loc);
	Location* GetRandomSettlementWeighted(delegate<float(Location*)> func);

	// travel
	void Travel(int index, bool send);
	void TravelPos(const Vec2& pos, bool send);
	void UpdateTravel(float dt);
	void StopTravel(const Vec2& pos, bool send);
	void EndTravel();
	int GetTravelLocationIndex() const { return travel_location_index; }
	float GetTravelDir() const { return travel_dir; }
	void SetTravelDir(const Vec3& pos);
	void GetOutsideSpawnPoint(Vec3& pos, float& dir) const;
	float GetTravelDays(float dist);

	// encounters
	void StartEncounter();
	Encounter* AddEncounter(int& index, Quest* quest = nullptr);
	Encounter* AddEncounterS(Quest* quest);
	void RemoveEncounter(int index);
	void RemoveEncounter(Quest* quest);
	Encounter* GetEncounter(int index);
	Encounter* RecreateEncounter(int index);
	const vector<Encounter*>& GetEncounters() const { return encounters; }
	int GetEncounterLocationIndex() const { return encounter_loc; }
	float GetEncounterChance() const { return encounter_chance; }
	EncounterData GetCurrentEncounter() const { return encounter; }

	// news
	void AddNews(cstring text);
	const vector<News*>& GetNews() const { return news; }

	// boss levels
	void AddBossLevel(const Int2& pos = Int2::Zero);
	bool RemoveBossLevel(const Int2& pos = Int2::Zero);
	bool IsBossLevel(const Int2& pos = Int2::Zero) const;

	// misc
	bool CheckFirstCity();
	int FindWorldUnit(Unit* unit, int hint_loc = -1, int hint_loc2 = -1, int* level = nullptr);
	void VerifyObjects();
	void VerifyObjects(vector<Object*>& objects, int& errors);
	const vector<int>& GetTiles() const { return tiles; }
	int& GetTileSt(const Vec2& pos);

private:
	WorldMapGui* gui;
	State state;
	Location* current_location; // current location or nullptr
	int current_location_index; // current location index or -1
	int travel_location_index; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (-1 otherwise)
	vector<Location*> locations; // can be nullptr
	vector<Encounter*> encounters;
	EncounterData encounter;
	vector<int> tiles;
	vector<Int2> boss_levels; // levels with boss music (x-location index, y-dungeon level)
	uint settlements, // count and index below this value is city/village
		empty_locations, // counter
		encounter_loc, // encounter location index
		create_camp; // counter to create new random camps
	Vec2 world_bounds,
		world_pos,
		travel_start_pos,
		travel_target_pos;
	int world_size;
	float travel_timer,
		day_timer,
		reveal_timer, // increase chance for encounter every 0.25 sec
		encounter_chance,
		travel_dir; // from which direction team will enter level after travel
	EncounterMode encounter_mode;
	int year, // in game year, starts at 100
		month, // in game month, 0 to 11
		day, // in game day, 0 to 29
		worldtime; // number of passed game days, starts at 0
	vector<News*> news;
	cstring txDate, txEncCrazyMage, txEncCrazyHeroes, txEncCrazyCook, txEncMerchant, txEncHeroes, txEncSingleHero, txEncBanditsAttackTravelers,
		txEncHeroesAttack, txEncGolem, txEncCrazy, txEncUnk, txEncEnemiesCombat;
	cstring txCamp, txCave, txCity, txCrypt, txDungeon, txForest, txVillage, txMoonwell, txOtherness, txRandomEncounter, txTower, txLabyrinth;
	bool first_city, // spawn more low level heroes in first city
		boss_level_mp, // used by clients instead boss_levels
		tomir_spawned,
		travel_first_frame;

	void UpdateDate(int days);
	void SpawnCamps(int days);
	void UpdateEncounters();
	void UpdateLocations();
	void UpdateNews();
	void StartEncounter(int enc, UnitGroup* group);
	void LoadLocations(GameReader& f, LoadingHandler& loading);
	void LoadNews(GameReader& f);
};
extern World W;
