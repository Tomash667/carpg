#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
struct Game;

//-----------------------------------------------------------------------------
class Journal : public GamePanel
{
public:
	struct Text
	{
		cstring text;
		int x, y, color;
	};

	enum Mode
	{
		Quests,
		Rumors,
		Notes,
		Invalid
	};

	Journal();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Reset();
	void Show();
	void Hide();
	void Build();
	void AddEntry(cstring text, int color, bool singleline);
	void OnAddNote(int id);
	void NeedUpdate(Mode mode, int quest_index = 0);

	static TEX tBook, tPage[3], tArrowL, tArrowR;
	Mode mode;
	int page, prev_page, open_quest, x, y, size_x, size_y, rect_w, rect_lines;
	RECT rect, rect2;
	vector<Text> texts;
	string input;
	cstring txAdd, txNoteText, txNoQuests, txNoRumors, txNoNotes, txAddNote, txAddTime;
	bool details;

private:
	Game& game;
	int font_height;
};
