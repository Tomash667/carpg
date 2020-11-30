#pragma once

#include "GamePanel.h"
#include <Grid.h>
#include <TooltipController.h>

class GuildPanel : public GamePanel
{
	friend class Guild;

	enum class Mode
	{
		Info,
		Members,
		Max
	};

public:
	GuildPanel();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	void GetTooltip(TooltipController*, int group, int id, bool refresh);
	void GetCell(int item, int column, Cell& cell);

	Mode mode;
	int buttonState[(int)Mode::Max];
	TooltipController tooltip;
	Grid grid;
	cstring txTitle, txName, txReputation, txMaster, txMembers, txNoGuild, txEnterName, txInfo, txInTeam;
	TexturePtr tButtons[(int)Mode::Max], tShortcut, tShortcutHover, tShortcutDown, tInTeam;
};
