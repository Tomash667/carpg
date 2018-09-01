#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"

//-----------------------------------------------------------------------------
class World
{
public:
	enum class State
	{
		ON_MAP, // on map (current_location set)
		INSIDE_LOCATION, // inside location (current_location set)
		INSIDE_ENCOUNTER, // inside encounter location (current_location set)
		TRAVEL, // traveling on map (current_location is nullptr)
		ENCOUNTER // shown encounter message, waiting to close & load level (current_location is nullptr)
	};

	void Init();
	void OnNewGame();
	void Update(int days);
	uint GenerateWorld(int start_location_type = -1, int start_location_target = -1);
	void ExitToMap();

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

	Location* current_location; // wskaŸnik na aktualn¹ lokacjê [odtwarzany]
	int current_location_index, // current location index or -1
		travel_location_index; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (-1 otherwise)
	vector<Location*> locations; // can be nullptr

public: // FIXME
	State state;
	uint settlements, // count and index below this value is city/village
		empty_locations, // counter
		encounter_loc, // encounter location index
		create_camp; // counter to create new random camps
	Vec2 world_pos;
	float encounter_timer, // increase chance for encounter every 0.25 sec
		encounter_chance,
		travel_dir; // from which direction team will enter level after travel
private:
	int year; // in game year, starts at 100
	int month; // in game month, 0 to 11
	int day; // in game day, 0 to 29
	int worldtime; // number of passed game days, starts at 0
	cstring txDate, txRandomEncounter;
};
extern World W;
