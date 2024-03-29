#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Converted to script in V_0_19
class Quest_LostArtifact final : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Finished,
		Timeout
	};

	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	LoadResult Load(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;

private:
	const Item* item;
	int st;
};
