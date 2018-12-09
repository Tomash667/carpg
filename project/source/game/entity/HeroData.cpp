#include "Pch.h"
#include "GameCore.h"
#include "HeroData.h"
#include "SaveState.h"
#include "Level.h"
#include "Unit.h"
#include "NameHelper.h"
#include "Const.h"

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
	split_gold = 0.f;

	if(!IS_SET(unit->data->flags2, F2_SPECIFIC_NAME))
		NameHelper::GenerateHeroName(*this);
}

//=================================================================================================
void HeroData::Save(FileWriter& f)
{
	f << name;
	f << know_name;
	f << team_member;
	f << mode;
	f << (following ? following->refid : -1);
	f << credit;
	f << expe;
	f << melee;
	f << phase;
	f << phase_timer;
	f << free;
	f << lost_pvp;
	f << split_gold;
}

//=================================================================================================
void HeroData::Load(FileReader& f)
{
	f >> name;
	if(LOAD_VERSION < V_0_7)
		f.Skip<Class>(); // old class info
	f >> know_name;
	f >> team_member;
	f >> mode;
	following = Unit::GetByRefid(f.Read<int>());
	f >> credit;
	f >> expe;
	f >> melee;
	f >> phase;
	f >> phase_timer;
	f >> free;
	if(LOAD_VERSION >= V_0_6)
		f >> lost_pvp;
	else
		lost_pvp = false;
	if(LOAD_VERSION >= V_0_7)
		f >> split_gold;
	else
		split_gold = 0.f;
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
void HeroData::PassTime(int days, bool travel)
{
	// czy bohater odpoczywa?
	bool resting;
	if(L.city_ctx && !travel)
	{
		resting = true;
		expe += days;
	}
	else
	{
		resting = false;
		expe += days * 2;
	}

	// zdobywanie doœwiadczenia
	if(unit->level != 20 && unit->IsHero() && unit->level != unit->data->level.y)
	{
		int req = (unit->level*(unit->level + 1) + 10) * 5;
		if(expe >= req)
		{
			expe -= req;
			LevelUp();
		}
	}

	// koñczenie efektów
	float natural_mod;
	unit->EndEffects(days, &natural_mod);

	// regeneracja hp
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(AttributeId::END);
		if(resting)
			heal *= 2;
		heal *= natural_mod * days;
		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;
	}
}

//=================================================================================================
void HeroData::LevelUp()
{
	if(unit->level >= MAX_LEVEL)
		return;

	++unit->level;
	unit->stats = unit->data->GetStats(unit->level);
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
