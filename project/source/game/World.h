#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"

//-----------------------------------------------------------------------------
class World
{
public:
	void Init();
	void OnNewGame();
	void Update(int days);
	uint GenerateWorld(int start_location_type = -1, int start_location_target = -1);

	Location* CreateLocation(LOCATION type, int levels = -1, bool is_village = false);
	typedef std::pair<LOCATION, bool>(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist, bool unique_name);

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void LoadOld(FileReader& f, int part);
	void WriteTime(BitStreamWriter& f);
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
	int GetEncounterLocationIndex() const { return encounter_loc; }
	int GetWorldtime() const { return worldtime; }
	int GetYear() const { return year; }

	vector<Location*> locations; // can be nullptr

public: // FIXME
	uint settlements; // count and index below this value is city/village
	uint empty_locations; // counter
	uint encounter_loc; // encounter location index
private:
	int year; // in game year, starts at 100
	int month; // in game month, 0 to 11
	int day; // in game day, 0 to 29
	int worldtime; // number of passed game days, starts at 0
	cstring txDate, txRandomEncounter;
};
extern World W;
