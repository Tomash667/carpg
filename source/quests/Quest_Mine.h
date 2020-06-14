#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// Investor wants to have cave cleared so mine can be build.
// Reward can be gold or monthly payment.
// After some time mine can be upgraded from iron to gold.
// Later portal to ancient armory is discovered.
class Quest_Mine final : public Quest_Dungeon, public LocationEventHandler, public ChestEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		ClearedLocation,
		SelectedShares,
		GotFirstGold,
		SelectedGold,
		NeedTalk,
		Talked,
		NotInvested,
		Invested,
		UpgradedMine,
		InfoAboutPortal,
		TalkedWithMiner,
		Finished
	};

	enum class State
	{
		None,
		SpawnedInvestor,
		Shares,
		BigShares
	};

	enum class State2
	{
		None,
		InBuild,
		Built,
		CanExpand,
		InExpand,
		Expanded,
		FoundPortal
	};

	enum class State3
	{
		None,
		GeneratedMine,
		GeneratedInBuild,
		GeneratedBuilt,
		GeneratedExpanded,
		GeneratedPortal
	};

	static const int PAYMENT = 750;
	static const int PAYMENT2 = 1500;

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	void HandleChestEvent(ChestEventHandler::Event event, Chest* chest) override;
	int GetLocationEventHandlerQuestId() override { return id; }
	int GetChestEventHandlerQuestId() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GenerateMine(CaveGenerator* cave_gen, bool first);
	int OnProgress(int days);

	Quest_Event sub;
	int dungeon_loc;
	State mine_state;
	State2 mine_state2;
	State3 mine_state3;
	int days, days_required, days_gold;
	Unit* messenger;

private:
	void InitSub();
};
