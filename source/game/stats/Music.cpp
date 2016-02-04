#include "Pch.h"
#include "Base.h"
#include "Music.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
extern string g_system_dir;
vector<Music*> g_musics;

//=================================================================================================
void LoadMusicDatafile()
{
	Tokenizer t(Tokenizer::F_UNESCAPE);
	if(!t.FromFile(Format("%s/music.txt", g_system_dir.c_str())))
		throw "Failed to open music.txt.";

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

	Music* music = nullptr;
	ResourceManager& resMgr = ResourceManager::Get();

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
							if(!music)
								music = new Music;
							music->music = resMgr.TryGetMusic(filename);
							if(music->music)
							{
								music->type = type;
								g_musics.push_back(music);
								music = nullptr;
							}
							else
								ERROR(Format("Missing music file '%s'.", filename.c_str()));
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
					if(!music)
						music = new Music;
					music->music = resMgr.TryGetMusic(filename);
					if(music->music)
					{
						music->type = type;
						g_musics.push_back(music);
						music = nullptr;
					}
					else
						ERROR(Format("Missing music file '%s'.", filename.c_str()));
					t.Next();
				}
			}
			catch(const Tokenizer::Exception& e)
			{
				ERROR(Format("Failed to parse music list: %s", e.ToString()));
				if(!t.SkipToKeywordGroup(0))
					break;
			}
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		ERROR(Format("Failed to parse music list: %s", e.ToString()));
	}

	delete music;
}

//=================================================================================================
void LoadMusic(MusicType type)
{
	bool first = true;
	ResourceManager& resMgr = ResourceManager::Get();

	for(Music* music : g_musics)
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
				resMgr.AddTaskCategory("");
				first = false;
			}
			resMgr.LoadMusic(music->music);
		}
	}
}
