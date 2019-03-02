#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class DialogLoader : public ContentLoader
{
	friend class Content;

	enum IfState
	{
		IFS_IF,
		IFS_INLINE_IF,
		IFS_ELSE,
		IFS_INLINE_ELSE,
		IFS_CHOICE,
		IFS_INLINE_CHOICE
	};

private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	GameDialog* LoadDialog(const string& id);
	DialogOp ParseOp();
	int ParseProgress();
	void LoadGlobals();
	void LoadTexts() override;

	vector<IfState> if_state;
	QuestScheme* quest;
	DialogScripts* scripts;

public:
	GameDialog* LoadSingleDialog(Tokenizer& t, QuestScheme* quest);
	bool LoadText(Tokenizer& t, QuestScheme* scheme = nullptr);
	void CheckDialogText(GameDialog* dialog, int index, DialogScripts* scripts);
};
