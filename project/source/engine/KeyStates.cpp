#include "Pch.h"
#include "Core.h"
#include "KeyStates.h"

// change keys state from pressed->down and released->up
void KeyStates::Update()
{
	byte printscreen = keystate[VK_SNAPSHOT];
	for(uint i = 0; i < 256; ++i)
	{
		if(keystate[i] & 1)
			--keystate[i];
		keyrepeat[i] = false;
	}
	for(uint i = 0; i < 5; ++i)
		doubleclk[i] = false;
	if(printscreen == IS_PRESSED)
		keystate[VK_SNAPSHOT] = IS_RELEASED;
	for(byte k : to_release)
		keystate[k] = IS_RELEASED;
	to_release.clear();
}

void KeyStates::UpdateShortcuts()
{
	shortcut_state = 0;
	if(Down(VK_SHIFT))
		shortcut_state |= KEY_SHIFT;
	if(Down(VK_CONTROL))
		shortcut_state |= KEY_CONTROL;
	if(Down(VK_MENU))
		shortcut_state |= KEY_ALT;
}

// release all pressed/down keys
void KeyStates::ReleaseKeys()
{
	for(uint i = 0; i < 255; ++i)
	{
		if(keystate[i] & 0x2)
			keystate[i] = IS_RELEASED;
	}
	for(uint i = 0; i < 5; ++i)
		doubleclk[i] = false;
	to_release.clear();
}

// handle key down/up
void KeyStates::Process(byte key, bool down)
{
	auto& k = keystate[key];
	if(key != VK_SNAPSHOT)
	{
		if(down)
		{
			if(k <= IS_RELEASED)
				k = IS_PRESSED;
			keyrepeat[key] = true;
		}
		else
		{
			if(k == IS_PRESSED)
				to_release.push_back(key);
			else if(k == IS_DOWN)
				k = IS_RELEASED;
		}
	}
	else
		k = IS_PRESSED;
}

void KeyStates::ProcessDoubleClick(byte key)
{
	assert(key >= VK_LBUTTON && key <= VK_XBUTTON2);
	Process(key, true);
	doubleclk[key] = true;
}
