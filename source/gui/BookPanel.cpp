#include "Pch.h"
#include "BookPanel.h"

#include "Item.h"

#include <Input.h>
#include <ResourceManager.h>
#include <SoundManager.h>

//=================================================================================================
BookPanel::BookPanel() : book(nullptr), scale(0, 0)
{
	visible = false;
}

//=================================================================================================
void BookPanel::LoadData()
{
	tArrowL = resMgr->Load<Texture>("page_prev.png");
	tArrowR = resMgr->Load<Texture>("page_next.png");

	sound = resMgr->Load<Sound>("page-turn.wav");

	gui->AddFont("Dwarf Runes.ttf");
	fontNormal = gui->GetFont("Arial", 16, 8);
	fontRunic = gui->GetFont("Dwarf Runes", 16, 8);
}

//=================================================================================================
void BookPanel::Draw()
{
	if(!book)
		return;

	// book background
	Int2 bookSize = book->scheme->size * scale;
	Int2 bookPos = (gui->wndSize - bookSize) / 2;
	Rect r = Rect::Create(bookPos, bookSize);
	gui->DrawSpriteRect(book->scheme->tex, r);

	// regions
	if(IsDebug() && input->Down(Key::B))
	{
		for(Rect& rect : book->scheme->regions)
			gui->DrawRect(Color::Red, rect * scale + bookPos);
	}

	// prev page
	if(currentPage != 0)
	{
		Int2 arrowSize = tArrowL->GetSize();
		gui->DrawSpriteRect(tArrowL, Rect::Create(bookPos + book->scheme->prev * scale, arrowSize * scale));
	}

	// next page
	if(currentPage != maxPage)
	{
		Int2 arrowSize = tArrowR->GetSize();
		gui->DrawSpriteRect(tArrowR, Rect::Create(bookPos + book->scheme->next * scale, arrowSize * scale));
	}

	// text
	Gui::DrawTextOptions options(GetFont(), book->text);
	options.lines = &textLines;
	options.scale = scale;
	for(auto& split : splits)
	{
		if(split.page < currentPage)
			continue;
		if(split.page > currentPage)
			break;
		options.rect = book->scheme->regions[split.region] * scale + bookPos;
		options.linesStart = split.linesStart;
		options.linesEnd = split.linesEnd;
		gui->DrawText2(options);
	}
}

//=================================================================================================
void BookPanel::Event(GuiEvent event)
{
	if(event == GuiEvent_WindowResize || event == GuiEvent_Show)
		scale = Vec2((float)gui->wndSize.x / 1024, (float)gui->wndSize.y / 768);
}

//=================================================================================================
void BookPanel::Update(float dt)
{
	Int2 bookSize = book->scheme->size * scale;
	Int2 bookPos = (gui->wndSize - bookSize) / 2;

	// prev page
	if(currentPage != 0)
	{
		Int2 arrowSize = tArrowL->GetSize();
		if(Rect::Create(bookPos + book->scheme->prev * scale, arrowSize * scale).IsInside(gui->cursorPos)
			&& input->PressedRelease(Key::LeftButton))
			ChangePage(-1);
	}

	// next page
	if(currentPage != maxPage)
	{
		Int2 arrowSize = tArrowR->GetSize();
		if(Rect::Create(bookPos + book->scheme->next * scale, arrowSize * scale).IsInside(gui->cursorPos)
			&& input->PressedRelease(Key::LeftButton))
			ChangePage(+1);
	}

	if(input->Focus() && input->PressedRelease(Key::Escape))
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
	currentPage = 0;
	Event(GuiEvent_Show);
	GainFocus();

	soundMgr->PlaySound2d(sound);
}

//=================================================================================================
void BookPanel::SplitBook()
{
	auto font = GetFont();
	auto& regions = book->scheme->regions;

	splits.clear();
	textLines.clear();

	uint textEnd = book->text.length();
	cstring text = book->text.c_str();
	uint index = 0, lineBegin, lineEnd = 0, region = 0, currentH = 0, page = 0, startLine = 0;
	int lineWidth;
	while(font->SplitLine(lineBegin, lineEnd, lineWidth, index, text, textEnd, 0, regions[region].Size().x))
	{
		textLines.push_back({ lineBegin, lineEnd, lineWidth });
		currentH += font->height;
		if(currentH + font->height > (uint)regions[region].Size().y)
		{
			currentH = 0;
			splits.push_back({ page, region, startLine, textLines.size() });
			startLine = textLines.size();
			++region;
			if(regions.size() == region)
			{
				region = 0;
				++page;
			}
		}
	}

	if(textLines.empty())
	{
		textLines.push_back({ 0, 0, 0 });
		splits.push_back({ 0, 0, 0, 0 });
	}
	else
	{
		if(textLines.back().end != textEnd)
			textLines.push_back({ textEnd, textEnd, 0 });
		splits.push_back({ page, region, startLine, textLines.size() });
	}

	maxPage = splits.back().page;
	currentPage = 0;
}

//=================================================================================================
Font* BookPanel::GetFont()
{
	if(book->runic)
		return fontRunic;
	else
		return fontNormal;
}

//=================================================================================================
void BookPanel::ChangePage(int change)
{
	currentPage += change;
	soundMgr->PlaySound2d(sound);
}
