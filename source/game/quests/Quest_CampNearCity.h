#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_CampNearCity : public Quest_Dungeon, public LocationEventHandler
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
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout();
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

private:
	SPAWN_GROUP group;
};
