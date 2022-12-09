#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Bandits stolen package that must be retrived from dungeon strongest unit.
class Quest_RetrievePackage final : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Timeout,
		Finished
	};

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool IfHaveQuestItem() const override;
	const Item* GetQuestItem() override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;

private:
	int GetReward() const;

	int fromLoc;
	OtherItem parcel;
	int st;
};
