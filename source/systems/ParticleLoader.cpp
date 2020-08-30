#include "Pch.h"
#include "ParticleLoader.h"

#include "ParticleEmitterModel.h"

#include <ParticleSystem.h>

enum KeywordGroup
{
	G_TOP,
	G_PROPERTY,
	G_MODE
};

enum TopKeyword
{
	T_PARTICLE
};

enum Property
{
	P_TEXTURE,
	P_LIFE,
	P_EMISSIONS,
	P_EMISSION_INTERVAL,
	P_SPAWN_MIN,
	P_SPAWN_MAX,
	P_MAX_PARTICLES,
	P_PARTICLE_LIFE,
	P_SIZE,
	P_ALPHA,
	P_SPEED_MIN,
	P_SPEED_MAX,
	P_POS_MIN,
	P_POS_MAX,
	P_OP_SIZE,
	P_OP_ALPHA,
	P_MODE
};

void ParticleLoader::DoLoading()
{
	Load("particles.txt", G_TOP);
}

void ParticleLoader::InitTokenizer()
{
	t.AddKeyword("particle", T_PARTICLE, G_TOP);

	t.AddKeywords(G_PROPERTY, {
		{ "texture", P_TEXTURE },
		{ "life", P_LIFE },
		{ "emissions", P_EMISSIONS },
		{ "emission_interval", P_EMISSION_INTERVAL },
		{ "spawn_min", P_SPAWN_MIN },
		{ "spawn_max", P_SPAWN_MAX },
		{ "max_particles", P_MAX_PARTICLES },
		{ "particle_life", P_PARTICLE_LIFE },
		{ "size", P_SIZE },
		{ "alpha", P_ALPHA },
		{ "speed_min", P_SPEED_MIN },
		{ "speed_max", P_SPEED_MAX },
		{ "pos_min", P_POS_MIN },
		{ "pos_max", P_POS_MAX },
		{ "op_size", P_OP_SIZE },
		{ "op_alpha", P_OP_ALPHA },
		{ "mode", P_MODE }
		});
}

void ParticleLoader::LoadEntity(int top, const string& id)
{
	ParticleEmitterModel* existing = ParticleEmitterModel::TryGet(id);
	if(existing)
		t.Throw("Id hash collision.");

	Ptr<ParticleEmitter> particle;
	particle->tex = nullptr;
	particle->life = -1;
	particle->emissions = -1;
	particle->emission_interval = 1;
	particle->spawn_min = 1;
	particle->spawn_max = 1;
	particle->max_particles = 1;
	particle->particle_life = 1;
	particle->size = 1;
	particle->alpha = 1;
	particle->speed_min = Vec3::Zero;
	particle->speed_max = Vec3::Zero;
	particle->pos_min = Vec3::Zero;
	particle->pos_max = Vec3::Zero;
	particle->op_alpha = ParticleEmitter::POP_CONST;
	particle->op_size = ParticleEmitter::POP_CONST;
	particle->mode = 0;
	string particleId = id;
	crc.Update(id);
	t.Next();

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Property prop = (Property)t.MustGetKeywordId(G_PROPERTY);
		t.Next();

		switch(prop)
		{
		case P_TEXTURE:
		case P_LIFE:
		case P_EMISSIONS:
		case P_EMISSION_INTERVAL:
		case P_SPAWN_MIN:
		case P_SPAWN_MAX:
		case P_MAX_PARTICLES:
		case P_PARTICLE_LIFE:
		case P_SIZE:
		case P_ALPHA:
		case P_SPEED_MIN:
		case P_SPEED_MAX:
		case P_POS_MIN:
		case P_POS_MAX:
		case P_OP_SIZE:
		case P_OP_ALPHA:
		case P_MODE:
		}
	}



	if(!particle->tex)
	{
	}
}
