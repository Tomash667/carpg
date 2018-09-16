#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_BanditsCollectToll : public Quest_Encounter, public LocationEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		Timout,
		KilledBandits,
		Finished
	};

	void Start();
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	bool Special(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	int GetLocationEventHandlerQuestRefid() override
	{
		return refid;
	}

private:
	int other_loc;
};
