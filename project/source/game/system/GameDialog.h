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
struct GameDialog
{
	typedef std::map<cstring, GameDialog*, CstringComparer> Map;

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

	static GameDialog* TryGet(cstring id);
	static void Cleanup();
	static void Verify(uint& errors);
	static Map dialogs;
};

//-----------------------------------------------------------------------------
uint LoadDialogs(uint& crc, uint& errors);
void LoadDialogTexts();
