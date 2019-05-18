#pragma once

//-----------------------------------------------------------------------------
// Visible buffs (on bottom left of screen)
enum BUFF_FLAGS
{
	BUFF_REGENERATION = 1 << 0,
	BUFF_NATURAL = 1 << 1,
	BUFF_FOOD = 1 << 2,
	BUFF_ALCOHOL = 1 << 3,
	BUFF_POISON = 1 << 4,
	BUFF_ANTIMAGIC = 1 << 5,
	BUFF_STAMINA = 1 << 6,
	BUFF_STUN = 1 << 7,
	BUFF_POISON_RES = 1 << 8,

	BUFF_COUNT = 9
};

//-----------------------------------------------------------------------------
// Buff info - name & image
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
