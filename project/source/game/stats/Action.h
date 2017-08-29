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

	cstring id;
	float cooldown, recharge, sound_dist;
	int charges;
	TexturePtr tex;
	string name, desc;
	Area area;
	Vec2 area_size; // for LINE,LINE_FORWARD w,h; for POINT radius,max_distance
	cstring sound_id;
	SoundPtr sound;
	bool pick_dir;

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size, cstring sound_id, float sound_dist, bool pick_dir) :
		id(id), cooldown(cooldown), recharge(recharge), charges(charges), tex(nullptr), area(area), area_size(area_size), sound_id(sound_id), sound(nullptr),
		sound_dist(sound_dist), pick_dir(pick_dir)
	{
	}

	static Action* Find(const string& id);
	static void LoadData();
};

//-----------------------------------------------------------------------------
extern Action g_actions[];
extern const uint n_actions;
