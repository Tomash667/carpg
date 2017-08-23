#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Action
{
	cstring id;
	float cooldown, recharge;
	int charges;
	TexturePtr tex;
	string name, desc;

	Action(cstring id, float cooldown, float recharge, int charges) : id(id), cooldown(cooldown), recharge(recharge), charges(charges), tex(nullptr)
	{
	}

	static Action* Find(const string& id);
	static void LoadImages();
};

//-----------------------------------------------------------------------------
extern Action g_actions[];
extern const uint n_actions;
