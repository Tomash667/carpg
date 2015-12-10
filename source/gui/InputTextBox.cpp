#include "Pch.h"
#include "Base.h"
#include "InputTextBox.h"
#include "TextBox.h"
#include "KeyStates.h"

//=================================================================================================
InputTextBox::InputTextBox() : added(false), background(nullptr)
{

}

//=================================================================================================
void InputTextBox::Draw(ControlDrawData*)
{
	// t³o
	if(background)
	{
		RECT r0 = {global_pos.x, global_pos.y, global_pos.x+textbox_size.x, global_pos.y+textbox_size.y};
		GUI.DrawSpriteRect(*background, r0, background_color);

		r0.top = inputbox_pos.y;
		r0.bottom = r0.top + inputbox_size.y;
		GUI.DrawSpriteRect(*background, r0, background_color);
	}

	// box na tekst
	GUI.DrawItem(TextBox::tBox, global_pos, textbox_size, WHITE, 8, 32);

	// box na input
	GUI.DrawItem(TextBox::tBox, inputbox_pos, inputbox_size, WHITE, 8, 32);

	// tekst
	RECT rc = {global_pos.x+4, global_pos.y+4, global_pos.x+textbox_size.x-4, global_pos.y+textbox_size.y-4};
	RECT r = {rc.left, rc.top-int(scrollbar.offset), rc.right, rc.bottom-int(scrollbar.offset)-20};
	GUI.DrawText(GUI.default_font, text, 0, BLACK, r, &rc, nullptr, nullptr, &lines);

	// input
	RECT r2 = {inputbox_pos.x+4, inputbox_pos.y, inputbox_pos.x+inputbox_size.x-4, inputbox_pos.y+inputbox_size.y};
	GUI.DrawText(GUI.default_font, kursor_mig >= 0.f ? Format("%s|", input.c_str()) : input, DT_LEFT|DT_VCENTER, BLACK, r2, &r2);

	// scrollbar
	scrollbar.Draw();
}

//=================================================================================================
void InputTextBox::Update(float dt)
{
	if(mouse_focus)
	{
		if(Key.Focus() && IsInside(GUI.cursor_pos))
			scrollbar.ApplyMouseWheel();
		if(PointInRect(GUI.cursor_pos, inputbox_pos, inputbox_size))
		{
			GUI.cursor_mode = CURSOR_TEXT;
			if(!focus && Key.Focus() && Key.PressedRelease(VK_LBUTTON))
				focus = true;
		}
		else if(focus && Key.Focus() && Key.PressedRelease(VK_LBUTTON))
			focus = false;
		scrollbar.Update(dt);
	}
	if(focus)
	{
		kursor_mig += dt*2;
		if(kursor_mig >= 1.f)
			kursor_mig = -1.f;
		if(Key.Focus())
		{
			if(Key.PressedRelease(VK_UP))
			{
				// poprzednia komenda
				if(input_counter == -1)
				{
					input_counter = last_input_counter-1;
					if(input_counter != -1)
						input = cache[input_counter];
				}
				else
				{
					--input_counter;
					if(input_counter == -1)
						input_counter = last_input_counter-1;
					input = cache[input_counter];
				}
			}
			else if(Key.PressedRelease(VK_DOWN))
			{
				// nastêpna komenda
				++input_counter;
				if(input_counter == last_input_counter)
				{
					if(last_input_counter == 0)
						input_counter = -1;
					else
						input_counter = 0;
				}
				if(input_counter != -1)
					input = cache[input_counter];
			}
			if(Key.PressedRelease(VK_RETURN))
			{
				if(!input.empty())
				{
					// dodaj ostatni¹ komendê
					if(last_input_counter == 0 || cache[last_input_counter-1] != input)
					{
						if(last_input_counter == max_cache)
						{
							for(int i=0; i<max_cache-1; ++i)
								cache[i] = cache[i+1];
							cache[max_cache-1] = input;
						}
						else
						{
							cache[last_input_counter] = input;
							++last_input_counter;
						}
					}
					// wykonaj
					event(input);
					// wyczyœæ
					input.clear();
					input_counter = -1;
				}
				if(lose_focus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
			else if(esc_clear && Key.PressedRelease(VK_ESCAPE))
			{
				input.clear();
				input_counter = -1;
				if(lose_focus)
				{
					focus = false;
					Event(GuiEvent_LostFocus);
				}
			}
		}
	}
	else
		kursor_mig = -1.f;
}

//=================================================================================================
void InputTextBox::Event(GuiEvent e)
{
	if(e == GuiEvent_LostFocus)
	{
		scrollbar.LostFocus();
		if(added)
		{
			kursor_mig = -1.f;
			added = false;
			GUI.RemoveOnCharHandler(this);
		}
	}
	else if(e == GuiEvent_Moved)
	{
		global_pos = pos + parent->global_pos;
		inputbox_pos = global_pos + INT2(0,textbox_size.y+6);
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_Resize)
	{
		global_pos = parent->global_pos;
		size = parent->size;
		textbox_size = size - INT2(18,30);
		inputbox_pos = global_pos + INT2(0,textbox_size.y+6);
		inputbox_size = INT2(textbox_size.x,24);
		scrollbar.pos = INT2(textbox_size.x+2,0);
		scrollbar.global_pos = global_pos + scrollbar.pos;
		scrollbar.size = INT2(16,textbox_size.y);
		scrollbar.part = textbox_size.y-8;

		size_t OutBegin, OutEnd, InOutIndex = 0;
		int OutWidth, Width = textbox_size.x-8;
		cstring Text = text.c_str();
		size_t TextEnd = text.length();

		bool skip_to_end = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

		// podziel tekst na linijki
		lines.clear();
		while(GUI.default_font->SplitLine(OutBegin, OutEnd, OutWidth, InOutIndex, Text, TextEnd, 0, Width))
			lines.push_back(TextLine(OutBegin, OutEnd, OutWidth));

		CheckLines();

		scrollbar.total = lines.size()*GUI.default_font->height;
		if(skip_to_end)
		{
			scrollbar.offset = float(scrollbar.total-scrollbar.part);
			if(scrollbar.offset+scrollbar.part > scrollbar.total)
				scrollbar.offset = float(scrollbar.total-scrollbar.part);
			if(scrollbar.offset < 0)
				scrollbar.offset = 0;
		}
	}
	else if(e == GuiEvent_GainFocus)
	{
		if(!added)
		{
			kursor_mig = 0.f;
			added = true;
			GUI.AddOnCharHandler(this);
		}
	}
}

//=================================================================================================
void InputTextBox::OnChar(char c)
{
	if(c == 0x08)
	{
		// backspace
		if(!input.empty())
			input.resize(input.size()-1);
	}
	else if(c == 0x0D)
	{
		// pomiñ znak
	}
	else
		input.push_back(c);
}

//=================================================================================================
void InputTextBox::Init()
{
	textbox_size = size - INT2(18,30);
	inputbox_pos = global_pos + INT2(0,textbox_size.y+6);
	inputbox_size = INT2(textbox_size.x,24);
	scrollbar.pos = INT2(textbox_size.x+2,0);
	scrollbar.size = INT2(16,textbox_size.y);
	scrollbar.total = 0;
	scrollbar.part = textbox_size.y-8;
	scrollbar.offset = 0.f;
	scrollbar.global_pos = global_pos + scrollbar.pos;
	cache.resize(max_cache);
	Reset();
}

//=================================================================================================
void InputTextBox::Reset()
{
	input.clear();
	text.clear();
	lines.clear();
	input_counter = -1;
	last_input_counter = 0;
	kursor_mig = 0.f;
	scrollbar.offset = 0.f;
	scrollbar.total = 0;
}

//=================================================================================================
void InputTextBox::Add(StringOrCstring str)
{
	if(!text.empty())
		text += '\n';
	size_t InOutIndex = text.length();
	str.AddTo(text);

	size_t OutBegin, OutEnd;
	int OutWidth, Width = textbox_size.x-8;
	cstring Text = text.c_str();
	size_t TextEnd = text.length();

	bool skip_to_end = (int(scrollbar.offset) >= (scrollbar.total - scrollbar.part));

	// podziel tekst na linijki
	while(GUI.default_font->SplitLine(OutBegin, OutEnd, OutWidth, InOutIndex, Text, TextEnd, 0, Width))
		lines.push_back(TextLine(OutBegin, OutEnd, OutWidth));

	// usuñ nadmiarowe linijki z pocz¹tku
	CheckLines();

	scrollbar.total = lines.size()*GUI.default_font->height;
	if(skip_to_end)
	{
		scrollbar.offset = float(scrollbar.total-scrollbar.part);
		if(scrollbar.offset+scrollbar.part > scrollbar.total)
			scrollbar.offset = float(scrollbar.total-scrollbar.part);
		if(scrollbar.offset < 0)
			scrollbar.offset = 0;
	}
}

//=================================================================================================
void InputTextBox::CheckLines()
{
	if((int)lines.size() > max_lines)
	{
		lines.erase(lines.begin(), lines.begin()+lines.size()-max_lines);
		int offset = lines[0].begin;
		text.erase(0, offset);
		for(vector<TextLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
		{
			it->begin -= offset;
			it->end -= offset;
		}
	}
}
