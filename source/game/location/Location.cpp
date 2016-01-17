#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Language.h"
#include "SaveState.h"

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
	WriteFile(file, &type, sizeof(type), &tmp, nullptr);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	WriteFile(file, name.c_str(), len, &tmp, nullptr);
	WriteFile(file, &state, sizeof(state), &tmp, nullptr);
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
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &last_visit, sizeof(last_visit), &tmp, nullptr);
	WriteFile(file, &st, sizeof(st), &tmp, nullptr);
	WriteFile(file, &outside, sizeof(reset), &tmp, nullptr);
	WriteFile(file, &reset, sizeof(reset), &tmp, nullptr);
	WriteFile(file, &spawn, sizeof(spawn), &tmp, nullptr);
	WriteFile(file, &dont_clean, sizeof(dont_clean), &tmp, nullptr);
	WriteFile(file, &seed, sizeof(seed), &tmp, nullptr);

	Portal* p = portal;
	const byte jeden = 1;

	while(p)
	{
		WriteFile(file, &jeden, sizeof(jeden), &tmp, nullptr);
		p->Save(file);
		p = p->next_portal;
	}

	const byte zero = 0;
	WriteFile(file, &zero, sizeof(zero), &tmp, nullptr);
}

//=================================================================================================
void Location::Load(HANDLE file, bool)
{
	ReadFile(file, &type, sizeof(type), &tmp, nullptr);
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	name.resize(len);
	ReadFile(file, (char*)name.c_str(), len, &tmp, nullptr);
	ReadFile(file, &state, sizeof(state), &tmp, nullptr);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	if(refid == -1)
		active_quest = nullptr;
	else if(refid == ACTIVE_QUEST_HOLDER)
		active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
	else
	{
		Game::Get().load_location_quest.push_back(this);
		active_quest = (Quest_Dungeon*)refid;
	}
	ReadFile(file, &last_visit, sizeof(last_visit), &tmp, nullptr);
	ReadFile(file, &st, sizeof(st), &tmp, nullptr);
	ReadFile(file, &outside, sizeof(reset), &tmp, nullptr);
	ReadFile(file, &reset, sizeof(reset), &tmp, nullptr);
	ReadFile(file, &spawn, sizeof(spawn), &tmp, nullptr);
	ReadFile(file, &dont_clean, sizeof(dont_clean), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &seed, sizeof(seed), &tmp, nullptr);
	else
		seed = 0;

	byte stan;
	ReadFile(file, &stan, sizeof(stan), &tmp, nullptr);
	if(stan == 1)
	{
		Portal* p = new Portal;
		portal = p;

		while(true)
		{
			p->Load(this, file);
			ReadFile(file, &stan, sizeof(stan), &tmp, nullptr);
			if(stan == 1)
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

//=================================================================================================
void Portal::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
	WriteFile(file, &target, sizeof(target), &tmp, nullptr);
	WriteFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);
}

//=================================================================================================
void Portal::Load(Location* loc, HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &at_level, sizeof(at_level), &tmp, nullptr);
	ReadFile(file, &target, sizeof(target), &tmp, nullptr);
	ReadFile(file, &target_loc, sizeof(target_loc), &tmp, nullptr);

	if(LOAD_VERSION < V_0_2_10)
	{
		// fix na portal na z³ym poziomie (kiedyœ nie uwzglêdnia³o at_level) do rysowania
		if(at_level == 0 && target_loc == -1)
			at_level = loc->GetLastLevel();
	}
}
