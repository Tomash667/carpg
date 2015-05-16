#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class MpBox;
class Journal;

//-----------------------------------------------------------------------------
class WorldMapGui : public Control
{
public:
	WorldMapGui();
	void Draw(ControlDrawData* cdd=NULL);
	void Update(float dt);
	bool NeedCursor() const { return true; }
	void Event(GuiEvent e);

	inline INT2 WorldPosToScreen(const INT2& pt) const
	{
		return INT2(pt.x, 600-pt.y);
	}
	inline VEC2 WorldPosToScreen(VEC2& pt) const
	{
		return VEC2(pt.x, 600.f-pt.y);
	}

	MpBox* mp_box;
	Journal* journal;
	cstring txGameTimeout, txWorldData, txCurrentLoc, txCitzens, txAvailable, txTarget, txDistance, txTravelTime, txDay, txDays, txOnlyLeaderCanTravel, txEncCrazyMage, txEncCrazyHeroes,
		txEncMerchant, txEncHeroes, txEncBanditsAttackTravelers, txEncHeroesAttack, txEncGolem, txEncCrazy, txEncUnk, txEncBandits, txEncAnimals, txEncOrcs, txEncGoblins;
};
