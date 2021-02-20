#pragma once

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
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);

private:
	void UpdateColor(Line& line);
	Unit* FindNextTarget();
};
