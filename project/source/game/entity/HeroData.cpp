// dane bohatera - jednostki któr¹ mo¿na zrekrutowaæ
#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
void HeroData::Init(Unit& _unit)
{
	know_name = false;
	team_member = false;
	mode = Follow;
	unit = &_unit;
	following = nullptr;
	credit = 0;
	on_credit = false;
	lost_pvp = false;
	expe = 0;
	free = false;
	melee = false;
	phase = false;
	phase_timer = 0.f;
	clas = unit->data->clas;

	if(!IS_SET(unit->data->flags2, F2_SPECIFIC_NAME))
		Game::Get().GenerateHeroName(*this);
}

//=================================================================================================
void HeroData::Save(HANDLE file)
{
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	WriteFile(file, name.c_str(), len, &tmp, nullptr);
	WriteFile(file, &clas, sizeof(clas), &tmp, nullptr);
	WriteFile(file, &know_name, sizeof(know_name), &tmp, nullptr);
	WriteFile(file, &team_member, sizeof(team_member), &tmp, nullptr);
	WriteFile(file, &mode, sizeof(mode), &tmp, nullptr);
	int refid = (following ? following->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &credit, sizeof(credit), &tmp, nullptr);
	WriteFile(file, &expe, sizeof(expe), &tmp, nullptr);
	WriteFile(file, &melee, sizeof(melee), &tmp, nullptr);
	WriteFile(file, &phase, sizeof(phase), &tmp, nullptr);
	WriteFile(file, &phase_timer, sizeof(phase_timer), &tmp, nullptr);
	WriteFile(file, &free, sizeof(free), &tmp, nullptr);
	WriteFile(file, &lost_pvp, sizeof(lost_pvp), &tmp, nullptr);
}

//=================================================================================================
void HeroData::Load(HANDLE file)
{
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	name.resize(len);
	ReadFile(file, (void*)name.c_str(), len, &tmp, nullptr);
	ReadFile(file, &clas, sizeof(clas), &tmp, nullptr);
	if(LOAD_VERSION < V_0_4)
		clas = Class::OldToNew((old::ClassId)clas);
	ReadFile(file, &know_name, sizeof(know_name), &tmp, nullptr);
	ReadFile(file, &team_member, sizeof(team_member), &tmp, nullptr);
	ReadFile(file, &mode, sizeof(mode), &tmp, nullptr);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	following = Unit::GetByRefid(refid);
	ReadFile(file, &credit, sizeof(credit), &tmp, nullptr);
	ReadFile(file, &expe, sizeof(expe), &tmp, nullptr);
	ReadFile(file, &melee, sizeof(melee), &tmp, nullptr);
	ReadFile(file, &phase, sizeof(phase), &tmp, nullptr);
	ReadFile(file, &phase_timer, sizeof(phase_timer), &tmp, nullptr);
	ReadFile(file, &free, sizeof(free), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_6)
		ReadFile(file, &lost_pvp, sizeof(lost_pvp), &tmp, nullptr);
	else
		lost_pvp = false;
}

//=================================================================================================
int HeroData::JoinCost() const
{
	if(IS_SET(unit->data->flags, F_CRAZY))
		return (unit->level - 1) * 100 + Random(50, 150);
	else
		return unit->level * 100;
}

//=================================================================================================
void HeroData::LevelUp()
{
	if(unit->level >= MAX_LEVEL)
		return;

	++unit->level;
	unit->statsx = unit->statsx->GetLevelUp();
	unit->CalculateStats();
}

//=================================================================================================
void HeroData::SetupMelee()
{
	if(IS_SET(unit->data->flags2, F2_MELEE))
		melee = true;
	else if(IS_SET(unit->data->flags2, F2_MELEE_50) && Rand() % 2 == 0)
		melee = true;
}
