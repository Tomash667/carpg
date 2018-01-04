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
	GK_ACTION, // 3
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

	void Set(byte k1 = VK_NONE, byte k2 = VK_NONE)
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
	enum AllowInput
	{
		ALLOW_NONE = 0,	// 00
		ALLOW_KEYBOARD = 1,	// 01
		ALLOW_MOUSE = 2,	// 10
		ALLOW_INPUT = 3	// 11
	};

	int KeyAllowState(byte k)
	{
		if(k > 7)
			return ALLOW_KEYBOARD;
		else if(k != 0)
			return ALLOW_MOUSE;
		else
			return ALLOW_NONE;
	}

	bool KeyAllowed(byte k)
	{
		return IS_SET(allow_input, KeyAllowState(k));
	}

	bool AllowKeyboard() const { return IS_SET(allow_input, ALLOW_KEYBOARD); }
	bool AllowMouse() const { return IS_SET(allow_input, ALLOW_MOUSE); }

	//===============================================================
	// Functions using GameKey
	//===============================================================

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

	//===============================================================
	// Functions using KeyAllowed
	//===============================================================

	byte KeyDoReturn(GAME_KEYS gk, KeyF f)
	{
		GameKey& k = keys[gk];
		if(k[0])
		{
			if(KeyAllowed(k[0]) && (Key.*f)(k[0]))
				return k[0];
		}
		if(k[1])
		{
			if(KeyAllowed(k[1]) && (Key.*f)(k[1]))
				return k[1];
		}
		return VK_NONE;
	}
	byte KeyDoReturn(GAME_KEYS gk, KeyFC f)
	{
		return KeyDoReturn(gk, (KeyF)f);
	}
	byte KeyDoReturnIgnore(GAME_KEYS gk, KeyF f, byte ignored_key)
	{
		GameKey& k = keys[gk];
		if(k[0] && k[0] != ignored_key)
		{
			if(KeyAllowed(k[0]) && (Key.*f)(k[0]))
				return k[0];
		}
		if(k[1] && k[1] != ignored_key)
		{
			if(KeyAllowed(k[1]) && (Key.*f)(k[1]))
				return k[1];
		}
		return VK_NONE;
	}
	byte KeyDoReturnIgnore(GAME_KEYS gk, KeyFC f, byte ignored_key)
	{
		return KeyDoReturnIgnore(gk, (KeyF)f, ignored_key);
	}
	bool KeyDo(GAME_KEYS gk, KeyF f)
	{
		return KeyDoReturn(gk, f) != VK_NONE;
	}
	bool KeyDo(GAME_KEYS gk, KeyFC f)
	{
		return KeyDo(gk, (KeyF)f);
	}
	bool KeyDownAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::Down);
	}
	bool KeyPressedReleaseAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedRelease);
	}
	bool KeyDownUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::DownUp);
	}
	bool KeyDownUp(GAME_KEYS gk)
	{
		GameKey& k = keys[gk];
		if(k[0])
		{
			if(Key.DownUp(k[0]))
				return true;
		}
		if(k[1])
		{
			if(Key.DownUp(k[1]))
				return true;
		}
		return false;
	}
	bool KeyPressedUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedUp);
	}
	// Zwraca czy dany klawisze jest wyciœniêty, jeœli nie jest to dozwolone to traktuje jak wyciœniêty
	bool KeyUpAllowed(byte key)
	{
		if(KeyAllowed(key))
			return Key.Up(key);
		else
			return true;
	}
	bool KeyPressedReleaseSpecial(GAME_KEYS gk, bool special)
	{
		if(special)
		{
			GameKey& k = keys[gk];
			for(int i = 0; i < 2; ++i)
			{
				if(k.key[i] >= VK_F1 && k.key[i] <= VK_F12)
				{
					if(Key.PressedRelease(k.key[i]))
						return true;
				}
			}
		}
		return KeyPressedReleaseAllowed(gk);
	}

	AllowInput allow_input;

private:
	GameKey keys[GK_MAX];
};

//-----------------------------------------------------------------------------
extern GameKeys GKey;
