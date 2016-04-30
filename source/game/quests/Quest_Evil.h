#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// zleca kap³an w losowym miejscu
// trzeba iœæ do podziemi zabiæ nekromantê ale go tam nie ma, jest zakrwawiony o³tarz, gdy siê podejdzie pojawiaj¹ siê nieumarli;
// potem trzeba zameldowaæ o postêpach, wysy³a nas do jakiegoœ maga po ksiêgê, mag mówi ¿e stra¿nicy j¹ zarewirowali, kapitan odsy³a do burmistrza
// potem do kapitana i j¹ daje, wracamy do kap³ana, wysy³a nas do podziemi na koñcu których jest portal do innych podziemi, na koñcu jest boss
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
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	inline int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	inline int GetLocId(int location_id)
	{
		for(int i = 0; i<3; ++i)
		{
			if(loc[i].target_loc == location_id)
				return i;
		}
		return -1;
	}

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
