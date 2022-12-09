#pragma once

//-----------------------------------------------------------------------------
#include "Event.h"
#include "Quest.h"

//-----------------------------------------------------------------------------
struct Quest2 : public Quest
{
	Quest2();
	QuestScheme* GetScheme() const { return scheme; }
	virtual asIScriptObject* GetInstance() { return CreateInstance(true); }
	void SetScheme(QuestScheme* scheme) { this->scheme = scheme; isNew = true; }
	void AddEventPtr(const EventPtr& event) { events.push_back(event); }
	void RemoveEventPtr(const EventPtr& event);
	virtual void FireEvent(ScriptEvent& event) {}
	cstring FormatString(const string& str) override;
	void RemoveEvent(ScriptEvent& event);
	void AddDialogPtr(Unit* unit) { unitDialogs.push_back(unit); }
	void RemoveDialogPtr(Unit* unit);
	GameDialog* GetDialog(Cstring name);
	GameDialog* GetDialog(int type2) override;
	cstring GetText(int index);
	Var* GetValue(int offset);
	void Save(GameWriter& f) override;
	virtual void SaveDetails(GameWriter& f) {};
	LoadResult Load(GameReader& f) override;
	virtual void LoadDetails(GameReader& f) {};
	void SetState(State state);
	void SetTimeout(int days);
	int GetTimeout() const;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;

protected:
	asIScriptObject* CreateInstance(bool shared);
	void Cleanup();

	QuestScheme* scheme;
	vector<EventPtr> events;
	vector<Unit*> unitDialogs;
	int timeoutDays;
};
