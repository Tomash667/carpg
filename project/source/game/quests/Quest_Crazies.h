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

	void Init();
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(GameWriter& f) override;
	bool Load(GameReader& f) override;
	void LoadOld(GameReader& f);
	void Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void CheckStone();

	State crazies_state;
	int days;
	bool check_stone;
};
