#pragma once

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
	bool use_mana, // true-use mana, false-use stamina
		use_cast; // use cast animation

	Action(cstring id, float cooldown, float recharge, int charges, Area area, const Vec2& area_size, cstring sound_id, float sound_dist, int flags,
		float cost, bool use_mana, bool use_cast) :
		id(id), cooldown(cooldown), recharge(recharge), charges(charges), tex(nullptr), area(area), area_size(area_size), sound_id(sound_id), sound(nullptr),
		sound_dist(sound_dist), flags(flags), cost(cost), use_mana(use_mana), use_cast(use_cast)
	{
	}

	static Action* Find(const string& id);
	static void LoadData();

	static Action actions[];
	static const uint n_actions;
};
