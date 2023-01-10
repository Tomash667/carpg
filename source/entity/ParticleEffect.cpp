#include "Pch.h"
#include "ParticleEffect.h"

#include <ParticleSystem.h>

void ParticleEffect::Apply(ParticleEmitter* pe)
{
	pe->tex = tex;
	pe->speedMin = speedMin;
	pe->speedMax = speedMax;
	pe->posMin = posMin;
	pe->posMax = posMax;
	pe->alpha = alpha;
	pe->size = size;
	pe->spawn = spawn;
	pe->emissionInterval = emissionInterval;
	pe->life = life;
	pe->particleLife = particleLife;
	pe->emissions = emissions;
	pe->maxParticles = maxParticles;
	pe->mode = mode;
	pe->gravity = gravity;
}
