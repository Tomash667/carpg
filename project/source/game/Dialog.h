#pragma once

//-----------------------------------------------------------------------------
enum DialogType
{
	DT_CHOICE,
	DT_TRADE,
	DT_END_CHOICE,
	DT_TALK,
	DT_TALK2,
	DT_RESTART,
	DT_END,
	DT_END2,
	DT_SHOW_CHOICES,
	DT_SPECIAL,
	DT_SET_QUEST_PROGRESS,
	DT_IF_QUEST_TIMEOUT,
	DT_END_IF,
	DT_IF_RAND,
	DT_ELSE,
	DT_CHECK_QUEST_TIMEOUT,
	DT_IF_HAVE_QUEST_ITEM,
	DT_DO_QUEST,
	DT_DO_QUEST_ITEM,
	DT_IF_QUEST_PROGRESS,
	DT_IF_NEED_TALK,
	DT_ESCAPE_CHOICE,
	DT_IF_SPECIAL,
	DT_IF_ONCE,
	DT_IF_CHOICES,
	DT_DO_QUEST2,
	DT_IF_HAVE_ITEM,
	DT_IF_QUEST_PROGRESS_RANGE,
	DT_IF_QUEST_EVENT,
	DT_END_OF_DIALOG,
	DT_DO_ONCE,
	DT_NOT_ACTIVE,
	DT_IF_QUEST_SPECIAL,
	DT_QUEST_SPECIAL
};

//-----------------------------------------------------------------------------
struct DialogEntry
{
	DialogType type;
	cstring msg;
};

//-----------------------------------------------------------------------------
struct DialogChoice
{
	int pos, lvl;
	cstring msg;

	DialogChoice(int _pos, cstring _msg, int _lvl) : pos(_pos), msg(_msg), lvl(_lvl)
	{
	}
};

//-----------------------------------------------------------------------------
struct Unit;
struct Quest;
struct Item;
struct PlayerController;

//-----------------------------------------------------------------------------
struct News
{
	string text;
	int add_time;
};

//-----------------------------------------------------------------------------
struct GameDialog
{
	struct Text
	{
		int id, next;
		bool exists;
	};

	string id;
	vector<DialogEntry> code;
	vector<string> strs;
	vector<Text> texts;
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
	int dialog_skip; // pomijanie opcji dialogowych u¿ywane przez DT_RANDOM_TEXT
	cstring dialog_text; // tekst dialogu
	string dialog_s_text; // tekst dialogu zmiennego
	Quest* dialog_quest; // quest zwi¹zany z dialogiem
	GameDialog* dialog; // aktualny dialog
	Unit* talker; // postaæ z któr¹ siê rozmawia
	float dialog_wait; // czas wyœwietlania opcji dialogowej
	bool dialog_once; // wyœwietlanie opcji dialogowej tylko raz
	cstring ostatnia_plotka;
	bool is_local;
	PlayerController* pc;
	int skip_id; // u¿ywane w mp do pomijania dialogów
	bool update_news;
	int update_locations; // 1-update, 0-updated, -1-no locations
	vector<News*> active_news;
	vector<std::pair<int, bool>> active_locations;
	int team_share_id;
	const Item* team_share_item;
	bool not_active, can_skip;
	vector<Entry> prev;

	cstring GetText(int index);
};

//-----------------------------------------------------------------------------
uint LoadDialogs(uint& crc, uint& errors);
void LoadDialogTexts();
GameDialog* FindDialog(cstring id);
void CleanupDialogs();
