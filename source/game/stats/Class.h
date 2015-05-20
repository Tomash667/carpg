// klasy gracza i npc
#pragma once

//-----------------------------------------------------------------------------
enum class Class
{
	WARRIOR,
	HUNTER,
	ROGUE,
	MAX_PICKABLE,
	MAGE = MAX_PICKABLE,
	CLERIC,
	MAX,
	INVALID,
	RANDOM
};

//-----------------------------------------------------------------------------
struct ClassInfo
{
	Class class_id;
	cstring id, unit_data, icon_file;
	string name, desc;
	TEX icon;

	inline ClassInfo(Class class_id, cstring id, cstring unit_data, cstring icon_file) : class_id(class_id), id(id), unit_data(unit_data), icon_file(icon_file), icon(NULL)
	{

	}

	static ClassInfo* Find(const string& id);
	static inline bool IsPickable(Class c)
	{
		return c >= Class::WARRIOR && c < Class::MAX_PICKABLE;
	}
	static inline Class GetRandom()
	{
		switch(rand2() % 7)
		{
		default:
		case 0:
		case 1:
			return Class::WARRIOR;
		case 2:
		case 3:
			return Class::HUNTER;
		case 4:
		case 5:
			return Class::ROGUE;
		case 6:
			return Class::MAGE;
		}
	}
	static inline Class GetRandomPlayer()
	{
		return (Class)(rand2() % 3);
	}
	static void Validate(int &err);
};

//-----------------------------------------------------------------------------
extern ClassInfo g_classes[(int)Class::MAX];

/*
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
*/
