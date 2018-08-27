#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Language.h"
#include "SaveState.h"
#include "City.h"
#include "Quest.h"
#include "BitStreamFunc.h"
#include "Portal.h"
#include "GameFile.h"

//-----------------------------------------------------------------------------
cstring txCamp, txCave, txCity, txCrypt, txDungeon, txForest, txVillage, txMoonwell, txOtherness, txAcademy;
vector<string> txLocationStart, txLocationEnd;

//=================================================================================================
void SetLocationNames()
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
	txAcademy = Str("academy");
}

//=================================================================================================
Location::~Location()
{
	Portal* p = portal;
	while(p)
	{
		Portal* next_portal = p->next_portal;
		delete p;
		p = next_portal;
	}
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
		if(((City*)this)->IsVillage())
			name = txVillage;
		else
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
	case L_MOONWELL:
		name = txMoonwell;
		return;
	case L_ACADEMY:
		name = txAcademy;
		return;
	default:
		assert(0);
		name = txOtherness;
		break;
	}

	name += " ";
	cstring s1 = random_item(txLocationStart).c_str();
	cstring s2;
	do
	{
		s2 = random_item(txLocationEnd).c_str();
	} while(_stricmp(s1, s2) == 0);
	name += s1;
	if(name[name.length() - 1] == s2[0])
		name += (s2 + 1);
	else
		name += s2;
}

//=================================================================================================
bool Location::CheckUpdate(int& days_passed, int worldtime)
{
	bool need_reset = reset;
	reset = false;
	if(last_visit == -1)
		days_passed = -1;
	else
		days_passed = worldtime - last_visit;
	last_visit = worldtime;
	if(dont_clean)
		days_passed = 0;
	return need_reset;
}

//=================================================================================================
void Location::Save(GameWriter& f, bool)
{
	f << type;
	f << pos;
	f << name;
	f << state;
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
	f << refid;
	f << last_visit;
	f << st;
	f << outside;
	f << reset;
	f << spawn;
	f << dont_clean;
	f << seed;
	f << image;

	// portals
	Portal* p = portal;
	while(p)
	{
		f.Write1();
		p->Save(f);
		p = p->next_portal;
	}
	f.Write0();
}

//=================================================================================================
void Location::Load(GameReader& f, bool, LOCATION_TOKEN token)
{
	f >> type;
	if(LOAD_VERSION < V_0_5 && type == L_VILLAGE_OLD)
		type = L_CITY;
	f >> pos;
	f >> name;
	f >> state;
	int refid = f.Read<int>();
	if(refid == -1)
		active_quest = nullptr;
	else if(refid == ACTIVE_QUEST_HOLDER)
		active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
	else
	{
		Game::Get().load_location_quest.push_back(this);
		active_quest = (Quest_Dungeon*)refid;
	}
	f >> last_visit;
	f >> st;
	f >> outside;
	f >> reset;
	f >> spawn;
	f >> dont_clean;
	if(LOAD_VERSION >= V_0_3)
		f >> seed;
	else
		seed = 0;
	if(LOAD_VERSION >= V_0_5)
		f >> image;
	else
	{
		switch(type)
		{
		case L_CITY:
			image = LI_CITY;
			break;
		case L_CAVE:
			image = LI_CAVE;
			break;
		case L_CAMP:
			image = LI_CAMP;
			break;
		default:
		case L_DUNGEON:
			image = LI_DUNGEON;
			break;
		case L_CRYPT:
			image = LI_CRYPT;
			break;
		case L_FOREST:
			image = LI_FOREST;
			break;
		case L_MOONWELL:
			image = LI_MOONWELL;
			break;
		case L_ACADEMY:
			image = LI_ACADEMY;
			break;
		}
	}

	// portals
	if(f.Read1())
	{
		Portal* p = new Portal;
		portal = p;
		while(true)
		{
			p->Load(f);
			if(f.Read1())
			{
				Portal* np = new Portal;
				p->next_portal = np;
				p = np;
			}
			else
			{
				p->next_portal = nullptr;
				break;
			}
		}
	}
	else
		portal = nullptr;
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
	return nullptr;
}

//=================================================================================================
Portal* Location::TryGetPortal(int index) const
{
	assert(index >= 0);

	Portal* cportal = portal;
	int cindex = 0;

	while(cportal)
	{
		if(index == cindex)
			return cportal;

		++cindex;
		cportal = cportal->next_portal;
	}

	return nullptr;
}

//=================================================================================================
void Location::WritePortals(BitStream& stream) const
{
	// count
	uint count = 0;
	Portal* cportal = portal;
	while(cportal)
	{
		++count;
		cportal = cportal->next_portal;
	}
	stream.WriteCasted<byte>(count);

	// portals
	cportal = portal;
	while(cportal)
	{
		stream.Write(cportal->pos);
		stream.Write(cportal->rot);
		WriteBool(stream, cportal->target_loc != -1);
		cportal = cportal->next_portal;
	}
}

//=================================================================================================
bool Location::ReadPortals(BitStream& stream, int at_level)
{
	byte count;
	if(!stream.Read(count)
		|| !EnsureSize(stream, count * Portal::MIN_SIZE))
		return false;

	Portal* cportal = nullptr;
	for(byte i = 0; i < count; ++i)
	{
		Portal* p = new Portal;
		bool active;
		if(!stream.Read(p->pos)
			|| !stream.Read(p->rot)
			|| !ReadBool(stream, active))
		{
			delete p;
			return false;
		}

		p->target_loc = (active ? 0 : -1);
		p->at_level = at_level;
		p->next_portal = nullptr;

		if(cportal)
		{
			cportal->next_portal = p;
			cportal = p;
		}
		else
			cportal = portal = p;
	}

	return true;
}
