#pragma once

//-----------------------------------------------------------------------------
#include "Quest2.h"
#include "Var.h"

//-----------------------------------------------------------------------------
class Quest_Scripted final : public Quest2
{
	enum class JournalState
	{
		None,
		Added,
		Changed
	};

public:
	Quest_Scripted();
	~Quest_Scripted();
	void Start() override;
	void Start(Vars* vars);
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void LoadVar(GameReader& f, Var::Type var_type, void* ptr);
	GameDialog* GetDialog(const string& dialog_id);
	void SetProgress(int prog2) override;
	int GetProgress() const { return prog; }
	void AddEntry(const string& str);
	void SetStarted(const string& name);
	void SetCompleted(bool cleanup);
	void SetFailed(bool cleanup);
	void Restart();
	void FireEvent(ScriptEvent& event) override;
	string GetString(int index);
	asIScriptObject* GetInstance() override { return instance; }
	void AddRumor(const string& str);
	void RemoveRumor();
	void Upgrade(Quest* quest);

private:
	void BeforeCall();
	void AfterCall();

	asIScriptObject* instance;
	JournalState journal_state;
	int journal_changes, call_depth;
	bool in_upgrade;
};
