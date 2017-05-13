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

	bool know_name, team_member, lost_pvp, melee, phase;
	bool free; // don't get shares
	Mode mode;
	Unit* following; // pod¹¿a za t¹ postaci¹ w czasie warpowania, nieu¿ywane?
	int expe;
	float phase_timer;
	// phase = antyblokowanie sojuszników
	bool gained_gold;

	void Init(Unit& unit);
	int JoinCost() const;
	void Save(HANDLE file);
	void Load(HANDLE file);
	void PassTime(int days=1, bool travel=false);
	void LevelUp();
};
