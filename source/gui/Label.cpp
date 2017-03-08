#include "Pch.h"
#include "Base.h"
#include "Label.h"

using namespace gui;

//Label::Label(cstring text, bool auto_size) : text(text), override(nullptr), auto_size(auto_size)
//{
//	if(auto_size)
//		CalculateSize();
//}

void Label::Draw(ControlDrawData*)
{
	//auto& l = GetLayout();
	//GUI.DrawText(l.font, text, DT_LEFT, l.color, )
	//GetLayout
}

void Label::Event(GuiEvent e)
{

}

void Label::SetLayoutOverride(Layout::Label* _override)
{
	override = _override;
}

void Label::SetText(const AnyString& s)
{
	text = s.s;
	if(auto_size)
		CalculateSize();
}

void Label::SetSize(const INT2& _size)
{
	assert(!auto_size);
	size = _size;
}

void Label::CalculatePos()
{

}

void Label::CalculateSize()
{
	auto& l = GetLayout();
	size = l.font->CalculateSize(text) + l.padding * 2;
	CalculatePos();
}
