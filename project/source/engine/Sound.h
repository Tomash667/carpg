#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Sound : public Resource
{
	SOUND sound;
	bool is_music;

	Sound() : sound(nullptr), is_music(false)
	{
	}
};
