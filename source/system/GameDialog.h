#pragma once

//-----------------------------------------------------------------------------
enum DialogType : short
{
	DTF_CHOICE,
	DTF_TRADE,
	DTF_TALK,
	DTF_RESTART,
	DTF_END,
	DTF_END2,
	DTF_SHOW_CHOICES,
	DTF_SPECIAL,
	DTF_SET_QUEST_PROGRESS,
	DTF_IF_QUEST_TIMEOUT,
	DTF_IF_RAND,
	DTF_CHECK_QUEST_TIMEOUT,
	DTF_IF_HAVE_QUEST_ITEM,
	DTF_IF_HAVE_QUEST_ITEM_CURRENT,
	DTF_IF_HAVE_QUEST_ITEM_NOT_ACTIVE,
	DTF_DO_QUEST,
	DTF_DO_QUEST_ITEM,
	DTF_IF_QUEST_PROGRESS,
	DTF_IF_NEED_TALK,
	DTF_IF_SPECIAL,
	DTF_IF_ONCE,
	DTF_IF_CHOICES,
	DTF_DO_QUEST2,
	DTF_IF_HAVE_ITEM,
	DTF_IF_QUEST_EVENT,
	DTF_END_OF_DIALOG,
	DTF_DO_ONCE,
	DTF_IF_QUEST_SPECIAL,
	DTF_QUEST_SPECIAL,
	DTF_SCRIPT,
	DTF_IF_SCRIPT,
	DTF_JMP,
	DTF_CJMP
};

//-----------------------------------------------------------------------------
enum DialogOp : short
{
	OP_NONE,
	OP_EQUAL,
	OP_NOT_EQUAL,
	OP_GREATER,
	OP_GREATER_EQUAL,
	OP_LESS,
	OP_LESS_EQUAL,
	OP_ESCAPE,
	OP_BETWEEN,
	OP_NOT_BETWEEN
};

//-----------------------------------------------------------------------------
struct DialogEntry
{
	DialogType type;
	DialogOp op;
	int value;

	DialogEntry(DialogType type, int value = 0) : type(type), op(OP_EQUAL), value(value) {}
	DialogEntry(DialogType type, DialogOp op, int value) : type(type), op(op), value(value) {}
};

//-----------------------------------------------------------------------------
struct DialogScripts
{
	enum FUNC
	{
		F_SCRIPT,
		F_IF_SCRIPT,
		F_FORMAT,
		F_MAX
	};

	DialogScripts() : func(), built(false) {}
	~DialogScripts();
	int AddCode(FUNC f, const string& code);
	void GetFormattedCode(FUNC f, string& code);
	int Build();
	asIScriptFunction* Get(FUNC f) { return func[f]; }
	void Set(asITypeInfo* type);

private:
	vector<string> scripts[F_MAX];
	asIScriptFunction* func[F_MAX];
	bool built;

public:
	static DialogScripts global;
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

	string id;
	vector<DialogEntry> code;
	vector<string> strs;
	vector<Text> texts;
	int max_index;
	QuestScheme* quest;

	Text& GetText(int index);

	static GameDialog* TryGet(cstring id);
	static GameDialog* GetS(const string& id)
	{
		return TryGet(id.c_str());
	}
	static void Cleanup();
	static Map dialogs;
};

//-----------------------------------------------------------------------------
struct QuestDialog
{
	GameDialog* dialog;
	Quest_Scripted* quest;
};
