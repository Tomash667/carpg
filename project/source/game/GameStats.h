#pragma once

struct GameStats : public Singleton<GameStats>
{
	int total_kills;
	// time spent playing on current save
	int hour, minute, second;
	float tick;

	void Reset();
	void Update(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void LoadOld(GameReader& f, int part);
	void Write(BitStreamWriter& f);
	void Read(BitStreamReader& f);
};
