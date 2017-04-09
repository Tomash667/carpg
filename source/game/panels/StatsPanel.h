#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "TooltipController.h"
#include "FlowContainer.h"
#include "UnitStats.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
class StatsPanel : public GamePanel
{
public:
	StatsPanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	void Show();
	void Hide();

	PlayerController* pc;

private:
	void SetText();
	void GetTooltip(TooltipController* tooltip, int group, int id);

	TooltipController tooltip;
	FlowContainer flowAttribs, flowStats, flowSkills, flowFeats;
	float last_update;
	cstring txAttributes, txStatsPanel, txTraitsClass, txTraitsText, txStatsText, txYearMonthDay, txBase, txRelatedAttributes, txFeats, txTraits, txStats, txStatsDate;
	vector<std::pair<cstring, int>> perks;
};
