#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Bandits are planing to take over a city.
// Talking with secret agent will tell to encounter some bandits and get message.
// After that need to clear bandits camp with guards.
// Later there is boss inside dungeon that needs to be killed.
class Quest_Bandits final : public Quest_Dungeon, public LocationEventHandler, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		NotAccepted,
		Started,
		Talked,
		FoundBandits,
		TalkAboutLetter,
		NeedTalkWithCaptain,
		NeedClearCamp,
		KilledBandits,
		TalkedWithAgent,
		KilledBoss,
		Finished
	};

	enum class State
	{
		None,
		GeneratedMaster,
		GenerateGuards,
		GeneratedGuards,
		Counting,
		AgentCome,
		AgentTalked,
		AgentLeft
	};

	void Init();
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetUnitEventHandlerQuestRefid() override { return id; }
	int GetLocationEventHandlerQuestRefid() override { return id; }
	void Update(float dt);

	State bandits_state;
	float timer;
	Unit* agent;

private:
	int enc, other_loc, camp_loc;
	bool get_letter;
};
