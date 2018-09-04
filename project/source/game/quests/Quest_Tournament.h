#pragma once

class Quest_Tournament
{
public:
	enum TOURNAMENT_STATE
	{
		TOURNAMENT_NOT_DONE,
		TOURNAMENT_STARTING,
		TOURNAMENT_IN_PROGRESS
	};

	void Init();
	bool HaveJoined(Unit* unit);
	void Save(GameWriter& f);
	void Load(GameReader& f);

	TOURNAMENT_STATE state;
	int year, city_year, city, state2, state3, round, arena;
	vector<SmartPtr<Unit>> units;
	float timer;
	Unit* master;
	SmartPtr<Unit> skipped_unit, other_fighter, winner;
	vector<std::pair<SmartPtr<Unit>, SmartPtr<Unit>>> pairs;
	bool generated;
};
