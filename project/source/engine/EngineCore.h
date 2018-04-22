#pragma once

#include "Core.h"

// Engine types
struct Sound;
struct StartupOptions;
class CustomCollisionWorld;
class SoundManager;

// FMod types
namespace FMOD
{
	class Channel;
	class ChannelGroup;
	class Sound;
	class System;
}
typedef FMOD::Sound* SOUND;
