#pragma once

//-----------------------------------------------------------------------------
#include "Input.h"

//-----------------------------------------------------------------------------
enum GAME_KEYS
{
	GK_MOVE_FORWARD, // W
	GK_MOVE_BACK, // S
	GK_MOVE_LEFT, // A
	GK_MOVE_RIGHT, // D
	GK_ROTATE_LEFT, // (None)
	GK_ROTATE_RIGHT, // (None)
	GK_TAKE_WEAPON, // spacebar
	GK_USE, // E
	GK_ATTACK_USE, // left mouse
	GK_SECONDARY_ATTACK, // Z, mouse 4
	GK_CANCEL_ATTACK, // X
	GK_BLOCK, // right mouse
	GK_WALK, // shift
	GK_TOGGLE_WALK, // Caps Lock
	GK_AUTOWALK, // F
	GK_STATS, // C
	GK_INVENTORY, // I
	GK_TEAM_PANEL, // T
	GK_GUILD_PANEL, // G
	GK_ABILITY_PANEL, // K
	GK_JOURNAL, // J
	GK_MINIMAP, // Tab
	GK_TALK_BOX, // '"
	GK_QUICKSAVE, // F5
	GK_QUICKLOAD, // F9
	GK_TAKE_ALL, // F
	GK_MAP_SEARCH, // F
	GK_SELECT_DIALOG, // Enter
	GK_SKIP_DIALOG, // Space
	GK_PAUSE, // Pause
	GK_YELL, // Y
	GK_ROTATE_CAMERA, // V
	GK_SHORTCUT1, // 1
	GK_SHORTCUT2, // 2
	GK_SHORTCUT3, // 3
	GK_SHORTCUT4, // 4
	GK_SHORTCUT5, // 5
	GK_SHORTCUT6, // 6
	GK_SHORTCUT7, // 7
	GK_SHORTCUT8, // 8
	GK_SHORTCUT9, // 9
	GK_SHORTCUT10, // 0
	GK_CONSOLE, // ~
	GK_MAX
};

//-----------------------------------------------------------------------------
struct GameKey
{
	cstring id, text;
	Key key[2];

	void Set(Key k1 = Key::None, Key k2 = Key::None)
	{
		key[0] = k1;
		key[1] = k2;
	}

	Key& operator [] (int n)
	{
		assert(n == 0 || n == 1);
		return key[n];
	}

	const Key& operator [] (int n) const
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

	bool Down(GAME_KEYS gk) const
	{
		return input->Down(keys[gk][0]) || input->Down(keys[gk][1]);
	}
	Key DownR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(input->Down(k[0]))
			return k[0];
		else if(input->Down(k[1]))
			return k[1];
		else
			return Key::None;
	}
	Key PressedR(GAME_KEYS gk) const
	{
		const GameKey& k = keys[gk];
		if(input->Pressed(k[0]))
			return k[0];
		else if(input->Pressed(k[1]))
			return k[1];
		else
			return Key::None;
	}
	bool PressedRelease(GAME_KEYS gk)
	{
		return input->PressedRelease(keys[gk][0]) || input->PressedRelease(keys[gk][1]);
	}
	GameKey& operator [] (int n)
	{
		return keys[n];
	}

	bool AllowKeyboard() const { return IsSet(allow_input, ALLOW_KEYBOARD); }
	bool AllowMouse() const { return IsSet(allow_input, ALLOW_MOUSE); }
	bool KeyAllowed(Key k)
	{
		return IsSet(allow_input, KeyAllowState((byte)k));
	}
	Key KeyDoReturn(GAME_KEYS gk, Input::Func f)
	{
		GameKey& k = keys[gk];
		if(k[0] != Key::None)
		{
			if(KeyAllowed(k[0]) && (input->*f)(k[0]))
				return k[0];
		}
		if(k[1] != Key::None)
		{
			if(KeyAllowed(k[1]) && (input->*f)(k[1]))
				return k[1];
		}
		return Key::None;
	}
	Key KeyDoReturn(GAME_KEYS gk, Input::FuncC f)
	{
		return KeyDoReturn(gk, reinterpret_cast<Input::Func>(f));
	}
	Key KeyDoReturnIgnore(GAME_KEYS gk, Input::Func f, Key ignored_key)
	{
		GameKey& k = keys[gk];
		if(k[0] != Key::None && k[0] != ignored_key)
		{
			if(KeyAllowed(k[0]) && (input->*f)(k[0]))
				return k[0];
		}
		if(k[1] != Key::None && k[1] != ignored_key)
		{
			if(KeyAllowed(k[1]) && (input->*f)(k[1]))
				return k[1];
		}
		return Key::None;
	}
	Key KeyDoReturnIgnore(GAME_KEYS gk, Input::FuncC f, Key ignored_key)
	{
		return KeyDoReturnIgnore(gk, reinterpret_cast<Input::Func>(f), ignored_key);
	}
	bool KeyDo(GAME_KEYS gk, Input::Func f)
	{
		return KeyDoReturn(gk, f) != Key::None;
	}
	bool KeyDo(GAME_KEYS gk, Input::FuncC f)
	{
		return KeyDo(gk, reinterpret_cast<Input::Func>(f));
	}
	bool KeyDownAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &Input::Down);
	}
	bool KeyPressedReleaseAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &Input::PressedRelease);
	}
	bool KeyDownUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &Input::DownUp);
	}
	bool KeyDownUp(GAME_KEYS gk)
	{
		GameKey& k = keys[gk];
		if(k[0] != Key::None)
		{
			if(input->DownUp(k[0]))
				return true;
		}
		if(k[1] != Key::None)
		{
			if(input->DownUp(k[1]))
				return true;
		}
		return false;
	}
	bool KeyPressedUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &Input::PressedUp);
	}
	// Return if key is up if is allowed, otherwise return true
	bool KeyUpAllowed(Key key)
	{
		if(KeyAllowed(key))
			return input->Up(key);
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
				if(k.key[i] >= Key::F1 && k.key[i] <= Key::F12)
				{
					if(input->PressedRelease(k.key[i]))
						return true;
				}
			}
		}
		return KeyPressedReleaseAllowed(gk);
	}
	bool DebugKey(Key k)
	{
		return IsDebug() && input->Focus() && input->Down(k);
	}

	AllowInput allow_input;

private:
	GameKey keys[GK_MAX];
};
extern GameKeys GKey;
