#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Location.h"

//-----------------------------------------------------------------------------
struct Game;
struct City;
class MpBox;
class Journal;
class GameMessages;

//-----------------------------------------------------------------------------
class WorldMapGui : public Control
{
public:
	WorldMapGui();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	bool NeedCursor() const override { return true; }
	void Event(GuiEvent e) override;

	void LoadData();
	INT2 WorldPosToScreen(const INT2& pt) const
	{
		return INT2(pt.x, 600 - pt.y);
	}
	VEC2 WorldPosToScreen(VEC2& pt) const
	{
		return VEC2(pt.x, 600.f - pt.y);
	}

	MpBox* mp_box;
	Journal* journal;
	GameMessages* game_messages;
	cstring txGameTimeout, txWorldData, txCurrentLoc, txCitizens, txAvailable, txTarget, txDistance, txTravelTime, txDay, txDays, txOnlyLeaderCanTravel,
		txEncCrazyMage, txEncCrazyHeroes, txEncMerchant, txEncHeroes, txEncBanditsAttackTravelers, txEncHeroesAttack, txEncGolem, txEncCrazy, txEncUnk,
		txEncBandits, txEncAnimals, txEncOrcs, txEncGoblins;

private:
	void GetCityText(City& city, string& s);

	Game& game;
	TEX tMapBg, tWorldMap, tMapIcon[LI_MAX], tEnc, tSelected[2], tMover;
	cstring txBuildings;
};
