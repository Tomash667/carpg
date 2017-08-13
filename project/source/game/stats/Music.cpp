#include "Pch.h"
#include "Core.h"
#include "Game.h"

//-----------------------------------------------------------------------------
extern string g_system_dir;

//=================================================================================================
MusicType Game::GetLocationMusic()
{
	switch(location->type)
	{
	case L_CITY:
		return MusicType::City;
	case L_CRYPT:
		return MusicType::Crypt;
	case L_DUNGEON:
	case L_CAVE:
		return MusicType::Dungeon;
	case L_FOREST:
	case L_CAMP:
		if(current_location == secret_where2)
			return MusicType::Moonwell;
		else
			return MusicType::Forest;
	case L_ENCOUNTER:
		return MusicType::Travel;
	case L_MOONWELL:
		return MusicType::Moonwell;
	default:
		assert(0);
		return MusicType::Dungeon;
	}
}

//=================================================================================================
void Game::SetMusic(MusicType type)
{
	if(nomusic || type == music_type)
		return;

	music_type = type;
	SetupTracks();
}

//=================================================================================================
void Game::SetupTracks()
{
	tracks.clear();

	for(Music* music : musics)
	{
		if(music->type == music_type)
		{
			assert(music->music);
			tracks.push_back(music);
		}
	}

	if(tracks.empty())
	{
		PlayMusic(nullptr);
		return;
	}

	if(tracks.size() != 1)
	{
		std::random_shuffle(tracks.begin(), tracks.end(), MyRand);

		if(tracks.front() == last_music)
			std::iter_swap(tracks.begin(), tracks.end() - 1);
	}

	track_id = 0;
	last_music = tracks.front();
	PlayMusic(last_music->music->sound);
	music_ended = false;
}

//=================================================================================================
void Game::UpdateMusic()
{
	if(nomusic || music_type == MusicType::None || tracks.empty())
		return;

	if(music_ended)
	{
		if(music_type == MusicType::Intro)
		{
			if(game_state == GS_LOAD)
			{
				music_type = MusicType::None;
				PlayMusic(nullptr);
			}
			else
				SetMusic(MusicType::Title);
		}
		else if(track_id == tracks.size() - 1)
			SetupTracks();
		else
		{
			++track_id;
			last_music = tracks[track_id];
			PlayMusic(last_music->music->sound);
			music_ended = false;
		}
	}
}

//=================================================================================================
void Game::SetMusic()
{
	if(nomusic)
		return;

	if(!IsLocal() && boss_level_mp)
	{
		SetMusic(MusicType::Boss);
		return;
	}

	for(vector<Int2>::iterator it = boss_levels.begin(), end = boss_levels.end(); it != end; ++it)
	{
		if(current_location == it->x && dungeon_level == it->y)
		{
			SetMusic(MusicType::Boss);
			return;
		}
	}

	SetMusic(GetLocationMusic());
}

//=================================================================================================
uint Game::LoadMusicDatafile(uint& errors)
{
	Tokenizer t(Tokenizer::F_UNESCAPE);
	if(!t.FromFile(Format("%s/music.txt", g_system_dir.c_str())))
	{
		Error("Failed to open music.txt.");
		++errors;
		return 0;
	}

	t.AddKeyword("music", 0, 0);

	t.AddEnums<MusicType>(1, {
		{ "title", MusicType::Title },
		{ "forest", MusicType::Forest },
		{ "city", MusicType::City },
		{ "crypt", MusicType::Crypt },
		{ "dungeon", MusicType::Dungeon },
		{ "boss", MusicType::Boss },
		{ "travel", MusicType::Travel },
		{ "moonwell", MusicType::Moonwell },
		{ "death", MusicType::Death }
	});

	auto& sound_mgr = ResourceManager::Get<Sound>();
	Ptr<Music> music(nullptr);

	try
	{
		t.Next();
		while(!t.IsEof())
		{
			try
			{
				t.AssertKeyword(0, 0);
				t.Next();
				MusicType type = (MusicType)t.MustGetKeywordId(1);
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				if(t.IsSymbol('{'))
				{
					t.Next();
					if(!t.IsSymbol('}'))
					{
						while(true)
						{
							const string& filename = t.MustGetString();
							music.Ensure();
							music->music = sound_mgr.TryGetMusic(filename);
							if(music->music)
							{
								music->type = type;
								musics.push_back(music.Pin());
							}
							else
							{
								Error("Missing music file '%s'.", filename.c_str());
								++errors;
							}
							t.Next();
							if(t.IsSymbol('}'))
								break;
							t.AssertSymbol(',');
							t.Next();
						}
					}
					t.Next();
				}
				else
				{
					const string& filename = t.MustGetString();
					music.Ensure();
					music->music = sound_mgr.TryGetMusic(filename);
					if(music->music)
					{
						music->type = type;
						musics.push_back(music.Pin());
					}
					else
					{
						Error("Missing music file '%s'.", filename.c_str());
						++errors;
					}
					t.Next();
				}
			}
			catch(const Tokenizer::Exception& e)
			{
				Error("Failed to parse music list: %s", e.ToString());
				++errors;
				if(!t.SkipToKeywordGroup(0))
					break;
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		Error("Failed to parse music list: %s", e.ToString());
		++errors;
	}

	return musics.size();
}

//=================================================================================================
void Game::LoadMusic(MusicType type, bool new_load_screen, bool task)
{
	bool first = true;
	auto& sound_mgr = ResourceManager::Get<Sound>();

	for(Music* music : musics)
	{
		if(music->type == type)
		{
			if(first)
			{
				if(music->music->IsLoaded())
				{
					// music for this type is loaded
					return;
				}
				if(new_load_screen)
					sound_mgr.AddTaskCategory(txLoadMusic);
				first = false;
			}
			if(task)
				sound_mgr.AddLoadTask(music->music);
			else
				sound_mgr.Load(music->music);
		}
	}
}
