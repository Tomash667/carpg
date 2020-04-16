#pragma once

//-----------------------------------------------------------------------------
// Count of beard, hair & mustache types
static const int MAX_BEARD = 6;
static const int MAX_HAIR = 6;
static const int MAX_MUSTACHE = 3;

//-----------------------------------------------------------------------------
// Settings for beard & mustache combination
extern bool g_beard_and_mustache[MAX_BEARD - 1];

//-----------------------------------------------------------------------------
// Allowed normal hair colors
extern const Vec4 g_hair_colors[];
extern const uint n_hair_colors;

//-----------------------------------------------------------------------------
// Human data
struct Human
{
	int hair, beard, mustache;
	Vec4 hair_color;
	float height; // 0...2
	vector<Matrix> mat_scale;

	Vec2 GetScale();
	void ApplyScale(MeshInstance* mesh_inst);
	void Save(FileWriter& f);
	void Load(FileReader& f);
};

//-----------------------------------------------------------------------------
// Like Human but without matrices
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
