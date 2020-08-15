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
	asITypeInfo* script_type;
	asIScriptFunction* f_startup, *f_progress, *f_event, *f_upgrade;
	DialogScripts scripts;
	string properties, code;
	int flags;
	bool set_progress_use_prev, startup_use_vars;

	QuestScheme() : category(QuestCategory::NotSet), flags(0) {}
	~QuestScheme();
	GameDialog* GetDialog(const string& id);
	int GetProgress(const string& progress_id);

	static vector<QuestScheme*> schemes;
	static QuestScheme* TryGet(const string& id);
};
