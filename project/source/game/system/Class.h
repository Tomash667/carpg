#pragma once

#include "Resource.h"

//-----------------------------------------------------------------------------
struct Action;
struct UnitData;

//-----------------------------------------------------------------------------
enum class ClassId
{
	None = -1,
	Random = -2
};

//-----------------------------------------------------------------------------
namespace old
{
	// pre 0.4 classes
	enum class ClassId
	{
		Warrior,
		Hunter,
		Rogue,
		Mage,
		Cleric,
		Max
	};
}

//-----------------------------------------------------------------------------
struct Class
{
	struct ClassesInfo
	{
		ClassId old_to_new[(int)old::ClassId::Max];
		vector<ClassId> player_classes;
		vector<std::pair<ClassId, uint>> hero_classes;
		vector<std::pair<ClassId, uint>> crazy_classes;
		uint hero_total, crazy_total;

		ClassId TryGetIndex(cstring id);
		void Initialize();
	};

	int index;
	uint chance;
	string id;
	// localized data
	string name, desc, about;
	vector<string> names, nicknames;
	TexturePtr icon;
	UnitData* player_data, *hero_data, *crazy_data;
	Action* action;
	bool defined;

	Class() : icon(nullptr), player_data(nullptr), hero_data(nullptr), crazy_data(nullptr), action(nullptr), chance(0), defined(false)
	{
	}

	bool IsPickable() const
	{
		return player_data != nullptr;
	}

	static Class* TryGet(const AnyString& id);
	static ClassId GetRandomPlayerClass();
	static ClassId GetRandomHeroClass(bool crazy = false);
	static UnitData& GetRandomHeroData(bool crazy = false);
	static UnitData& GetHeroData(ClassId clas, bool crazy = false);
	static ClassId OldToNew(old::ClassId clas);
	static bool IsPickable(ClassId clas); // spr czy nie jest Invalid/Random
	static void Validate(uint& errors);
	static vector<Class*> classes;
	static ClassesInfo info;
};
