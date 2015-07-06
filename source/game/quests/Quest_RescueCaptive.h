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
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
	inline int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}

private:
	SPAWN_GROUP group;
	Unit* captive;
};
