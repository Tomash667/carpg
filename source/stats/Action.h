#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
// Player special action
struct Action
{
	enum Area
	{
		LINE,
		POINT,
		TARGET
	};

	enum Flags
	{
		F_PICK_DIR = 1 << 0,
		F_IGNORE_UNITS = 1 << 1
	};

	cstring id;
	float cooldown, recharge, sound_dist, cost;
	int charges, flags;
	TexturePtr tex;
	string name, desc;
	Area area;
	Vec2 area_size; // for LINE w,h; for POINT radius,max_distance
	cstring sound_id;
	SoundPtr sound;
	bool use_mana; // false - use stamina

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size, cstring sound_id, float sound_dist, int flags,
		float cost, bool use_mana) :
		id(id), cooldown(cooldown), recharge(recharge), charges(charges), tex(nullptr), area(area), area_size(area_size), sound_id(sound_id), sound(nullptr),
		sound_dist(sound_dist), flags(flags),cost(cost), use_mana(use_mana)
	{
	}

	static Action* Find(const string& id);
	static void LoadData();

	static Action actions[];
	static const uint n_actions;
};
