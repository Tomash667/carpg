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
	switch(rand2()%7)
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
