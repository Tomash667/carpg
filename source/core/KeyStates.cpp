#include "Pch.h"
#include "Base.h"
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
	if(printscreen == IS_PRESSED)
		keystate[VK_SNAPSHOT] = IS_RELEASED;
}

// release all pressed/down keys
void KeyStates::ReleaseKeys()
{
	for(byte i = 0; i<255; ++i)
	{
		if(keystate[i] & 0x2)
			keystate[i] = IS_RELEASED;
	}
}

// handle key down/up
void KeyStates::Process(byte key, bool down)
{
	if(key != VK_SNAPSHOT)
	{
		if(down)
		{
			if(keystate[key] <= IS_RELEASED)
				keystate[key] = IS_PRESSED;
			keyrepeat[key] = true;
		}
		else
		{
			if(keystate[key] >= IS_DOWN)
				keystate[key] = IS_RELEASED;
		}
	}
	else
		keystate[key] = IS_PRESSED;
}
