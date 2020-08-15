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

	DialogChoice(int pos, cstring msg, int quest_dialog_index, string* pooled = nullptr) : pos(pos), msg(msg),
		quest_dialog_index(quest_dialog_index), pooled(pooled), type(Normal), talk_msg(nullptr) {}
};

//-----------------------------------------------------------------------------
struct DialogContext
{
	static constexpr int QUEST_INDEX_NONE = -1;
	static constexpr int QUEST_INDEX_RESTART = -2;

	struct Entry
	{
		GameDialog* dialog;
		Quest* quest;
		int pos;
	};

	bool dialog_mode; // czy jest tryb dialogowy
	bool show_choices; // czy wyœwietlono opcje dialogowe do wyboru
	vector<DialogChoice> choices; // opcje dialogowe do wyboru
	int dialog_pos; // pozycja w dialogu
	int choice_selected; // zaznaczona opcja dialogowa
	int dialog_esc; // opcja dialogowa wybierana po wciœniêciu ESC
	int dialog_skip; // pomijanie opcji dialogowych u¿ywane przez DTF_RANDOM_TEXT
	cstring dialog_text; // tekst dialogu
	string dialog_s_text; // tekst dialogu zmiennego
	Quest* dialog_quest; // quest zwi¹zany z dialogiem
	GameDialog* dialog; // aktualny dialog
	Unit* talker; // postaæ z któr¹ siê rozmawia
	float dialog_wait; // czas wyœwietlania opcji dialogowej
	cstring last_rumor;
	bool is_local;
	PlayerController* pc;
	int skip_id; // u¿ywane w mp do pomijania dialogów
	bool update_news;
	int update_locations; // 1-update, 0-updated, -1-no locations
	vector<News*> active_news;
	vector<pair<int, bool>> active_locations;
	int team_share_id;
	const Item* team_share_item;
	bool can_skip, force_end;
	vector<Entry> prev;
	cstring talk_msg;
	vector<QuestDialog> quest_dialogs;
	int quest_dialog_index;

	static DialogContext* current;
	static int var;

	~DialogContext() { ClearChoices(); }
	void StartDialog(Unit* talker, GameDialog* dialog = nullptr, Quest* quest = nullptr);
	void StartNextDialog(GameDialog* dialog, Quest* quest = nullptr, bool returns = true);
	void Update(float dt);
	void EndDialog();
	void ClearChoices();
	cstring GetText(int index, bool multi = false);
	bool ExecuteSpecial(cstring msg);
	bool ExecuteSpecialIf(cstring msg);
	cstring FormatString(const string& str_part);
	void DialogTalk(cstring msg);
	void RemoveQuestDialog(Quest2* quest);
	cstring GetIdleText(Unit& talker);
private:
	void UpdateLoop();
	bool DoIfOp(int value1, int value2, DialogOp op);
	bool LearnPerk(Perk* perk);
	bool RecruitHero(Class* clas);

	bool once;
	bool idleMode;
};
