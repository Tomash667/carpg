#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Action
{
	enum Area
	{
		LINE,
		LINE_FORWARD,
		POINT
	};

	cstring id;
	float cooldown, recharge;
	int charges;
	TexturePtr tex;
	string name, desc;
	Area area;
	Vec2 area_size; // for LINE,LINE_FORWARD w,h; for POINT radius,max_distance
	cstring sound_id;
	SoundPtr sound;

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size, cstring sound_id) : id(id), cooldown(cooldown), recharge(recharge),
		charges(charges), tex(nullptr), area(area), area_size(area_size), sound_id(sound_id), sound(nullptr)
	{
	}

	static Action* Find(const string& id);
	static void LoadData();
};

//-----------------------------------------------------------------------------
extern Action g_actions[];
extern const uint n_actions;
