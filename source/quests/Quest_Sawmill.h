#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Clear forest so woodcutters can build sawmill there, monthly payment.
class Quest_Sawmill final : public Quest_Dungeon, public LocationEventHandler
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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	int GetLocationEventHandlerQuestId() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void GenerateSawmill(bool in_progress);
	void OnProgress(int days);

	State sawmill_state;
	BuildState build_state;
	int days;
	HumanData hd_lumberjack;
	Unit* messenger;
};
