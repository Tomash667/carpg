#include "Pch.h"
#include "Base.h"
#include "KeyStates.h"
#include "Scrollbar.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
TEX TextBox::tBox;
static const int padding = 4;

//=================================================================================================
TextBox::TextBox(bool with_scrollbar, bool is_new) : Control(is_new), added(false), multiline(false), numeric(false), label(nullptr), scrollbar(nullptr), readonly(false),
	with_scrollbar(with_scrollbar), caret_index(-1), select_start_index(-1), down(false), offset(0), offset_move(0.f), tBackground(nullptr)
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
void TextBox::Draw(ControlDrawData* cdd)
{
	TEX background = tBackground ? tBackground : tBox;

	if(!is_new)
	{
		cstring txt = (caret_blink >= 0.f ? Format("%s|", text.c_str()) : text.c_str());

		if(with_scrollbar)
		{
			INT2 textbox_size(size.x - 18, size.y);

			// border
			GUI.DrawItem(background, global_pos, textbox_size, WHITE, 4, 32);

			// text
			RECT r = { global_pos.x + padding, global_pos.y + padding, global_pos.x + textbox_size.x - padding, global_pos.y + textbox_size.y - padding };
			GUI.DrawText(GUI.default_font, txt, DT_TOP, BLACK, r, &r);

			// scrollbar
			if(scrollbar)
				scrollbar->Draw();
		}
		else
		{
			GUI.DrawItem(background, global_pos, size, WHITE, 4, 32);

			RECT r = { global_pos.x + padding, global_pos.y + padding, global_pos.x + size.x - padding, global_pos.y + size.y - padding };

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
		assert(!multiline);

		BOX2D* clip_rect = nullptr;
		if(cdd)
			clip_rect = cdd->clipping;

		GUI.DrawItem(background, global_pos, size, WHITE, 4, 32, clip_rect);

		RECT rclip;
		RECT textbox_rect = { global_pos.x + padding, global_pos.y + padding, global_pos.x + size.x - padding, global_pos.y + size.y - padding };
		if(clip_rect)
			IntersectRect(&rclip, &clip_rect->ToRect(), &textbox_rect);
		else
			rclip = textbox_rect;

		if(select_start_index != -1 && select_start_index != select_end_index)
		{
			RECT r = {
				global_pos.x + padding + select_start_pos - offset,
				global_pos.y + padding,
				global_pos.x + padding + select_end_pos - offset,
				global_pos.y + size.y - padding
			};
			RECT area;
			IntersectRect(&area, &r, &rclip);
			GUI.DrawArea(COLOR_RGBA(0, 148, 255, 128), INT2(area.left, area.top), INT2(area.right - area.left, area.bottom - area.top));
		}

		RECT r = { global_pos.x + padding - offset, global_pos.y + padding, global_pos.x + size.x - padding - offset, global_pos.y + size.y - padding };
		RECT area;
		IntersectRect(&area, &r, &rclip);
		GUI.DrawText(GUI.default_font, text, DT_VCENTER | DT_SINGLELINE, BLACK, r, &area);

		if(caret_blink >= 0.f)
		{
			INT2 p(global_pos.x + padding + caret_pos - offset, global_pos.y + padding);
			if(p.x >= rclip.left && p.x <= rclip.right)
				GUI.DrawArea(BLACK, p, INT2(1, GUI.default_font->height), clip_rect);
		}
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
					// set caret position, update selection
					bool prev_focus = focus;
					int new_index, new_pos;
					GetCaretPos(GUI.cursor_pos.x, new_index, new_pos);
					caret_blink = 0.f;
					TakeFocus(true);
					if(Key.Down(VK_SHIFT) && prev_focus)
						CalculateSelection(new_index, new_pos);
					else
						select_start_index = -1;
					caret_index = new_index;
					caret_pos = new_pos;
					down = true;
				}
			}
		}
		if(scrollbar)
			scrollbar->Update(dt);
	}

	if(focus)
	{
		// update caret blinking
		caret_blink += dt * 2;
		if(caret_blink >= 1.f)
			caret_blink = -1.f;

		if(is_new)
		{
			// update selecting with mouse
			if(down)
			{
				if(Key.Up(VK_LBUTTON))
					down = false;
				else
				{
					const int MOVE_SPEED = 300;
					int local_x = GUI.cursor_pos.x - global_pos.x - padding;
					if(local_x <= 0.1f * size.x && offset != 0)
					{
						offset_move -= dt * MOVE_SPEED;
						int offset_move_i = -(int)ceil(offset_move);
						offset_move += offset_move_i;
						offset -= offset_move_i;
						if(offset < 0)
							offset = 0;
					}
					else if(local_x >= 0.9f * size.x)
					{
						offset_move += dt * MOVE_SPEED;
						int offset_move_i = (int)floor(offset_move);
						offset_move -= offset_move_i;
						offset += offset_move_i;
						const int real_size = size.x - padding * 2;
						const int total_width = GUI.default_font->CalculateSize(text).x;
						int max_offset = total_width - real_size;
						if(offset > max_offset)
							offset = max_offset;
						if(offset < 0)
							offset = 0;
					}

					int new_index, new_pos;
					GetCaretPos(GUI.cursor_pos.x, new_index, new_pos);
					if(new_index != caret_index)
					{
						CalculateSelection(new_index, new_pos);
						caret_index = new_index;
						caret_pos = new_pos;
						caret_blink = 0.f;
					}
				}
			}

			// move caret left/right
			int move = 0;
			if(Key.DownRepeat(VK_LEFT))
			{
				if(caret_index > 0)
					move = -1;
			}
			else if(Key.DownRepeat(VK_RIGHT))
			{
				if(caret_index != text.length())
					move = +1;
			}

			if(move != 0)
			{
				int new_index, new_pos;
				if(Key.Down(VK_SHIFT) || select_start_index == -1)
				{
					new_index = caret_index + move;
					if(move == +1)
						new_pos = caret_pos + GUI.default_font->GetCharWidthA(text[caret_index]);
					else
						new_pos = caret_pos - GUI.default_font->GetCharWidthA(text[new_index]);

					if(Key.Down(VK_SHIFT))
						CalculateSelection(new_index, new_pos);
				}
				else if(select_start_index != -1)
				{
					if(move == -1)
					{
						new_index = select_start_index;
						new_pos = select_start_pos;
					}
					else
					{
						new_index = select_end_index;
						new_pos = select_end_pos;
					}
					select_start_index = -1;
				}
				
				caret_index = new_index;
				caret_pos = new_pos;
				caret_blink = 0.f;
				CalculateOffset(false);
			}

			// select all
			if(Key.Shortcut(KEY_CONTROL, 'A'))
			{
				caret_index = 0;
				caret_pos = 0;
				caret_blink = 0.f;
				select_start_index = 0;
				select_start_pos = 0;
				select_end_index = text.size();
				select_end_pos = IndexToPos(select_end_index);
				select_fixed_index = 0;
			}

			// copy
			if(select_start_index != -1 && Key.Shortcut(KEY_CONTROL, 'C'))
			{
				GUI.SetClipboard(text.substr(select_start_index, select_end_index - select_start_index).c_str());
			}

			// paste
			if(Key.Shortcut(KEY_CONTROL, 'V'))
			{
				cstring clipboard = GUI.GetClipboard();
				if(clipboard)
				{
					if(select_start_index != -1)
						DeleteSelection();
					int len = strlen(clipboard);
					if(limit <= 0 || text.length() + len <= (uint)limit)
						text.insert(caret_index, clipboard);
					else
					{
						int max_chars = limit - text.length();
						text.insert(caret_index, clipboard, max_chars);
					}
					caret_index += strlen(clipboard);
					caret_pos = IndexToPos(caret_index);
					CalculateOffset(true);
				}
			}

			// cut
			if(select_start_index != -1 && Key.Shortcut(KEY_CONTROL, 'X'))
			{
				GUI.SetClipboard(text.substr(select_start_index, select_end_index - select_start_index).c_str());
				DeleteSelection();
				CalculateOffset(true);
			}
		}
	}
	else
	{
		caret_blink = -1.f;
		down = false;
	}
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
			select_start_index = -1;
			caret_blink = -1.f;
			GUI.RemoveOnCharHandler(this);
			added = false;
			down = false;
			offset_move = 0.f;
		}
	}
}

//=================================================================================================
void TextBox::OnChar(char c)
{
	if(c == VK_BACK)
	{
		// backspace
		if(!is_new)
		{
			if(!text.empty())
				text.pop_back();
		}
		else
		{
			if(select_start_index != -1)
			{
				DeleteSelection();
				CalculateOffset(true);
			}
			else if(caret_index > 0)
			{
				--caret_index;
				caret_pos -= GUI.default_font->GetCharWidthA(text[caret_index]);
				caret_blink = 0.f;
				text.erase(caret_index, 1);
				CalculateOffset(true);
			}
		}
	}
	else if(c == VK_DELETE)
	{
		if(is_new)
		{
			if(select_start_index != -1)
			{
				DeleteSelection();
				CalculateOffset(true);
			}
			else if(caret_index != text.size())
			{
				text.erase(caret_index, 1);
				caret_blink = 0.f;
				CalculateOffset(true);
			}
		}
	}
	else if(c == VK_RETURN && !multiline)
	{
		// enter - skip
	}
	else
	{
		if(numeric)
		{
			assert(!is_new);
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
			if(select_start_index != -1)
				DeleteSelection();
			if(limit <= 0 || limit > (int)text.size())
			{
				if(!is_new)
					text.push_back(c);
				else
				{
					text.insert(caret_index, 1, c);
					caret_pos += GUI.default_font->GetCharWidthA(c);
					caret_blink = 0.f;
					++caret_index;
					CalculateOffset(true);
				}
			}
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
	assert(!is_new);
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
void TextBox::GetCaretPos(int x, int& out_index, int& out_pos)
{
	assert(!multiline);

	int local_x = x - global_pos.x - padding + offset;
	if(local_x < 0)
	{
		out_index = 0;
		out_pos = 0;
		return;
	}

	const int real_size = size.x - padding * 2;
	if(local_x > real_size + offset)
		local_x = real_size + offset;
	else if(local_x < offset)
		local_x = offset;

	int total = 0, index = 0;
	for(char c : text)
	{
		int width = GUI.default_font->GetCharWidthA(c);
		if(local_x < total + width / 2)
		{
			out_index = index;
			out_pos = total;
			return;
		}
		++index;
		total += width;
	}

	out_index = text.length();
	out_pos = total;
}

//=================================================================================================
void TextBox::CalculateSelection(int new_index, int new_pos)
{
	if(select_start_index == -1)
	{
		CalculateSelection(new_index, new_pos, caret_index, caret_pos);
		select_fixed_index = caret_index;
	}
	else if(select_fixed_index == select_start_index)
		CalculateSelection(new_index, new_pos, select_start_index, select_start_pos);
	else
		CalculateSelection(new_index, new_pos, select_end_index, select_end_pos);
}

//=================================================================================================
void TextBox::CalculateSelection(int index1, int pos1, int index2, int pos2)
{
	if(index1 < index2)
	{
		select_start_index = index1;
		select_start_pos = pos1;
		select_end_index = index2;
		select_end_pos = pos2;
	}
	else
	{
		select_start_index = index2;
		select_start_pos = pos2;
		select_end_index = index1;
		select_end_pos = pos1;
	}
}

//=================================================================================================
void TextBox::DeleteSelection()
{
	text.erase(select_start_index, select_end_index - select_start_index);
	caret_index = select_start_index;
	caret_pos = select_start_pos;
	caret_blink = 0.f;
	select_start_index = -1;
}

//=================================================================================================
int TextBox::IndexToPos(int index)
{
	int total = 0, i = 0;
	for(char c : text)
	{
		if(index == i)
			return total;
		total += GUI.default_font->GetCharWidthA(c);
		++i;
	}
	return total;
}

//=================================================================================================
void TextBox::SetText(cstring new_text)
{
	if(new_text)
		text = new_text;
	else
		text.clear();
	select_start_index = -1;
	offset = 0;
}

//=================================================================================================
void TextBox::CalculateOffset(bool center)
{
	const int real_pos = caret_pos - offset;
	const int real_size = size.x - padding * 2;
	const int total_width = GUI.default_font->CalculateSize(text).x;
	if(real_pos < 0 || real_pos >= real_size)
	{
		if(center)
			offset = caret_pos - real_size / 2;
		else if(real_pos < 0)
			offset = caret_pos;
		else
			offset = caret_pos - real_size;
	}

	int max_offset = total_width - real_size;
	if(offset > max_offset)
		offset = max_offset;
	if(offset < 0)
		offset = 0;
}

//=================================================================================================
void TextBox::SelectAll()
{
	SetFocus();
	if(text.empty())
	{
		caret_index = 0;
		caret_pos = 0;
		select_start_index = -1;
	}
	else
	{
		caret_index = text.size();
		caret_pos = IndexToPos(caret_index);
		select_start_index = 0;
		select_end_index = caret_index;
		select_start_pos = 0;
		select_end_pos = caret_pos;
		select_fixed_index = 0;
	}
}
