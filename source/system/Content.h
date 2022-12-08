#pragma once

//-----------------------------------------------------------------------------
class AbilityLoader;
class BuildingLoader;
class ClassLoader;
class DialogLoader;
class ItemLoader;
class LocationLoader;
class MusicListLoader;
class ObjectLoader;
class PerkLoader;
class QuestLoader;
class RequiredLoader;
class UnitLoader;

//-----------------------------------------------------------------------------
class Content
{
public:
	enum class Id
	{
		Abilities,
		Buildings,
		Classes,
		Dialogs,
		Items,
		Locations,
		Musics,
		Objects,
		Perks,
		Quests,
		Required,
		Units,

		Max
	};

	Content();
	~Content();
	void Cleanup();
	void LoadContent(delegate<void(Id)> callback);
	void LoadVersion();
	void CleanupContent();
	void WriteCrc(BitStreamWriter& f);
	void ReadCrc(BitStreamReader& f);
	bool GetCrc(Id type, uint& myCrc, cstring& typeCrc);
	bool ValidateCrc(Id& type, uint& myCrc, uint& playerCrc, cstring& typeStr);

	string systemDir;
	uint errors;
	uint warnings;
	uint crc[(int)Id::Max];
	uint clientCrc[(int)Id::Max];
	uint version;
	bool requireUpdate;

private:
	AbilityLoader* abilityLoader;
	BuildingLoader* buildingLoader;
	ClassLoader* classLoader;
	DialogLoader* dialogLoader;
	ItemLoader* itemLoader;
	LocationLoader* locationLoader;
	MusicListLoader* musicLoader;
	ObjectLoader* objectLoader;
	PerkLoader* perkLoader;
	QuestLoader* questLoader;
	RequiredLoader* requiredLoader;
	UnitLoader* unitLoader;
};

extern Content content;
