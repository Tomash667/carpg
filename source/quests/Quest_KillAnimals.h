#pragma once

//-----------------------------------------------------------------------------
#include "Quest2.h"

//-----------------------------------------------------------------------------
// Converted to script in V_0_20
class Quest_KillAnimals final : public Quest2, public LocationEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		ClearedLocation,
		Finished,
		Timeout,
		OnTimeout
	};

	void Start() override {}
	void SetProgress(int p) override {}
	LoadResult Load(GameReader& f) override;
	void LoadDetails(GameReader& f) override;
	void GetConversionData(ConversionData& data) override;
	bool HandleLocationEvent(Event event) override { return false; }
	int GetLocationEventHandlerQuestId() override { return id; }

private:
	Location* targetLoc;
	int st;
};
