#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Language.h"
#include "SaveState.h"

//-----------------------------------------------------------------------------
cstring txCamp, txCave, txCity, txCrypt, txDungeon, txForest, txVillage, txMoonwell, txOtherness;

//-----------------------------------------------------------------------------
cstring name_start[] = {
	"Amber",
	"Aqua",
	"Black",
	"Blue",
	"Chaos",
	"Crash",
	"Dark",
	"Dawn",
	"Deep",
	"Dust",
	"Ever",
	"East",
	"Fire",
	"Gate",
	"Ghost",
	"Golden",
	"Gray",
	"Green",
	"Helm",
	"Hill",
	"Hole",
	"Hold",
	"Holy",
	"Hog",
	"Ice",
	"Ivory",
	"Jade",
	"Lords",
	"Lost",
	"Moon",
	"More",
	"Never",
	"Night",
	"Over",
	"Owl",
	"Pass",
	"Port",
	"Red",
	"Rock",
	"Run",
	"Storm",
	"Silent",
	"Silver",
	"Star",
	"Timber",
	"Town",
	"Tower",
	"Under",
	"Ward",
	"Water",
	"White",
	"Wind",
	"Winter",
	"Wood",
	"Yellow",
	"Zany"
};

//-----------------------------------------------------------------------------
cstring name_end[] = {
	"arc",
	"beam",
	"chasm",
	"deep",
	"dust",
	"gate",
	"helm",
	"hill",
	"hole",
	"hold",
	"ice",
	"keep",
	"mine",
	"moon",
	"more",
	"nest",
	"pass",
	"rock",
	"run",
	"star",
	"town",
	"tower",
	"ward",
	"winter",
	"wood",
	"zone"
};

//=================================================================================================
void LoadLocationNames()
{
	txCamp = Str("camp");
	txCave = Str("cave");
	txCity = Str("city");
	txCrypt = Str("crypt");
	txDungeon = Str("dungeon");
	txForest = Str("forest");
	txVillage = Str("village");
	txMoonwell = Str("moonwell");
	txOtherness = Str("otherness");
}

//=================================================================================================
void Location::GenerateName()
{
	name.clear();

	switch(type)
	{	
	case L_CAMP:
		name = txCamp;
		break;
	case L_CAVE:
		name = txCave;
		break;
	case L_CITY:
		name = txCity;
		break;
	case L_CRYPT:
		name = txCrypt;
		break;
	case L_DUNGEON:
		name = txDungeon;
		break;
	case L_FOREST:
		name = txForest;
		break;
	case L_VILLAGE:
		name = txVillage;
		break;
	case L_MOONWELL:
		name = txMoonwell;
		return;
	default:
		assert(0);
		name = txOtherness;
		break;
	}

	name += " ";
	cstring s1 = random_string(name_start);
	cstring s2;
	do 
	{
		s2 = random_string(name_end);
	}
	while(_stricmp(s1, s2) == 0);
	name += s1;
	if(name[name.length()-1] == s2[0])
		name += (s2+1);
	else
		name += s2;
}

//=================================================================================================
void Location::Save(HANDLE file, bool)
{
	WriteFile(file, &type, sizeof(type), &tmp, NULL);
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, name.c_str(), len, &tmp, NULL);
	WriteFile(file, &state, sizeof(state), &tmp, NULL);
	int refid;
	if(active_quest)
	{
		if(active_quest == (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
			refid = ACTIVE_QUEST_HOLDER;
		else
			refid = active_quest->refid;
	}
	else
		refid = -1;
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteFile(file, &last_visit, sizeof(last_visit), &tmp, NULL);
	WriteFile(file, &st, sizeof(st), &tmp, NULL);
	WriteFile(file, &outside, sizeof(reset), &tmp, NULL);
	WriteFile(file, &reset, sizeof(reset), &tmp, NULL);
	WriteFile(file, &spawn, sizeof(spawn), &tmp, NULL);
	WriteFile(file, &dont_clean, sizeof(dont_clean), &tmp, NULL);
	WriteFile(file, &seed, sizeof(seed), &tmp, NULL);

	Portal* p = portal;
	const byte jeden = 1;

	while(p)
	{
		WriteFile(file, &jeden, sizeof(jeden), &tmp, NULL);
		p->Save(file);
		p = p->next_portal;
	}

	const byte zero = 0;
	WriteFile(file, &zero, sizeof(zero), &tmp, NULL);
}

//=================================================================================================
void Location::Load(HANDLE file, bool)
{
	ReadFile(file, &type, sizeof(type), &tmp, NULL);
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	name.resize(len);
	ReadFile(file, (char*)name.c_str(), len, &tmp, NULL);
	ReadFile(file, &state, sizeof(state), &tmp, NULL);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	if(refid == -1)
		active_quest = NULL;
	else if(refid == ACTIVE_QUEST_HOLDER)
		active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
	else
	{
		Game::Get().load_location_quest.push_back(this);
		active_quest = (Quest_Dungeon*)refid;
	}
	ReadFile(file, &last_visit, sizeof(last_visit), &tmp, NULL);
	ReadFile(file, &st, sizeof(st), &tmp, NULL);
	ReadFile(file, &outside, sizeof(reset), &tmp, NULL);
	ReadFile(file, &reset, sizeof(reset), &tmp, NULL);
	ReadFile(file, &spawn, sizeof(spawn), &tmp, NULL);
	ReadFile(file, &dont_clean, sizeof(dont_clean), &tmp, NULL);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &seed, sizeof(seed), &tmp, NULL);
	else
		seed = 0;

	byte stan;
	ReadFile(file, &stan, sizeof(stan), &tmp, NULL);
	if(stan == 1)
	{
		Portal* p = new Portal;
		portal = p;

		while(true)
		{
			p->Load(this, file);
			ReadFile(file, &stan, sizeof(stan), &tmp, NULL);
			if(stan == 1)
			{
				Portal* np = new Portal;
				p->next_portal = np;
				p = np;
			}
			else
			{
				p->next_portal = NULL;
				break;
			}
		}
	}
	else
		portal = NULL;
}

//=================================================================================================
Portal* Location::GetPortal(int index)
{
	assert(portal && index >= 0);

	Portal* cportal = portal;
	int cindex = 0;

	while(cportal)
	{
		if(index == cindex)
			return cportal;

		++cindex;
		cportal = cportal->next_portal;
	}

	assert(0);
	return NULL;
}

//=================================================================================================
void Portal::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &at_level, sizeof(at_level), &tmp, NULL);
	WriteFile(file, &target, sizeof(target), &tmp, NULL);
	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);
}

//=================================================================================================
void Portal::Load(Location* loc, HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &at_level, sizeof(at_level), &tmp, NULL);
	ReadFile(file, &target, sizeof(target), &tmp, NULL);
	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, NULL);

	if(LOAD_VERSION < V_0_2_10)
	{
		// fix na portal na z³ym poziomie (kiedyœ nie uwzglêdnia³o at_level) do rysowania
		if(at_level == 0 && target_loc == -1)
			at_level = loc->GetLastLevel();
	}
}
