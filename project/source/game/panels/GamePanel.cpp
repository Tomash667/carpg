#include "Pch.h"
#include "GameCore.h"
#include "GamePanel.h"
#include "GameDialogBox.h"
#include "KeyStates.h"
#include "Game.h"
#include "Language.h"

//-----------------------------------------------------------------------------
TEX GamePanel::tBackground;
Game* GameDialogBox::game;

//-----------------------------------------------------------------------------
enum MenuId
{
	Menu_Edit,
	Menu_Save,
	Menu_Load,
	Menu_LoadDefault
};

//=================================================================================================
GamePanel::GamePanel() : /*resizing(false), draging(false),*/ box_state(BOX_NOT_VISIBLE), order(0), last_index(INDEX_INVALID), last_index2(INDEX_INVALID)
{
	focusable = true;
}

//=================================================================================================
void GamePanel::Draw(ControlDrawData*)
{
	GUI.DrawItem(tBackground, pos, size, Color::Alpha(222), 16);
}

//=================================================================================================
void GamePanel::Update(float dt)
{
	// for version > 0.3 panels are no longer moveable/sizeable
	// maybe it will be allowed in future

	/*if(!focus)
		return;

	Int2 cpos = GetCursorPos();

	if(resizing)
	{
		if(!Key.Focus() || Key.Up(VK_LBUTTON))
		{
			resizing = false;
			return;
		}

		Int2 off = (cpos - move_offset);

		switch(move_what)
		{
		case 0:
			if(size.x - off.x < min_size.x)
				off.x = size.x - min_size.x;
			if(size.y - off.y < min_size.y)
				off.y = size.y - min_size.y;
			pos += off;
			global_pos += off;
			size -= off;
			if(off != Int2(0,0))
			{
				Event(GuiEvent_Moved);
				Event(GuiEvent_Resize);
			}
			break;
		case 1:
			if(size.x + off.x < min_size.x)
				off.x = min_size.x - size.x;
			if(size.y - off.y < min_size.y)
				off.y = size.y - min_size.y;
			pos.y += off.y;
			global_pos.y += off.y;
			size.x += off.x;
			size.y -= off.y;
			move_offset.x += off.x;
			if(off.y != 0)
				Event(GuiEvent_Moved);
			if(off != Int2(0,0))
				Event(GuiEvent_Resize);
			break;
		case 2:
			if(size.x - off.x < min_size.x)
				off.x = size.x - min_size.x;
			if(size.y + off.y < min_size.y)
				off.y = min_size.y - size.y;
			pos.x += off.x;
			global_pos.x += off.x;
			size.x -= off.x;
			size.y += off.y;
			move_offset.y += off.y;
			if(off.x != 0)
				Event(GuiEvent_Moved);
			if(off != Int2(0,0))
				Event(GuiEvent_Resize);
			break;
		case 3:
			if(size.x + off.x < min_size.x)
				off.x = min_size.x - size.x;
			if(size.y + off.y < min_size.y)
				off.y = min_size.y - size.y;
			if(shift_size)
			{
				if(off.x > off.y)
				{
					off.y = off.x;
					if(size.y + off.y < min_size.y)
						off.x = off.y = min_size.y - size.y;
				}
				else if(off.y > off.x)
				{
					off.x = off.y;
					if(size.x + off.x < min_size.x)
						off.y = off.x = min_size.x - size.x;
				}
			}
			if(off != Int2(0,0))
			{
				size += off;
				move_offset += off;
				Event(GuiEvent_Resize);
			}
			break;
		}
	}
	else if(draging)
	{
		if(!Key.Focus() || Key.Up(VK_LBUTTON))
		{
			draging = false;
			return;
		}

		Int2 off = (cpos - move_offset);
		if(off != Int2(0,0))
		{
			pos += off;
			global_pos += off;
			Event(GuiEvent_Moved);
		}
	}
	else if(Key.Focus() && Key.Pressed(VK_LBUTTON))
	{
		const int siz = 12;

		move_what = -1;
		if(cpos.x >= 0 && cpos.x < siz && cpos.y >= 0 && cpos.y < siz)
		{
			resizing = true;
			move_what = 0;
		}
		else if(cpos.x >= size.x-siz && cpos.x < size.x && cpos.y >= 0 && cpos.y < siz)
		{
			resizing = true;
			move_what = 1;
		}
		else if(cpos.x >= 0 && cpos.x < siz && cpos.y >= size.y-siz && cpos.y < size.y)
		{
			resizing = true;
			move_what = 2;
		}
		else if(cpos.x >= size.x-siz && cpos.x < size.x && cpos.y >= size.y-siz && cpos.y < size.y)
		{
			resizing = true;
			move_what = 3;
		}
		else if(cpos.x >= siz && cpos.x < size.x-siz && cpos.y >= 0 && cpos.y < siz)
		{
			draging = true;
			move_what = 0;
		}
		else if(cpos.x >= siz && cpos.x < size.x-siz && cpos.y >= size.y-siz && cpos.y < size.y)
		{
			draging = true;
			move_what = 0;
		}
		else if(cpos.x >= 0 && cpos.x < siz && cpos.y >= siz && cpos.y < size.y-siz)
		{
			draging = true;
			move_what = 0;
		}
		else if(cpos.x >= size.x-siz && cpos.x < size.x && cpos.y >= siz && cpos.y < size.y-siz)
		{
			draging = true;
			move_what = 0;
		}

		if(move_what != -1)
		{
			move_offset = cpos;
			shift_size = Key.Down(VK_SHIFT);
		}
	}*/
}

//=================================================================================================
void GamePanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show)
	{
		box_state = BOX_NOT_VISIBLE;
		last_index = INDEX_INVALID;
	}
	/*else if(e == GuiEvent_LostFocus)
	{
		resizing = false;
		draging = false;
	}*/
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
	GUI.DrawItem(tDialog, box_pos, box_size, Color::Alpha(alpha), 12);

	// obrazek
	if(box_img)
		GUI.DrawSprite(box_img, box_img_pos, Color::Alpha(alpha2));

	// du¿y tekst
	GUI.DrawText(GUI.default_font, box_text, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), box_big);

	// ma³y tekst
	if(!box_text_small.empty())
		GUI.DrawText(GUI.fSmall, box_text_small, DTF_PARSE_SPECIAL, Color(0, 0, 0, alpha2), box_small);
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
				box_img_size = gui::GetSize(box_img);
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
		Int2 text_size = GUI.default_font->CalculateSize(box_text);
		box_big = Rect::Create(Int2(0, 0), text_size);
		Int2 size = text_size + Int2(24, 24);
		Int2 pos2 = Int2(GUI.cursor_pos) + Int2(24, 24);
		Int2 text_pos(12, 12);

		// uwzglêdnij rozmiar obrazka
		if(box_img)
		{
			text_pos.x += box_img_size.x + 8;
			size.x += box_img_size.x + 8;
			if(size.y < box_img_size.y + 24)
				size.y = box_img_size.y + 24;
		}

		// minimalna szerokoœæ
		if(size.x < 256)
			size.x = 256;

		Int2 text_pos2(12, text_pos.y);
		text_pos2.y += size.y - 12;

		if(!box_text_small.empty())
		{
			Int2 size_small = GUI.fSmall->CalculateSize(box_text_small, size.x - 24);
			box_small = Rect::Create(Int2(0, 0), size_small);
			int size_y = size_small.y;
			size.y += size_y + 12;
		}

		if(pos2.x + size.x >= GUI.wnd_size.x)
			pos2.x = GUI.wnd_size.x - size.x - 1;
		if(pos2.y + size.y >= GUI.wnd_size.y)
			pos2.y = GUI.wnd_size.y - size.y - 1;

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
void GamePanelContainer::Draw(ControlDrawData* /*cdd*/)
{
	draw_box = nullptr;

	for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
			(*it)->Draw(nullptr);
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

		Int2 cp = GUI.cursor_pos;
		GamePanel* top = nullptr;

		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			GamePanel* gp = (GamePanel*)*it;
			gp->mouse_focus = false;
			if(gp->IsInside(cp))
				top = gp;
		}

		if(top)
		{
			top->mouse_focus = true;
			if((Key.Pressed(VK_LBUTTON) || Key.Pressed(VK_RBUTTON)) && top->order != order - 1)
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
			(*it)->mouse_focus = false;
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
