#pragma once

struct ParticleEffect
{
	string id;
	TexturePtr tex;
	Vec3 speedMin, speedMax, posMin, posMax;
	Vec2 alpha, size;
	Int2 spawn;
	float emissionInterval, life, particleLife;
	int emissions, maxParticles, mode;
	bool gravity;

	ParticleEffect() : mode(0), gravity(true) {}
	void Apply(ParticleEmitter* pe);
};
