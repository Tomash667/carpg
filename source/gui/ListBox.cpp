#include "Pch.h"
#include "Base.h"
#include "ListBox.h"
#include "KeyStates.h"

//=================================================================================================
ListBox::ListBox() : selected(-1), event_handler(nullptr), menu(nullptr), force_img_size(0, 0), item_height(20)
{

}

//=================================================================================================
ListBox::~ListBox()
{
	DeleteElements(items);
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
			GUI.DrawText(GUI.default_font, items[selected]->ToString(), DT_SINGLELINE, BLACK, rc, &rc);
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
			RECT rs = {global_pos.x+2, global_pos.y-int(scrollbar.offset)+2+selected*item_height, global_pos.x+real_size.x-2};
			rs.bottom = rs.top + item_height;
			RECT out;
			if(IntersectRect(&out, &rs, &rc))
				GUI.DrawSpriteRect(GUI.tPix, out, COLOR_RGBA(0,255,0,128));
		}

		// elementy
		RECT r = {global_pos.x+2, global_pos.y-int(scrollbar.offset)+2, global_pos.x+real_size.x-2, rc.bottom-2};
		int orig_x = global_pos.x+2;
		MATRIX mat;
		for(GuiElement* e : items)
		{
			if(e->tex)
			{
				INT2 size = force_img_size, img_size;
				VEC2 scale;
				Control::ResizeImage(e->tex, size, img_size, scale);
				D3DXMatrixTransformation2D(&mat, nullptr, 0.f, &scale, nullptr, 0.f, &VEC2((float)orig_x, float(r.top+(item_height-size.y)/2)));
				GUI.DrawSprite2(e->tex, &mat, nullptr, &rc, WHITE);
				r.left = orig_x + size.x;
			}
			else
				r.left = orig_x;
			if(!GUI.DrawText(GUI.default_font, e->ToString(), DT_SINGLELINE, BLACK, r, &rc))
				break;
			r.top += item_height;
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
			int n = (GUI.cursor_pos.y-global_pos.y+int(scrollbar.offset))/item_height;
			if(n >= 0 && n < (int)items.size() && n != selected)
			{
				selected = n;
				if(event_handler)
					event_handler(n);
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
	scrollbar.total += item_height;
}

//=================================================================================================
void ListBox::Init(bool _extended)
{
	extended = _extended;
	real_size = INT2(size.x-20,size.y);
	scrollbar.pos = INT2(size.x-16,0);
	scrollbar.size = INT2(16,size.y);
	scrollbar.offset = 0.f;
	scrollbar.total = items.size()*item_height;
	scrollbar.part = size.y-4;

	if(extended)
	{
		menu = new MenuList;
		menu->AddItems(items, false);
		menu->visible = false;
		menu->Init();
		menu->size.x = size.x;
		menu->event_handler = DialogEvent(this, &ListBox::OnSelect);
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
		scrollbar.offset = float((index-n)*item_height);
}

//=================================================================================================
void ListBox::OnSelect(int index)
{
	menu->visible = false;
	if(index != selected)
	{
		selected = index;
		if(event_handler)
			event_handler(selected);
	}
}

//=================================================================================================
void ListBox::Sort()
{
	std::sort(items.begin(), items.end(), 
		[](GuiElement* e1, GuiElement* e2)
		{
			return strcoll(e1->ToString(), e2->ToString()) < 0;
		});
}

//=================================================================================================
GuiElement* ListBox::Find(int value)
{
	for(GuiElement* e : items)
	{
		if(e->value == value)
			return e;
	}

	return nullptr;
}

//=================================================================================================
int ListBox::FindIndex(int value)
{
	for(int i = 0; i < (int)items.size(); ++i)
	{
		if(items[i]->value == value)
			return i;
	}

	return -1;
}

//=================================================================================================
void ListBox::Select(int index)
{
	selected = index;
	ScrollTo(index);
}
