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

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	void Save(HANDLE file);
	void Load(HANDLE file);
};
