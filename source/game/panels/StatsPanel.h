#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "Scrollbar.h"
#include "FlowText.h"
#include "TooltipController.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
class StatsPanel : public GamePanel
{
public:
	StatsPanel();
	~StatsPanel();
	void Draw(ControlDrawData* cdd=NULL);
	void Event(GuiEvent e);
	void Update(float dt);
	void Show();
	void Hide();

	void SetText();

	PlayerController* pc;
	FlowText flow;
	StaticText* tAttribs, *tSkills, *tTraits, *tStats;
	Scrollbar scrollbar;
	cstring txStatsPanel, txTraitsText, txStatsText, txYearMonthDay, txBase, txRelatedAttributes;

private:
	void GetTooltip(TooltipController* tooltip, int group, int id);

	TooltipController tooltip;
	TextBox tbAttribs, tbStats, tbSkills, tbFeats;
};
