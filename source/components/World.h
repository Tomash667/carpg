#pragma once

//-----------------------------------------------------------------------------
#include "Date.h"
#include "GameCommon.h"
#include "Location.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
struct EncounterData
{
	EncounterMode mode;
	union
	{
		Encounter* encounter;
		GlobalEncounter* global;
		UnitGroup* group;
		SpecialEncounter special;
	};
	int st;
};

//-----------------------------------------------------------------------------
enum GetLocationFlag
{
	F_ALLOW_ACTIVE = 1 << 0,
	F_EXCLUDED = 1 << 1
};

//-----------------------------------------------------------------------------
// World handling
// General notes:
// + don't create settlements after GenerateWorld (currently it is hardcoded that they are before other locations - here and in Quest_SpreadNews)
// + currently only camps can be removed
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
	static const float MAP_KM_RATIO;
	static const int TILE_SIZE = 30;

	// general
	void LoadLanguage();
	void Init();
	void Cleanup();
	void OnNewGame();
	void Reset();
	void Update(int days, UpdateMode mode);
	void ExitToMap();
	void ChangeLevel(int index, bool encounter);
	void StartInLocation(Location* loc);
	void Warp(int index, bool order);
	void WarpPos(const Vec2& pos, bool order);
	void Reveal();

	// save/load
	void Save(GameWriter& f);
	void Load(GameReader& f, LoadingHandler& loading);
	void Write(BitStreamWriter& f);
	void WriteTime(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void ReadTime(BitStreamReader& f);

	// world generation
	void GenerateWorld();
	void SetStartLocation(Location* loc) { start_location = loc; }
	void StartInLocation();
	void CalculateTiles();
	void SmoothTiles();
	void CreateCity(const Vec2& pos, int target);
	void SetLocationImageAndName(Location* l);
	Location* CreateCamp(const Vec2& pos, UnitGroup* group);
private:
	typedef LOCATION(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist);
	Location* CreateLocation(LOCATION type, int levels = -1, int city_target = -1);
public:
	Location* CreateLocation(LOCATION type, const Vec2& pos, int target = -1, int dungeon_levels = -1);
	int AddLocation(Location* loc);
	void AddLocationAtIndex(Location* loc);
	void RemoveLocation(int index);
	bool VerifyLocation(int index) const { return index >= 0 && index < (int)locations.size() && locations[index]; }
	void DeleteCamp(Camp* camp, bool remove = true);
	void AbadonLocation(Location* loc);

	// datetime
	bool IsSameWeek(int worldtime2) const;
	cstring GetDate() const;
	cstring GetDate(const Date& date) const;
	int GetWorldtime() const { return worldtime; }
	const Date& GetDateValue() const { return date; }
	const Date& GetStartDate() const { return startDate; }
	void SetStartDate(const Date& date);

	// world state
	State GetState() const { return state; }
	void SetState(State state) { this->state = state; }
	Location* GetCurrentLocation() const { return current_location; }
	int GetCurrentLocationIndex() const { return current_location_index; }
	const vector<Location*>& GetLocations() const { return locations; }
	Location* GetLocation(int index) const { assert(index >= 0 && index < (int)locations.size()); return locations[index]; }
	Location* GetLocationByType(LOCATION type, int target = ANY_TARGET) const;
	const Vec2& GetWorldPos() const { return world_pos; }
	const Vec2& GetTargetPos() const { return travel_target_pos; }
	void SetWorldPos(const Vec2& world_pos) { this->world_pos = world_pos; }
	uint GetSettlements() { return settlements; }
	Location* GetRandomSettlement(Location* excluded = nullptr) const;
	Location* GetRandomSettlement(vector<Location*>& used, int target = ANY_TARGET) const;
	Location* GetRandomFreeSettlement(Location* excluded = nullptr) const;
	Location* GetRandomCity(Location* excluded = nullptr) const;
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, int target = ANY_TARGET, int flags = 0);
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, const int* targets, int n_targets, int flags = 0);
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, std::initializer_list<int> const& targets, int flags = 0)
	{
		return GetClosestLocation(type, pos, targets.begin(), targets.size(), flags);
	}
	Location* GetClosestLocationArrayS(LOCATION type, const Vec2& pos, CScriptArray* array);
	bool TryFindPlace(Vec2& pos, float range, bool allow_exact = false);
	Vec2 FindPlace(const Vec2& pos, float range, bool allow_exact = false);
	Vec2 FindPlace(const Vec2& pos, float min_range, float max_range);
	Vec2 GetRandomPlace();
	Location* GetRandomSpawnLocation(const Vec2& pos, UnitGroup* group, float range = 160.f);
	Location* GetNearestSettlement(const Vec2& pos) { return GetClosestLocation(L_CITY, pos); }
	City* GetRandomSettlement(delegate<bool(City*)> pred);
	Location* GetRandomSettlementWeighted(delegate<float(Location*)> func);
	Location* GetRandomLocation(delegate<bool(Location*)> pred);
	Vec2 GetSize() const { return Vec2((float)world_size, (float)world_size); }
	Vec2 GetPos() const { return world_pos; }
	Vec2 GetWorldBounds() const { return world_bounds; }

	// travel
	void Travel(int index, bool order);
	void TravelPos(const Vec2& pos, bool order);
	void UpdateTravel(float dt);
	void StopTravel(const Vec2& pos, bool send);
	void EndTravel();
	Location* GetTravelLocation() const { return travel_location; }
	float GetTravelDir() const { return travel_dir; }
	void SetTravelDir(const Vec3& pos);
	void GetOutsideSpawnPoint(Vec3& pos, float& dir) const;
	float GetTravelDays(float dist);

	// encounters
	void StartEncounter();
	Encounter* AddEncounter(int& index, Quest* quest = nullptr);
	Encounter* AddEncounterS(Quest* quest);
	void AddGlobalEncounter(GlobalEncounter* globalEnc) { globalEncounters.push_back(globalEnc); }
	void RemoveEncounter(int index);
	void RemoveEncounter(Quest* quest);
	void RemoveGlobalEncounter(Quest* quest);
	Encounter* GetEncounter(int index);
	Encounter* RecreateEncounter(int index);
	Encounter* RecreateEncounterS(Quest* quest, int index);
	const vector<Encounter*>& GetEncounters() const { return encounters; }
	OutsideLocation* GetEncounterLocation() const { return encounter_loc; }
	float GetEncounterChance() const { return encounter_chance; }
	EncounterData GetCurrentEncounter() const { return encounter; }

	// news
	void AddNews(cstring text);
	void AddNewsS(const string& tex) { AddNews(tex.c_str()); }
	const vector<News*>& GetNews() const { return news; }

	// misc
	void VerifyObjects();
	void VerifyObjects(vector<Object*>& objects, int& errors);
	const vector<int>& GetTiles() const { return tiles; }
	int& GetTileSt(const Vec2& pos);

	// offscreen
	OffscreenLocation* GetOffscreenLocation() { return offscreen_loc; }
	Unit* CreateUnit(UnitData* data, int level = -1);

private:
	WorldMapGui* gui;
	State state;
	Location* current_location; // current location or nullptr
	Location* travel_location; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (nullptr otherwise)
	Location* start_location;
	int current_location_index; // current location index or -1
	vector<Location*> locations; // can be nullptr
	OutsideLocation* encounter_loc;
	OffscreenLocation* offscreen_loc;
	vector<Encounter*> encounters;
	vector<GlobalEncounter*> globalEncounters;
	EncounterData encounter;
	vector<int> tiles;
	uint settlements, // count and index below this value is city/village
		empty_locations; // counter
	Vec2 world_bounds,
		world_pos,
		travel_start_pos,
		travel_target_pos;
	int world_size,
		create_camp, // counter to create new random camps
		worldtime; // number of passed game days since startDate, starts at 0
	float travel_timer,
		day_timer,
		reveal_timer, // increase chance for encounter every 0.25 sec
		encounter_chance,
		travel_dir; // direction from start to target point (uses NEW rotation)
	EncounterMode encounter_mode;
	Date date, startDate;
	vector<News*> news;
	cstring txDate, txEncCrazyMage, txEncCrazyHeroes, txEncCrazyCook, txEncMerchant, txEncHeroes, txEncSingleHero, txEncBanditsAttackTravelers,
		txEncHeroesAttack, txEncEnemiesCombat;
	cstring txCamp, txCave, txCity, txCrypt, txDungeon, txForest, txVillage, txMoonwell, txOtherness, txRandomEncounter, txTower, txLabyrinth, txAcademy,
		txHuntersCamp, txHills;
	cstring txMonth[12];
	bool tomir_spawned,
		travel_first_frame,
		startup;

	void UpdateDate(int days);
	void SpawnCamps(int days);
	void UpdateEncounters();
	void UpdateLocations();
	void UpdateNews();
	void StartEncounter(int enc, UnitGroup* group);
	void LoadLocations(GameReader& f, LoadingHandler& loading);
	void LoadNews(GameReader& f);
};
