#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Location.h"

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
	Int2 WorldPosToScreen(const Int2& pt) const
	{
		return Int2(pt.x, 600 - pt.y);
	}
	Vec2 WorldPosToScreen(const Vec2& pt) const
	{
		return Vec2(pt.x, 600.f - pt.y);
	}
	void ShowEncounterMessage(cstring text);

	MpBox* mp_box;
	Journal* journal;
	GameMessages* game_messages;
	cstring txGameTimeout, txWorldDate, txCurrentLoc, txCitizens, txAvailable, txTarget, txDistance, txTravelTime, txDay, txDays, txOnlyLeaderCanTravel;
	int picked_location;

private:
	void AppendLocationText(Location& loc, string& s);
	void GetCityText(City& city, string& s);

	Game& game;
	TEX tMapBg, tWorldMap, tMapIcon[LI_MAX], tEnc, tSelected[2], tMover;
	cstring txBuildings;
};
