#include "Pch.h"
#include "ParticleLoader.h"

#include "ParticleEffect.h"

#include <ResourceManager.h>

enum Group
{
	G_TOP,
	G_KEYWORD,
	G_OP,
	G_MODE
};

enum Keyword
{
	K_TEX,
	K_EMISSIONS,
	K_EMISSION_INTERVAL,
	K_LIFE,
	K_PARTICLE_LIFE,
	K_SPAWN_MIN,
	K_SPAWN_MAX,
	K_MAX_PARTICLES,
	K_SPEED_MIN,
	K_SPEED_MAX,
	K_POS_MIN,
	K_POS_MAX,
	K_SIZE,
	K_ALPHA,
	K_OP_SIZE,
	K_OP_ALPHA,
	K_MODE
};

//=================================================================================================
void ParticleLoader::DoLoading()
{
	Load("particles.txt", G_TOP);
}

void ParticleLoader::InitTokenizer()
{
	t.AddKeyword("effect", 0, G_TOP);

	t.AddKeywords(G_KEYWORD, {
		{ "tex", K_TEX },
		{ "emissions", K_EMISSIONS },
		{ "emissionInterval", K_EMISSION_INTERVAL },
		{ "life", K_LIFE },
		{ "particleLife", K_PARTICLE_LIFE },
		{ "spawnMin", K_SPAWN_MIN },
		{ "spawnMax", K_SPAWN_MAX },
		{ "maxParticles", K_MAX_PARTICLES },
		{ "speedMin", K_SPEED_MIN },
		{ "speedMax", K_SPEED_MAX },
		{ "posMin", K_POS_MIN },
		{ "posMax", K_POS_MAX },
		{ "size", K_SIZE },
		{ "alpha", K_ALPHA },
		{ "opSize", K_OP_SIZE },
		{ "opAlpha", K_OP_ALPHA} ,
		{ "mode", K_MODE }
		});

	t.AddKeywords(G_OP, {
		{ "const", POP_CONST },
		{ "linearShrink", POP_LINEAR_SHRINK }
		});

	t.AddKeywords(G_MODE, {
		{ "add", 0 },
		{ "addOne", 1 }
		});
}

void ParticleLoader::LoadEntity(int top, const string& id)
{
	const int hash = Hash(id);
	ParticleEffect* existingEffect = ParticleEffect::Get(hash);
	if(existingEffect)
	{
		if(existingEffect->id == id)
			t.Throw("Id must be unique.");
		else
			t.Throw("Id hash collision.");
	}

	Ptr<ParticleEffect> effect;
	effect->id = id;
	effect->hash = hash;

	t.AssertSymbol('{');
	t.Next();

	while(!t.IsSymbol('}'))
	{
		Keyword keyword = (Keyword)t.MustGetKeywordId(G_KEYWORD);
		t.Next();
		switch(keyword)
		{
		case K_TEX:
			{
				const string& texId = t.MustGetString();
				effect->tex = resMgr->Get<Texture>(texId);
			}
			break;
		case K_EMISSIONS:
			effect->emissions = t.MustGetInt();
			break;
		case K_EMISSION_INTERVAL:
			effect->emissionInterval = t.MustGetFloat();
			break;
		case K_LIFE:
			effect->life = t.MustGetFloat();
			break;
		case K_PARTICLE_LIFE:
			effect->particleLife = t.MustGetFloat();
			break;
		case K_SPAWN_MIN:
			effect->spawnMin = t.MustGetInt();
			break;
		case K_SPAWN_MAX:
			effect->spawnMax = t.MustGetInt();
			break;
		case K_MAX_PARTICLES:
			effect->maxParticles = t.MustGetInt();
			break;
		case K_SPEED_MIN:
			t.Parse(effect->speedMin);
			break;
		case K_SPEED_MAX:
			t.Parse(effect->speedMax);
			break;
		case K_POS_MIN:
			t.Parse(effect->posMin);
			break;
		case K_POS_MAX:
			t.Parse(effect->posMax);
			break;
		case K_SIZE:
			effect->size = t.MustGetFloat();
			break;
		case K_ALPHA:
			effect->alpha = t.MustGetFloat();
			break;
		case K_OP_SIZE:
			effect->opSize = (PARTICLE_OP)t.MustGetKeywordId(G_OP);
			break;
		case K_OP_ALPHA:
			effect->opAlpha = (PARTICLE_OP)t.MustGetKeywordId(G_OP);
			break;
		case K_MODE:
			effect->mode = t.MustGetKeywordId(G_MODE);
			break;
		}
		t.Next();
	}

	ParticleEffect::hashEffects[hash] = effect;
	ParticleEffect::effects.push_back(effect.Pin());
}
