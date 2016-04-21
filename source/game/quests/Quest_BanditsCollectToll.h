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
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IsTimedout() const;
	bool OnTimeout(TimeoutType ttype);
	void Special(DialogContext& ctx, cstring msg);
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void HandleLocationEvent(LocationEventHandler::Event event);
	bool IfNeedTalk(cstring topic) const;
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

private:
	int other_loc;
};
