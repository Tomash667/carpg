#include "Pch.h"
#include "EngineCore.h"
#include "SoundManager.h"
#include "StartupOptions.h"
#include "Stream.h"
#include "ResourceManager.h"
#include <fmod.hpp>

//=================================================================================================
SoundManager::SoundManager() : system(nullptr), current_music(nullptr), music_ended(false)
{
}

//=================================================================================================
SoundManager::~SoundManager()
{
	if(system)
		system->release();
}

//=================================================================================================
void SoundManager::Init(StartupOptions& options)
{
	nosound = options.nosound;
	nomusic = options.nomusic;
	sound_volume = options.sound_volume;
	music_volume = options.music_volume;
	disabled_sound = nosound && nomusic;
	play_sound = !nosound && sound_volume > 0;

	// if disabled, log it
	if(disabled_sound)
	{
		Info("Engine: Sound and music is disabled.");
		return;
	}

	// create FMOD system
	FMOD_RESULT result = FMOD::System_Create(&system);
	if(result != FMOD_OK)
		throw Format("Engine: Failed to create FMOD system (%d).", result);

	// get number of drivers
	int count;
	result = system->getNumDrivers(&count);
	if(result != FMOD_OK)
		throw Format("Engine: Failed to get FMOD number of drivers (%d).", result);
	if(count == 0)
	{
		Warn("Engine: No sound drivers.");
		disabled_sound = true;
		return;
	}

	// log drivers
	Info("Engine: Sound drivers (%d):", count);
	for(int i = 0; i < count; ++i)
	{
		result = system->getDriverInfo(i, BUF, 256, nullptr);
		if(result == FMOD_OK)
			Info("Engine: Driver %d - %s", i, BUF);
		else
			Error("Engine: Failed to get driver %d info (%d).", i, result);
	}

	// get info about selected driver and output device
	int driver;
	FMOD_OUTPUTTYPE output;
	system->getDriver(&driver);
	system->getOutput(&output);
	Info("Engine: Using driver %d and output type %d.", driver, output);

	// initialize FMOD system
	const int tries = 3;
	bool ok = false;
	for(int i = 0; i < 3; ++i)
	{
		result = system->init(128, FMOD_INIT_NORMAL, nullptr);
		if(result != FMOD_OK)
		{
			Error("Engine: Failed to initialize FMOD system (%d).", result);
			Sleep(100);
		}
		else
		{
			ok = true;
			break;
		}
	}
	if(!ok)
	{
		Error("Engine: Failed to initialize FMOD, disabling sound!");
		disabled_sound = true;
		return;
	}

	// create group for sounds and music
	system->createChannelGroup("default", &group_default);
	system->createChannelGroup("music", &group_music);
	group_default->setVolume(float(nosound ? 0 : sound_volume) / 100);
	group_music->setVolume(float(nomusic ? 0 : music_volume) / 100);

	Info("Engine: FMOD sound system created.");
}

//=================================================================================================
void SoundManager::Update(float dt)
{
	if(disabled_sound)
		return;

	bool deletions = false;
	float volume;

	for(vector<FMOD::Channel*>::iterator it = fallbacks.begin(), end = fallbacks.end(); it != end; ++it)
	{
		(*it)->getVolume(&volume);
		if((volume -= dt) <= 0.f)
		{
			(*it)->stop();
			*it = nullptr;
			deletions = true;
		}
		else
			(*it)->setVolume(volume);
	}

	if(deletions)
		RemoveNullElements(fallbacks);

	if(current_music)
	{
		current_music->getVolume(&volume);
		if(volume != 1.f)
		{
			volume = min(1.f, volume + dt);
			current_music->setVolume(volume);
		}

		bool playing;
		current_music->isPlaying(&playing);
		if(!playing)
			music_ended = true;
	}

	system->update();
}

//=================================================================================================
int SoundManager::LoadSound(Sound* sound)
{
	assert(sound);

	int flags = FMOD_HARDWARE | FMOD_LOWMEM;
	if(sound->is_music)
		flags |= FMOD_2D;
	else
		flags |= FMOD_3D | FMOD_LOOP_OFF;

	FMOD_RESULT result;
	if(sound->IsFile())
		result = system->createStream(sound->path.c_str(), flags, nullptr, &sound->sound);
	else
	{
		BufferHandle&& buf = ResourceManager::Get().GetBuffer(sound);
		FMOD_CREATESOUNDEXINFO info = { 0 };
		info.cbsize = sizeof(info);
		info.length = buf->Size();
		result = system->createStream((cstring)buf->Data(), flags | FMOD_OPENMEMORY, &info, &sound->sound);
		if(result == FMOD_OK)
			sound_bufs.push_back(buf.Pin());
	}

	return (int)result;
}

//=================================================================================================
void SoundManager::PlayMusic(FMOD::Sound* music)
{
	if(nomusic || (!music && !current_music))
	{
		music_ended = true;
		return;
	}

	music_ended = false;
	if(music && current_music)
	{
		FMOD::Sound* music_sound;
		current_music->getCurrentSound(&music_sound);

		if(music_sound == music)
			return;
	}

	if(current_music)
		fallbacks.push_back(current_music);

	if(music)
	{
		system->playSound(FMOD_CHANNEL_FREE, music, true, &current_music);
		current_music->setVolume(0.f);
		current_music->setPaused(false);
		current_music->setChannelGroup(group_music);
	}
	else
		current_music = nullptr;
}

//=================================================================================================
void SoundManager::PlaySound2d(FMOD::Sound* sound)
{
	assert(sound);

	if(!play_sound)
		return;

	FMOD::Channel* channel;
	system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
	channel->setChannelGroup(group_default);
	playing_sounds.push_back(channel);
}

//=================================================================================================
void SoundManager::PlaySound3d(FMOD::Sound* sound, const Vec3& pos, float smin, float smax)
{
	assert(sound);

	if(!play_sound)
		return;

	FMOD::Channel* channel;
	system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(smin, 10000.f/*smax*/);
	channel->setPaused(false);
	channel->setChannelGroup(group_default);
	playing_sounds.push_back(channel);
}

//=================================================================================================
FMOD::Channel* SoundManager::CreateChannel(FMOD::Sound* sound, const Vec3& pos, float smin)
{
	assert(play_sound && sound);

	FMOD::Channel* channel;
	system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
	channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
	channel->set3DMinMaxDistance(smin, 10000.f/*smax*/);
	channel->setChannelGroup(group_default);
	channel->setPaused(false);
	playing_sounds.push_back(channel);
	return channel;
}

//=================================================================================================
void SoundManager::StopSounds()
{
	if(disabled_sound)
		return;
	for(vector<FMOD::Channel*>::iterator it = playing_sounds.begin(), end = playing_sounds.end(); it != end; ++it)
		(*it)->stop();
	playing_sounds.clear();
}

//=================================================================================================
void SoundManager::SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up)
{
	if(disabled_sound)
		return;
	system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&pos, nullptr, (const FMOD_VECTOR*)&dir, (const FMOD_VECTOR*)&up);
}

//=================================================================================================
void SoundManager::SetSoundVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	sound_volume = volume;
	play_sound = !nosound && volume > 0;
	if(!disabled_sound)
		group_default->setVolume(float(nosound ? 0 : sound_volume) / 100);
}

//=================================================================================================
void SoundManager::SetMusicVolume(int volume)
{
	assert(InRange(volume, 0, 100));
	music_volume = volume;
	if(!disabled_sound)
		group_music->setVolume(float(nomusic ? 0 : music_volume) / 100);
}

//=================================================================================================
bool SoundManager::UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos)
{
	assert(channel);

	bool is_playing;
	if(channel->isPlaying(&is_playing) == FMOD_OK && is_playing)
	{
		channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool SoundManager::IsPlaying(FMOD::Channel* channel)
{
	assert(channel);
	bool is_playing;
	if(channel->isPlaying(&is_playing) == FMOD_OK)
		return is_playing;
	else
		return false;
}
