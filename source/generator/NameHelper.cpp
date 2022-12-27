#include "Pch.h"
#include "NameHelper.h"

#include "Language.h"
#include "OutsideLocation.h"
#include "Unit.h"
#include "World.h"

//-----------------------------------------------------------------------------
cstring txNameFrom, txNameSonOf, txNameSonOfPost, txNameSonOfInvalid, txNamePrefix;
vector<string> nameRandom, nicknameRandom, crazyName;

//=================================================================================================
void NameHelper::SetHeroNames()
{
	txNameFrom = Str("nameFrom");
	txNameSonOf = Str("nameSonOf");
	txNameSonOfPost = Str("nameSonOfPost");
	txNameSonOfInvalid = Str("nameSonOfInvalid");
	txNamePrefix = Str("namePrefix");
}

//=================================================================================================
void NameHelper::GenerateHeroName(Hero& hero)
{
	return GenerateHeroName(hero.unit->GetClass(), IsSet(hero.unit->data->flags, F_CRAZY), hero.name);
}

//=================================================================================================
void NameHelper::GenerateHeroName(Class* clas, bool crazy, string& name)
{
	if(crazy)
	{
		name = RandomItem(crazyName);
		return;
	}

	int index = Rand() % (clas->names.size() + nameRandom.size());
	if(index < (int)clas->names.size())
		name = clas->names[index];
	else
		name = nameRandom[index - clas->names.size()];

	name += " ";

	int type = Rand() % 7;
	if(type < 4 && !clas->nicknames.empty())
	{
		name += txNamePrefix;
		name += RandomItem(clas->nicknames);
	}
	else if((type == 0 || type == 4) && !world->GetLocations().empty())
	{
		name += txNameFrom;
		if(IsSet(clas->flags, Class::F_FROM_FOREST))
			name += world->GetRandomLocation([](Location* loc) { return loc->type == L_OUTSIDE && loc->target == FOREST; })->name;
		else
			name += world->GetRandomSettlement()->name;
	}
	else if(type == 0 || type == 1 || type == 4 || type == 5)
	{
		name += txNamePrefix;
		name += RandomItem(nicknameRandom);
	}
	else
	{
		cstring who;
		if(txNameSonOfInvalid[0])
		{
			do
			{
				who = RandomItem(nameRandom).c_str();
			}
			while(who[strlen(who) - 1] == txNameSonOfInvalid[0]);
		}
		else
			who = RandomItem(nameRandom).c_str();
		name += txNameSonOf;
		name += who;
		name += txNameSonOfPost;
	}
}
