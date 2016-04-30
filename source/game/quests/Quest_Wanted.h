#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_Wanted : public Quest_Dungeon, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		Timeout,
		Killed,
		Finished,
		Recruited
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	bool IfHaveQuestItem() const;
	const Item* GetQuestItem()
	{
		return &letter;
	}
	bool IfNeedTalk(cstring topic) const;

private:
	int level, in_location;
	bool crazy;
	Class clas;
	string unit_name;
	OtherItem letter;
	Unit* target_unit;
};
