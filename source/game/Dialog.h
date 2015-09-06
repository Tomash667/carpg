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
	DT_RANDOM_TEXT,
	DT_IF_CHOICES,
	DT_DO_QUEST2,
	DT_IF_HAVE_ITEM,
	DT_IF_QUEST_PROGRESS_RANGE,
	DT_IF_QUEST_EVENT,
	DT_END_OF_DIALOG,
	DT_DO_ONCE,
	DT_NOT_ACTIVE
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
extern DialogEntry dialog_kowal[];
extern DialogEntry dialog_kupiec[];
extern DialogEntry dialog_alchemik[];
extern DialogEntry dialog_burmistrz[];
extern DialogEntry dialog_mieszkaniec[];
extern DialogEntry dialog_widz[];
extern DialogEntry dialog_straznik[];
extern DialogEntry dialog_trener[];
extern DialogEntry dialog_dowodca_strazy[];
extern DialogEntry dialog_karczmarz[];
extern DialogEntry dialog_urzednik[];
extern DialogEntry dialog_mistrz_areny[];
extern DialogEntry dialog_hero[];
extern DialogEntry dialog_hero_przedmiot[];
extern DialogEntry dialog_hero_przedmiot_kup[];
extern DialogEntry dialog_hero_pvp[];
extern DialogEntry dialog_szalony[];
extern DialogEntry dialog_szalony_przedmiot[];
extern DialogEntry dialog_szalony_przedmiot_kup[];
extern DialogEntry dialog_szalony_pvp[];
extern DialogEntry dialog_bandyci[];
extern DialogEntry dialog_bandyta[];
extern DialogEntry dialog_szalony_mag[];
extern DialogEntry dialog_porwany[];
extern DialogEntry dialog_straznicy_nagroda[];
extern DialogEntry dialog_zadanie[];
extern DialogEntry dialog_artur_drwal[];
extern DialogEntry dialog_drwal[];
extern DialogEntry dialog_inwestor[];
extern DialogEntry dialog_gornik[];
extern DialogEntry dialog_pijak[];
extern DialogEntry dialog_q_bandyci[];
extern DialogEntry dialog_q_magowie[];
extern DialogEntry dialog_q_magowie2[];
extern DialogEntry dialog_q_orkowie[];
extern DialogEntry dialog_q_orkowie2[];
extern DialogEntry dialog_q_gobliny[];
extern DialogEntry dialog_q_zlo[];
extern DialogEntry dialog_tut_czlowiek[];
extern DialogEntry dialog_q_szaleni[];
extern DialogEntry dialog_szaleni[];
extern DialogEntry dialog_tomashu[];
extern DialogEntry dialog_ochroniarz[];
extern DialogEntry dialog_mag_obstawa[];
extern DialogEntry dialog_sprzedawca_jedzenia[];

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
struct Dialog2Text
{
	int id, next;
	bool exists;
};

//-----------------------------------------------------------------------------
struct Dialog2
{
	string id;
	vector<DialogEntry> code;
	vector<string> strs;
	vector<Dialog2Text> texts;
	int max_index;
};

//-----------------------------------------------------------------------------
struct DialogContext
{
	bool dialog_mode; // czy jest tryb dialogowy
	bool show_choices; // czy wyœwietlono opcje dialogowe do wyboru
	vector<DialogChoice> choices; // opcje dialogowe do wyboru
	int dialog_pos; // pozycja w dialogu
	int prev_dialog_pos; // poprzednia pozycja w dialogu
	int choice_selected; // zaznaczona opcja dialogowa
	int dialog_level; // poziom zagnie¿d¿enia w dialogu
	int prev_dialog_level; // poprzednit poziom zagnie¿d¿enia
	int dialog_esc; // opcja dialogowa wybierana po wciœniêciu ESC
	int dialog_skip; // pomijanie opcji dialogowych u¿ywane przez DT_RANDOM_TEXT
	cstring dialog_text; // tekst dialogu
	string dialog_s_text; // tekst dialogu zmiennego
	Quest* dialog_quest; // quest zwi¹zany z dialogiem
	DialogEntry* dialog; // aktualny dialog
	DialogEntry* prev_dialog; // poprzedni dialog
	Unit* talker; // postaæ z któr¹ siê rozmawia
	float dialog_wait; // czas wyœwietlania opcji dialogowej
	bool dialog_once; // wyœwietlanie opcji dialogowej tylko raz
	cstring ostatnia_plotka;
	bool is_local;
	Unit* next_talker;
	PlayerController* pc;
	int skip_id; // u¿ywane w mp do pomijania dialogów
	bool update_news;
	vector<News*> active_news;
	DialogEntry* next_dialog;
	int team_share_id;
	const Item* team_share_item;
	bool is_new, is_prev_new, is_next_new, not_active;

	cstring GetText(int index);
};

//-----------------------------------------------------------------------------
void ExportDialogs();
void LoadDialogs(uint& crc);
void LoadDialogTexts();

extern vector<Dialog2*> dialogs;
