#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Bandits captures someone and are holding him inside dungeon/camp.
// Need to rescue him and deliver back to city.
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
	void HandleUnitEvent(UnitEventHandler::TYPE eventType, Unit* unit) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetUnitEventHandlerQuestId() override { return id; }

private:
	int GetReward() const;

	UnitGroup* group;
	Unit* captive;
	int st;
};
