#include "Pch.h"
#include "Base.h"
#include "ListBox.h"
#include "KeyStates.h"

//=================================================================================================
ListBox::ListBox() : selected(-1), e_change_index(NULL), menu(NULL)
{

}

//=================================================================================================
ListBox::~ListBox()
{
	DeleteElements(items);
	StringPool.Free(texts);
	delete menu;
}

//=================================================================================================
void ListBox::Draw(ControlDrawData*)
{
	if(extended)
	{
		// box
		GUI.DrawItem(GUI.tBox, global_pos, size, WHITE, 8, 32);

		// element
		if(selected != -1)
		{
			RECT rc = {global_pos.x+2, global_pos.y+2, global_pos.x+size.x-12, global_pos.y+size.y-2};
			GUI.DrawText(GUI.default_font, *texts[selected], DT_SINGLELINE, BLACK, rc, &rc);
		}

		// obrazek
		GUI.DrawSprite(GUI.tDown, INT2(global_pos.x+size.x-10, global_pos.y+(size.y-10)/2));

		// powinno byæ tu ale wtedy by³a by z³a kolejnoœæ rysowania
		//if(menu->visible)
		//	menu->Draw();
	}
	else
	{
		// box
		GUI.DrawItem(GUI.tBox, global_pos, real_size, WHITE, 8, 32);

		// zaznaczenie
		RECT rc = {global_pos.x, global_pos.y, global_pos.x+real_size.x, global_pos.y+real_size.y};
		if(selected != -1)
		{
			RECT rs = {global_pos.x+2, global_pos.y-int(scrollbar.offset)+2+selected*20, global_pos.x+real_size.x-2};
			rs.bottom = rs.top + 20;
			RECT out;
			if(IntersectRect(&out, &rs, &rc))
				GUI.DrawSpriteRect(GUI.tPix, out, COLOR_RGBA(0,255,0,128));
		}

		// elementy
		RECT r = {global_pos.x+2, global_pos.y-int(scrollbar.offset)+2, global_pos.x+real_size.x-2, rc.bottom-2};
		for(vector<string*>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			if(!GUI.DrawText(GUI.default_font, **it, DT_SINGLELINE, BLACK, r, &rc))
				break;
			r.top += 20;
		}

		// pasek przewijania
		scrollbar.Draw();
	}
}

//=================================================================================================
void ListBox::Update(float dt)
{
	if(extended)
	{
		if(menu->visible)
		{
			// powinno byæ aktualizowane tu ale niestety wed³ug kolejnoœci musi byæ na samym pocz¹tku
			//menu->Update(dt);
			if(!menu->focus)
				menu->visible = false;
		}
		else if(mouse_focus && Key.Focus() && PointInRect(GUI.cursor_pos, global_pos, size) && Key.PressedRelease(VK_LBUTTON))
		{
			menu->global_pos = global_pos + INT2(0,size.y);
			if(menu->global_pos.y+menu->size.y >= GUI.wnd_size.y)
				menu->global_pos.y = GUI.wnd_size.y-menu->size.y;
			menu->visible = true;
			menu->focus = true;
		}
	}
	else
	{
		if(mouse_focus && Key.Focus() && PointInRect(GUI.cursor_pos, global_pos, real_size) && Key.PressedRelease(VK_LBUTTON))
		{
			int n = (GUI.cursor_pos.y-global_pos.y+int(scrollbar.offset))/20;
			if(n >= 0 && n < (int)items.size() && n != selected)
			{
				selected = n;
				if(e_change_index)
					e_change_index(n);
			}
		}

		if(IsInside(GUI.cursor_pos))
			scrollbar.ApplyMouseWheel();
		scrollbar.Update(dt);
	}
}

//=================================================================================================
void ListBox::Event(GuiEvent e)
{
	if(e == GuiEvent_Moved)
	{
		global_pos = parent->global_pos + pos;
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
}

//=================================================================================================
void ListBox::Add(GuiElement* e)
{
	assert(e);

	items.push_back(e);
	string* s = StringPool.Get();
	*s = e->ToString();
	texts.push_back(s);

	scrollbar.total += 20;
}

//=================================================================================================
void ListBox::Init(bool _extended)
{
	extended = _extended;
	real_size = INT2(size.x-20,size.y);
	scrollbar.pos = INT2(size.x-16,0);
	scrollbar.size = INT2(16,size.y);
	scrollbar.offset = 0.f;
	scrollbar.total = items.size()*20;
	scrollbar.part = size.y-4;

	if(extended)
	{
		menu = new MenuList;
		menu->AddItems(texts);
		menu->visible = false;
		menu->Init();
		menu->size.x = size.x;
		menu->event = DialogEvent(this, &ListBox::OnSelect);
	}
}

//=================================================================================================
void ListBox::ScrollTo(int index)
{
	const int count = (int)items.size();
	assert(index >= 0 && index < count);
	int n = int(real_size.y/40);
	if(index < n)
		scrollbar.offset = 0.f;
	else if(index > count-n)
		scrollbar.offset = float(scrollbar.total-scrollbar.part);
	else
		scrollbar.offset = float((index-n)*20);
}

//=================================================================================================
void ListBox::OnSelect(int index)
{
	menu->visible = false;
	if(index != selected)
	{
		selected = index;
		if(e_change_index)
			e_change_index(selected);
	}
}
