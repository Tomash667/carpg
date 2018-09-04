#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Content.h"
#include "SoundManager.h"
#include "Level.h"
#include "World.h"
#include "QuestManager.h"
#include "Quest_Secret.h"

//-----------------------------------------------------------------------------
vector<Music*> Music::musics;
extern string g_system_dir;

//=================================================================================================
MusicType Game::GetLocationMusic()
{
	switch(L.location->type)
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
		if(L.location_index == QM.quest_secret->where2)
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
	if(sound_mgr->IsMusicDisabled() || type == music_type)
		return;

	music_type = type;
	SetupTracks();
}

//=================================================================================================
void Game::SetupTracks()
{
	tracks.clear();

	for(Music* music : Music::musics)
	{
		if(music->type == music_type)
		{
			assert(music->music);
			tracks.push_back(music);
		}
	}

	if(tracks.empty())
	{
		sound_mgr->PlayMusic(nullptr);
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
	sound_mgr->PlayMusic(last_music->music->sound);
}

//=================================================================================================
void Game::UpdateMusic()
{
	if(sound_mgr->IsMusicDisabled() || music_type == MusicType::None || tracks.empty())
		return;

	if(sound_mgr->IsMusicEnded())
	{
		if(music_type == MusicType::Intro)
		{
			if(game_state == GS_LOAD)
			{
				music_type = MusicType::None;
				sound_mgr->PlayMusic(nullptr);
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
			sound_mgr->PlayMusic(last_music->music->sound);
		}
	}
}

//=================================================================================================
void Game::SetMusic()
{
	if(sound_mgr->IsMusicDisabled())
		return;

	if(W.IsBossLevel(Int2(L.location_index, dungeon_level)))
	{
		SetMusic(MusicType::Boss);
		return;
	}

	SetMusic(GetLocationMusic());
}

//=================================================================================================
uint LoadMusics(uint& errors)
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
								Music::musics.push_back(music.Pin());
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
						Music::musics.push_back(music.Pin());
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

	return  Music::musics.size();
}

//=================================================================================================
void Game::LoadMusic(MusicType type, bool new_load_screen, bool task)
{
	bool first = true;
	auto& sound_mgr = ResourceManager::Get<Sound>();

	for(Music* music : Music::musics)
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

//=================================================================================================
void content::CleanupMusics()
{
	DeleteElements(Music::musics);
}
