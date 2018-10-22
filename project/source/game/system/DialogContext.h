#pragma once

//-----------------------------------------------------------------------------
#include "GameDialog.h"

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

	static DialogContext* current;

	void StartNextDialog(GameDialog* dialog, int& if_level, Quest* quest = nullptr);
	void Update(float dt);
	void EndDialog();
	cstring GetText(int index);
	GameDialog::Text& GetTextInner(int index);
	cstring FormatString(const string& str_part);
};
