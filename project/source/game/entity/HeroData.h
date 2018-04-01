// dane bohatera - jednostki któr¹ mo¿na zrekrutowaæ
#pragma once

//-----------------------------------------------------------------------------
#include "HeroPlayerCommon.h"

//-----------------------------------------------------------------------------
struct HeroData : public HeroPlayerCommon
{
	enum Mode
	{
		Wander,
		Wait,
		Follow,
		Leave
	};

	Mode mode;
	Unit* following; // pod¹¿a za t¹ postaci¹ w czasie warpowania, nieu¿ywane?
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
	void LevelUp();
	void SetupMelee();
};
