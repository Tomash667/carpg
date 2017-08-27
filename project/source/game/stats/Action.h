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

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size) : id(id), cooldown(cooldown), recharge(recharge), charges(charges),
		tex(nullptr), area(area), area_size(area_size)
	{
	}

	static Action* Find(const string& id);
	static void LoadImages();
};

//-----------------------------------------------------------------------------
extern Action g_actions[];
extern const uint n_actions;
