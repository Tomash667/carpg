// dane cz³owieka
#pragma once

//-----------------------------------------------------------------------------
// Iloœæ bród, w³osów i w¹sów
#define MAX_BEARD 6
#define MAX_HAIR 6
#define MAX_MUSTACHE 3

//-----------------------------------------------------------------------------
// Ustawienia brodow¹sów
extern bool g_beard_and_mustache[MAX_BEARD-1];

//-----------------------------------------------------------------------------
// Dostêpne kolory w³osów
extern const VEC4 g_hair_colors[];
extern const uint n_hair_colors;

//-----------------------------------------------------------------------------
// Dane cz³owieka
struct Human
{
	int hair, beard, mustache;
	VEC4 hair_color;
	float height; // 0...2
	vector<MATRIX> mat_scale;

	VEC2 GetScale();
	void ApplyScale(Animesh* mesh);
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
// Jak Human ale bez macierzy, u¿ywane do zapamiêtania jak wygl¹da³a jakaœ postaæ
struct HumanData
{
	int hair, beard, mustache;
	VEC4 hair_color;
	float height;

	void Get(const Human& h);
	void Set(Human& h) const;
	void CopyFrom(HumanData& hd);
	void Save(HANDLE file) const;
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	// 0 - ok, 1 - ready error, 2 - value error
	int Read(BitStream& stream);
	void Random();
};
