#pragma once

#include "QuestHandler.h"
#include "Item.h"

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
	void Init();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Special(DialogContext& ctx, cstring msg) override;
	bool SpecialIf(DialogContext& ctx, cstring msg) override;
	bool CheckMoonStone(GroundItem* item, Unit& unit);
	static Item& GetNote() { return *Item::Get("sekret_kartka"); }

	SecretState state;
	int where, where2;

private:
	cstring txSecretAppear;
};
