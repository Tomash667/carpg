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
	enum class Id
	{
		Buildings,
		Objects
	};

	extern string system_dir;
	extern uint errors;
	extern uint warnings;
	extern uint buildings_crc;
	extern uint objects_crc;

	void LoadContent(delegate<void(Id)> callback);
	void LoadBuildings();
	void LoadObjects();
	void CleanupContent();
	void CleanupBuildings();
	void CleanupObjects();
	bool ReadCrc(BitStream& stream);
	void WriteCrc(BitStream& stream);
	bool GetCrc(Id type, uint& my_crc, cstring& type_crc);
	bool ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str);
}
