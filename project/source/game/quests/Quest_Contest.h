#pragma once

class Quest_Contest
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

	void Init();
	void Start(PlayerController* pc);
	void AddPlayer(PlayerController* pc);
	bool HaveJoined(Unit* unit);
	void Save(GameWriter& f);
	void Load(GameReader& f);

	State state;
	int where, state2;
	vector<Unit*> units;
	Unit* winner;
	float time;
	bool generated;
};
