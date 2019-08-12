#include "Pch.h"
#include "GameCore.h"
#include "NameHelper.h"
#include "Language.h"
#include "World.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
cstring txNameFrom, txNameSonOf, txNameSonOfPost, txNameSonOfInvalid, txNamePrefix;
vector<string> name_random, nickname_random, crazy_name;

//=================================================================================================
void NameHelper::SetHeroNames()
{
	txNameFrom = Str("name_from");
	txNameSonOf = Str("name_sonOf");
	txNameSonOfPost = Str("name_sonOfPost");
	txNameSonOfInvalid = Str("name_sonOfInvalid");
	txNamePrefix = Str("name_prefix");
}

//=================================================================================================
void NameHelper::GenerateHeroName(HeroData& hero)
{
	return GenerateHeroName(hero.unit->GetClass(), IsSet(hero.unit->data->flags, F_CRAZY), hero.name);
}

//=================================================================================================
void NameHelper::GenerateHeroName(Class clas, bool crazy, string& hero_name)
{
	if(crazy)
	{
		hero_name = RandomItem(crazy_name);
		return;
	}

	ClassInfo& ci = ClassInfo::classes[(int)clas];
	if(Rand() % 2 == 0 && !ci.names.empty())
		hero_name = RandomItem(ci.names);
	else
		hero_name = RandomItem(name_random);

	hero_name += " ";

	int type = Rand() % 7;
	if(type < 4 && !ci.nicknames.empty())
	{
		hero_name += txNamePrefix;
		hero_name += RandomItem(ci.nicknames);
	}
	else if((type == 0 || type == 4) && !W.GetLocations().empty())
	{
		hero_name += txNameFrom;
		hero_name += W.GetRandomSettlement()->name;
	}
	else if(type == 0 || type == 1 || type == 4 || type == 5)
	{
		hero_name += txNamePrefix;
		hero_name += RandomItem(nickname_random);
	}
	else
	{
		cstring who;
		if(txNameSonOfInvalid[0])
		{
			do
			{
				who = RandomItem(name_random).c_str();
			} while(who[strlen(who) - 1] == txNameSonOfInvalid[0]);
		}
		else
			who = RandomItem(name_random).c_str();
		hero_name += txNameSonOf;
		hero_name += who;
		hero_name += txNameSonOfPost;
	}
}
