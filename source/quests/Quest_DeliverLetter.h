#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Converted to script in V_DEV
class Quest_DeliverLetter final : public Quest
{
public:
	enum Progress
	{
		None,
		Started,
		Timeout,
		GotResponse,
		Finished
	};

	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	LoadResult Load(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;

private:
	int end_loc;
	OtherItem letter;
};
