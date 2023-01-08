#pragma once

//-----------------------------------------------------------------------------
#include "Quest2.h"

//-----------------------------------------------------------------------------
// Converted to script in V_0_20
class Quest_DireWolf : public Quest2
{
public:
	void Start() override {}
	void SetProgress(int p) override {}
	LoadResult Load(GameReader& f) override;
	void LoadDetails(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;

private:
	Location* forest;
};
