// efekty czarów, eksplozje, electro, drain
#pragma once

//-----------------------------------------------------------------------------
struct Explo
{
	Entity<Unit> owner;
	Vec3 pos;
	float size, sizemax, dmg;
	Ability* ability;
	vector<Entity<Unit>> hitted;

	static const int MIN_SIZE = 21;

	bool Update(float dt, LevelArea& area);
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
		TrailParticleEmitter* trail;
		Vec3 from, to;
		float t; // 0.0 -> 0.25 animation -> 0.5 removed

		static const int SIZE = sizeof(Vec3) * 2 + sizeof(float);
	};

	LevelArea* area;
	Ability* ability;
	Entity<Unit> owner;
	vector<Line> lines;
	vector<Entity<Unit>> hitted;
	float dmg;
	bool valid, hitsome;
	Vec3 startPos;

	static const int MIN_SIZE = 5;

	~Electro();
	void AddLine(const Vec3& from, const Vec3& to, float t = 0);
	bool Update(float dt);
	void UpdateColor(Line& line);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};

//-----------------------------------------------------------------------------
struct Drain
{
	Entity<Unit> target;
	ParticleEmitter* pe;
	float t;

	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f);
};
