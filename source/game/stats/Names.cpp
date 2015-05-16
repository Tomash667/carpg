#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
cstring txNameFrom, txNameSonOf, txNameSonOfPost, txNameSonOfInvalid, txNamePrefix;
vector<cstring> imie_losowe, przyd_losowy, imie_woj, przyd_woj, imie_lotr, przyd_lotr, imie_lowc, przyd_lowc, imie_mag, przyd_mag, imie_szalony, przyd_szalony;


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
void Game::LoadNames()
{
	txNameFrom = Str("name_from");
	txNameSonOf = Str("name_sonOf");
	txNameSonOfPost = Str("name_sonOfPost");
	txNameSonOfInvalid = Str("name_sonOfInvalid");
	txNamePrefix = Str("name_prefix");
	LoadStrArray(imie_losowe, "name_random");
	LoadStrArray(przyd_losowy, "surname_random");
	LoadStrArray(imie_woj, "name_warrior");
	LoadStrArray(przyd_woj, "surname_warrior");
	LoadStrArray(imie_lowc, "name_hunter");
	LoadStrArray(przyd_lowc, "surname_hunter");
	LoadStrArray(imie_lotr, "name_rogue");
	LoadStrArray(przyd_lotr, "surname_rogue");
	LoadStrArray(imie_mag, "name_mage");
	LoadStrArray(przyd_mag, "surname_mage");
	LoadStrArray(imie_szalony, "name_crazy");
}

//=================================================================================================
void Game::GenerateHeroName(HeroData& hero)
{
	return GenerateHeroName(hero.clas, IS_SET(hero.unit->data->flagi, F_SZALONY), hero.name);
}

//=================================================================================================
void Game::GenerateHeroName(CLASS clas, bool szalony, string& name)
{
	if(szalony)
	{
		name = random_item(imie_szalony);
		return;
	}

	vector<cstring>* imie, *przyd;
	if(clas == MAGE)
	{
		imie = &imie_mag;
		przyd = &przyd_mag;
	}
	else if(clas == WARRIOR)
	{
		imie = &imie_woj;
		przyd = &przyd_woj;
	}
	else if(clas == HUNTER)
	{
		imie = &imie_lowc;
		przyd = &przyd_lowc;
	}
	else
	{
		imie = &imie_lotr;
		przyd = &przyd_lotr;
	}

	if(rand2()%2 == 0)
		name = random_item(*imie);
	else
		name = random_item(imie_losowe);

	name += " ";

	int co = rand2()%7;
	if(co == 0)
	{
		cstring kto;
		if(txNameSonOfInvalid[0])
		{
			do 
			{
				kto = random_item(imie_losowe);
			}
			while(kto[strlen(kto)-1] == txNameSonOfInvalid[0]);
		}
		else
			kto = random_item(imie_losowe);
		name += txNameSonOf;
		name += kto;
		name += txNameSonOfPost;
	}
	else if(co == 1 && !locations.empty())
	{
		name += txNameFrom;
		name += locations[rand2()%cities]->name;
	}
	else if(in_range(co, 2, 5))
	{
		name += txNamePrefix;
		name += random_item(*przyd);
	}
	else
	{
		name += txNamePrefix;
		name += random_item(przyd_losowy);
	}
}
