#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "Var.h"

//-----------------------------------------------------------------------------
// Used to keep info about all events associated with quest
struct EventPtr
{
	enum Source
	{
		LOCATION,
		UNIT
	};

	Source source;
	EventType type;
	union
	{
		Location* location;
		Unit* unit;
	};

	bool operator == (const EventPtr& e) const
	{
		return source == e.source
			&& location == e.location
			&& (type == e.type || e.type == EVENT_ANY || type == EVENT_ANY);
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
	Quest_Scripted();
	~Quest_Scripted();
	void Init(QuestScheme* scheme) { this->scheme = scheme; }
	void Start() override;
	void Start(Vars* vars);
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void LoadVar(GameReader& f, Var::Type var_type, void* ptr);
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
	void AddRumor(const string& str);
	void RemoveRumor();
	void Upgrade(Quest* quest);

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
	bool in_upgrade;
};
