#pragma once

//-----------------------------------------------------------------------------
class GameStats
{
public:
	int total_kills;
	// time spent playing on current save
	int hour, minute, second;
	float tick;

	void Reset();
	void Update(float dt);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	void Read(BitStreamReader& f);
};
