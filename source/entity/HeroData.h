#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"

//-----------------------------------------------------------------------------
enum class HeroType
{
	Normal,
	Free, // no gold shares
	Visitor // no gold/exp
};

//-----------------------------------------------------------------------------
struct HeroData : public HeroPlayerCommon
{
	int expe;
	float phase_timer;
	HeroType type;
	bool know_name, team_member, lost_pvp, melee, phase,
		gained_gold;

	void Init(Unit& unit);
	int JoinCost() const;
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void PassTime(int days = 1, bool travel = false);
	void SetupMelee();
	void AddExp(int exp);
	float GetExpMod() const;

private:
	void LevelUp();
};
