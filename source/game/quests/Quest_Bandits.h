#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_Bandits : public Quest_Dungeon, public LocationEventHandler, public UnitEventHandler
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

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	void Special(DialogContext& ctx, cstring msg);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

	State bandits_state;
	float timer;
	Unit* agent;

private:
	int enc, other_loc, camp_loc;
	bool get_letter;
};
