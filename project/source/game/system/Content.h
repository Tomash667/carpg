#pragma once

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
	extern uint version;
	extern bool require_update;

	void LoadContent(delegate<void(Id)> callback);
	void LoadVersion();
	void LoadItems();
	void LoadObjects();
	void LoadDialogs();
	void LoadSpells();
	void LoadUnits();
	void LoadBuildings();
	void CleanupContent();
	void CleanupItems();
	void CleanupObjects();
	void CleanupSpells();
	void CleanupDialogs();
	void CleanupUnits();
	void CleanupBuildings();
	void CleanupMusics();
	void WriteCrc(BitStreamWriter& f);
	void ReadCrc(BitStreamReader& f);
	bool GetCrc(Id type, uint& my_crc, cstring& type_crc);
	bool ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str);
}
