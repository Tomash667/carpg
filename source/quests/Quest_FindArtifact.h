#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Go to crypt and recover artifact from treasure.
class Quest_FindArtifact final : public Quest_Dungeon
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
	LoadResult Load(GameReader& f) override;

private:
	int GetReward() const;

	const Item* item;
	OtherItem quest_item;
	int st;
};
