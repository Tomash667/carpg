#pragma once

//-----------------------------------------------------------------------------
#include "KeyStates.h"

//-----------------------------------------------------------------------------
enum GAME_KEYS
{
	GK_MOVE_FORWARD, // W, up arrow
	GK_MOVE_BACK, // S, down arrow
	GK_MOVE_LEFT, // A, left arrow
	GK_MOVE_RIGHT, // D, right arrow
	GK_WALK, // shift
	GK_ROTATE_LEFT, // Q
	GK_ROTATE_RIGHT, // E
	GK_TAKE_WEAPON, // spacebar
	GK_ATTACK_USE, // Z, left mouse
	GK_USE, // R
	GK_BLOCK, // X, right mouse
	GK_STATS, // C
	GK_INVENTORY, // I
	GK_TEAM_PANEL, // T
	GK_ACTION_PANEL, // K
	GK_JOURNAL, // J
	GK_MINIMAP, // M
	GK_QUICKSAVE, // F5
	GK_QUICKLOAD, // F9
	GK_POTION, // H
	GK_MELEE_WEAPON, // 1
	GK_RANGED_WEAPON, // 2
	GK_TAKE_ALL, // F
	GK_SELECT_DIALOG, // Enter
	GK_SKIP_DIALOG, // Space
	GK_TALK_BOX, // N
	GK_PAUSE, // Pause
	GK_YELL, // Y
	GK_CONSOLE, // ~
	GK_ROTATE_CAMERA, // V
	GK_AUTOWALK, // F
	GK_MAX
};

//-----------------------------------------------------------------------------
struct GameKey
{
	cstring id, text;
	byte key[2];

	void Set(byte k1=VK_NONE, byte k2=VK_NONE)
	{
		key[0] = k1;
		key[1] = k2;
	}

	byte& operator [] (int n)
	{
		assert(n == 0 || n == 1);
		return key[n];
	}

	const byte& operator [] (int n) const
	{
		assert(n == 0 || n == 1);
		return key[n];
	}
};

//-----------------------------------------------------------------------------
class GameKeys
{
public:
	bool Down(GAME_KEYS gk) const
	{
		return Key.Down(keys[gk][0]) || Key.Down(keys[gk][1]);
	}
	byte DownR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(Key.Down(k[0]))
			return k[0];
		else if(Key.Down(k[1]))
			return k[1];
		else
			return VK_NONE;
	}
	byte PressedR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(Key.Pressed(k[0]))
			return k[0];
		else if(Key.Pressed(k[1]))
			return k[1];
		else
			return VK_NONE;
	}
	bool PressedRelease(GAME_KEYS gk)
	{
		return Key.PressedRelease(keys[gk][0]) || Key.PressedRelease(keys[gk][1]);
	}
	GameKey& operator [] (int n)
	{
		return keys[n];
	}

private:
	GameKey keys[GK_MAX];
};

//-----------------------------------------------------------------------------
extern GameKeys GKey;
