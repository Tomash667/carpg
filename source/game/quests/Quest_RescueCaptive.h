#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_RescueCaptive : public Quest_Dungeon, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		FoundCaptive,
		CaptiveDie,
		Timeout,
		Finished,
		CaptiveEscape,
		ReportDeath,
		ReportEscape,
		CaptiveLeftInCity
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	bool IfNeedTalk(cstring topic) const;
	void Save(HANDLE file);
	void Load(HANDLE file);
	inline int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}

private:
	SPAWN_GROUP GetRandomGroup() const;

	SPAWN_GROUP group;
	Unit* captive;
};
