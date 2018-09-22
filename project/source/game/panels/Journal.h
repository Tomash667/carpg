#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

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
	void LoadLanguage();
	void LoadData();
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
	void AddRumor(cstring text);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	vector<string>& GetNotes() { return notes; }
	vector<string>& GetRumors() { return rumors; }

private:
	Game& game;
	Mode mode;
	TEX tBook, tPage[3], tArrowL, tArrowR;
	cstring txAdd, txNoteText, txNoQuests, txNoRumors, txNoNotes, txAddNote, txAddTime;
	int font_height, page, prev_page, open_quest, x, y, size_x, size_y, rect_w, rect_lines;
	Rect rect, rect2;
	vector<Text> texts;
	vector<string> notes;
	vector<string> rumors;
	vector<std::pair<Mode, int>> changes;
	string input;
	bool details;
};
