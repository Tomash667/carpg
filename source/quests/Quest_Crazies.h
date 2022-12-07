#pragma once

//-----------------------------------------------------------------------------
#include "Quest.h"

//-----------------------------------------------------------------------------
// After asking madman why he is crazy will cause another madman encounter.
// He will have cursed stone in inventory that will always appear in player inventory.
// After encountering unknown enemies player can ask trainer about it.
// Deliverying cursed stone to labyrinth will end curse.
class Quest_Crazies final : public Quest_Dungeon
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

	void Init() override;
	void Start() override;
	GameDialog* GetDialog(int type2) override;
	void SetProgress(int prog2) override;
	cstring FormatString(const string& str) override;
	bool IfNeedTalk(cstring topic) const override;
	void Save(GameWriter& f) override;
	LoadResult Load(GameReader& f) override;
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	void CheckStone();
	void OnProgress(int days);
	void OnEncounter(EncounterSpawn& spawn);

	State craziesState;
	bool checkStone;

private:
	const Item* stone;
	int days;
};
