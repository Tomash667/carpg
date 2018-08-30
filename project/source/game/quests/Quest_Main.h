#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
/*
1. start in city with messagebox
2. go & talk with city mayor
3. this will reveal location near other city
4. trying enter there shows messagebox that it's incomplete
*/
class Quest_Main : public Quest
{
public:
	enum Progress
	{
		Started,
		TalkedWithMayor
	};

	int target_loc, close_loc;
	float timer;

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
};
