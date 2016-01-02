#pragma once

//-----------------------------------------------------------------------------
struct Particle
{
	VEC3 pos, speed;
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
	TextureResourcePtr tex;
	float emision_interval, life, particle_life, alpha, size;
	int emisions, spawn_min, spawn_max, max_particles, mode;
	VEC3 pos, speed_min, speed_max, pos_min, pos_max;
	PARTICLE_OP op_size, op_alpha;

	// nowe wartoœci, dla kompatybilnoœci zerowane w Init
	int manual_delete;

	// automatycznie ustawiane
	float time, radius;
	vector<Particle> particles;
	int alive, refid;
	bool destroy;

	inline float GetAlpha(const Particle &p) const
	{
		if(op_alpha == POP_CONST)
			return alpha;
		else
			return lerp(0.f,alpha,p.life/particle_life);
	}

	inline float GetScale(const Particle &p) const
	{
		if(op_size == POP_CONST)
			return size;
		else
			return lerp(0.f,size,p.life/particle_life);
	}

	void Init();
	void Save(HANDLE file);
	void Load(HANDLE file);

	static inline ParticleEmitter* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}
	static vector<ParticleEmitter*> refid_table;

	static inline void AddRefid(ParticleEmitter* pe)
	{
		assert(pe);
		pe->refid = (int)refid_table.size();
		refid_table.push_back(pe);
	}
};

//-----------------------------------------------------------------------------
struct TrailParticle
{
	VEC3 pt1, pt2;
	float t;
	int next;
	bool exists;
};

//-----------------------------------------------------------------------------
struct TrailParticleEmitter
{
	void Init(int maxp);
	bool Update(float dt, VEC3* pt1, VEC3* pt2);

	float fade, timer;
	VEC4 color1, color2;

	//private:
	vector<TrailParticle> parts;
	int first, last, alive, refid;
	bool destroy;

	void Save(HANDLE file);
	void Load(HANDLE file);

	inline static TrailParticleEmitter* GetByRefid(int _refid)
	{
		if(_refid == -1 || _refid >= (int)refid_table.size())
			return nullptr;
		else
			return refid_table[_refid];
	}
	static vector<TrailParticleEmitter*> refid_table;

	static inline void AddRefid(TrailParticleEmitter* pe)
	{
		assert(pe);
		pe->refid = (int)refid_table.size();
		refid_table.push_back(pe);
	}
};
