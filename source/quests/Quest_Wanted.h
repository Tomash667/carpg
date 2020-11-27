#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Converted to script in V_DEV
class Quest_Wanted final : public Quest_Dungeon
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
	LoadResult Load(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;

private:
	int level, in_location;
	bool crazy;
	Class* clas;
	string unit_name;
	OtherItem letter;
	Unit* target_unit;
};
