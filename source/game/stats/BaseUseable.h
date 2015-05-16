// bazowy u¿ywalny obiekt
#pragma once

//-----------------------------------------------------------------------------
struct Item;
struct Obj;

//-----------------------------------------------------------------------------
enum USEABLE_ID
{
	U_KRZESLO,
	U_LAWA,
	U_KOWADLO,
	U_KOCIOLEK,
	U_ZYLA_ZELAZA,
	U_ZYLA_ZLOTA,
	U_TRON,
	U_STOLEK,
	U_LAWA_DIR,
	U_MAX
};

//-----------------------------------------------------------------------------
struct BaseUsable
{
	cstring id, name, obj_name, anim, item_id, sound_id;
	float sound_timer;
	const Item* item;
	SOUND sound;
	int limit_rot; // spr przyk³adowe obiekty ¿eby zobaczyæ jak to dzia³a
	bool allow_use;
	Obj* obj;

	BaseUsable(cstring id, cstring obj_name, cstring anim, int limit_rot, bool allow_use, cstring item_id=NULL, cstring sound_id=NULL, float sound_timer=0.f) :
		id(id), name(NULL), anim(anim), item_id(item_id), sound_id(sound_id), sound_timer(sound_timer), obj_name(obj_name), item(NULL), sound(NULL), limit_rot(limit_rot), allow_use(allow_use),
		obj(NULL)
	{

	}
};
extern BaseUsable g_base_usables[];
extern const uint n_base_usables;
