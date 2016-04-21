#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_KillAnimals : public Quest_Dungeon, public LocationEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		ClearedLocation,
		Finished,
		Timeout
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic) const;
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
};
