#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
struct Book;

//-----------------------------------------------------------------------------
class BookPanel : public GamePanel
{
public:
	BookPanel();

	void Draw(ControlDrawData*) override;
	void Event(GuiEvent event) override;
	void Update(float dt) override;

	void Show(const Book* book);
	void LoadData();

private:
	struct Split
	{
		uint page, region, lines_start, lines_end;
	};

	void SplitBook();
	Font* GetFont();
	void ChangePage(int change);

	const Book* book;
	vector<TextLine> text_lines;
	vector<Split> splits;
	uint max_page, current_page;
	Vec2 scale;
	TEX tArrowL, tArrowR;
	SOUND sound;
	Font* runic_font;
};
