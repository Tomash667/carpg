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
	void LoadVar(GameReader& f, Var::Type varType, void* ptr);
	GameDialog* GetDialog(const string& dialog_id);
	void SetProgress(int prog2) override;
	int GetProgress() const { return prog; }
	void AddEntry(const string& str);
	void SetStarted(const string& name);
	void SetCompleted();
	void SetFailed();
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
	JournalState journalState;
	int journalChanges, cellDepth;
	bool inUpgrade;
};
