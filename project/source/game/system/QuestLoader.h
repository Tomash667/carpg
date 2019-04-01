#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class QuestLoader : public ContentLoader
{
	friend class Content;
private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void ParseQuest(const string& id);
	void ParseQuestList(const string& id);
	void LoadTexts() override;
	void Finalize() override;
	void BuildQuest(QuestScheme* scheme);

	string code;
	DialogLoader* dialog_loader;
	asIScriptEngine* engine;
	asIScriptModule* module;
};
