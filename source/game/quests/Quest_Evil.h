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
		VEC3 pos;
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
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	bool IfQuestEvent() const;
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit);
	int GetUnitEventHandlerQuestRefid() { return refid; }
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	int GetLocId(int location_id);

	Loc loc[3];
	int closed, mage_loc;
	bool changed, told_about_boss;
	State evil_state;
	VEC3 pos;
	float timer;
	Unit* cleric;

private:
	void GenerateBloodyAltar();
	void GeneratePortal();
	void WarpEvilBossToAltar();
};
