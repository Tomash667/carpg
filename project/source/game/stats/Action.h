#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Action
{
	enum Area
	{
		LINE,
		POINT
	};

	enum Flags
	{
		F_PICK_DIR = 1 << 0,
		F_IGNORE_UNITS = 1 << 1
	};

	cstring id;
	float cooldown, recharge, sound_dist, stamina_cost;
	int charges, flags;
	TexturePtr tex;
	string name, desc;
	Area area;
	Vec2 area_size; // for LINE,LINE_FORWARD w,h; for POINT radius,max_distance
	cstring sound_id;
	SoundPtr sound;

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size, cstring sound_id, float sound_dist, int flags,
		float stamina_cost) :
		id(id), cooldown(cooldown), recharge(recharge), charges(charges), tex(nullptr), area(area), area_size(area_size), sound_id(sound_id), sound(nullptr),
		sound_dist(sound_dist), flags(flags), stamina_cost(stamina_cost)
	{
	}

	static Action* Find(const string& id);
	static void LoadData();

	static Action actions[];
	static const uint n_actions;
};
