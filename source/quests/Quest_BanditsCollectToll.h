#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Converted to script in V_0_13
class Quest_BanditsCollectToll final : public Quest_Encounter, public LocationEventHandler
{
public:
	void Start() override {}
	GameDialog* GetDialog(int type2) override { return nullptr; }
	void SetProgress(int prog2) override {}
	LoadResult Load(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;
	bool HandleLocationEvent(Event event) override { return false; }
	int GetLocationEventHandlerQuestId() override { return id; }

private:
	int other_loc;
};
