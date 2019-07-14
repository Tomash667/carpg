#pragma once

//-----------------------------------------------------------------------------
struct Particle
{
	Vec3 pos, speed;
	float life, gravity;
	bool exists;
};

//-----------------------------------------------------------------------------
enum PARTICLE_OP
{
	POP_CONST,
	POP_LINEAR_SHRINK
};

//-----------------------------------------------------------------------------
struct ParticleEmitter
{
	TexturePtr tex;
	float emision_interval, life, particle_life, alpha, size;
	int emisions, spawn_min, spawn_max, max_particles, mode;
	Vec3 pos, speed_min, speed_max, pos_min, pos_max;
	PARTICLE_OP op_size, op_alpha;

	// nowe wartoœci, dla kompatybilnoœci zerowane w Init
	int manual_delete;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive, refid;
	bool destroy;

	void Init();
	bool Update(float dt);
	void Save(FileWriter& f);
	void Load(FileReader& f);
	float GetAlpha(const Particle &p) const
	{
		if(op_alpha == POP_CONST)
			return alpha;
		else
			return Lerp(0.f, alpha, p.life / particle_life);
	}

	float GetScale(const Particle &p) const
	{
		if(op_size == POP_CONST)
			return size;
		else
			return Lerp(0.f, size, p.life / particle_life);
	}

	static vector<ParticleEmitter*> refid_table;
	static ParticleEmitter* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}
	static void AddRefid(ParticleEmitter* pe)
	{
		assert(pe);
		pe->refid = (int)refid_table.size();
		refid_table.push_back(pe);
	}
};

//-----------------------------------------------------------------------------
struct TrailParticle
{
	Vec3 pt1, pt2;
	float t;
	int next;
	bool exists;
};

//-----------------------------------------------------------------------------
struct TrailParticleEmitter
{
	float fade, timer;
	Vec4 color1, color2;
	vector<TrailParticle> parts;
	int first, last, alive, refid;
	bool destroy;

	void Init(int maxp);
	bool Update(float dt, Vec3* pt1, Vec3* pt2);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	static vector<TrailParticleEmitter*> refid_table;
	static TrailParticleEmitter* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}
	static void AddRefid(TrailParticleEmitter* pe)
	{
		assert(pe);
		pe->refid = (int)refid_table.size();
		refid_table.push_back(pe);
	}
};
