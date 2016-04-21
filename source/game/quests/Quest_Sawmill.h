#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_Sawmill : public Quest_Dungeon, public LocationEventHandler
{
public:
	enum Progress
	{
		None,
		NotAccepted,
		Started,
		ClearedLocation,
		Talked,
		Finished
	};

	enum class State
	{
		None,
		GeneratedUnit,
		InBuild,
		Working
	};

	enum class BuildState
	{
		None,
		LumberjackLeft,
		InProgress,
		Finished
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void HandleLocationEvent(LocationEventHandler::Event event);
	inline int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	State sawmill_state;
	BuildState build_state;
	int days;
	HumanData hd_lumberjack;
	Unit* messenger;
};
