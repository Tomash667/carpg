#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "Scrollbar.h"
#include "FlowText.h"

//-----------------------------------------------------------------------------
struct PlayerController;

//-----------------------------------------------------------------------------
class StatsPanel : public GamePanel
{
public:
	StatsPanel(const INT2& pos, const INT2& size);
	~StatsPanel();
	void Draw(ControlDrawData* cdd=NULL);
	void Event(GuiEvent e);
	void Update(float dt);
	void FormatBox();

	void SetText();

	PlayerController* pc;
	FlowText flow;
	StaticText* tAttribs, *tSkills, *tTraits, *tStats;
	Scrollbar scrollbar;
	cstring txStatsPanel, txTraitsText, txStatsText, txYearMonthDay, txBase;
};
