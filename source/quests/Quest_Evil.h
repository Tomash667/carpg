#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// zleca kap³an w losowym miejscu
// trzeba iœæ do podziemi zabiæ nekromantê ale go tam nie ma, jest zakrwawiony o³tarz, gdy siê podejdzie pojawiaj¹ siê nieumarli;
// potem trzeba zameldowaæ o postêpach, wysy³a nas do jakiegoœ maga po ksiêgê, mag mówi ¿e stra¿nicy j¹ zarewirowali, kapitan odsy³a do burmistrza
// potem do kapitana i j¹ daje, wracamy do kap³ana, wysy³a nas do podziemi na koñcu których jest portal do innych podziemi, na koñcu jest boss
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
	bool Load(GameReader& f) override;
	int GetLocId(int location_id);

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
