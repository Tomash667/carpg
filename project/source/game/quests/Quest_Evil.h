#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// zleca kap�an w losowym miejscu
// trzeba i�� do podziemi zabi� nekromant� ale go tam nie ma, jest zakrwawiony o�tarz, gdy si� podejdzie pojawiaj� si� nieumarli;
// potem trzeba zameldowa� o post�pach, wysy�a nas do jakiego� maga po ksi�g�, mag m�wi �e stra�nicy j� zarewirowali, kapitan odsy�a do burmistrza
// potem do kapitana i j� daje, wracamy do kap�ana, wysy�a nas do podziemi na ko�cu kt�rych jest portal do innych podziemi, na ko�cu jest boss
class Quest_Evil : public Quest_Dungeon, public UnitEventHandler
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

	void Start();
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	bool IfQuestEvent() const override;
	bool IfSpecial(DialogContext& ctx, cstring msg) override;
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit) override;
	int GetUnitEventHandlerQuestRefid() override { return refid; }
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	void LoadOld(GameReader& f);

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
