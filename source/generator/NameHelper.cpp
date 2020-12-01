#include "Pch.h"
#include "NameHelper.h"

#include "Language.h"
#include "OutsideLocation.h"
#include "Unit.h"
#include "World.h"

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
void NameHelper::GenerateHeroName(Hero& hero)
{
	return GenerateHeroName(hero.unit->GetClass(), IsSet(hero.unit->data->flags, F_CRAZY), hero.name);
}

//=================================================================================================
void NameHelper::GenerateHeroName(const Class* clas, bool crazy, string& hero_name)
{
	if(crazy)
	{
		hero_name = RandomItem(crazy_name);
		return;
	}

	int index = Rand() % (clas->names.size() + name_random.size());
	if(index < (int)clas->names.size())
		hero_name = clas->names[index];
	else
		hero_name = name_random[index - clas->names.size()];

	hero_name += " ";

	int type = Rand() % 7;
	if(type < 4 && !clas->nicknames.empty())
	{
		hero_name += txNamePrefix;
		hero_name += RandomItem(clas->nicknames);
	}
	else if((type == 0 || type == 4) && !world->GetLocations().empty())
	{
		hero_name += txNameFrom;
		if(IsSet(clas->flags, Class::F_FROM_FOREST))
			hero_name += world->GetRandomLocation([](Location* loc) { return loc->type == L_OUTSIDE && loc->target == FOREST; })->name;
		else
			hero_name += world->GetRandomSettlement()->name;
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
			}
			while(who[strlen(who) - 1] == txNameSonOfInvalid[0]);
		}
		else
			who = RandomItem(name_random).c_str();
		hero_name += txNameSonOf;
		hero_name += who;
		hero_name += txNameSonOfPost;
	}
}
