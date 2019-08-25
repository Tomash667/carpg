#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
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

	static const int PAYMENT = 500;

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	int GetLocationEventHandlerQuestRefid() override { return id; }
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	void GenerateSawmill(bool in_progress);

	State sawmill_state;
	BuildState build_state;
	int days;
	HumanData hd_lumberjack;
	Unit* messenger;
};
