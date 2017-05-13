#include "Pch.h"
#include "Base.h"
#include "Label.h"

using namespace gui;

Label::Label(cstring text, bool auto_size) : text(text), custom_layout(nullptr), own_custom_layout(false), auto_size(auto_size)
{
	if(auto_size)
		CalculateSize();
}

Label::~Label()
{
	if(own_custom_layout)
		delete custom_layout;
}

void Label::Draw(ControlDrawData*)
{
	auto& l = GetLayout();
	RECT rect = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	GUI.DrawText(l.font, text, l.align, l.color, rect);
}

void Label::SetAlign(DWORD align)
{
	if(align == GetAlign())
		return;
	EnsureLayout()->align = align;
}

void Label::SetColor(DWORD color)
{
	if(color == GetColor())
		return;
	EnsureLayout()->color = color;
}

void Label::SetCustomLayout(LabelLayout* layout)
{
	assert(layout);

	if(own_custom_layout)
	{
		delete custom_layout;
		own_custom_layout = false;
	}

	custom_layout = layout;
	CalculateSize();
}

void Label::SetFont(Font* font)
{
	assert(font);

	if(font == GetFont())
		return;
	EnsureLayout()->font = font;
	CalculateSize();
}

void Label::SetPadding(const INT2& padding)
{
	if(padding == GetPadding())
		return;
	EnsureLayout()->padding = padding;
	CalculateSize();
}

void Label::SetText(const AnyString& s)
{
	text = s.s;
	CalculateSize();
}

void Label::SetSize(const INT2& _size)
{
	assert(!auto_size);
	size = _size;
}

void Label::CalculateSize()
{
	if(!auto_size)
		return;
	auto& l = GetLayout();
	size = l.font->CalculateSize(text) + l.padding * 2;
}

LabelLayout* Label::EnsureLayout()
{
	if(!own_custom_layout)
	{
		custom_layout = new LabelLayout(GetLayout());
		own_custom_layout = true;
	}
	return custom_layout;
}
