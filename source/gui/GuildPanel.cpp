#include "Pch.h"
#include "GuildPanel.h"

#include "GameGui.h"
#include "Guild.h"
#include "Language.h"
#include "Unit.h"

GuildPanel::GuildPanel()
{
	visible = false;
}

void GuildPanel::LoadLanguage()
{
	auto s = Language::GetSection("GuildPanel");
	txTitle = s.Get("title");
	txName = s.Get("name");
	txReputation = s.Get("reputation");
	txMaster = s.Get("master");
	txMembers = s.Get("members");
	txNoGuild = s.Get("noGuild");
	txEnterName = s.Get("enterName");
}

void GuildPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	// title
	Rect rect = {
		pos.x,
		pos.y + 10,
		pos.x + size.x,
		pos.y + size.y
	};
	gui->DrawText(GameGui::font_big, txTitle, DTF_TOP | DTF_CENTER, Color::Black, rect);

	rect = Rect(pos.x + 16, pos.y + 80, pos.x + size.x - 16, pos.y + size.y - 16);
	cstring text;
	if(guild->created)
	{
		Unit* leader = guild->master;
		text = Format("%s%s\n%s%d\n%s%s\n%s%u/%u",
			txName, guild->name.c_str(),
			txReputation, guild->reputation,
			txMaster, leader ? leader->GetName() : nullptr,
			txMembers, guild->members.size(), Guild::MAX_SIZE);
	}
	else
		text = txNoGuild;
	gui->DrawText(GameGui::font, text, DTF_LEFT | DTF_TOP, Color::Black, rect);
}
