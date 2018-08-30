#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
class ActionPanel : public GamePanel
{
public:
	ActionPanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

	void LoadData();
	void Init(Action* action);

private:
	void GetTooltip(TooltipController* tooltip, int group, int id);

	TooltipController tooltip;
	vector<Action*> actions;
	cstring txActions, txCooldown, txCooldownCharges;
	TEX tItemBar;
};
