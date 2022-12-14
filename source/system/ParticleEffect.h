#pragma once

enum PARTICLE_OP
{
	POP_CONST,
	POP_LINEAR_SHRINK
};

struct ParticleEffect
{
	string id;
	TexturePtr tex;
	Vec3 pos, speedMin, speedMax, posMin, posMax;
	float emissionInterval, life, particleLife, alpha, size;
	int hash, emissions, spawnMin, spawnMax, maxParticles, mode;
	PARTICLE_OP opSize, opAlpha;

	static vector<ParticleEffect*> effects;
	static std::map<int, ParticleEffect*> hashEffects;
	static ParticleEffect* Get(int hash);
};
