// efekty czarów, eksplozje, electro, drain
#pragma once

//-----------------------------------------------------------------------------
struct Explo
{
	Unit* owner;
	Vec3 pos;
	float size, sizemax, dmg;
	TexturePtr tex;
	vector<Unit*> hitted;

	static const int MIN_SIZE = 21;

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};

//-----------------------------------------------------------------------------
struct Electro : public EntityType<Electro>
{
	struct Line
	{
		vector<Vec3> pts;
		float t;
	};

	Spell* spell;
	Entity<Unit> owner;
	vector<Line> lines;
	vector<Entity<Unit>> hitted;
	float dmg;
	bool valid, hitsome;
	Vec3 start_pos;

	static const int MIN_SIZE = 5;
	static const int LINE_MIN_SIZE = 28;

	void AddLine(const Vec3& from, const Vec3& to);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};

//-----------------------------------------------------------------------------
struct Drain
{
	Unit* from, *to;
	ParticleEmitter* pe;
	float t;

	void Save(FileWriter& f);
	void Load(FileReader& f);
};
