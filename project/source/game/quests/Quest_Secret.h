#pragma once

class Quest_Secret
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

	void Init();
	void Save(GameWriter& f);
	void Load(GameReader& f);

	SecretState state;
	int where, where2;
};
