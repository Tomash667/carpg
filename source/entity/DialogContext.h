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

	int pos, questDialogIndex;
	cstring msg, talkMsg;
	string* pooled; // used for dialog choices when message is formatted
	Type type;

	DialogChoice() {}
	DialogChoice(int pos, cstring msg, int questDialogIndex, string* pooled = nullptr) : pos(pos), msg(msg),
		questDialogIndex(questDialogIndex), pooled(pooled), type(Normal), talkMsg(nullptr) {}
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
		WAIT_MP_RESPONSE,
		WAIT_DIALOG
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
	bool dialogMode; // czy jest tryb dialogowy
	bool isLocal;
	int dialogPos; // pozycja w dialogu
	int choiceSelected; // zaznaczona opcja dialogowa
	int dialogEsc; // opcja dialogowa wybierana po wciœniêciu ESC
	int skipId; // u¿ywane w mp do pomijania dialogów
	int teamShareId;
	float timer;
	const Item* teamShareItem;
	vector<DialogChoice> choices; // opcje dialogowe do wyboru
	cstring dialogText; // tekst dialogu
	string dialogString; // tekst dialogu zmiennego
	string predialog;
	SpeechBubble* predialogBubble;

	static DialogContext* current;
	static int var;

	~DialogContext() { ClearChoices(); }
	void StartDialog(Unit* talker, GameDialog* dialog = nullptr, Quest* quest = nullptr);
	void StartNextDialog(GameDialog* dialog, Quest* quest = nullptr, bool returns = true);
	void Update(float dt);
	void UpdateClient();
	void EndDialog();
	void Talk(cstring msg);
	void ClientTalk(Unit* unit, const string& text, int skipId, int animation);
	void RemoveQuestDialog(Quest2* quest);
	cstring GetIdleText(Unit& talker);
	void Wait(float _timer)
	{
		mode = WAIT_TIMER;
		timer = _timer;
	}
	void OnPickRestDays(int days);

private:
	void UpdateLoop();
	cstring GetText(int index, bool multi = false);
	bool ExecuteSpecial(cstring msg);
	bool ExecuteSpecialIf(cstring msg);
	cstring FormatString(const string& strPart);
	void ClearChoices();
	bool DoIfOp(int value1, int value2, DialogOp op);
	bool LearnPerk(Perk* perk);
	bool RecruitHero(Class* clas);

	GameDialog* dialog; // aktualny dialog
	Quest* dialogQuest; // quest zwi¹zany z dialogiem
	cstring lastRumor;
	cstring talkMsg;
	vector<Entry> prev;
	vector<QuestDialog> questDialogs;
	vector<News*> activeNews;
	vector<pair<int, bool>> activeLocations;
	int updateLocations, // 1-update, 0-updated, -1-no locations
		questDialogIndex;
	bool once, updateNews, forceEnd;
};
