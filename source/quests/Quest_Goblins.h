#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// Nobleman asks to go to forest and find his father lost bow.
// While returning with it golbins will ambush player and steal it.
// Returning to nobleman will mark quest as failed but after some time he will send messanger to player.
// He send player to goblin dungeon to recover bow.
// Nobleman pays some small amount of gold and disappear.
// Later mage asks player about this bow and tell that it was artifact.
// Need to go to dungeon and kill nobleman and recover bow.
class Quest_Goblins final : public Quest_Dungeon, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		NotAccepted,
		Started,
		BowStolen,
		TalkedAboutStolenBow,
		InfoAboutGoblinBase,
		GivenBow,
		DidntTalkedAboutBow,
		TalkedAboutBow,
		PayedAndTalkedAboutBow,
		TalkedWithInnkeeper,
		KilledBoss
	};

	enum class State
	{
		None,
		GeneratedNobleman,
		Counting,
		MessengerTalked,
		GivenBow,
		NoblemanLeft,
		GeneratedMage,
		MageTalkedStart,
		MageTalked,
		MageLeft,
		KnownLocation
	};

	void Init();
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit) override;
	int GetUnitEventHandlerQuestRefid() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void OnProgress(int days);

	State goblins_state;
	int days;
	Unit* nobleman, *messenger;
	HumanData hd_nobleman;

private:
	int enc;
};
