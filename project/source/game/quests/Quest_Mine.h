#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_Mine : public Quest_Dungeon, public LocationEventHandler, public ChestEventHandler
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
	int GetLocationEventHandlerQuestRefid() override { return refid; }
	int GetChestEventHandlerQuestRefid() override { return refid; }
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	void LoadOld(GameReader& f);
	int GetIncome(int days_passed);
	bool GenerateMine(CaveGenerator* cave_gen);

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
