#pragma once

//-----------------------------------------------------------------------------
#include "GameDialog.h"

//-----------------------------------------------------------------------------
struct DialogChoice
{
	enum Type
	{
		Normal,
		Perk,
		Hero
	};

	int pos, quest_dialog_index;
	cstring msg, talk_msg;
	string* pooled; // used for dialog choices when message is formatted
	Type type;

	DialogChoice() {}
	DialogChoice(int pos, cstring msg, int quest_dialog_index, string* pooled = nullptr) : pos(pos), msg(msg),
		quest_dialog_index(quest_dialog_index), pooled(pooled), type(Normal), talk_msg(nullptr) {}
	explicit DialogChoice(string* pooled) : msg(pooled->c_str()), pooled(pooled), type(Normal) {}
};

//-----------------------------------------------------------------------------
struct DialogContext
{
	static constexpr int QUEST_INDEX_NONE = -1;
	static constexpr int QUEST_INDEX_RESTART = -2;

	enum Mode
	{
		NONE,
		IDLE,
		WAIT_CHOICES,
		WAIT_TALK,
		WAIT_TIMER,
		WAIT_INPUT
	};

	struct Entry
	{
		GameDialog* dialog;
		Quest* quest;
		int pos;
	};

	Mode mode;
	PlayerController* pc;
	Unit* talker; // postaæ z któr¹ siê rozmawia
	bool dialog_mode; // czy jest tryb dialogowy
	bool is_local;
	int dialog_pos; // pozycja w dialogu
	int choice_selected; // zaznaczona opcja dialogowa
	int dialog_esc; // opcja dialogowa wybierana po wciœniêciu ESC
	int skip_id; // u¿ywane w mp do pomijania dialogów
	int team_share_id;
	float timer;
	const Item* team_share_item;
	vector<DialogChoice> choices; // opcje dialogowe do wyboru
	cstring dialog_text; // tekst dialogu
	string dialog_s_text; // tekst dialogu zmiennego
	string predialog;

	static DialogContext* current;
	static int var;

	~DialogContext() { ClearChoices(); }
	void StartDialog(Unit* talker, GameDialog* dialog = nullptr, Quest* quest = nullptr);
	void StartNextDialog(GameDialog* dialog, Quest* quest = nullptr, bool returns = true);
	void Update(float dt);
	void UpdateClient();
	void EndDialog();
	void DialogTalk(cstring msg);
	void RemoveQuestDialog(Quest2* quest);
	cstring GetIdleText(Unit& talker);
	void Wait(float _timer)
	{
		mode = WAIT_TIMER;
		timer = _timer;
	}

private:
	void UpdateLoop();
	cstring GetText(int index, bool multi = false);
	bool ExecuteSpecial(cstring msg);
	bool ExecuteSpecialIf(cstring msg);
	cstring FormatString(const string& str_part);
	void ClearChoices();
	bool DoIfOp(int value1, int value2, DialogOp op);
	bool LearnPerk(Perk* perk);
	bool RecruitHero(Class* clas);

	GameDialog* dialog; // aktualny dialog
	Quest* dialog_quest; // quest zwi¹zany z dialogiem
	cstring last_rumor;
	cstring talk_msg;
	vector<Entry> prev;
	vector<QuestDialog> quest_dialogs;
	vector<News*> active_news;
	vector<pair<int, bool>> active_locations;
	int update_locations, // 1-update, 0-updated, -1-no locations
		quest_dialog_index;
	bool once, update_news, force_end;
};
