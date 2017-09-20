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

	if(!IS_SET(unit->data->flags2, F2_NO_CLASS))
	{
		if(IS_SET(unit->data->flags2, F2_CLASS_FLAG))
		{
			if(IS_SET(unit->data->flags, F_MAGE))
				clas = Class::MAGE;
			else if(IS_SET(unit->data->flags2, F2_WARRIOR))
				clas = Class::WARRIOR;
			else if(IS_SET(unit->data->flags2, F2_HUNTER))
				clas = Class::HUNTER;
			else if(IS_SET(unit->data->flags2, F2_CLERIC))
				clas = Class::CLERIC;
			else
			{
				assert(IS_SET(unit->data->flags2, F2_ROGUE));
				clas = Class::ROGUE;
			}
		}
		else
		{
			const string& id = unit->data->id;

			if(id == "hero_mage" || id == "crazy_mage")
				clas = Class::MAGE;
			else if(id == "hero_warrior" || id == "crazy_warrior")
				clas = Class::WARRIOR;
			else if(id == "hero_hunter" || id == "crazy_hunter")
				clas = Class::HUNTER;
			else
			{
				assert(id == "hero_rogue" || id == "crazy_rogue");
				clas = Class::ROGUE;
			}
		}
	}
	else
		clas = Class::ROGUE;

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
		clas = ClassInfo::OldToNew(clas);
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
	if(LOAD_VERSION == V_0_2_5)
	{
		// w wersji 1 by³a zapisywana waga, teraz jest ona w Unit
		float old_weight;
		ReadFile(file, &old_weight, sizeof(old_weight), &tmp, nullptr);
	}
	if(LOAD_VERSION >= V_CURRENT)
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
void HeroData::PassTime(int days, bool travel)
{
	// czy bohater odpoczywa?
	bool resting;
	if(Game::Get().city_ctx && !travel)
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
	int best_nat;
	unit->EndEffects(days, &best_nat);

	// regeneracja hp
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(Attribute::END);
		if(resting)
			heal *= 2;
		if(best_nat)
		{
			if(best_nat != days)
				heal = heal*best_nat * 2 + heal*(days - best_nat);
			else
				heal *= 2 * days;
		}
		else
			heal *= days;

		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;
	}
}

//=================================================================================================
void HeroData::LevelUp()
{
	if(unit->level == 25)
		return;

	++unit->level;
	unit->data->GetStatProfile().Set(unit->level, unit->stats.attrib, unit->stats.skill);
	unit->CalculateStats();
	unit->RecalculateHp();
}
