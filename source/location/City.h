#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocation.h"
#include "InsideBuilding.h"
#include "EntryPoint.h"
#include "Content.h"
#include "BuildingGroup.h"

//-----------------------------------------------------------------------------
enum CityTarget
{
	VILLAGE,
	CITY,
	CAPITAL,
	VILLAGE_EMPTY,
	VILLAGE_DESTROYED,
	VILLAGE_DESTROYED2
};

//-----------------------------------------------------------------------------
// Budynek w mieœcie
struct CityBuilding
{
	Building* building;
	Int2 pt, unitPt;
	GameDirection dir;
	Vec3 walkPt;

	CityBuilding() {}
	explicit CityBuilding(Building* building) : building(building) {}
	Vec3 GetUnitPos();
	float GetUnitRot();
};

//-----------------------------------------------------------------------------
enum class CityQuestState
{
	None,
	InProgress,
	Failed
};

//-----------------------------------------------------------------------------
struct City : public OutsideLocation
{
	enum Flags
	{
		HaveExit = 1 << 0,
		HaveTrainingGrounds = 1 << 1,
		HaveBlacksmith = 1 << 2,
		HaveMerchant = 1 << 3,
		HaveAlchemist = 1 << 4,
		HaveFoodSeller = 1 << 5,
		HaveInn = 1 << 6,
		HaveArena = 1 << 7
		// saved as byte in PreparedWorldData
	};

	int citizens, citizensWorld, questMayorTime, questCaptainTime,
		arenaTime, // last arena combat worldtime or -1
		gates, flags, variant;
	CityQuestState questMayor, questCaptain;
	vector<CityBuilding> buildings; // when visited this contain buildings to spawn (only type), after entering it is fully filled
	vector<InsideBuilding*> insideBuildings;
	Int2 insideOffset;
	Vec3 arenaPos;
	vector<EntryPoint> entryPoints;

	City() : questMayor(CityQuestState::None), questCaptain(CityQuestState::None), questMayorTime(-1), questCaptainTime(-1),
		insideOffset(1, 0), arenaTime(-1), flags(HaveExit), variant(-1)
	{
	}
	~City();

	// from Location
	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override;
	void Save(GameWriter& f) override;
	void Load(GameReader& f) override;
	void Write(BitStreamWriter& f) override;
	bool Read(BitStreamReader& f) override;

	void GenerateCityBuildings(vector<Building*>& buildings, bool required);
	void PrepareCityBuildings(vector<ToBuild>& tobuild);
	bool IsInsideCity(const Vec3& pos);
	InsideBuilding* FindInsideBuilding(Building* building, int* index = nullptr);
	InsideBuilding* FindInsideBuilding(BuildingGroup* group, int* index = nullptr);
	InsideBuilding* FindInn(int* index = nullptr) { return FindInsideBuilding(BuildingGroup::BG_INN, index); }
	CityBuilding* FindBuilding(BuildingGroup* group, int* index = nullptr);
	CityBuilding* FindBuilding(Building* building, int* index = nullptr);
	bool IsVillage() const { return Any(target, VILLAGE, VILLAGE_EMPTY, VILLAGE_DESTROYED, VILLAGE_DESTROYED2); }
	bool IsCity() const { return Any(target, CITY, CAPITAL); }
	void GetEntry(Vec3& pos, float& rot);
};
