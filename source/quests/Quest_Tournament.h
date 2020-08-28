#pragma once

//-----------------------------------------------------------------------------
#include "QuestHandler.h"

//-----------------------------------------------------------------------------
// Arena tournament that is hold once per year
class Quest_Tournament : public QuestHandler
{
public:
	enum State
	{
		TOURNAMENT_NOT_DONE,
		TOURNAMENT_STARTING,
		TOURNAMENT_IN_PROGRESS
	};

	void InitOnce();
	void LoadLanguage();
	void Init();
	void Clear();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	cstring FormatString(const string& str) override;
	void OnProgress();
	void Update(float dt);
	bool IsGenerated() const { return generated; }
	Unit* GetMaster() const { return master; }
	State GetState() const { return state; }
	int GetCity() const { return city; }
	void Train(PlayerController& player);
	void GenerateUnits();
	void Clean();
	void FinishCombat() { state3 = 5; }

private:
	UnitData& GetRandomHeroData();
	void StartTournament(Unit* arena_master);
	bool ShouldJoin(Unit& u);
	void VerifyUnit(Unit* unit);
	void StartRound();
	void Talk(cstring text);

	State state;
	int year, city_year, city, state2, state3, round;
	vector<SmartPtr<Unit>> units;
	float timer;
	InsideBuilding* arena;
	Unit* master;
	SmartPtr<Unit> skipped_unit, other_fighter, winner;
	vector<pair<SmartPtr<Unit>, SmartPtr<Unit>>> pairs;
	bool generated;
	cstring txTour[24], txAiJoinTour[4];
};
