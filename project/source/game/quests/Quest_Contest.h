#pragma once

#include "QuestHandler.h"

class Quest_Contest : public QuestHandler
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
	void Init();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	cstring FormatString(const string& str) override;

	State state;
	int where, state2, year;
	vector<Unit*> units;
	Unit* winner;
	float time;
	bool generated;
};
