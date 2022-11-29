#include "Pch.h"
#include "GamePanel.h"

#include "GameGui.h"
#include "Language.h"

#include <DialogBox.h>
#include <Input.h>

//-----------------------------------------------------------------------------
TexturePtr GamePanel::tBackground, GamePanel::tDialog;

//-----------------------------------------------------------------------------
enum MenuId
{
	Menu_Edit,
	Menu_Save,
	Menu_Load,
	Menu_LoadDefault
};

//=================================================================================================
GamePanel::GamePanel() : box_state(BOX_NOT_VISIBLE), order(0), last_index(INDEX_INVALID), last_index2(INDEX_INVALID)
{
	focusable = true;
}

//=================================================================================================
void GamePanel::Draw()
{
	gui->DrawItem(tBackground, pos, size, Color::Alpha(222), 16);
}

//=================================================================================================
void GamePanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		box_state = BOX_NOT_VISIBLE;
		last_index = INDEX_INVALID;
	}
}

//=================================================================================================
void GamePanel::DrawBox()
{
	if(box_state == BOX_VISIBLE)
		static_cast<GamePanelContainer*>(parent)->draw_box = this;
}

//=================================================================================================
void GamePanel::DrawBoxInternal()
{
	int alpha = int(box_alpha * 222),
		alpha2 = int(box_alpha * 255);

	// box
	gui->DrawItem(tDialog, box_pos, box_size, Color::Alpha(alpha), 12);

	// obrazek
	if(box_img)
		gui->DrawSprite(box_img, box_img_pos, Color::Alpha(alpha2));

	// du�y tekst
	gui->DrawText(GameGui::font, box_text, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), box_big);

	// ma�y tekst
	if(!box_text_small.empty())
		gui->DrawText(GameGui::font_small, box_text_small, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), box_small);
}

//=================================================================================================
void GamePanel::UpdateBoxIndex(float dt, int index, int index2, bool refresh)
{
	if(index != INDEX_INVALID)
	{
		if(index != last_index || index2 != last_index2)
		{
			box_state = BOX_COUNTING;
			last_index = index;
			last_index2 = index2;
			show_timer = 0.3f;
		}
		else
			show_timer -= dt;

		if(box_state == BOX_COUNTING)
		{
			if(show_timer <= 0.f)
			{
				box_state = BOX_VISIBLE;
				box_alpha = 0.f;
				refresh = true;
			}
		}
		else
		{
			box_alpha += dt * 5;
			if(box_alpha >= 1.f)
				box_alpha = 1.f;
		}

		if(refresh)
		{
			FormatBox(refresh);
			if(box_img)
				box_img_size = box_img->GetSize();
		}
	}
	else
	{
		box_state = BOX_NOT_VISIBLE;
		last_index = INDEX_INVALID;
		last_index2 = INDEX_INVALID;
	}

	if(box_state == BOX_VISIBLE)
	{
		Int2 text_size = GameGui::font->CalculateSize(box_text);
		box_big = Rect::Create(Int2(0, 0), text_size);
		Int2 size = text_size + Int2(24, 24);
		Int2 pos2 = Int2(gui->cursorPos) + Int2(24, 24);
		Int2 text_pos(12, 12);

		// uwzgl�dnij rozmiar obrazka
		if(box_img)
		{
			text_pos.x += box_img_size.x + 8;
			size.x += box_img_size.x + 8;
			if(size.y < box_img_size.y + 24)
				size.y = box_img_size.y + 24;
		}

		// minimalna szeroko��
		if(size.x < 256)
			size.x = 256;

		Int2 text_pos2(12, text_pos.y);
		text_pos2.y += size.y - 12;

		if(!box_text_small.empty())
		{
			Int2 size_small = GameGui::font_small->CalculateSize(box_text_small, size.x - 24);
			box_small = Rect::Create(Int2(0, 0), size_small);
			int size_y = size_small.y;
			size.y += size_y + 12;
		}

		if(pos2.x + size.x >= gui->wndSize.x)
			pos2.x = gui->wndSize.x - size.x - 1;
		if(pos2.y + size.y >= gui->wndSize.y)
			pos2.y = gui->wndSize.y - size.y - 1;

		box_img_pos = Int2(pos2.x + 12, pos2.y + 12);
		box_big = Rect::Create(text_pos + pos2, text_size);
		box_small += pos2 + text_pos2;
		box_small.Right() = box_small.Left() + box_size.x - 24;

		box_size = size;
		box_pos = pos2;
	}
}

//=================================================================================================
// GAME PANEL CONTAINER
//=================================================================================================
GamePanelContainer::GamePanelContainer() : order(0), lost_focus(false)
{
}

//=================================================================================================
void GamePanelContainer::Draw()
{
	draw_box = nullptr;

	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
			(*it)->Draw();
	}

	if(draw_box)
		draw_box->DrawBoxInternal();
}

//=================================================================================================
void GamePanelContainer::Update(float dt)
{
	if(focus)
	{
		if(lost_focus)
		{
			lost_focus = false;
			ctrls.back()->Event(GuiEvent_GainFocus);
			ctrls.back()->focus = true;
		}

		Int2 cp = gui->cursorPos;
		GamePanel* top = nullptr;

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			GamePanel* gp = (GamePanel*)*it;
			gp->mouseFocus = false;
			if(gp->IsInside(cp))
				top = gp;
		}

		if(top)
		{
			top->mouseFocus = true;
			if((input->Pressed(Key::LeftButton) || input->Pressed(Key::RightButton)) && top->order != order - 1)
			{
				ctrls.back()->Event(GuiEvent_LostFocus);
				ctrls.back()->focus = false;
				top->order = order;
				++order;
				std::sort(ctrls.begin(), ctrls.end(), [](const Control* c1, const Control* c2)
				{
					return static_cast<const GamePanel*>(c1)->order < static_cast<const GamePanel*>(c2)->order;
				});
				top->Event(GuiEvent_GainFocus);
				top->focus = true;
			}
		}

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
			(*it)->Update(dt);
	}
	else
	{
		if(!lost_focus)
		{
			lost_focus = true;
			ctrls.back()->Event(GuiEvent_LostFocus);
			ctrls.back()->focus = false;
		}

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			(*it)->mouseFocus = false;
			(*it)->Update(dt);
		}
	}
}

//=================================================================================================
void GamePanelContainer::Add(GamePanel* panel)
{
	Container::Add(panel);
	panel->order = order;
	++order;
}

//=================================================================================================
void GamePanelContainer::Show()
{
	visible = true;
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		(*it)->Event(GuiEvent_Show);
	ctrls.back()->Event(GuiEvent_GainFocus);
	ctrls.back()->focus = true;
}

//=================================================================================================
void GamePanelContainer::Hide()
{
	visible = false;
	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->focus)
		{
			(*it)->Event(GuiEvent_LostFocus);
			(*it)->focus = false;
		}
	}
}
