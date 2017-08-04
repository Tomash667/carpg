// bazowy u¿ywalny obiekt
#pragma once

//-----------------------------------------------------------------------------
struct Item;
struct Obj;

//-----------------------------------------------------------------------------
enum USEABLE_ID
{
	U_CHAIR,
	U_BENCH,
	U_ANVIL,
	U_CAULDRON,
	U_IRON_VEIN,
	U_GOLD_VEIN,
	U_THRONE,
	U_STOOL,
	U_BENCH_ROT,
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
	Obj* obj;
	bool allow_use;
	bool stamina_slow_restore;

	BaseUsable(cstring id, cstring obj_name, cstring anim, int limit_rot, bool allow_use, bool stamina_slow_restore, cstring item_id = nullptr,
		cstring sound_id = nullptr, float sound_timer = 0.f) :
		id(id), name(nullptr), anim(anim), item_id(item_id), sound_id(sound_id), sound_timer(sound_timer), obj_name(obj_name), item(nullptr), sound(nullptr),
		limit_rot(limit_rot), allow_use(allow_use), stamina_slow_restore(stamina_slow_restore), obj(nullptr)
	{
	}
};
extern BaseUsable g_base_usables[];
extern const uint n_base_usables;
