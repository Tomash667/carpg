#pragma once

//-----------------------------------------------------------------------------
enum BUFF_FLAGS
{
	BUFF_REGENERATION = 1 << 0,
	BUFF_NATURAL = 1 << 1,
	BUFF_FOOD = 1 << 2,
	BUFF_ALCOHOL = 1 << 3,
	BUFF_POISON = 1 << 4,
	BUFF_ANTIMAGIC = 1 << 5,
	BUFF_STAMINA = 1 << 6,

	BUFF_COUNT = 7
};
static_assert(BUFF_COUNT <= 8, "MP currently allows max 8 buffs.");

//-----------------------------------------------------------------------------
struct BuffInfo
{
	cstring id;
	string text;
	TEX img;

	BuffInfo(cstring id) : id(id)
	{
	}

	static BuffInfo info[];

	static void LoadImages();
	static void LoadText();
};
