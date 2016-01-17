#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Location.h"

//-----------------------------------------------------------------------------
struct Game;
class MpBox;
class Journal;
class GameMessages;

//-----------------------------------------------------------------------------
class WorldMapGui : public Control
{
public:
	WorldMapGui();
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	bool NeedCursor() const { return true; }
	void Event(GuiEvent e);
	void LoadData();

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
	GameMessages* game_messages;
	cstring txGameTimeout, txWorldData, txCurrentLoc, txCitizens, txAvailable, txTarget, txDistance, txTravelTime, txDay, txDays, txOnlyLeaderCanTravel, txEncCrazyMage, txEncCrazyHeroes,
		txEncMerchant, txEncHeroes, txEncBanditsAttackTravelers, txEncHeroesAttack, txEncGolem, txEncCrazy, txEncUnk, txEncBandits, txEncAnimals, txEncOrcs, txEncGoblins;

private:
	Game& game;
	TEX tMapBg, tWorldMap, tMapIcon[L_MAX], tEnc, tSelected[2], tMover;
};
