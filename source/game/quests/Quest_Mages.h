#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// jakiœ facet chce ¿eby odnaleŸæ dla niego magiczny artefakt
// idziemy do krypty i go przynosimy
// p³aci i znika jak pos³aniec
// po jakimœ czasie natrafiamy na golema na drodze który ka¿e nam oddaæ ca³e z³oto, dostajemy questa "Golem na drodze"
// mo¿emy porozmawiaæ z dowolnym kapitanem stra¿y który mówi o tym ¿e ma wiêcej informacji o golemach bududowanych przez magów
// wysy³a nas do jakiegoœ maga w wiosce
// jest pijany i chce piwo, potem wódkê, potem prowadzi nas do jakichœ podziemi w których nic nie ma
// wracamy do kapitana który nam to zleci³
// ka¿e nam przynieœæ miksturkê oczyszczenia umys³u (mo¿na kupiæ u alchemika za 100 z³ota)
// dajemy j¹ magowie [mówi ¿e sobie wszystko przypomnia³ i nienawidzi piwa]
// informuje nas o tym ¿e jakiœ poszukiwacz przygód pomóg³ jednemu z magów znaleŸæ kule wiêzi potrzebn¹ do budowy golemów
// mówi nam gdzie trzeba iœæ zabiæ maga
// wracamy do kapitana, daje nagrodê
class Quest_Magowie : public Quest_Dungeon
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
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
class Quest_Mages : public Quest_Dungeon, public UnitEventHandler
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

	enum Talked
	{
		No,
		AboutHisTower,
		AfterEnter,
		BeforeBoss
	};
	//0-nie powiedzia³, 1-pogada³ w jego wie¿y, 2-pogada³ po wejœciu, 3-pogada³ przed bossem

	void Start();
	DialogEntry* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic);
	void HandleUnitEvent(UnitEventHandler::TYPE type, Unit* unit);
	void Save(HANDLE file);
	void Load(HANDLE file);
	int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}

	Talked talked;

private:
	int mage_loc;
};
