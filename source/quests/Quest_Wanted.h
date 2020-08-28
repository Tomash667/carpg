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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit) override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetUnitEventHandlerQuestId() override { return id; }
	bool IfHaveQuestItem() const override;
	const Item* GetQuestItem() override { return &letter; }
	bool IfNeedTalk(cstring topic) const override;

private:
	int level, in_location;
	bool crazy;
	Class* clas;
	string unit_name;
	OtherItem letter;
	Unit* target_unit;
};
