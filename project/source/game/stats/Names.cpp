#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
cstring txNameFrom, txNameSonOf, txNameSonOfPost, txNameSonOfInvalid, txNamePrefix;
vector<string> name_random, nickname_random, crazy_name;

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
	GenerateHeroName(hero.clas, IS_SET(hero.unit->data->flags, F_CRAZY), hero.name);
}

//=================================================================================================
void Game::GenerateHeroName(ClassId clas, bool crazy, string& hero_name)
{
	if(crazy)
	{
		hero_name = random_item(crazy_name);
		return;
	}

	Class& c = *Class::classes[(int)clas];
	if(Rand() % 2 == 0)
		hero_name = random_item(c.names);
	else
		hero_name = random_item(name_random);

	hero_name += " ";

	int type = Rand() % 7;
	if(type == 0)
	{
		cstring kto;
		if(txNameSonOfInvalid[0])
		{
			do
			{
				kto = random_item(name_random).c_str();
			} while(kto[strlen(kto) - 1] == txNameSonOfInvalid[0]);
		}
		else
			kto = random_item(name_random).c_str();
		hero_name += txNameSonOf;
		hero_name += kto;
		hero_name += txNameSonOfPost;
	}
	else if(type == 1 && !locations.empty())
	{
		hero_name += txNameFrom;
		hero_name += locations[Rand() % settlements]->name;
	}
	else if(InRange(type, 2, 5))
	{
		hero_name += txNamePrefix;
		hero_name += random_item(c.nicknames);
	}
	else
	{
		hero_name += txNamePrefix;
		hero_name += random_item(nickname_random);
	}
}
