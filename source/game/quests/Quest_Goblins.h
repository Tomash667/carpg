#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// quest gobliny
// "Odzyskaj antyczny ³uk" - jest w lesie, szlachcic uciek³ przed wilkami
// po zdobyciu go na drodze napadaj¹ gobliny i go kradn¹
// wraca siê do szlachcica i on to komentuje
// po jakimœ czasie przychodzi pos³aniec i mówi w których podziemiach s¹ gobliny
// jest tam ³uk, zanosimy go a szlachcic mówi ¿e to nie ten ale kupuje go za 500 sz
// potem spotykamy wêdrownego maga i pyta czy mamy ³uk, bo jest na nim jakaœ iluzja
// gdy wracamy to szlachcica nie ma, mo¿na zapytaæ karczmarza
// mówi ¿e poszed³ œmiej¹c siê z naiwnego gracza, do swojego fortu
// w forcie s¹ gobliny i na koñcu szlachcic i dwóch stra¿ników
// zabija siê go i koniec
class Quest_Goblins : public Quest_Dungeon, public UnitEventHandler
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

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	inline int GetUnitEventHandlerQuestRefid()
	{
		return refid;
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	State goblins_state;
	int days;
	Unit* nobleman, *messenger;
	HumanData hd_nobleman;

private:
	int enc;
};
