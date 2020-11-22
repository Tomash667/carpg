#pragma once

#include "GamePanel.h"

class GuildPanel : public GamePanel
{
	friend class Guild;
public:
	GuildPanel();
	void LoadLanguage();
	void Draw(ControlDrawData*) override;

private:
	cstring txTitle, txName, txReputation, txMaster, txMembers, txNoGuild, txEnterName;
};
