#pragma once

//-----------------------------------------------------------------------------
// Count of beard, hair & mustache types
static const int MAX_BEARD = 6;
static const int MAX_HAIR = 6;
static const int MAX_MUSTACHE = 3;
static const float MIN_HEIGHT = 0.9f;
static const float MAX_HEIGHT = 1.1f;

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
	float height; // 0.9...1.1
	vector<Matrix> mat_scale;

	Vec2 GetScale();
	void ApplyScale(MeshInstance* meshInst);
	void Init(const HumanData* hd);
	void Save(GameWriter& f);
	void Load(GameReader& f);
};

//-----------------------------------------------------------------------------
// Like Human but without matrices
struct HumanData
{
	enum class HairColorType
	{
		Default,
		Fixed,
		Grayscale,
		Random
	};

	enum Flags
	{
		F_HAIR = 1 << 0,
		F_BEARD = 1 << 1,
		F_MUSTACHE = 1 << 2,
		F_HAIR_COLOR = 1 << 3,
		F_HEIGHT = 1 << 4
	};

	HairColorType hair_type;
	int hair, beard, mustache, defaultFlags;
	Vec4 hair_color;
	float height;

	void Get(const Human& h);
	void Set(Human& h) const;
	void CopyFrom(HumanData& hd);
	void Save(GameWriter& f) const;
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	// 0 - ok, 1 - ready error, 2 - value error
	int Read(BitStreamReader& f);
	void Random();
};

//-----------------------------------------------------------------------------
inline void operator << (GameWriter& f, const HumanData& hd)
{
	hd.Save(f);
}
inline void operator >> (GameReader& f, HumanData& hd)
{
	hd.Load(f);
}
