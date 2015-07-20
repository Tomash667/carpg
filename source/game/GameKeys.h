#pragma once

//-----------------------------------------------------------------------------
#include "KeyStates.h"

//-----------------------------------------------------------------------------
enum GAME_KEYS
{
	GK_MOVE_FORWARD, // W, strza³ka w górê
	GK_MOVE_BACK, // S, strza³ka w dó³
	GK_MOVE_LEFT, // A, strza³ka w lewo
	GK_MOVE_RIGHT, // D, strza³ka w prawo
	GK_WALK, // shift
	GK_ROTATE_LEFT, // Q
	GK_ROTATE_RIGHT, // E
	GK_TAKE_WEAPON, // spacja
	GK_ATTACK_USE, // Z, lewy myszki
	GK_USE, // R
	GK_BLOCK, // X, prawy myszki
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
	GK_TAKE_ALL, // Enter
	GK_SELECT_DIALOG, // Enter
	GK_SKIP_DIALOG, // Space
	GK_TALK_BOX, // N
	GK_PAUSE, // Pause
	GK_YELL, // Y
	GK_CONSOLE, // ~
	GK_ROTATE, // V
	GK_MAX
};

//-----------------------------------------------------------------------------
struct GameKey
{
	cstring id, text;
	byte key[2];

	inline void Set(byte k1=VK_NONE, byte k2=VK_NONE)
	{
		key[0] = k1;
		key[1] = k2;
	}

	inline byte& operator [] (int n)
	{
		assert(n == 0 || n == 1);
		return key[n];
	}

	inline const byte& operator [] (int n) const
	{
		assert(n == 0 || n == 1);
		return key[n];
	}
};

//-----------------------------------------------------------------------------
class GameKeys
{
public:
	inline bool Down(GAME_KEYS gk) const
	{
		return Key.Down(keys[gk][0]) || Key.Down(keys[gk][1]);
	}
	inline byte DownR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(Key.Down(k[0]))
			return k[0];
		else if(Key.Down(k[1]))
			return k[1];
		else
			return VK_NONE;
	}
	inline byte PressedR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(Key.Pressed(k[0]))
			return k[0];
		else if(Key.Pressed(k[1]))
			return k[1];
		else
			return VK_NONE;
	}
	inline bool PressedRelease(GAME_KEYS gk)
	{
		return Key.PressedRelease(keys[gk][0]) || Key.PressedRelease(keys[gk][1]);
	}
	inline GameKey& operator [] (int n)
	{
		return keys[n];
	}

private:
	GameKey keys[GK_MAX];
};

//-----------------------------------------------------------------------------
extern GameKeys GKey;
