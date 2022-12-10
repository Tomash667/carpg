#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"
#include "GameDialog.h"

//-----------------------------------------------------------------------------
struct QuestScheme
{
	enum Flags
	{
		DONT_COUNT = 1 << 0,
		NOT_SCRIPTED = 1 << 1
	};

	string id;
	QuestCategory category;
	vector<GameDialog*> dialogs;
	vector<string> progress;
	vector<pair<uint, uint>> varAlias;
	asITypeInfo* scriptType;
	asIScriptFunction* fStartup, *fProgress, *fEvent, *fUpgrade;
	DialogScripts scripts;
	string properties, code;
	int flags;
	bool setProgressUsePrev, startupUseVars;

	QuestScheme() : category(QuestCategory::NotSet), flags(0) {}
	~QuestScheme();
	GameDialog* GetDialog(const string& id);
	int GetProgress(const string& progressId);
	int GetPropertyId(uint nameHash);

	static vector<QuestScheme*> schemes;
	static QuestScheme* TryGet(const string& id);
};
