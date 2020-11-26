#pragma once

#include "GamePanel.h"
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

	Mode mode;
	int buttonState[(int)Mode::Max];
	TooltipController tooltip;
	cstring txTitle, txName, txReputation, txMaster, txMembers, txNoGuild, txEnterName;
	TexturePtr tButtons[(int)Mode::Max], tShortcut, tShortcutHover, tShortcutDown;
};
