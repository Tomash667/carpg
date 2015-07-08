#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// orkowie napadaj¹ na karawany, trzeba iœæ do podziemi i ich pozabijaæ
// w podziemiach jest zamkniêty pokój który klucz ma najsilniejsza jednostka
// w zamkniêtym pokoju jest ork który siê do nas przy³¹cza [Gorush]
// po jakimœ czasie mówi nam o obozie orków którzy go porwali gdy by³ m³ody
// po zniszczeniu awansuje na szamana/³owce/wojownika
// po jakimœ czasie mówi o swoim klanie który zosta³ podbity przez silnego orka
// trzeba iœæ i ich pobiæ a wtedy ork zostaje nowym wodzem, mo¿na tam spotkaæ orkowego kowala który sprzedaje orkowe przedmioty
class Quest_Orcs : public Quest_Dungeon, public LocationEventHandler
{
public:
	enum Progress
	{
		None,
		TalkedWithGuard,
		NotAccepted,
		Started,
		ClearedLocation,
		Finished
	};

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleLocationEvent(LocationEventHandler::Event event);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

private:
	int dungeon_levels, levels_cleared;
};

//-----------------------------------------------------------------------------
class Quest_Orcs2 : public Quest_Dungeon, public LocationEventHandler, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		TalkedOrc,
		NotJoined,
		Joined,
		TalkedAboutCamp,
		TalkedWhereIsCamp,
		ClearedCamp,
		TalkedAfterClearingCamp,
		SelectWarrior,
		SelectHunter,
		SelectShaman,
		SelectRandom,
		ChangedClass,
		TalkedAboutBase,
		TalkedWhereIsBase,
		KilledBoss,
		Finished
	};

	enum Talked
	{
		No,
		AboutCamp,
		AboutBase,
		AboutBoss
	};

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	bool IfQuestEvent();
	void HandleLocationEvent(LocationEventHandler::Event event);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	int GetLocationEventHandlerQuestRefid()
	{
		return refid;
	}

	Talked talked;

private:
	void ChangeClass(int klasa);

	int near_loc;
};
