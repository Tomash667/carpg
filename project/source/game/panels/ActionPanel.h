#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
struct Action;

//-----------------------------------------------------------------------------
class ActionPanel : public GamePanel
{
public:
	ActionPanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;

private:
	vector<Action*> actions;
	cstring txActions;
};
