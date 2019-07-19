// dane cz³owieka
#pragma once

//-----------------------------------------------------------------------------
// Iloœæ bród, w³osów i w¹sów
static const int MAX_BEARD = 6;
static const int MAX_HAIR = 6;
static const int MAX_MUSTACHE = 3;

//-----------------------------------------------------------------------------
// Ustawienia brodow¹sów
extern bool g_beard_and_mustache[MAX_BEARD - 1];

//-----------------------------------------------------------------------------
// Dostêpne kolory w³osów
extern const Vec4 g_hair_colors[];
extern const uint n_hair_colors;

//-----------------------------------------------------------------------------
// Dane cz³owieka
struct Human
{
	int hair, beard, mustache;
	Vec4 hair_color;
	float height; // 0...2
	vector<Matrix> mat_scale;

	Vec2 GetScale();
	void ApplyScale(Mesh* mesh);
	void Save(FileWriter& f);
	void Load(FileReader& f);
};

//-----------------------------------------------------------------------------
// Jak Human ale bez macierzy, u¿ywane do zapamiêtania jak wygl¹da³a jakaœ postaæ
struct HumanData
{
	int hair, beard, mustache;
	Vec4 hair_color;
	float height;

	void Get(const Human& h);
	void Set(Human& h) const;
	void CopyFrom(HumanData& hd);
	void Save(FileWriter& f) const;
	void Load(FileReader& f);
	void Write(BitStreamWriter& f) const;
	// 0 - ok, 1 - ready error, 2 - value error
	int Read(BitStreamReader& f);
	void Random();
};
