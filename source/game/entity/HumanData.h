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

	void ApplyScale(Animesh* ani);
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

	void Get(const Human& h)
	{
		hair = h.hair;
		beard = h.beard;
		mustache = h.mustache;
		hair_color = h.hair_color;
		height = h.height;
	}

	void Set(Human& h) const
	{
		h.hair = hair;
		h.beard = beard;
		h.mustache = mustache;
		h.hair_color = hair_color;
		h.height = height;
	}

	void CopyFrom(HumanData& hd)
	{
		hair = hd.hair;
		beard = hd.beard;
		mustache = hd.mustache;
		hair_color = hd.hair_color;
		height = hd.height;
	}

	void Save(HANDLE file);
	void Load(HANDLE file);
};
