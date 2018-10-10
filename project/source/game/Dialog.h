#pragma once

//-----------------------------------------------------------------------------
enum DialogType
{
	DTF_CHOICE,
	DTF_TRADE,
	DTF_END_CHOICE,
	DTF_TALK,
	DTF_RESTART,
	DTF_END,
	DTF_END2,
	DTF_SHOW_CHOICES,
	DTF_SPECIAL,
	DTF_SET_QUEST_PROGRESS,
	DTF_IF_QUEST_TIMEOUT,
	DTF_END_IF,
	DTF_IF_RAND,
	DTF_ELSE,
	DTF_CHECK_QUEST_TIMEOUT,
	DTF_IF_HAVE_QUEST_ITEM,
	DTF_DO_QUEST,
	DTF_DO_QUEST_ITEM,
	DTF_IF_QUEST_PROGRESS,
	DTF_IF_NEED_TALK,
	DTF_ESCAPE_CHOICE,
	DTF_IF_SPECIAL,
	DTF_IF_ONCE,
	DTF_IF_CHOICES,
	DTF_DO_QUEST2,
	DTF_IF_HAVE_ITEM,
	DTF_IF_QUEST_PROGRESS_RANGE,
	DTF_IF_QUEST_EVENT,
	DTF_END_OF_DIALOG,
	DTF_DO_ONCE,
	DTF_NOT_ACTIVE,
	DTF_IF_QUEST_SPECIAL,
	DTF_QUEST_SPECIAL,
	DTF_NOT,
	DTF_SCRIPT,
	DTF_IF_SCRIPT
};

//-----------------------------------------------------------------------------
struct DialogEntry
{
	DialogType type;
	int value;
};

//-----------------------------------------------------------------------------
struct DialogChoice
{
	int pos, lvl;
	cstring msg;

	DialogChoice(int _pos, cstring _msg, int _lvl) : pos(_pos), msg(_msg), lvl(_lvl) {}
};

//-----------------------------------------------------------------------------
struct GameDialog
{
	struct Text
	{
		int index, next, script;
		bool exists, formatted;

		Text() : index(-1), next(-1), script(-1), exists(false), formatted(false) {}
		Text(int index) : index(index), next(-1), script(-1), exists(true), formatted(false) {}
	};

	struct Script
	{
		enum Type
		{
			NORMAL,
			IF,
			STRING
		};
		int index, next;
		Type type;

		Script(int index, Type type) : index(index), next(-1), type(type) {}
	};

	string id;
	vector<DialogEntry> code;
	vector<string> strs;
	vector<Text> texts;
	vector<Script> scripts;
	int max_index;
};

//-----------------------------------------------------------------------------
struct DialogContext
{
	struct Entry
	{
		GameDialog* dialog;
		Quest* quest;
		int pos, level;
	};

	bool dialog_mode; // czy jest tryb dialogowy
	bool show_choices; // czy wyœwietlono opcje dialogowe do wyboru
	vector<DialogChoice> choices; // opcje dialogowe do wyboru
	int dialog_pos; // pozycja w dialogu
	int choice_selected; // zaznaczona opcja dialogowa
	int dialog_level; // poziom zagnie¿d¿enia w dialogu
	int dialog_esc; // opcja dialogowa wybierana po wciœniêciu ESC
	int dialog_skip; // pomijanie opcji dialogowych u¿ywane przez DTF_RANDOM_TEXT
	cstring dialog_text; // tekst dialogu
	string dialog_s_text; // tekst dialogu zmiennego
	Quest* dialog_quest; // quest zwi¹zany z dialogiem
	GameDialog* dialog; // aktualny dialog
	Unit* talker; // postaæ z któr¹ siê rozmawia
	float dialog_wait; // czas wyœwietlania opcji dialogowej
	bool dialog_once; // wyœwietlanie opcji dialogowej tylko raz
	cstring last_rumor;
	bool is_local;
	PlayerController* pc;
	int skip_id; // u¿ywane w mp do pomijania dialogów
	bool update_news;
	int update_locations; // 1-update, 0-updated, -1-no locations
	vector<News*> active_news;
	vector<std::pair<int, bool>> active_locations;
	int team_share_id;
	const Item* team_share_item;
	bool not_active, can_skip, force_end, negate_if;
	vector<Entry> prev;

	cstring GetText(int index);
	GameDialog::Text& GetTextInner(int index);
};

//-----------------------------------------------------------------------------
uint LoadDialogs(uint& crc, uint& errors);
void LoadDialogTexts();
GameDialog* FindDialog(cstring id);
void CleanupDialogs();
void VerifyDialogs(uint& errors);
