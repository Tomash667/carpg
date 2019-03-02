#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Used to keep info about all events associated with quest
struct EventPtr
{
	enum Source
	{
		LOCATION
	};
	Source source;
	Location* location;

	bool operator == (const EventPtr& e) const
	{
		return source == e.source && location == e.location;
	}
};

//-----------------------------------------------------------------------------
class Quest_Scripted final : public Quest
{
	enum class JournalState
	{
		None,
		Added,
		Changed
	};

public:
	Quest_Scripted() : instance(nullptr), timeout_days(-1), call_depth(0) {}
	~Quest_Scripted();
	void Init(QuestScheme* scheme) { this->scheme = scheme; }
	void Start() override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	GameDialog* GetDialog(int type2) override;
	GameDialog* GetDialog(const string& dialog_id);
	void SetProgress(int prog2) override;
	int GetProgress() const { return prog; }
	void AddEntry(const string& str);
	void SetStarted(const string& name);
	void SetCompleted();
	void SetFailed();
	void SetTimeout(int days);
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	void FireEvent(ScriptEvent& event);
	void AddEventPtr(const EventPtr& event) { events.push_back(event); }
	void RemoveEventPtr(const EventPtr& event);
	void AddDialogPtr(Unit* unit) { unit_dialogs.push_back(unit); }
	void RemoveDialogPtr(Unit* unit);
	cstring FormatString(const string& str) override;
	string GetString(int index);
	QuestScheme* GetScheme() { return scheme; }
	asIScriptObject* GetInstance() { return instance; }

private:
	void BeforeCall();
	void AfterCall();
	void Cleanup();
	void CreateInstance();

	QuestScheme* scheme;
	asIScriptObject* instance;
	vector<EventPtr> events;
	vector<Unit*> unit_dialogs;
	JournalState journal_state;
	int journal_changes, timeout_days, call_depth;
};
