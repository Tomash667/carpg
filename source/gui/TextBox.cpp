#include "Pch.h"
#include "Base.h"
#include "TextBox.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
TEX TextBox::tBox;

//=================================================================================================
TextBox::TextBox() : added(false), multiline(false), numeric(false), label(NULL), scrollbar(NULL), readonly(false)
{

}

//=================================================================================================
TextBox::~TextBox()
{
	delete scrollbar;
}

//=================================================================================================
void TextBox::Draw(ControlDrawData* cdd/* =NULL */)
{
	GUI.DrawItem(tBox, global_pos, size, WHITE, 4, 32);
	
	RECT r = {global_pos.x+4, global_pos.y+4, global_pos.x+size.x-4, global_pos.y+size.y-4};
	
	if(!scrollbar)		
		GUI.DrawText(GUI.default_font, kursor_mig >= 0.f ? Format("%s|", text.c_str()) : text, multiline ? DT_TOP : DT_VCENTER, BLACK, r);
	else
	{
		RECT r2 = {r.left, r.top - int(scrollbar->offset), r.right, r.bottom - int(scrollbar->offset)};
		GUI.DrawText(GUI.default_font, kursor_mig >= 0.f ? Format("%s|", text.c_str()) : text, DT_TOP, BLACK, r2, &r);
		scrollbar->Draw();
	}

	if(label)
	{
		r.top -= 20;
		GUI.DrawText(GUI.default_font, label, DT_NOCLIP, BLACK, r);
	}
}

//=================================================================================================
void TextBox::Update(float dt)
{
	if(mouse_focus)
	{
		if(IsInside(GUI.cursor_pos) && !readonly)
			GUI.cursor_mode = CURSOR_TEXT;
		if(scrollbar)
		{
			///if(Key.Focus() && PointInRect(GUI.cursor_pos, global_pos, size+INT2(8,0)))
			{
				//if(Key.)
			}
			scrollbar->Update(dt);
		}
	}
	if(focus)
	{
		kursor_mig += dt*2;
		if(kursor_mig >= 1.f)
			kursor_mig = -1.f;
	}
	else
		kursor_mig = -1.f;
}

//=================================================================================================
void TextBox::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		if(!added)
		{
			kursor_mig = 0.f;
			GUI.AddOnCharHandler(this);
			added = true;
		}
	}
	else if(e == GuiEvent_LostFocus)
	{
		if(added)
		{
			kursor_mig = -1.f;
			GUI.RemoveOnCharHandler(this);
			added = false;
		}
	}
}

//=================================================================================================
void TextBox::OnChar(char c)
{
	if(c == 0x08)
	{
		// backspace
		if(!text.empty())
			text.resize(text.size()-1);
	}
	else if(c == 0x0D && !multiline)
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
	scrollbar->pos = INT2(size.x+2,0);
	scrollbar->size = INT2(16,size.y);
	scrollbar->total = 8;
	scrollbar->part = size.y-8;
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
	INT2 str_size = GUI.default_font->CalculateSize(str, size.x-8);
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
		scrollbar->part = size.y-8;
		scrollbar->offset = 0.f;
	}
}

//=================================================================================================
void TextBox::UpdateScrollbar()
{
	INT2 text_size = GUI.default_font->CalculateSize(text, size.x-8);
	scrollbar->total = text_size.y;
}
