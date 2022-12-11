#include "Pch.h"
#include "MusicListLoader.h"

#include "MusicType.h"

#include <ResourceManager.h>

MusicList* musicLists[(int)MusicType::Max];

enum KeywordGroup
{
	G_TOP,
	G_TYPE
};

//=================================================================================================
void MusicListLoader::DoLoading()
{
	for(int i = 0; i < (int)MusicType::Max; ++i)
	{
		if(!musicLists[i])
			musicLists[i] = new MusicList;
	}

	bool requireId = false;
	Load("music.txt", G_TOP, &requireId);
}

//=================================================================================================
void MusicListLoader::Cleanup()
{
	for(int i = 0; i < (int)MusicType::Max; ++i)
		delete musicLists[i];
}

//=================================================================================================
void MusicListLoader::InitTokenizer()
{
	t.AddKeyword("music", 0, G_TOP);

	t.AddKeywords<MusicType>(G_TYPE, {
		{ "title", MusicType::Title },
		{ "forest", MusicType::Forest },
		{ "city", MusicType::City },
		{ "crypt", MusicType::Crypt },
		{ "dungeon", MusicType::Dungeon },
		{ "boss", MusicType::Boss },
		{ "travel", MusicType::Travel },
		{ "moonwell", MusicType::Moonwell },
		{ "death", MusicType::Death },
		{ "empty_city", MusicType::EmptyCity }
		});
}

//=================================================================================================
void MusicListLoader::LoadEntity(int top, const string& id)
{
	int type = t.MustGetKeywordId(G_TYPE);
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
				Music* music = resMgr->TryGet<Music>(filename);
				if(music)
				{
					musicLists[type]->musics.push_back(music);
					++count;
				}
				else
					LoadError("Missing music file '%s'.", filename.c_str());
				t.Next();
				if(t.IsSymbol('}'))
					break;
				t.AssertSymbol(',');
				t.Next();
			}
		}
	}
	else
	{
		const string& filename = t.MustGetString();
		Music* music = resMgr->TryGet<Music>(filename);
		if(music)
		{
			musicLists[type]->musics.push_back(music);
			++count;
		}
		else
			LoadError("Missing music file '%s'.", filename.c_str());
	}
}

//=================================================================================================
void MusicListLoader::Finalize()
{
	Info("Loaded music: %u.", count);
}
