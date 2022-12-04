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
		ON_MAP, // on map (currentLocation set)
		INSIDE_LOCATION, // inside location (currentLocation set)
		INSIDE_ENCOUNTER, // inside encounter location (currentLocation set)
		TRAVEL, // traveling on map (currentLocation is nullptr)
		ENCOUNTER // shown encounter message, waiting to close & load level (currentLocation is nullptr)
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
	void SetStartLocation(Location* loc) { startLocation = loc; }
	void StartInLocation();
	void CalculateTiles();
	void SmoothTiles();
	void CreateCity(const Vec2& pos, int target);
	void SetLocationImageAndName(Location* l);
	Location* CreateCamp(const Vec2& pos, UnitGroup* group);
private:
	typedef LOCATION(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float validDist);
	Location* CreateLocation(LOCATION type, int levels = -1, int cityTarget = -1);
public:
	Location* CreateLocation(LOCATION type, const Vec2& pos, int target = -1, int dungeonLevels = -1);
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
	Location* GetCurrentLocation() const { return currentLocation; }
	int GetCurrentLocationIndex() const { return currentLocationIndex; }
	const vector<Location*>& GetLocations() const { return locations; }
	Location* GetLocation(int index) const { assert(index >= 0 && index < (int)locations.size()); return locations[index]; }
	Location* GetLocationByType(LOCATION type, int target = ANY_TARGET) const;
	const Vec2& GetWorldPos() const { return worldPos; }
	const Vec2& GetTargetPos() const { return travelTargetPos; }
	void SetWorldPos(const Vec2& worldPos) { this->worldPos = worldPos; }
	uint GetSettlements() { return settlements; }
	Location* GetRandomSettlement(Location* excluded = nullptr) const;
	Location* GetRandomSettlement(vector<Location*>& used, int target = ANY_TARGET) const;
	Location* GetRandomFreeSettlement(Location* excluded = nullptr) const;
	Location* GetRandomCity(Location* excluded = nullptr) const;
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, int target = ANY_TARGET, int flags = 0);
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, const int* targets, int targetsCount, int flags = 0);
	Location* GetClosestLocation(LOCATION type, const Vec2& pos, std::initializer_list<int> const& targets, int flags = 0)
	{
		return GetClosestLocation(type, pos, targets.begin(), targets.size(), flags);
	}
	Location* GetClosestLocationArrayS(LOCATION type, const Vec2& pos, CScriptArray* array);
	bool TryFindPlace(Vec2& pos, float range, bool allowExact = false);
	Vec2 FindPlace(const Vec2& pos, float range, bool allowExact = false);
	Vec2 FindPlace(const Vec2& pos, float minRange, float maxRange);
	Vec2 GetRandomPlace();
	Location* GetRandomSpawnLocation(const Vec2& pos, UnitGroup* group, float range = 160.f);
	Location* GetNearestSettlement(const Vec2& pos) { return GetClosestLocation(L_CITY, pos); }
	City* GetRandomSettlement(delegate<bool(City*)> pred);
	Location* GetRandomSettlementWeighted(delegate<float(Location*)> func);
	Location* GetRandomLocation(delegate<bool(Location*)> pred);
	Vec2 GetSize() const { return Vec2((float)worldSize, (float)worldSize); }
	Vec2 GetPos() const { return worldPos; }
	Vec2 GetWorldBounds() const { return worldBounds; }

	// travel
	void Travel(int index, bool order);
	void TravelPos(const Vec2& pos, bool order);
	void UpdateTravel(float dt);
	void StopTravel(const Vec2& pos, bool send);
	void EndTravel();
	Location* GetTravelLocation() const { return travelLocation; }
	float GetTravelDir() const { return travelDir; }
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
	OutsideLocation* GetEncounterLocation() const { return encounterLoc; }
	float GetEncounterChance() const { return encounterChance; }
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
	OffscreenLocation* GetOffscreenLocation() { return offscreenLoc; }
	Unit* CreateUnit(UnitData* data, int level = -1);

private:
	void UpdateDate(int days);
	void SpawnCamps(int days);
	void UpdateEncounters();
	void UpdateLocations();
	void UpdateNews();
	void StartEncounter(int enc, UnitGroup* group);
	void LoadLocations(GameReader& f, LoadingHandler& loading);
	void LoadNews(GameReader& f);

	WorldMapGui* gui;
	State state;
	Location* currentLocation; // current location or nullptr
	Location* travelLocation; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (nullptr otherwise)
	Location* startLocation;
	int currentLocationIndex; // current location index or -1
	vector<Location*> locations; // can be nullptr
	OutsideLocation* encounterLoc;
	OffscreenLocation* offscreenLoc;
	vector<Encounter*> encounters;
	vector<GlobalEncounter*> globalEncounters;
	EncounterData encounter;
	vector<int> tiles;
	uint settlements, // count and index below this value is city/village
		emptyLocations; // counter
	Vec2 worldBounds,
		worldPos,
		travelStartPos,
		travelTargetPos;
	int worldSize,
		createCamp, // counter to create new random camps
		worldtime; // number of passed game days since startDate, starts at 0
	float travelTimer,
		dayTimer,
		revealTimer, // increase chance for encounter every 0.25 sec
		encounterChance,
		travelDir; // direction from start to target point (uses NEW rotation)
	Date date, startDate;
	vector<News*> news;
	cstring txDate, txEncCrazyMage, txEncCrazyHeroes, txEncCrazyCook, txEncMerchant, txEncHeroes, txEncSingleHero, txEncBanditsAttackTravelers,
		txEncHeroesAttack, txEncEnemiesCombat;
	cstring txCamp, txCave, txCity, txCrypt, txDungeon, txForest, txVillage, txMoonwell, txOtherness, txRandomEncounter, txTower, txLabyrinth, txAcademy,
		txHuntersCamp, txHills;
	cstring txMonth[12];
	bool tomirSpawned,
		travelFirstFrame,
		startup;
};
