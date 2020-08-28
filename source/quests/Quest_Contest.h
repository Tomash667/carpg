#pragma once

//-----------------------------------------------------------------------------
#include "UnitEventHandler.h"
#include "QuestHandler.h"

//-----------------------------------------------------------------------------
// Once per year drinking contest
class Quest_Contest : public QuestHandler, public UnitEventHandler
{
public:
	enum State
	{
		CONTEST_NOT_DONE,
		CONTEST_DONE,
		CONTEST_TODAY,
		CONTEST_STARTING,
		CONTEST_IN_PROGRESS,
		CONTEST_FINISH
	};

	void InitOnce();
	void LoadLanguage();
	void Init();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	cstring FormatString(const string& str) override;
	void OnProgress();
	void Update(float dt);
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	int GetUnitEventHandlerQuestId() override { return -2; } // special value hack
	void Cleanup();
	void SpawnDrunkmans();

	State state;
	int where, state2, year, rumor;
	vector<Unit*> units;
	Unit* winner;
	float time;
	bool generated;

private:
	cstring txContestNoWinner, txContestStart, txContestTalk[14], txContestWin, txContestWinNews, txContestDraw, txContestPrize, txContestNoPeople;
};
