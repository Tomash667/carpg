#pragma once

//-----------------------------------------------------------------------------
// dodatkowe definicje klawiszy
#define VK_NONE 0

//-----------------------------------------------------------------------------
// stan klawisza
enum InputState
{
	IS_UP,			// 00
	IS_RELEASED,	// 01
	IS_DOWN,		// 10
	IS_PRESSED		// 11
};

enum ShortcutKey
{
	KEY_SHIFT = 1 << 0,
	KEY_CONTROL = 1 << 1,
	KEY_ALT = 1 << 2
};

//-----------------------------------------------------------------------------
// stan klawiatury
struct KeyStates
{
	// proste sprawdzanie czy klawisz zosta³ wciœniêty, wyciœniêty, jest wciœniêty, jest wyciœniêty
	bool Pressed(byte key) const { return keystate[key] == IS_PRESSED; }
	bool Released(byte key) const { return keystate[key] == IS_RELEASED; }
	bool Down(byte key) const { return keystate[key] >= IS_DOWN; }
	bool Up(byte key) const { return keystate[key] <= IS_RELEASED; }

	// jednorazowe sprawdzanie czy klawisz jest wciœniêty, jeœli by³ to go ustawia na wyciœniêtego
	bool PressedRelease(byte key)
	{
		if(Pressed(key))
		{
			keystate[key] = IS_DOWN;
			return true;
		}
		else
			return false;
	}

	bool PressedUp(byte key)
	{
		if(Pressed(key))
		{
			keystate[key] = IS_UP;
			return true;
		}
		else
			return false;
	}

	// sprawdza czy jeden z dwóch klawiszy zosta³ wciœniêty
	byte Pressed2(byte k1, byte k2) const { return ReturnState2(k1, k2, IS_PRESSED); }
	// jw ale ustawia na wyciœniêty
	byte Pressed2Release(byte k1, byte k2)
	{
		if(keystate[k1] == IS_PRESSED)
		{
			keystate[k1] = IS_DOWN;
			return k1;
		}
		else if(keystate[k2] == IS_PRESSED)
		{
			keystate[k2] = IS_DOWN;
			return k2;
		}
		else
			return VK_NONE;
	}

	// sprawdza czy zosta³a wprowadzona kombinacja klawiszy (np alt+f4)
	bool DownPressed(byte k1, byte k2) const { return ((Down(k1) && Pressed(k2)) || (Down(k2) && Pressed(k1))); }

	// zwraca który z podanych klawiszy ma taki stan
	byte ReturnState2(byte k1, byte k2, InputState state) const
	{
		if(keystate[k1] == state)
			return k1;
		else if(keystate[k2] == state)
			return k2;
		else
			return VK_NONE;
	}

	// ustawia stan klawisza
	void SetState(byte key, InputState istate) { keystate[key] = (byte)istate; }

	void Update();
	void UpdateShortcuts();
	void ReleaseKeys();
	void Process(byte key, bool down);
	void ProcessDoubleClick(byte key);

	byte* GetKeystateData()
	{
		return keystate;
	}

	void SetFocus(bool f) { focus = f; }
	bool Focus() const { return focus; }

	// shortcut, checks if other modifiers are not down
	// for example: Ctrl+A, shift and alt must not be pressed
	bool Shortcut(int modifier, byte key, bool up = true)
	{
		if(shortcut_state == modifier && Pressed(key))
		{
			if(up)
				SetState(key, IS_DOWN);
			return true;
		}
		else
			return false;
	}

	bool DownUp(byte key)
	{
		if(Down(key))
		{
			keystate[key] = IS_UP;
			return true;
		}
		else
			return false;
	}

	bool DownRepeat(byte key)
	{
		return Down(key) && keyrepeat[key];
	}

	bool DoubleClick(byte key)
	{
		assert(key >= VK_LBUTTON && key <= VK_XBUTTON2);
		return doubleclk[key];
	}

private:
	byte keystate[256];
	bool keyrepeat[256];
	bool doubleclk[5];
	int shortcut_state;
	bool focus;
};
extern KeyStates Key;

//-----------------------------------------------------------------------------
typedef bool (KeyStates::*KeyF)(byte);
typedef bool (KeyStates::*KeyFC)(byte) const;
