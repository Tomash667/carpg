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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	bool IfNeedTalk(cstring topic) const override;
	bool Load(GameReader& f) override;
	int GetLocationEventHandlerQuestRefid() override
	{
		return refid;
	}
};
