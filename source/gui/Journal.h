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
		int x, y, h, color;
	};

	enum Mode
	{
		Invalid = -1,
		Quests,
		Rumors,
		Notes,
		Investments,
		Max
	};

	Journal();
	~Journal();
	void LoadLanguage();
	void LoadData();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Reset();
	void Show();
	void Hide();
	void AddRumor(cstring text);
	void NeedUpdate(Mode mode, int questIndex = 0);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	vector<string>& GetNotes() { return notes; }
	vector<string>& GetRumors() { return rumors; }

private:
	void Build();
	void AddEntry(cstring str, int color = 0, bool spacing = true, bool pooled = false);
	void OnAddNote(int id);

	Mode mode;
	TexturePtr tBook, tButtonOn, tButtonHover, tButtonOff, tPage[3], tArrowL, tArrowR, tIcons[4];
	cstring txAdd, txNoteText, txNoQuests, txNoRumors, txNoNotes, txAddNote, txAddTime, txGoldMonth, txTotal, txNoInvestments;
	int fontHeight, page, prevPage, openQuest, x, y, rectW, rectLines, hover;
	Rect rect, rect2;
	vector<Text> texts;
	vector<string> notes;
	vector<string> rumors;
	vector<pair<Mode, int>> changes;
	string inputStr;
	bool details;
};
