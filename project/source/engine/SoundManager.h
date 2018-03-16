#pragma once

class SoundManager
{
public:
	SoundManager();
	~SoundManager();
	void Init(StartupOptions& options);
	void Update(float dt);
	int LoadSound(Sound* sound);
	void PlayMusic(FMOD::Sound* music);
	void PlaySound2d(FMOD::Sound* sound);
	void PlaySound3d(FMOD::Sound* sound, const Vec3& pos, float smin, float smax = 0.f); // smax jest nieu�ywane
	FMOD::Channel* CreateChannel(FMOD::Sound* sound, const Vec3& pos, float smin);
	void StopSounds();
	void SetListenerPosition(const Vec3& pos, const Vec3& dir, const Vec3& up = Vec3(0, 1, 0));
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);
	bool UpdateChannelPosition(FMOD::Channel* channel, const Vec3& pos);

	bool IsPlaying(FMOD::Channel* channel);
	bool IsDisabled() const { return disabled_sound; }
	bool IsSoundDisabled() const { return nosound; }
	bool IsMusicDisabled() const { return nomusic; }
	bool IsMusicEnded() const { return music_ended; }
	bool CanPlaySound() const { return play_sound; }
	int GetSoundVolume() const { return sound_volume; }
	int GetMusicVolume() const { return music_volume; }

private:
	FMOD::System* system;
	FMOD::ChannelGroup* group_default, *group_music;
	FMOD::Channel* current_music;
	vector<FMOD::Channel*> playing_sounds;
	vector<FMOD::Channel*> fallbacks;
	vector<Buffer*> sound_bufs;
	int sound_volume, music_volume; // 0-100
	bool music_ended, disabled_sound, play_sound, nosound, nomusic;
};
