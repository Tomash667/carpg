// dane bohatera - jednostki któr¹ mo¿na zrekrutowaæ
#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
void HeroData::Init(Unit& _unit)
{
	know_name = false;
	team_member = false;
	mode = Follow;
	unit = &_unit;
	following = NULL;
	kredyt = 0;
	na_kredycie = false;
	lost_pvp = false;
	expe = 0;
	free = false;
	melee = false;
	phase = false;
	phase_timer = 0.f;

	if(!IS_SET(unit->data->flagi2, F2_BEZ_KLASY))
	{
		if(IS_SET(unit->data->flagi2, F2_FLAGA_KLASY))
		{
			if(IS_SET(unit->data->flagi, F_MAG))
				clas = Class::MAGE;
			else if(IS_SET(unit->data->flagi2, F2_WOJOWNIK))
				clas = Class::WARRIOR;
			else if(IS_SET(unit->data->flagi2, F2_LOWCA))
				clas = Class::HUNTER;
			else if(IS_SET(unit->data->flagi2, F2_KAPLAN))
				clas = Class::CLERIC;
			else
			{
				assert(IS_SET(unit->data->flagi2, F2_LOTRZYK));
				clas = Class::ROGUE;
			}
		}
		else
		{
			if(strcmp(unit->data->id, "hero_mage") == 0 || strcmp(unit->data->id, "crazy_mage") == 0)
				clas = Class::MAGE;
			else if(strcmp(unit->data->id, "hero_warrior") == 0 || strcmp(unit->data->id, "crazy_warrior") == 0)
				clas = Class::WARRIOR;
			else if(strcmp(unit->data->id, "hero_hunter") == 0 || strcmp(unit->data->id, "crazy_hunter") == 0)
				clas = Class::HUNTER;
			else
			{
				assert(strcmp(unit->data->id, "hero_rogue") == 0 || strcmp(unit->data->id, "crazy_rogue") == 0);
				clas = Class::ROGUE;
			}
		}
	}
	else
		clas = Class::ROGUE;

	if(!IS_SET(unit->data->flagi2, F2_OKRESLONE_IMIE))
		Game::Get().GenerateHeroName(*this);
}

//=================================================================================================
void HeroData::Save(HANDLE file)
{
	byte len = (byte)name.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, name.c_str(), len, &tmp, NULL);
	WriteFile(file, &clas, sizeof(clas), &tmp, NULL);
	WriteFile(file, &know_name, sizeof(know_name), &tmp, NULL);
	WriteFile(file, &team_member, sizeof(team_member), &tmp, NULL);
	WriteFile(file, &mode, sizeof(mode), &tmp, NULL);
	int refid = (following ? following->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteFile(file, &kredyt, sizeof(kredyt), &tmp, NULL);
	WriteFile(file, &expe, sizeof(expe), &tmp, NULL);
	WriteFile(file, &melee, sizeof(melee), &tmp, NULL);
	WriteFile(file, &phase, sizeof(phase), &tmp, NULL);
	WriteFile(file, &phase_timer, sizeof(phase_timer), &tmp, NULL);
	WriteFile(file, &free, sizeof(free), &tmp, NULL);
}

//=================================================================================================
void HeroData::Load(HANDLE file)
{
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	name.resize(len);
	ReadFile(file, (void*)name.c_str(), len, &tmp, NULL);
	ReadFile(file, &clas, sizeof(clas), &tmp, NULL);
	if(LOAD_VERSION < V_DEVEL)
		clas = ClassInfo::OldToNew(clas);
	ReadFile(file, &know_name, sizeof(know_name), &tmp, NULL);
	ReadFile(file, &team_member, sizeof(team_member), &tmp, NULL);
	ReadFile(file, &mode, sizeof(mode), &tmp, NULL);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	following = Unit::GetByRefid(refid);
	ReadFile(file, &kredyt, sizeof(kredyt), &tmp, NULL);
	ReadFile(file, &expe, sizeof(expe), &tmp, NULL);
	ReadFile(file, &melee, sizeof(melee), &tmp, NULL);
	ReadFile(file, &phase, sizeof(phase), &tmp, NULL);
	ReadFile(file, &phase_timer, sizeof(phase_timer), &tmp, NULL);
	ReadFile(file, &free, sizeof(free), &tmp, NULL);
	if(LOAD_VERSION == V_0_2_5)
	{
		// w wersji 1 by³a zapisywana waga, teraz jest ona w Unit
		float old_weight;
		ReadFile(file, &old_weight, sizeof(old_weight), &tmp, NULL);
	}
}

//=================================================================================================
int HeroData::JoinCost() const
{
	if(IS_SET(unit->data->flagi, F_SZALONY))
		return (unit->level-1)*100 + random(50,150);
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
		expe += days*2;
	}

	// zdobywanie doœwiadczenia
	if(unit->level != 20 && unit->IsHero() && unit->level != unit->data->level.y)
	{
		int req = (unit->level*(unit->level+1)+10)*5;
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
		float heal = 0.5f * unit->Get(Attribute::CON);
		if(resting)
			heal *= 2;
		if(best_nat)
		{
			if(best_nat != days)
				heal = heal*best_nat*2 + heal*(days-best_nat);
			else
				heal *= 2*days;
		}
		else
			heal *= days;

		heal = min(heal, unit->hpmax-unit->hp);
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
