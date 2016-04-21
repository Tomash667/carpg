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

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleChestEvent(ChestEventHandler::Event event);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetChestEventHandlerQuestRefid()
	{
		return refid;
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	int GetIncome(int days);

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
