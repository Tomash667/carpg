#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"

//-----------------------------------------------------------------------------
struct HeroData : public HeroPlayerCommon
{
	int expe;
	float phase_timer;
	bool know_name, team_member, lost_pvp, melee, phase,
		free, // don't get shares
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
