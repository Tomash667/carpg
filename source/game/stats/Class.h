// klasy gracza i npc
#pragma once

//-----------------------------------------------------------------------------
struct Class
{
	cstring name, id, desc;
};

//-----------------------------------------------------------------------------
enum CLASS
{
	WARRIOR,
	HUNTER,
	ROGUE,
	MAX_PICKABLE,
	MAGE = MAX_PICKABLE,
	CLERIC,
	CLASS_MAX,
	INVALID_CLASS,
	RANDOM_CLASS
};

//-----------------------------------------------------------------------------
inline bool IsPickableClass(CLASS c)
{
	return c >= WARRIOR && c < MAX_PICKABLE;
}

//-----------------------------------------------------------------------------
// zwraca losow¹ klasê, mniejsza szansa na maga
inline CLASS GetRandomClass()
{
	switch(rand2() % 7)
	{
	case 0:
	case 1:
		return WARRIOR;
	case 2:
	case 3:
		return HUNTER;
	case 4:
	case 5:
		return ROGUE;
	case 6:
		return MAGE;
	default:
		return INVALID_CLASS;
	}
}

//-----------------------------------------------------------------------------
extern Class g_classes[];
extern const uint n_classes;

/*
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
struct ClassInfo
{
	Class class_id;
	cstring id, icon_file;
	string name, desc;
	TEX icon;

	inline ClassInfo(Class class_id, cstring id, cstring icon_file) : class_id(class_id), id(id), icon_file(icon_file), icon(NULL)
	{

	}

	static ClassInfo* Find(const string& id);
};

//-----------------------------------------------------------------------------
inline Class GetRandomClass()
{
	return (Class)(rand2() % (int)Class::MAX);
}
void Validateclasses();

//-----------------------------------------------------------------------------
extern ClassInfo g_classes[(int)Class::MAX];
*/
