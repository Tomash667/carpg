#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "SoundManager.h"
#include "Level.h"
#include "World.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
vector<MusicTrack*> MusicTrack::tracks;
extern string g_system_dir;

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

	for(MusicTrack* track : MusicTrack::tracks)
	{
		if(track->type == music_type)
		{
			assert(track->music);
			tracks.push_back(track);
		}
	}

	if(tracks.empty())
	{
		sound_mgr->PlayMusic(nullptr);
		return;
	}

	if(tracks.size() != 1)
	{
		Shuffle(tracks.begin(), tracks.end());

		if(tracks.front() == last_music)
			std::iter_swap(tracks.begin(), tracks.end() - 1);
	}

	track_id = 0;
	last_music = tracks.front();
	sound_mgr->PlayMusic(last_music->music);
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
			sound_mgr->PlayMusic(last_music->music);
		}
	}
}

//=================================================================================================
void Game::SetMusic()
{
	if(sound_mgr->IsMusicDisabled())
		return;

	if(world->IsBossLevel(Int2(game_level->location_index, game_level->dungeon_level)))
	{
		SetMusic(MusicType::Boss);
		return;
	}

	SetMusic(game_level->GetLocationMusic());
}

//=================================================================================================
uint MusicTrack::Load(uint& errors)
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

	Ptr<MusicTrack> track(nullptr);

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
							track.Ensure();
							track->music = res_mgr->TryGet<Music>(filename);
							if(track->music)
							{
								track->type = type;
								MusicTrack::tracks.push_back(track.Pin());
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
					track.Ensure();
					track->music = res_mgr->TryGet<Music>(filename);
					if(track->music)
					{
						track->type = type;
						MusicTrack::tracks.push_back(track.Pin());
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

	return MusicTrack::tracks.size();
}

//=================================================================================================
void MusicTrack::Cleanup()
{
	DeleteElements(MusicTrack::tracks);
}
