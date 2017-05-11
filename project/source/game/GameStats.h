#pragma once

struct GameStats : public Singleton<GameStats>
{
	// time spent playing on current save
	int hour, minute, second;
	float tick;

	void Reset();
	void Update(float dt);
	void Write(BitStream& stream);
	bool Read(BitStream& stream);
	void Save(HANDLE file);
	void Load(HANDLE file);
};
