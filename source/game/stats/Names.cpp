#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
cstring txNameFrom, txNameSonOf, txNameSonOfPost, txNameSonOfInvalid, txNamePrefix;
vector<string> name_random, nickname_random, crazy_name;

//=================================================================================================
void LoadStrArray(vector<cstring>& items, cstring name)
{
	assert(name);

	cstring scount = StrT(Format("%s_count", name), false);
	if(!scount)
		goto err;
	int count = atoi(scount);
	if(count <= 0)
		goto err;

	items.resize(count);
	for(int i=0; i<count; ++i)
		items[i] = Str(Format("%s%d", name, i));
	return;

err:
	ERROR(Format("Missing texts for array '%s'.", name));
	items.push_back("!MissingArray!");
}

//=================================================================================================
void Game::SetHeroNames()
{
	txNameFrom = Str("name_from");
	txNameSonOf = Str("name_sonOf");
	txNameSonOfPost = Str("name_sonOfPost");
	txNameSonOfInvalid = Str("name_sonOfInvalid");
	txNamePrefix = Str("name_prefix");
}

//=================================================================================================
void Game::GenerateHeroName(HeroData& hero)
{
	return GenerateHeroName(hero.clas, IS_SET(hero.unit->data->flags, F_CRAZY), hero.name);
}

//=================================================================================================
void Game::GenerateHeroName(Class clas, bool szalony, string& hero_name)
{
	if(szalony)
	{
		hero_name = random_item(crazy_name);
		return;
	}

	ClassInfo& ci = g_classes[(int)clas];
	if(rand2()%2 == 0)
		hero_name = random_item(ci.names);
	else
		hero_name = random_item(name_random);

	hero_name += " ";

	int co = rand2()%7;
	if(co == 0)
	{
		cstring kto;
		if(txNameSonOfInvalid[0])
		{
			do 
			{
				kto = random_item(name_random).c_str();
			}
			while(kto[strlen(kto)-1] == txNameSonOfInvalid[0]);
		}
		else
			kto = random_item(name_random).c_str();
		hero_name += txNameSonOf;
		hero_name += kto;
		hero_name += txNameSonOfPost;
	}
	else if(co == 1 && !locations.empty())
	{
		hero_name += txNameFrom;
		hero_name += locations[rand2()%cities]->name;
	}
	else if(in_range(co, 2, 5))
	{
		hero_name += txNamePrefix;
		hero_name += random_item(ci.nicknames);
	}
	else
	{
		hero_name += txNamePrefix;
		hero_name += random_item(nickname_random);
	}
}
