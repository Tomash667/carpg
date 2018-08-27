#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_RetrievePackage : public Quest_Dungeon
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
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;

private:
	int from_loc;
	OtherItem parcel;
};
