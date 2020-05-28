#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// One mage wants to recover artifact from crypt. Give gold and disappear after that.
class Quest_Mages final : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Finished,
		EncounteredGolem
	};

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	LoadResult Load(GameReader& f) override;
};

//-----------------------------------------------------------------------------
// After some time there are golems on road that demand gold from players.
// Talking with guard captain will lead to drunk mage.
// Visiting mage tower will reveal that he forget everything.
// Need to create alchemic potion to recover his memory.
// After that we can go to enemy mage tower and kill him.
//-----------------------------------------------------------------------------
// start_loc = location with guard captain
// mage_loc = location with drunk mage
// target_loc = drunk mage tower, evil mage tower
class Quest_Mages2 final : public Quest_Dungeon, public UnitEventHandler
{
public:
	enum Progress
	{
		None,
		Started,
		MageWantsBeer,
		MageWantsVodka,
		GivenVodka,
		GotoTower,
		MageTalkedAboutTower,
		TalkedWithCaptain,
		BoughtPotion,
		MageDrinkPotion,
		NotRecruitMage,
		RecruitMage,
		KilledBoss,
		TalkedWithMage,
		Finished
	};

	enum class State
	{
		None,
		GeneratedScholar,
		ScholarWaits,
		Counting,
		Encounter,
		EncounteredGolem,
		TalkedWithCaptain,
		GeneratedOldMage,
		OldMageJoined,
		OldMageRemembers,
		BuyPotion,
		MageCured,
		MageLeaving,
		MageLeft,
		MageGeneratedInCity,
		MageRecruited,
		Completed
	};

	enum class Talked
	{
		No,
		AboutHisTower,
		AfterEnter,
		BeforeBoss
	};

	void Init();
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit) override;
	int GetUnitEventHandlerQuestRefid() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	void Update(float dt);

	Talked talked;
	State mages_state;
	Unit* scholar;
	string evil_mage_name, good_mage_name;
	HumanData hd_mage;
	int mage_loc, days;
	float timer;
	bool paid;
};
