#pragma once

//-----------------------------------------------------------------------------
// Rodzaj krwi postaci
enum BLOOD
{
	BLOOD_RED,
	BLOOD_GREEN,
	BLOOD_BLACK,
	BLOOD_BONE,
	BLOOD_ROCK,
	BLOOD_IRON,
	BLOOD_MAX
};

//-----------------------------------------------------------------------------
// Krew na ziemi
struct Blood
{
	BLOOD type;
	VEC3 pos, normal;
	float size, rot;
	int lights;

	static const int MIN_SIZE = 29;

	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
};
