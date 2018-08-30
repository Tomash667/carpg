#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_FindArtifact : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Finished,
		Timeout
	};

	void Start();
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool IfHaveQuestItem2(cstring id) const override;
	const Item* GetQuestItem() override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;

private:
	const Item* item;
	OtherItem quest_item;
};
