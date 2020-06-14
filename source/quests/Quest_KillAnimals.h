#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Go to forest/cave and kill all animals.
class Quest_KillAnimals final : public Quest_Dungeon, public LocationEventHandler
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
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetLocationEventHandlerQuestId() override { return id; }

private:
	int GetReward() const;

	int st;
};
