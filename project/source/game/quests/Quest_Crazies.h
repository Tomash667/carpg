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

	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(HANDLE file) override;
	bool Load(HANDLE file) override;
	void LoadOld(HANDLE file);

	State crazies_state;
	int days;
	bool check_stone;
};
