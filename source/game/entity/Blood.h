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

	void Save(File& f) const;
	void Load(File& f);
	void Write(BitStream& s) const;
	bool Read(BitStream& s);
};
