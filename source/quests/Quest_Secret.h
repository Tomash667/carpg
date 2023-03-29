#pragma once

//-----------------------------------------------------------------------------
#include "QuestHandler.h"
#include "Item.h"

//-----------------------------------------------------------------------------
// Throw moonstone to moonwell to get info about hard dungeon
// At end of dungeon there is portal to secret location (hardcore only)
class Quest_Secret : public QuestHandler
{
public:
	enum SecretState
	{
		SECRET_OFF,
		SECRET_NONE,
		SECRET_DROPPED_STONE,
		SECRET_GENERATED,
		SECRET_CLOSED,
		SECRET_GENERATED2,
		SECRET_TALKED,
		SECRET_FIGHT,
		SECRET_LOST,
		SECRET_WIN,
		SECRET_REWARD
	};

	void InitOnce();
	void LoadLanguage();
	void Init();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	bool Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool CheckMoonStone(GroundItem* item, Unit& unit);
	static Item& GetNote() { return const_cast<Item&>(*Item::qSecretNote); }
	void UpdateFight();

	SecretState state;
	int where, where2;

private:
	cstring txSecretAppear;
};
