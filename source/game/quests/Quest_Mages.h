#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// jaki� facet chce �eby odnale�� dla niego magiczny artefakt
// idziemy do krypty i go przynosimy
// p�aci i znika jak pos�aniec
// po jakim� czasie natrafiamy na golema na drodze kt�ry ka�e nam odda� ca�e z�oto, dostajemy questa "Golem na drodze"
// mo�emy porozmawia� z dowolnym kapitanem stra�y kt�ry m�wi o tym �e ma wi�cej informacji o golemach bududowanych przez mag�w
// wysy�a nas do jakiego� maga w wiosce
// jest pijany i chce piwo, potem w�dk�, potem prowadzi nas do jakich� podziemi w kt�rych nic nie ma
// wracamy do kapitana kt�ry nam to zleci�
// ka�e nam przynie�� miksturk� oczyszczenia umys�u (mo�na kupi� u alchemika za 100 z�ota)
// dajemy j� magowie [m�wi �e sobie wszystko przypomnia� i nienawidzi piwa]
// informuje nas o tym �e jaki� poszukiwacz przyg�d pom�g� jednemu z mag�w znale�� kule wi�zi potrzebn� do budowy golem�w
// m�wi nam gdzie trzeba i�� zabi� maga
// wracamy do kapitana, daje nagrod�
class Quest_Mages : public Quest_Dungeon
{
public:
	enum Progress
	{
		None,
		Started,
		Finished,
		EncounteredGolem
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	void Special(DialogContext& ctx, cstring msg);
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
// start_loc = location with guard captain
// mage_loc = location with drunk mage
// target_loc = drunk mage tower, evil mage tower
class Quest_Mages2 : public Quest_Dungeon, public UnitEventHandler
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

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	bool IfSpecial(DialogContext& ctx, cstring msg);
	void HandleUnitEvent(UnitEventHandler::TYPE event_type, Unit* unit);
	int GetUnitEventHandlerQuestRefid() { return refid; }
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	Talked talked;
	State mages_state;
	Unit* scholar;
	string evil_mage_name, good_mage_name;
	HumanData hd_mage;
	int mage_loc, days;
	float timer;
	bool paid;
};
