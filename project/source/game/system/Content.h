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
		Items,
		Objects,
		Spells,
		Dialogs,
		Units,
		Buildings,
		Musics,

		Max
	};

	extern string system_dir;
	extern uint errors;
	extern uint warnings;
	extern uint crc[(int)Id::Max];

	void LoadContent(delegate<void(Id)> callback);
	void LoadItems();
	void LoadObjects();
	void LoadBuildings();
	void CleanupContent();
	void CleanupItems();
	void CleanupObjects();
	void CleanupSpells();
	void CleanupUnits();
	void CleanupBuildings();
	void CleanupMusics();
	bool ReadCrc(BitStream& stream);
	void WriteCrc(BitStream& stream);
	bool GetCrc(Id type, uint& my_crc, cstring& type_crc);
	bool ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str);
}
