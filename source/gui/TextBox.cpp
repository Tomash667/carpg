#include "Pch.h"
#include "Base.h"
#include "KeyStates.h"
#include "Scrollbar.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
TEX TextBox::tBox;

//=================================================================================================
TextBox::TextBox(bool with_scrollbar, bool is_new) : Control(is_new), added(false), multiline(false), numeric(false), label(nullptr), scrollbar(nullptr), readonly(false),
with_scrollbar(with_scrollbar), select_pos(-1)
{
	if(with_scrollbar)
		AddScrollbar();
}

//=================================================================================================
TextBox::~TextBox()
{
	delete scrollbar;
}

//=================================================================================================
void TextBox::Draw(ControlDrawData*)
{
	if(!is_new)
	{
		cstring txt = (caret_blink >= 0.f ? Format("%s|", text.c_str()) : text.c_str());

		if(with_scrollbar)
		{
			INT2 textbox_size(size.x - 18, size.y);

			// border
			GUI.DrawItem(tBox, global_pos, textbox_size, WHITE, 4, 32);

			// text
			RECT r = { global_pos.x + 4, global_pos.y + 4, global_pos.x + textbox_size.x - 4, global_pos.y + textbox_size.y - 4 };
			GUI.DrawText(GUI.default_font, txt, DT_TOP, BLACK, r, &r);

			// scrollbar
			if(scrollbar)
				scrollbar->Draw();
		}
		else
		{
			GUI.DrawItem(tBox, global_pos, size, WHITE, 4, 32);

			RECT r = { global_pos.x + 4, global_pos.y + 4, global_pos.x + size.x - 4, global_pos.y + size.y - 4 };

			if(!scrollbar)
				GUI.DrawText(GUI.default_font, txt, multiline ? DT_TOP : DT_VCENTER, BLACK, r);
			else
			{
				RECT r2 = { r.left, r.top - int(scrollbar->offset), r.right, r.bottom - int(scrollbar->offset) };
				GUI.DrawText(GUI.default_font, txt, DT_TOP, BLACK, r2, &r);
				scrollbar->Draw();
			}

			if(label)
			{
				r.top -= 20;
				GUI.DrawText(GUI.default_font, label, DT_NOCLIP, BLACK, r);
			}
		}
	}
	else
	{
		// not coded yet
		assert(!with_scrollbar);
		assert(!label);

		GUI.DrawItem(tBox, global_pos, size, WHITE, 4, 32);

		RECT r = { global_pos.x + 4, global_pos.y + 4, global_pos.x + size.x - 4, global_pos.y + size.y - 4 };
		GUI.DrawText(GUI.default_font, text, DT_VCENTER, BLACK, r);

		if(caret_blink >= 0.f)
			GUI.DrawArea(BLACK, INT2(global_pos.x + 4 + caret_pos, global_pos.y + 4), INT2(1, GUI.default_font->height));
	}
}

//=================================================================================================
void TextBox::Update(float dt)
{
	if(mouse_focus)
	{
		if(!readonly)
		{
			bool inside;
			if(with_scrollbar)
				inside = PointInRect(GUI.cursor_pos, global_pos, INT2(size.x - 10, size.y));
			else
				inside = IsInside(GUI.cursor_pos);

			if(inside)
			{
				GUI.cursor_mode = CURSOR_TEXT;
				if(is_new && (Key.PressedRelease(VK_LBUTTON) || Key.PressedRelease(VK_RBUTTON)))
				{
					SetCaretPos(GUI.cursor_pos.x);
					caret_blink = 0.f;
					TakeFocus(true);
				}
			}
		}
		if(scrollbar)
			scrollbar->Update(dt);
	}

	if(focus)
	{
		caret_blink += dt * 2;
		if(caret_blink >= 1.f)
			caret_blink = -1.f;

		if(is_new)
		{
			if(Key.PressedRelease(VK_LEFT))
			{
				if(select_pos > 0)
				{
					--select_pos;
					caret_pos -= GUI.default_font->GetCharWidthA(text[select_pos]);
					caret_blink = 0.f;
				}
			}
			else if(Key.PressedRelease(VK_RIGHT))
			{
				if(select_pos != text.length())
				{
					caret_pos += GUI.default_font->GetCharWidthA(text[select_pos]);
					++select_pos;
					caret_blink = 0.f;
				}
			}
		}
	}
	else
		caret_blink = -1.f;
}

//=================================================================================================
void TextBox::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		if(!added)
		{
			if(!is_new)
				caret_blink = 0.f;
			GUI.AddOnCharHandler(this);
			added = true;
		}
	}
	else if(e == GuiEvent_LostFocus)
	{
		if(added)
		{
			caret_blink = -1.f;
			GUI.RemoveOnCharHandler(this);
			added = false;
		}
	}
}

//=================================================================================================
void TextBox::OnChar(char c)
{
	if(c == VK_BACK)
	{
		// backspace
		if(!text.empty())
			text.resize(text.size() - 1);
	}
	else if(c == VK_RETURN && !multiline)
	{
		// pomiñ znak
	}
	else
	{
		if(numeric)
		{
			if(c == '-')
			{
				if(text.empty())
					text.push_back('-');
				else if(text[0] == '-')
					text.erase(text.begin());
				else
					text.insert(text.begin(), '-');
				ValidateNumber();
			}
			else if(c == '+')
			{
				if(!text.empty() && text[0] == '-')
				{
					text.erase(text.begin());
					ValidateNumber();
				}
			}
			else if(isdigit(byte(c)))
			{
				int len = text.length();
				if(!text.empty() && text[0] == '-')
					--len;
				text.push_back(c);
				ValidateNumber();
			}
		}
		else
		{
			if(limit <= 0 || limit > (int)text.size())
				text.push_back(c);
		}
	}
}

//=================================================================================================
void TextBox::ValidateNumber()
{
	int n = atoi(text.c_str());
	if(n < num_min)
		text = Format("%d", num_min);
	else if(n > num_max)
		text = Format("%d", num_max);
}

//=================================================================================================
void TextBox::AddScrollbar()
{
	scrollbar = new Scrollbar;
	scrollbar->pos = INT2(size.x + 2, 0);
	scrollbar->size = INT2(16, size.y);
	scrollbar->total = 8;
	scrollbar->part = size.y - 8;
	scrollbar->offset = 0.f;
}

//=================================================================================================
void TextBox::Move(const INT2& _global_pos)
{
	global_pos = _global_pos + pos;
	if(scrollbar)
		scrollbar->global_pos = global_pos + scrollbar->pos;
}

//=================================================================================================
void TextBox::Add(cstring str)
{
	assert(scrollbar);
	INT2 str_size = GUI.default_font->CalculateSize(str, size.x - 8);
	bool skip_to_end = (int(scrollbar->offset) >= (scrollbar->total - scrollbar->part));
	scrollbar->total += str_size.y;
	if(text.empty())
		text = str;
	else
	{
		text += '\n';
		text += str;
	}
	if(skip_to_end)
	{
		scrollbar->offset = float(scrollbar->total - scrollbar->part);
		if(scrollbar->offset < 0.f)
			scrollbar->offset = 0.f;
	}
}

//=================================================================================================
void TextBox::Reset()
{
	text.clear();
	if(scrollbar)
	{
		scrollbar->total = 8;
		scrollbar->part = size.y - 8;
		scrollbar->offset = 0.f;
	}
}

//=================================================================================================
void TextBox::UpdateScrollbar()
{
	INT2 text_size = GUI.default_font->CalculateSize(text, size.x - (with_scrollbar ? 18 : 8));
	scrollbar->total = text_size.y;
}

//=================================================================================================
void TextBox::UpdateSize(const INT2& new_pos, const INT2& new_size)
{
	global_pos = pos = new_pos;
	size = new_size;

	scrollbar->global_pos = scrollbar->pos = INT2(global_pos.x + size.x - 16, global_pos.y);
	scrollbar->size = INT2(16, size.y);
	scrollbar->offset = 0.f;
	scrollbar->part = size.y - 8;

	UpdateScrollbar();
}

//=================================================================================================
void TextBox::SetCaretPos(int x)
{
	assert(!multiline); // not implemented yet

	int local_x = x - global_pos.x - 4;
	if(local_x < 0)
	{
		select_pos = 0;
		caret_pos = 0;
		return;
	}

	int offset = 0, index = 0;
	for(char c : text)
	{
		int width = GUI.default_font->GetCharWidthA(c);
		if(local_x < offset + width / 2)
		{
			select_pos = index;
			caret_pos = offset;
			return;
		}
		++index;
		offset += width;
	}

	select_pos = text.length();
	caret_pos = offset;
}
