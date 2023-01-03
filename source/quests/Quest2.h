#pragma once

//-----------------------------------------------------------------------------
#include "Event.h"
#include "Quest.h"

//-----------------------------------------------------------------------------
struct Quest2 : public Quest
{
	Quest2();
	QuestScheme* GetScheme() const { return scheme; }
	virtual asIScriptObject* GetInstance() { return nullptr; }
	void SetScheme(QuestScheme* scheme) { this->scheme = scheme; isNew = true; }
	void AddEventPtr(const EventPtr& event) { events.push_back(event); }
	void RemoveEventPtr(const EventPtr& event);
	virtual void FireEvent(ScriptEvent& event) {}
	cstring FormatString(const string& str) override;
	void AddDialogPtr(Unit* unit) { unitDialogs.push_back(unit); }
	void RemoveDialogPtr(Unit* unit);
	GameDialog* GetDialog(Cstring name);
	GameDialog* GetDialog(int type2) override;
	void LoadQuest2(GameReader& f, cstring schemeId);
	virtual void LoadDetails(GameReader& f) {};
	void SetTimeout(int days);
	void AddTimer(int days);
	int GetTimeout() const;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	virtual bool PostRun() { return false; }

protected:
	void Cleanup();

	QuestScheme* scheme;
	vector<EventPtr> events;
	vector<Unit*> unitDialogs;
	int timeoutDays;
};
