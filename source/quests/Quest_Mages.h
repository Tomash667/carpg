#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

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
	bool Load(GameReader& f) override;
};

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
	bool Load(GameReader& f) override;
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
