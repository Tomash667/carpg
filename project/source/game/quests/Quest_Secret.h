#pragma once

#include "QuestHandler.h"

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

	SecretState state;
	int where, where2;
};
