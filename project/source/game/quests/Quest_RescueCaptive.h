#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
class Quest_RescueCaptive final : public Quest_Dungeon, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		FoundCaptive,
		CaptiveDie,
		Timeout,
		Finished,
		CaptiveEscape,
		ReportDeath,
		ReportEscape,
		CaptiveLeftInCity
	};

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IsTimedout() const override;
	bool OnTimeout(TimeoutType ttype) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	int GetUnitEventHandlerQuestRefid() override { return refid; }

private:
	int GetReward() const;

	UnitGroup* group;
	Unit* captive;
	int st;
};
