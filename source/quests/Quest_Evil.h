#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// In front of inn is standing cleric.
// He asks you to go to some crypt. Inside are no enemies, but bloody altar. Spawn enemies when near.
// Cleric then send player for book.
// Later player need to go to 3 dungeons and close portals.
// After that in first dungeon there is a boss that need to be killed.
class Quest_Evil final : public Quest_Dungeon, public UnitEventHandler
{
public:
	struct Loc : public Quest_Event
	{
		enum State
		{
			None,
			TalkedAfterEnterLocation,
			TalkedAfterEnterLevel,
			PortalClosed
		};

		int near_loc;
		Vec3 pos;
		State state;
	};

	enum Progress
	{
		None,
		NotAccepted,
		Started,
		Talked,
		AltarEvent,
		TalkedAboutBook,
		MageToldAboutStolenBook,
		TalkedWithCaptain,
		TalkedWithMayor,
		GotBook,
		GivenBook,
		PortalClosed,
		AllPortalsClosed,
		KilledBoss,
		Finished
	};

	enum class State
	{
		None,
		GeneratedCleric,
		SpawnedAltar,
		Summoning,
		GenerateMage,
		GeneratedMage,
		ClosingPortals,
		KillBoss,
		ClericWantTalk,
		ClericLeaving,
		ClericLeft
	};

	void Init();
	void Start();
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool IfQuestEvent() const override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit) override;
	int GetUnitEventHandlerQuestRefid() override { return id; }
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	int GetLocId(int location_id);
	void Update(float dt);

	Loc loc[3];
	int closed, mage_loc;
	bool changed, told_about_boss;
	State evil_state;
	Vec3 pos;
	float timer;
	Unit* cleric;

private:
	void GenerateBloodyAltar();
	void GeneratePortal();
	void WarpEvilBossToAltar();
};
