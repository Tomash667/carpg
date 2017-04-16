#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"
#include "UnitEventHandler.h"

//-----------------------------------------------------------------------------
// quest gobliny
// "Odzyskaj antyczny �uk" - jest w lesie, szlachcic uciek� przed wilkami
// po zdobyciu go na drodze napadaj� gobliny i go kradn�
// wraca si� do szlachcica i on to komentuje
// po jakim� czasie przychodzi pos�aniec i m�wi w kt�rych podziemiach s� gobliny
// jest tam �uk, zanosimy go a szlachcic m�wi �e to nie ten ale kupuje go za 500 sz
// potem spotykamy w�drownego maga i pyta czy mamy �uk, bo jest na nim jaka� iluzja
// gdy wracamy to szlachcica nie ma, mo�na zapyta� karczmarza
// m�wi �e poszed� �miej�c si� z naiwnego gracza, do swojego fortu
// w forcie s� gobliny i na ko�cu szlachcic i dw�ch stra�nik�w
// zabija si� go i koniec
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
	int GetUnitEventHandlerQuestRefid() { return refid; }
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
