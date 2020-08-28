#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"

//-----------------------------------------------------------------------------
// Book with all quest, rumors & notes
class Journal : public GamePanel
{
public:
	struct Text
	{
		cstring text;
		string* pooled;
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
	~Journal();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Reset();
	void Show();
	void Hide();
	void AddRumor(cstring text);
	void NeedUpdate(Mode mode, int quest_index = 0);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	vector<string>& GetNotes() { return notes; }
	vector<string>& GetRumors() { return rumors; }

private:
	void Build();
	void AddEntry(cstring text, int color, bool singleline, bool pooled = false);
	void OnAddNote(int id);

	Mode mode;
	TexturePtr tBook, tPage[3], tArrowL, tArrowR;
	cstring txAdd, txNoteText, txNoQuests, txNoRumors, txNoNotes, txAddNote, txAddTime;
	int font_height, page, prev_page, open_quest, x, y, size_x, size_y, rect_w, rect_lines;
	Rect rect, rect2;
	vector<Text> texts;
	vector<string> notes;
	vector<string> rumors;
	vector<pair<Mode, int>> changes;
	string input_str;
	bool details;
};
