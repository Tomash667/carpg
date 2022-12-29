#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// There is wanted hero in one of cities, killing him will get reward
class Quest_Wanted final : public Quest_Dungeon, public UnitEventHandler
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

	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	bool OnTimeout(TimeoutType ttype) override;
	void HandleUnitEvent(UnitEventHandler::TYPE eventType, Unit* unit) override;
	LoadResult Load(GameReader& f) override;
	int GetUnitEventHandlerQuestId() override { return id; }

private:
	int level, inLocation;
	bool crazy;
	Class* clas;
	string unitName;
	OtherItem letter;
	Unit* targetUnit;
};
