#include "Pch.h"
#include "Core.h"
#include "BookPanel.h"
#include "Item.h"
#include "KeyStates.h"
#include "ResourceManager.h"

BookPanel::BookPanel() : book(nullptr), scale(0, 0)
{

}

void BookPanel::Draw(ControlDrawData*)
{
	// book background
	Int2 book_size = Int2(Vec2(book->scheme->size) * scale);
	Int2 book_pos = (GUI.wnd_size - book_size) / 2;
	Rect r = Rect::Create(book_pos, book_size);
	GUI.DrawSpriteRect(book->scheme->tex->tex, r);

	// prev page
	if(current_page != 0)
	{
		//GUI.DrawSpriteRect(tArrowL, Rect::Create(
	}

	// next page
	if(current_page != max_page)
	{

	}

	// text
	auto font = GetFont();
}

void BookPanel::Event(GuiEvent event)
{
	if(event == GuiEvent_WindowResize || event == GuiEvent_Show)
		scale = Vec2((float)GUI.wnd_size.x / 1024, (float)GUI.wnd_size.y / 768);
}

void BookPanel::Update(float dt)
{
	if(!focus)
		return;

	// prev page
	if(current_page != 0)
	{

	}

	// next page
	if(current_page != max_page)
	{

	}

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

void BookPanel::Show(const Book* book)
{
	assert(book);
	if(this->book != book)
	{
		this->book = book;
		SplitBook();
	}
	visible = true;
	Event(GuiEvent_Show);
	GainFocus();
}

void BookPanel::SplitBook()
{
	auto font = GetFont();
	auto& regions = book->scheme->regions;

	splits.clear();
	font_lines.clear();

	uint text_end = book->text.length();
	cstring text = book->text.c_str();
	uint index = 0, line_begin, line_end = 0, region = 0, current_h = 0, page = 0, start_line = 0;
	int line_width;
	while(font->SplitLine(line_begin, line_end, line_width, index, text, text_end, 0, regions[region].Size().x))
	{
		font_lines.push_back({ line_begin, line_end, line_end - line_begin, (uint)line_width });
		current_h += font->height;
		if(current_h + font->height > regions[region].Size().y)
		{
			splits.push_back({ page, region, start_line, font_lines.size() - 1 });
			start_line = font_lines.size();
			++region;
			if(regions.size() == region)
			{
				region = 0;
				++page;
			}
		}
	}

	if(font_lines.empty())
	{
		font_lines.push_back({ 0, 0, 0, 0 });
		splits.push_back({ 0, 0, 0, 0 });
	}
	else
	{
		if(font_lines.back().end != text_end)
			font_lines.push_back({ text_end, text_end, 0, 0 });
		splits.push_back({ page, region, start_line, font_lines.size() });
	}

	max_page = splits.back().page;
	current_page = 0;
}

Font* BookPanel::GetFont()
{
	if(book->runic)
		return GUI.default_font;
	else
		return GUI.default_font;
}

void BookPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("strzalka_l.png", tArrowL);
	tex_mgr.AddLoadTask("strzalka_p.png", tArrowR);
}
