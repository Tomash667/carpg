#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "TooltipController.h"
#include "FlowContainer2.h"
#include "UnitStats.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
class StatsPanel : public GamePanel
{
public:
	StatsPanel();
	void Draw(ControlDrawData* cdd=NULL);
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
	StatInfo attributes[(int)Attribute::MAX], skills[(int)Skill::MAX];
	cstring txAttributes, txStatsPanel, txTraitsText, txStatsText, txYearMonthDay, txBase, txRelatedAttributes, txFeats, txTraits, txStats, txStatsDate;
};
