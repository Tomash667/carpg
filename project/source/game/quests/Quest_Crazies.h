#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
class Quest_Crazies : public Quest_Dungeon
{
public:
	enum Progress
	{
		Started,
		KnowLocation,
		Finished
	};

	enum class State
	{
		None,
		TalkedWithCrazy,
		PickedStone,
		FirstAttack,
		TalkedTrainer,
		End
	};

	void Start();
	GameDialog* GetDialog(int type2);
	void SetProgress(int prog2);
	cstring FormatString(const string& str);
	bool IfNeedTalk(cstring topic) const;
	void Save(HANDLE file);
	void Load(HANDLE file);
	void LoadOld(HANDLE file);

	State crazies_state;
	int days;
	bool check_stone;
};
