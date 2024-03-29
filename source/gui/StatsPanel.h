#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include <TooltipController.h>
#include <FlowContainer.h>
#include "UnitStats.h"
#include "Perk.h"

//-----------------------------------------------------------------------------
// Display player stats (attributes, skills, feats, etc)
class StatsPanel : public GamePanel
{
public:
	StatsPanel();
	void LoadLanguage();
	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;
	static char StatStateToColor(StatState s);

	void Show();
	void Hide();

	PlayerController* pc;

private:
	void SetText();
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);

	TooltipController tooltip;
	FlowContainer flowAttribs, flowStats, flowSkills, flowFeats;
	float lastUpdate;
	cstring txAttributes, txTitle, txClass, txTraitsStart, txTraitsStartMp, txTraitsEnd, txStatsText, txBase, txRelatedAttributes, txFeats, txTraits, txStats,
		txDate, txAttack, txMeleeAttack, txRangedAttack, txHardcoreMode;
	vector<pair<cstring, int>> perks;
};
