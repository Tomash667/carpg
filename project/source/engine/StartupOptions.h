#pragma once

struct StartupOptions
{
	cstring title;
	Int2 size, force_size, force_pos;
	int sound_volume, music_volume;
	bool fullscreen, vsync, hidden_window, nosound, nomusic;

	StartupOptions() : title(nullptr), size(0, 0), force_size(-1, -1), force_pos(-1, -1), fullscreen(false), vsync(true), hidden_window(false), nosound(false),
		nomusic(false), sound_volume(100), music_volume(100)
	{
	}
};
