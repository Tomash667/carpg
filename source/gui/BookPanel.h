#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
// Show book text for player to read
class BookPanel : public GamePanel
{
public:
	BookPanel();
	void LoadData();
	void Draw() override;
	void Event(GuiEvent event) override;
	void Update(float dt) override;
	void Show(const Book* book);

private:
	struct Split
	{
		uint page, region, linesStart, linesEnd;
	};

	void SplitBook();
	Font* GetFont();
	void ChangePage(int change);

	const Book* book;
	vector<TextLine> textLines;
	vector<Split> splits;
	uint maxPage, currentPage;
	Vec2 scale;
	TexturePtr tArrowL, tArrowR;
	Sound* sound;
	Font* fontNormal, *fontRunic;
};
