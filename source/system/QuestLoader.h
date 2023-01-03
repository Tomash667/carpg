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
	void ParseCode();
	void LoadTexts() override;
	int LoadQuestTexts(Tokenizer& t);
	void Finalize() override;
	void BuildQuest(QuestScheme* scheme);

	string code, globalCode;
	DialogLoader* dialogLoader;
	asIScriptEngine* engine;
	asIScriptModule* module;
};
