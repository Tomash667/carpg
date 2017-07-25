#pragma once

//-----------------------------------------------------------------------------
struct Building;
struct BuildingGroup;
struct BuildingScript;
struct UnitData;
enum class OLD_BUILDING;

//-----------------------------------------------------------------------------
namespace content
{
	extern string system_dir;
	extern uint errors;
	extern uint warnings;

	extern vector<Building*> buildings;
	extern vector<BuildingGroup*> building_groups;
	extern vector<BuildingScript*> building_scripts;
	extern uint buildings_crc;

	// Hardcoded building groups
	extern BuildingGroup* BG_INN;
	extern BuildingGroup* BG_HALL;
	extern BuildingGroup* BG_TRAINING_GROUNDS;
	extern BuildingGroup* BG_ARENA;
	extern BuildingGroup* BG_FOOD_SELLER;
	extern BuildingGroup* BG_ALCHEMIST;
	extern BuildingGroup* BG_BLACKSMITH;
	extern BuildingGroup* BG_MERCHANT;

	Building* FindBuilding(const AnyString& id);
	BuildingGroup* FindBuildingGroup(const AnyString& id);
	BuildingScript* FindBuildingScript(const AnyString& id);
	Building* FindOldBuilding(OLD_BUILDING type);
	UnitData* FindUnit(const AnyString& id);

	void LoadContent();
	void LoadStrings();
	void LoadBuildings();
	bool ReadCrc(BitStream& stream);
	void WriteCrc(BitStream& stream);
	bool GetCrc(byte type, uint& my_crc, cstring& type_crc);
	bool ValidateCrc(byte& type, uint& my_crc, uint& player_crc, cstring& type_str);
}
