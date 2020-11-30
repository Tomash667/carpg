#include "Pch.h"
#include "GuildPanel.h"

#include "GameGui.h"
#include "Guild.h"
#include "Language.h"
#include "Unit.h"

#include <ResourceManager.h>

enum class TooltipGroup
{
	None = -1,
	SideButton,
	MemberClass,
	MemberStatus
};

GuildPanel::GuildPanel() : mode(Mode::Info)
{
	visible = false;

	tooltip.Init(TooltipController::Callback(this, &GuildPanel::GetTooltip));

	grid.pos = Int2(10, 50);
	grid.size = Int2(500, 300);
	grid.event = GridEvent(this, &GuildPanel::GetCell);
	grid.selection_type = Grid::BACKGROUND;
	grid.selection_color = Color(0, 255, 0, 128);
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
	txInfo = s.Get("info");
	txInTeam = s.Get("inTeam");

	grid.AddColumn(Grid::TEXT, 200, s.Get("memberName"));
	grid.AddColumn(Grid::IMG_TEXT, 75, s.Get("class")).size = Int2(16, 16);
	grid.AddColumn(Grid::IMG, 75, s.Get("state")).size = Int2(16, 16);
	grid.Init();
	grid.AddItem();
}

void GuildPanel::LoadData()
{
	tButtons[(int)Mode::Info] = res_mgr->Load<Texture>("bt_info.png");
	tButtons[(int)Mode::Members] = res_mgr->Load<Texture>("bt_guild.png");
	tShortcut = res_mgr->Load<Texture>("shortcut.png");
	tShortcutHover = res_mgr->Load<Texture>("shortcut_hover.png");
	tShortcutDown = res_mgr->Load<Texture>("shortcut_down.png");
	tInTeam = res_mgr->Load<Texture>("bt_team.png");
}

void GuildPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();

	Matrix mat;

	// left side buttons
	if(guild->created)
	{
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
				text = Format("%s: %s\n%s: %d\n%s: %s\n%s: %u/%u",
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
			// title
			Rect rect = {
				pos.x,
				pos.y + 10,
				pos.x + size.x,
				pos.y + size.y
			};
			gui->DrawText(GameGui::font_big, Format("%s: %u/%u", txMembers, guild->members.size(), Guild::MAX_SIZE), DTF_TOP | DTF_CENTER, Color::Black, rect);

			// members grid
			grid.Draw();
		}
		break;
	}

	// tooltip
	tooltip.Draw();
}

void GuildPanel::Update(float dt)
{
	int group = (int)TooltipGroup::None, id = -1;

	GamePanel::Update(dt);

	if(guild->created)
	{
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
					group = (int)TooltipGroup::SideButton;
					id = i;

					if(buttonState[i] == 0)
						buttonState[i] = 1;
					if(input->PressedRelease(Key::LeftButton))
						mode = (Mode)i;

					break;
				}
			}
		}

		if(mode == Mode::Members)
		{
			grid.focus = focus;
			grid.Update(dt);

			Int2 cell = grid.GetCell(gui->cursor_pos);
			if(cell.x != -1 && Any(cell.y, 1, 2) && grid.GetImgIndex(gui->cursor_pos, cell) == 0)
			{
				if(cell.y == 1)
				{
					group = (int)TooltipGroup::MemberClass;
					id = cell.x;
				}
				else
				{
					group = (int)TooltipGroup::MemberStatus;
					id = 0;
				}
			}
		}
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Hide();

	tooltip.UpdateTooltip(dt, (int)group, id);
}

void GuildPanel::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Show:
	case GuiEvent_Resize:
		if(e == GuiEvent_Show)
		{
			if(mode != Mode::Info && !guild->created)
				mode = Mode::Info;
			tooltip.Clear();
		}
		grid.Move(global_pos);
		break;
	}
	GamePanel::Event(e);
}

void GuildPanel::GetTooltip(TooltipController*, int group, int id, bool refresh)
{
	if(group == -1)
	{
		tooltip.anything = false;
		return;
	}

	tooltip.anything = true;
	switch((TooltipGroup)group)
	{
	case TooltipGroup::SideButton:
		switch((Mode)id)
		{
		case Mode::Info:
			tooltip.text = txInfo;
			break;
		case Mode::Members:
			tooltip.text = txMembers;
			break;
		}
		break;
	case TooltipGroup::MemberClass:
		{
			Unit* unit = guild->members[id];
			Class* clas = unit->GetClass();
			tooltip.text = clas->name;
		}
		break;
	case TooltipGroup::MemberStatus:
		tooltip.text = txInTeam;
		break;
	}
}

void GuildPanel::GetCell(int item, int column, Cell& cell)
{
	Unit* unit = guild->members[item];
	switch(column)
	{
	case 0: // name
		cell.text = unit->GetName();
		break;
	case 1: // class
		cell.img = unit->GetClass()->icon;
		cell.text = Format("%d", unit->level);
		break;
	case 2: // status
		cell.img = tInTeam;
		break;
	}
}
