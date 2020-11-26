#include "Pch.h"
#include "GuildPanel.h"

#include "GameGui.h"
#include "Guild.h"
#include "Language.h"
#include "Unit.h"

#include <ResourceManager.h>

GuildPanel::GuildPanel() : mode(Mode::Info)
{
	visible = false;

	tooltip.Init(TooltipController::Callback(this, &GuildPanel::GetTooltip));
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

void GuildPanel::LoadData()
{
	tButtons[(int)Mode::Info] = res_mgr->Load<Texture>("bt_info.png");
	tButtons[(int)Mode::Members] = res_mgr->Load<Texture>("bt_guild.png");
	tShortcut = res_mgr->Load<Texture>("shortcut.png");
	tShortcutHover = res_mgr->Load<Texture>("shortcut_hover.png");
	tShortcutDown = res_mgr->Load<Texture>("shortcut_down.png");
}

void GuildPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	Matrix mat;

	// left side buttons
	const int img_size = 76 * gui->wnd_size.x / 1920;
	const int offset = img_size + 2;
	const float scale = float(img_size) / 64;
	const int max = (int)Mode::Max;
	const int total = offset * max;
	const int sposy = (gui->wnd_size.y - total) / 2;
	for(int i = 0; i < max; ++i)
	{
		Texture* t;
		if(buttonState[i] == 0)
			t = tShortcut;
		else if(buttonState[i] == 1)
			t = tShortcutHover;
		else
			t = tShortcutDown;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(2.f, float(sposy + i * offset)));
		gui->DrawSprite2(t, mat, nullptr, nullptr, Color::White);
		gui->DrawSprite2(tButtons[i], mat, nullptr, nullptr, Color::White);
	}

	switch(mode)
	{
	case Mode::Info:
		{
			// title
			Rect rect = {
				pos.x,
				pos.y + 10,
				pos.x + size.x,
				pos.y + size.y
			};
			gui->DrawText(GameGui::font_big, txTitle, DTF_TOP | DTF_CENTER, Color::Black, rect);

			// info
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
		break;

	case Mode::Members:
		{

		}
		break;
	}

	// tooltip
	tooltip.Draw();
}

void GuildPanel::Update(float dt)
{
	int group = -1, id = -1;

	GamePanel::Update(dt);

	// reset button hover
	const int max = (int)Mode::Max;
	for(int i = 0; i < (int)Mode::Max; ++i)
		buttonState[i] = (i == (int)mode ? 2 : 0);

	// update side buttons
	if(!gui->HaveDialog())
	{
		const int img_size = 76 * gui->wnd_size.x / 1920;
		const int offset = img_size + 2;
		const int total = offset * max;
		const int sposy = (gui->wnd_size.y - total) / 2;
		for(int i = 0; i < max; ++i)
		{
			if(PointInRect(gui->cursor_pos, Int2(2, sposy + i * offset), Int2(img_size, img_size)))
			{
				group = 0;
				id = i;

				if(buttonState[i] == 0)
					buttonState[i] = 1;
				if(input->PressedRelease(Key::LeftButton))
					mode = (Mode)i;

				break;
			}
		}
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Hide();

	tooltip.UpdateTooltip(dt, (int)group, id);
}

void GuildPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		if(mode != Mode::Info && !guild->created)
			mode = Mode::Info;
		tooltip.Clear();
	}
	GamePanel::Event(e);
}

void GuildPanel::GetTooltip(TooltipController*, int group, int id, bool refresh)
{
	if(group == -1)
		tooltip.anything = false;
	else
	{
		tooltip.anything = true;
		tooltip.text = "bobo";
	}
}
