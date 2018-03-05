#include "Pch.h"
#include "GameCore.h"
#include "BookPanel.h"
#include "Item.h"
#include "KeyStates.h"
#include "ResourceManager.h"
#include "Game.h"

//=================================================================================================
BookPanel::BookPanel() : book(nullptr), scale(0, 0)
{
	visible = false;
}

//=================================================================================================
void BookPanel::Draw(ControlDrawData*)
{
	if(!book)
		return;

	// book background
	Int2 book_size = book->scheme->size * scale;
	Int2 book_pos = (GUI.wnd_size - book_size) / 2;
	Rect r = Rect::Create(book_pos, book_size);
	GUI.DrawSpriteRect(book->scheme->tex->tex, r);

	// prev page
	if(current_page != 0)
	{
		D3DSURFACE_DESC desc;
		tArrowL->GetLevelDesc(0, &desc);
		GUI.DrawSpriteRect(tArrowL, Rect::Create(book_pos + book->scheme->prev * scale, Int2(desc.Width, desc.Height) * scale));
	}

	// next page
	if(current_page != max_page)
	{
		D3DSURFACE_DESC desc;
		tArrowR->GetLevelDesc(0, &desc);
		GUI.DrawSpriteRect(tArrowR, Rect::Create(book_pos + book->scheme->next * scale, Int2(desc.Width, desc.Height) * scale));
	}

	// text
	IGUI::DrawTextOptions options(GetFont(), book->text);
	options.lines = &text_lines;
	options.scale = scale;
	for(auto& split : splits)
	{
		if(split.page < current_page)
			continue;
		if(split.page > current_page)
			break;
		options.rect = book->scheme->regions[split.region] * scale + book_pos;
		options.lines_start = split.lines_start;
		options.lines_end = split.lines_end;
		GUI.DrawText2(options);
	}

#ifdef _DEBUG
	if(Key.Down('B'))
	{
		for(auto& rect : book->scheme->regions)
		{
			GUI.DrawArea(COLOR_RGBA(255, 0, 0, 128), rect * scale + book_pos);
		}
	}
#endif
}

//=================================================================================================
void BookPanel::Event(GuiEvent event)
{
	if(event == GuiEvent_WindowResize || event == GuiEvent_Show)
		scale = Vec2((float)GUI.wnd_size.x / 1024, (float)GUI.wnd_size.y / 768);
}

//=================================================================================================
void BookPanel::Update(float dt)
{
	Int2 book_size = book->scheme->size * scale;
	Int2 book_pos = (GUI.wnd_size - book_size) / 2;

	// prev page
	if(current_page != 0)
	{
		D3DSURFACE_DESC desc;
		tArrowL->GetLevelDesc(0, &desc);
		if(Rect::Create(book_pos + book->scheme->prev * scale, Int2(desc.Width, desc.Height) * scale).IsInside(GUI.cursor_pos)
			&& Key.PressedRelease(VK_LBUTTON))
			ChangePage(-1);
	}

	// next page
	if(current_page != max_page)
	{
		D3DSURFACE_DESC desc;
		tArrowR->GetLevelDesc(0, &desc);
		if(Rect::Create(book_pos + book->scheme->next * scale, Int2(desc.Width, desc.Height) * scale).IsInside(GUI.cursor_pos)
			&& Key.PressedRelease(VK_LBUTTON))
			ChangePage(+1);
	}

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void BookPanel::Show(const Book* book)
{
	assert(book);
	if(this->book != book)
	{
		this->book = book;
		SplitBook();
	}
	visible = true;
	current_page = 0;
	Event(GuiEvent_Show);
	GainFocus();

	auto& game = Game::Get();
	if(game.sound_volume)
		game.PlaySound2d(sound);
}

//=================================================================================================
void BookPanel::SplitBook()
{
	auto font = GetFont();
	auto& regions = book->scheme->regions;

	splits.clear();
	text_lines.clear();

	uint text_end = book->text.length();
	cstring text = book->text.c_str();
	uint index = 0, line_begin, line_end = 0, region = 0, current_h = 0, page = 0, start_line = 0;
	int line_width;
	while(font->SplitLine(line_begin, line_end, line_width, index, text, text_end, 0, regions[region].Size().x))
	{
		text_lines.push_back({ line_begin, line_end, line_width });
		current_h += font->height;
		if(current_h + font->height > (uint)regions[region].Size().y)
		{
			current_h = 0;
			splits.push_back({ page, region, start_line, text_lines.size() });
			start_line = text_lines.size();
			++region;
			if(regions.size() == region)
			{
				region = 0;
				++page;
			}
		}
	}

	if(text_lines.empty())
	{
		text_lines.push_back({ 0, 0, 0 });
		splits.push_back({ 0, 0, 0, 0 });
	}
	else
	{
		if(text_lines.back().end != text_end)
			text_lines.push_back({ text_end, text_end, 0 });
		splits.push_back({ page, region, start_line, text_lines.size() });
	}

	max_page = splits.back().page;
	current_page = 0;
}

//=================================================================================================
Font* BookPanel::GetFont()
{
	if(book->runic)
		return runic_font;
	else
		return normal_font;
}

//=================================================================================================
void BookPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("strzalka_l.png", tArrowL);
	tex_mgr.AddLoadTask("strzalka_p.png", tArrowR);

	ResourceManager::Get<Sound>().AddLoadTask("page-turn.wav", sound);

	GUI.AddFont("Dwarf Runes.ttf");
	normal_font = GUI.CreateFont("Arial", 16, 800, 512);
	runic_font = GUI.CreateFont("Dwarf Runes", 16, 800, 512);
}

//=================================================================================================
void BookPanel::ChangePage(int change)
{
	current_page += change;
	auto& game = Game::Get();
	if(game.sound_volume)
		game.PlaySound2d(sound);
}
