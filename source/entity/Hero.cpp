#include "Pch.h"
#include "Hero.h"

#include "AIManager.h"
#include "AITeam.h"
#include "Const.h"
#include "Level.h"
#include "NameHelper.h"
#include "Team.h"
#include "Unit.h"

// pre V_0_10 compatibility
namespace old
{
	enum Mode
	{
		Wander,
		Wait,
		Follow,
		Leave
	};
}

//=================================================================================================
void Hero::Init(Unit& _unit)
{
	knowName = false;
	teamMember = false;
	unit = &_unit;
	credit = 0;
	onCredit = false;
	lostPvp = false;
	expe = 0;
	type = HeroType::Normal;
	melee = false;
	phase = false;
	phaseTimer = 0.f;
	splitGold = 0.f;
	otherTeam = nullptr;
	loner = IsSet(_unit.data->flags, F_LONER) || Rand() % 5 == 0;
	investment = 0;

	if(!unit->data->real_name.empty())
		name = unit->data->real_name;
	else if(!IsSet(unit->data->flags2, F2_SPECIFIC_NAME))
		NameHelper::GenerateHeroName(*this);
}

//=================================================================================================
void Hero::Save(GameWriter& f)
{
	f << name;
	f << knowName;
	f << teamMember;
	f << credit;
	f << expe;
	f << melee;
	f << phase;
	f << phaseTimer;
	f << type;
	f << lostPvp;
	f << splitGold;
	f << (otherTeam ? otherTeam->id : -1);
	f << loner;
	f << investment;
}

//=================================================================================================
void Hero::Load(GameReader& f)
{
	f >> name;
	f >> knowName;
	f >> teamMember;
	if(LOAD_VERSION < V_0_10)
	{
		old::Mode mode;
		f >> mode;
		if(teamMember || mode == old::Leave)
		{
			UnitOrderEntry* order = UnitOrderEntry::Get();
			switch(mode)
			{
			case old::Wander:
				order->order = ORDER_WANDER;
				break;
			case old::Wait:
				order->order = ORDER_WAIT;
				break;
			case old::Follow:
				order->order = ORDER_FOLLOW;
				team->GetLeaderRequest(&order->unit);
				break;
			case old::Leave:
				order->order = ORDER_LEAVE;
				break;
			}
			order->timer = 0.f;
			unit->order = order;
		}
	}
	if(LOAD_VERSION < V_0_11)
		f.Skip<int>(); // old following
	f >> credit;
	f >> expe;
	f >> melee;
	f >> phase;
	f >> phaseTimer;
	if(LOAD_VERSION >= V_0_12)
		f >> type;
	else
	{
		bool free;
		f >> free;
		type = free ? HeroType::Visitor : HeroType::Normal;
	}
	f >> lostPvp;
	f >> splitGold;
	if(LOAD_VERSION >= V_0_17)
	{
		int teamId;
		f >> teamId;
		if(teamId == -1)
			otherTeam = nullptr;
		else
			otherTeam = aiMgr->GetTeam(teamId);
		f >> loner;
	}
	else
	{
		otherTeam = nullptr;
		loner = false;
	}
	if(LOAD_VERSION >= V_0_18)
		f >> investment;
	else
		investment = 0;
}

//=================================================================================================
int Hero::JoinCost() const
{
	if(IsSet(unit->data->flags, F_CRAZY))
		return (unit->level - 1) * 100 + Random(50, 150);
	else
		return unit->level * 100;
}

//=================================================================================================
void Hero::PassTime(int days, bool travel)
{
	// koñczenie efektów
	float natural_mod;
	unit->EndEffects(days, &natural_mod);

	// regeneracja hp
	if(unit->hp != unit->hpmax)
	{
		float heal = 0.5f * unit->Get(AttributeId::END);
		if(gameLevel->cityCtx && !travel)
			heal *= 2;
		heal *= natural_mod * days;
		heal = min(heal, unit->hpmax - unit->hp);
		unit->hp += heal;
	}
}

//=================================================================================================
void Hero::LevelUp()
{
	++unit->level;
	SubprofileInfo sub = unit->stats->subprofile;
	sub.level = unit->level;
	unit->stats = unit->data->GetStats(sub);
	unit->CalculateStats();
}

//=================================================================================================
void Hero::SetupMelee()
{
	if(IsSet(unit->data->flags2, F2_MELEE))
		melee = true;
	else if(IsSet(unit->data->flags2, F2_MELEE_50) && Rand() % 2 == 0)
		melee = true;
}

//=================================================================================================
void Hero::AddExp(int exp)
{
	if(unit->level == MAX_LEVEL)
		return;
	expe += int(GetExpMod() * exp);
	int exp_need = 10000 * unit->level;
	if(expe >= exp_need)
	{
		expe -= exp_need;
		LevelUp();
	}
}

//=================================================================================================
float Hero::GetExpMod() const
{
	int dif = unit->level - team->playersLevel;
	if(IsSet(unit->data->flags2, F2_FAST_LEARNER))
		--dif;
	switch(dif)
	{
	case 3:
		return 0.125f;
	case 2:
		return 0.25f;
	case 1:
		return 0.5f;
	case 0:
		return 1.f;
	case -1:
		return 2.f;
	case -2:
		return 3.f;
	case -3:
		return 4.f;
	default:
		if(dif > 0)
			return 0.f;
		else
			return 5.f;
	}
}

//=================================================================================================
int Hero::GetPersuasionCheckValue() const
{
	int level = 0;
	if(otherTeam)
		level = (otherTeam->leader == unit ? 50 : 0);
	if(loner)
	{
		if(level == 0)
			level = 25;
		else
			level += 10;
		uint size = team->GetActiveTeamSize();
		level += (size - 4) * 10;
	}
	return level;
}

//=================================================================================================
bool Hero::WantJoin() const
{
	if(!loner)
		return true;
	return team->GetActiveTeamSize() < 4u;
}
