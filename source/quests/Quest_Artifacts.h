#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Converted to script in V_0_14
class Quest_Artifacts : public Quest_Dungeon
{
public:
	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	LoadResult Load(GameReader& f);
	void GetConversionData(ConversionData& data) override;
};
