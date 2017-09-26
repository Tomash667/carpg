// character class
#pragma once

//-----------------------------------------------------------------------------
enum class Class
{
	BARBARIAN,
	BARD,
	CLERIC,
	DRUID,
	HUNTER,
	MAGE,
	MONK,
	PALADIN,
	ROGUE,
	WARRIOR,

	MAX,
	INVALID,
	RANDOM
};

//-----------------------------------------------------------------------------
struct Action;
struct UnitData;

//-----------------------------------------------------------------------------
struct ClassInfo
{
	Class class_id;
	cstring id, unit_data_id, icon_file, action_id;
	string name, desc, about;
	vector<string> names, nicknames;
	TEX icon;
	bool pickable;
	UnitData* unit_data;
	Action* action;

	ClassInfo(Class class_id, cstring id, cstring unit_data_id, cstring icon_file, bool pickable, cstring action_id) : class_id(class_id), id(id),
		unit_data_id(unit_data_id), icon_file(icon_file), icon(nullptr), pickable(pickable), unit_data(nullptr), action_id(action_id), action(nullptr)
	{
	}

	bool IsPickable() const
	{
		return pickable;
	}

	static ClassInfo* Find(const string& id);
	static bool IsPickable(Class c);
	static Class GetRandom();
	static Class GetRandomPlayer();
	static Class GetRandomEvil();
	static void Validate(uint &err);
	static Class OldToNew(Class c);

	static ClassInfo classes[(int)Class::MAX];
};

//=================================================================================================
inline bool ClassInfo::IsPickable(Class c)
{
	if(c < (Class)0 || c >= Class::MAX)
		return false;
	return ClassInfo::classes[(int)c].pickable;
}
