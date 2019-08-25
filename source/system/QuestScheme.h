#pragma once

//-----------------------------------------------------------------------------
#include "QuestConsts.h"
#include "GameDialog.h"

//-----------------------------------------------------------------------------
struct QuestScheme
{
	string id;
	QuestCategory category;
	vector<GameDialog*> dialogs;
	vector<string> progress;
	asITypeInfo* script_type;
	asIScriptFunction* f_startup, *f_progress, *f_event, *f_format_string;
	DialogScripts scripts;
	string code;
	bool set_progress_use_prev;

	QuestScheme() : category(QuestCategory::NotSet) {}
	~QuestScheme();
	GameDialog* GetDialog(const string& id);
	int GetProgress(const string& progress_id);

	static vector<QuestScheme*> schemes;
	static QuestScheme* TryGet(const string& id);
};
