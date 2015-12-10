#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "TooltipController.h"
#include "FlowContainer2.h"
#include "UnitStats.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
class StatsPanel : public GamePanel
{
public:
	StatsPanel();
	void Draw(ControlDrawData* cdd=nullptr);
	void Event(GuiEvent e);
	void Update(float dt);
	void Show();
	void Hide();

	PlayerController* pc;

private:
	void SetText();
	void GetTooltip(TooltipController* tooltip, int group, int id);
	
	TooltipController tooltip;
	FlowContainer2 flowAttribs, flowStats, flowSkills, flowFeats;
	float last_update;
	cstring txAttributes, txStatsPanel, txTraitsClass, txTraitsText, txStatsText, txYearMonthDay, txBase, txRelatedAttributes, txFeats, txTraits, txStats, txStatsDate;
	vector<std::pair<cstring, int>> perks;
};
