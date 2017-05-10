#include "Pch.h"
#include "Base.h"
#include "KeyStates.h"
#include "ListBox.h"
#include "MenuList.h"
#include "MenuStrip.h"

using namespace gui;

//=================================================================================================
ListBox::ListBox(bool is_new) : Control(is_new), scrollbar(false, is_new), selected(-1), event_handler(nullptr), event_handler2(nullptr), menu(nullptr), menu_strip(nullptr),
force_img_size(0, 0), item_height(20)
{
	SetOnCharHandler(true);
}

//=================================================================================================
ListBox::~ListBox()
{
	DeleteElements(items);
	delete menu;
	delete menu_strip;
}

//=================================================================================================                                                                                
void ListBox::Draw(ControlDrawData*)
{
	if(collapsed)
	{
		// box
		GUI.DrawItem(GUI.tBox, global_pos, size, WHITE, 8, 32);

		// element
		if(selected != -1)
		{
			RECT rc = { global_pos.x + 2, global_pos.y + 2, global_pos.x + size.x - 12, global_pos.y + size.y - 2 };
			GUI.DrawText(GUI.default_font, items[selected]->ToString(), DT_SINGLELINE, BLACK, rc, &rc);
		}

		// obrazek
		GUI.DrawSprite(GUI.tDown, INT2(global_pos.x + size.x - 10, global_pos.y + (size.y - 10) / 2));

		// powinno byæ tu ale wtedy by³a by z³a kolejnoœæ rysowania
		//if(menu->visible)
		//	menu->Draw();
	}
	else
	{
		// box
		GUI.DrawItem(GUI.tBox, global_pos, real_size, WHITE, 8, 32);

		// zaznaczenie
		RECT rc = { global_pos.x, global_pos.y, global_pos.x + real_size.x, global_pos.y + real_size.y };
		if(selected != -1)
		{
			RECT rs = { global_pos.x + 2, global_pos.y - int(scrollbar.offset) + 2 + selected*item_height, global_pos.x + real_size.x - 2 };
			rs.bottom = rs.top + item_height;
			RECT out;
			if(IntersectRect(&out, &rs, &rc))
				GUI.DrawSpriteRect(GUI.tPix, out, COLOR_RGBA(0, 255, 0, 128));
		}

		// elementy
		RECT r = { global_pos.x + 2, global_pos.y - int(scrollbar.offset) + 2, global_pos.x + real_size.x - 2, rc.bottom - 2 };
		int orig_x = global_pos.x + 2;
		MATRIX mat;
		for(GuiElement* e : items)
		{
			if(e->tex)
			{
				INT2 required_size = force_img_size, img_size;
				VEC2 scale;
				Control::ResizeImage(e->tex, required_size, img_size, scale);
				D3DXMatrixTransformation2D(&mat, nullptr, 0.f, &scale, nullptr, 0.f, &VEC2((float)orig_x, float(r.top + (item_height - required_size.y) / 2)));
				GUI.DrawSprite2(e->tex, &mat, nullptr, &rc, WHITE);
				r.left = orig_x + required_size.x;
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
	if(collapsed)
	{
		if(is_new)
		{
			if(mouse_focus && Key.PressedRelease(VK_LBUTTON))
			{
				if(menu_strip->IsOpen())
					TakeFocus(true);
				else
				{
					menu_strip->ShowMenu(global_pos + INT2(0, size.y));
					menu_strip->SetSelectedIndex(selected);
				}
			}
		}
		else
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
				menu->global_pos = global_pos + INT2(0, size.y);
				if(menu->global_pos.y + menu->size.y >= GUI.wnd_size.y)
					menu->global_pos.y = GUI.wnd_size.y - menu->size.y;
				menu->visible = true;
				menu->focus = true;
			}
		}
	}
	else
	{
		if(is_new)
		{
			if(focus)
			{
				int dir = 0;
				if(Key.DownRepeat(VK_UP))
					dir = -1;
				else if(Key.DownRepeat(VK_DOWN))
					dir = 1;
				else if(Key.PressedRelease(VK_SPACE) && selected == -1 && !items.empty())
					ChangeIndexEvent(0, false, true);

				if(dir != 0)
				{
					int new_index = modulo(selected + dir, items.size());
					if(new_index != selected)
						ChangeIndexEvent(new_index, false, true);
				}
			}

			if(mouse_focus)
				scrollbar.ApplyMouseWheel();
			UpdateControl(&scrollbar, dt);
		}

		if(mouse_focus && Key.Focus() && PointInRect(GUI.cursor_pos, global_pos, real_size))
		{
			int bt = 0;

			if(event_handler2 && Key.DoubleClick(VK_LBUTTON))
			{
				int new_index = PosToIndex(GUI.cursor_pos.y);
				if(new_index != -1 && new_index == selected)
				{
					event_handler2(A_DOUBLE_CLICK, selected);
					bt = -1;
				}
			}

			if(bt != -1)
			{
				if(Key.PressedRelease(VK_LBUTTON))
					bt = 1;
				else if(Key.PressedRelease(VK_RBUTTON))
					bt = 2;
			}

			if(bt > 0)
			{
				int new_index = PosToIndex(GUI.cursor_pos.y);
				bool ok = true;
				if(new_index != -1 && new_index != selected)
					ok = ChangeIndexEvent(new_index, false, false);

				if(bt == 2 && menu_strip && ok)
				{
					if(event_handler2)
					{
						if(!event_handler2(A_BEFORE_MENU_SHOW, new_index))
							ok = false;
					}
					if(ok)
					{
						menu_strip->SetHandler(delegate<void(int)>(this, &ListBox::OnSelect));
						menu_strip->ShowMenu();
					}
				}
				else if(is_new)
					TakeFocus(true);
			}
		}

		if(!is_new)
		{
			if(IsInside(GUI.cursor_pos))
				scrollbar.ApplyMouseWheel();
			scrollbar.Update(dt);
		}
	}
}

//=================================================================================================
void ListBox::Event(GuiEvent e)
{
	if(e == GuiEvent_Moved)
	{
		if(!is_new)
			global_pos = parent->global_pos + pos;
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_LostFocus)
		scrollbar.LostFocus();
}

//=================================================================================================
void ListBox::OnChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = tolower(c);

	bool first = true;
	int start_index = selected;
	if(start_index == -1)
		start_index = 0;
	int index = start_index;

	while(true)
	{
		auto item = items[index];
		char starts_with = tolower(item->ToString()[0]);
		if(starts_with == c)
		{
			if(index == selected && first)
				first = false;
			else
			{
				ChangeIndexEvent(index, false, true);
				return;
			}
		}
		else if(index == start_index)
		{
			if(first)
				first = false;
			else
				return;
		}
		index = (index + 1) % items.size();
	}
}

//=================================================================================================
void ListBox::Add(GuiElement* e)
{
	assert(e);
	items.push_back(e);
	scrollbar.total += item_height;
}

//=================================================================================================
void ListBox::Init(bool _collapsed)
{
	collapsed = _collapsed;
	real_size = INT2(size.x - 20, size.y);
	scrollbar.pos = INT2(size.x - 16, 0);
	scrollbar.size = INT2(16, size.y);
	scrollbar.offset = 0.f;
	scrollbar.total = items.size()*item_height;
	scrollbar.part = size.y - 4;

	if(collapsed)
	{
		if(is_new)
		{
			menu_strip = new MenuStrip(items, size.x);
			menu_strip->SetHandler(DialogEvent(this, &ListBox::OnSelect));
		}
		else
		{
			menu = new MenuList;
			menu->AddItems(items, false);
			menu->visible = false;
			menu->Init();
			menu->size.x = size.x;
			menu->event_handler = DialogEvent(this, &ListBox::OnSelect);
		}
	}
}

//=================================================================================================
void ListBox::ScrollTo(int index, bool center)
{
	const int count = (int)items.size();
	assert(index >= 0 && index < count);
	if(center)
	{
		int n = int(real_size.y / item_height);
		if(index < n)
			scrollbar.offset = 0.f;
		else if(index > count - n)
			scrollbar.offset = float(scrollbar.total - scrollbar.part);
		else
			scrollbar.offset = float((index - n)*item_height);
	}
	else
	{
		int posy = index * item_height;
		int offsety = posy - (int)scrollbar.offset;
		if(offsety < 0)
			scrollbar.offset = (float)posy;
		else if(offsety + item_height > size.y)
			scrollbar.offset = (float)(item_height + posy - size.y);
	}
}

//=================================================================================================
void ListBox::OnSelect(int index)
{
	if(is_new)
	{
		if(collapsed)
		{
			if(index != selected)
				ChangeIndexEvent(index, false, false);
		}
		else
		{
			if(event_handler2)
				event_handler2(A_MENU, index);
		}
	}
	else
	{
		menu->visible = false;
		if(index != selected)
			ChangeIndexEvent(index, false, false);
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
void ListBox::Select(int index, bool send_event)
{
	if(send_event)
	{
		if(!ChangeIndexEvent(index, false, false))
			return;
	}
	else
		selected = index;
	ScrollTo(index);
}

//=================================================================================================
void ListBox::Select(delegate<bool(GuiElement*)> pred, bool send_event)
{
	int index = 0;
	for(GuiElement* item : items)
	{
		if(pred(item))
		{
			if(selected != index)
			{
				selected = index;
				if(event_handler)
					event_handler(selected);
				if(event_handler2)
					event_handler2(A_INDEX_CHANGED, selected);
			}
			break;
		}
		++index;
	}
}

//=================================================================================================
void ListBox::ForceSelect(int index)
{
	ChangeIndexEvent(index, true, true);
	ScrollTo(index);
}

//=================================================================================================
void ListBox::Insert(GuiElement* e, int index)
{
	assert(e && index >= 0);
	if(index >= (int)items.size())
	{
		Add(e);
		return;
	}
	items.insert(items.begin() + index, e);
	scrollbar.total += item_height;
	if(selected >= index)
		++selected;
}

//=================================================================================================
void ListBox::Remove(int index)
{
	assert(index >= 0 && index < (int)items.size());
	delete items[index];
	items.erase(items.begin() + index);
	if(selected > index || (selected == index && selected == (int)items.size()))
		--selected;
	if(event_handler)
		event_handler(selected);
	if(event_handler2)
		event_handler2(A_INDEX_CHANGED, selected);
}

//=================================================================================================
int ListBox::PosToIndex(int y)
{
	int n = (y - global_pos.y + int(scrollbar.offset)) / item_height;
	if(n >= 0 && n < (int)items.size())
		return n;
	else
		return -1;
}

//=================================================================================================
bool ListBox::ChangeIndexEvent(int index, bool force, bool scroll_to)
{
	if(!force && event_handler2)
	{
		if(!event_handler2(A_BEFORE_CHANGE_INDEX, index))
			return false;
	}

	selected = index;

	if(event_handler)
		event_handler(selected);
	if(event_handler2)
		event_handler2(A_INDEX_CHANGED, selected);

	if(scroll_to && !collapsed)
		ScrollTo(index);

	return true;
}

//=================================================================================================
void ListBox::Reset()
{
	scrollbar.total = 0;
	selected = -1;
	DeleteElements(items);
}
