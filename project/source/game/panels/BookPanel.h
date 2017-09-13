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
		uint page, region, line_start, line_end;
	};

	void SplitBook();
	Font* GetFont();

	const Book* book;
	vector<FontLine> font_lines;
	vector<Split> splits;
	uint max_page, current_page;
	Vec2 scale;
	TEX tArrowL, tArrowR;
};
