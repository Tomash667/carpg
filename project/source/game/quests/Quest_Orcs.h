#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool IfSpecial(DialogContext& ctx, cstring msg) override;
	void HandleLocationEvent(LocationEventHandler::Event event) override;
	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;
	int GetLocationEventHandlerQuestRefid() override
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

	enum class State
	{
		None,
		GeneratedGuard,
		GuardTalked,
		Accepted,
		OrcJoined,
		Completed,
		CompletedJoined,
		ToldAboutCamp,
		CampCleared,
		PickedClass,
		ToldAboutBase,
		GenerateOrcs,
		GeneratedOrcs,
		ClearDungeon,
		End
	};

	enum class OrcClass
	{
		None,
		Warrior,
		Hunter,
		Shaman
	};

	enum class Talked
	{
		No,
		AboutCamp,
		AboutBase,
		AboutBoss
	};

	void Start();
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool IfQuestEvent() const override;
	bool IfSpecial(DialogContext& ctx, cstring msg) override;
	void HandleLocationEvent(LocationEventHandler::Event event) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	int GetUnitEventHandlerQuestRefid() override { return refid; }
	int GetLocationEventHandlerQuestRefid() override { return refid; }
	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;
	void LoadOld(HANDLE file);

	OrcClass GetOrcClass() const { return orc_class; }

	State orcs_state;
	Talked talked;
	int days;
	Unit* guard, *orc;
	vector<ItemSlot> wares;

private:
	void ChangeClass(OrcClass new_orc_class);

	int near_loc;
	OrcClass orc_class;
};
