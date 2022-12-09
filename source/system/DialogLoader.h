#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"
#include "GameDialog.h"

//-----------------------------------------------------------------------------
class DialogLoader : public ContentLoader
{
	friend class Content;

public:
	GameDialog* LoadSingleDialog(Tokenizer& t, QuestScheme* quest);
	bool LoadText(Tokenizer& t, QuestScheme* scheme = nullptr);
	void CheckDialogText(GameDialog* dialog, int index, DialogScripts* scripts);

private:
	enum IfState
	{
		IFS_IF,
		IFS_INLINE_IF,
		IFS_ELSE,
		IFS_INLINE_ELSE,
		IFS_CHOICE,
		IFS_INLINE_CHOICE
	};

	enum class NodeOp
	{
		If,
		Choice,
		Block,
		Statement,
		Switch,
		Case,
		Random,
		Chance,
		Goto
	};

	enum class IfOp
	{
		Equal,
		NotEqual,
		Greater,
		GreaterEqual,
		Less,
		LessEqual,
		Between
	};

	struct Node : ObjectPoolProxy<Node>
	{
		NodeOp nodeOp;
		DialogType type;
		DialogOp op;
		int value;
		vector<Node*> childs;
		string str;
#ifdef _DEBUG
		int line;
#endif

		void OnFree() { Free(childs); }
	};

	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	GameDialog* LoadDialog(const string& id);
	DialogOp ParseOp();
	int ParseProgressOrInt(int keyword);
	int ParseProgress();
	void LoadGlobals();
	void LoadTexts() override;
	cstring GetEntityName() override;
	Node* GetNode()
	{
		Node* node = Node::Get();
#ifdef _DEBUG
		node->line = t.GetLine();
#endif
		return node;
	}
	Node* ParseStatement();
	Node* ParseBlock();
	Node* ParseIf();
	Node* ParseChoice();
	Node* ParseSwitch();
	Node* ParseRandom();
	DialogOp GetNegatedOp(DialogOp op);
	bool BuildDialog(Node* node);
	bool BuildDialogBlock(Node* node);
	void PatchJumps();

	GameDialog* currentDialog;
	vector<std::pair<string, uint>> labels;
	vector<std::pair<string, uint>> jumps;
	QuestScheme* quest;
	DialogScripts* scripts;
	bool loadingTexts;
};
