#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Orcs are attacking caravans, need to go to dungeon and kill them.
class Quest_Orcs final : public Quest_Dungeon, public LocationEventHandler
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

	void Init() override;
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetLocationEventHandlerQuestId() override { return id; }

private:
	int dungeon_levels;
};

//-----------------------------------------------------------------------------
// Inside dungeon there is locked orc Gorush that joins team, strongest enemy has the key.
// After some time he talks about orcs camp that kidnapped him.
// Destroying camp allow to chance Gorush class to shaman/warrior/hunter.
// After some time he talks about dungeon where is orc boss.
// Killing him will make Gorush new orc leader and leave team.
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

	void Init() override;
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool IfQuestEvent() const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool HandleLocationEvent(LocationEventHandler::Event event) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	int GetUnitEventHandlerQuestId() override { return id; }
	int GetLocationEventHandlerQuestId() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	OrcClass GetOrcClass() const { return orc_class; }
	void OnProgress(int days);

	State orcs_state;
	Talked talked;
	int days;
	Unit* guard, *orc;

private:
	void ChangeClass(OrcClass new_orc_class);

	int near_loc;
	OrcClass orc_class;
};
