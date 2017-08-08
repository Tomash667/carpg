// dane cz�owieka
#pragma once

//-----------------------------------------------------------------------------
// Ilo�� br�d, w�os�w i w�s�w
#define MAX_BEARD 6
#define MAX_HAIR 6
#define MAX_MUSTACHE 3

//-----------------------------------------------------------------------------
// Ustawienia brodow�s�w
extern bool g_beard_and_mustache[MAX_BEARD - 1];

//-----------------------------------------------------------------------------
// Dost�pne kolory w�os�w
extern const Vec4 g_hair_colors[];
extern const uint n_hair_colors;

//-----------------------------------------------------------------------------
// Dane cz�owieka
struct Human
{
	int hair, beard, mustache;
	Vec4 hair_color;
	float height; // 0...2
	vector<Matrix> mat_scale;

	Vec2 GetScale();
	void ApplyScale(Mesh* mesh);
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
// Jak Human ale bez macierzy, u�ywane do zapami�tania jak wygl�da�a jaka� posta�
struct HumanData
{
	int hair, beard, mustache;
	Vec4 hair_color;
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
