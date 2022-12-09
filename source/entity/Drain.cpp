#include "Pch.h"
#include "Drain.h"

#include "Unit.h"

#include <ParticleSystem.h>

//=================================================================================================
bool Drain::Update(float dt)
{
	t += dt;

	if(pe->manualDelete == 2)
	{
		delete pe;
		return true;
	}

	if(Unit* unit = target)
	{
		Vec3 center = unit->GetCenter();
		for(ParticleEmitter::Particle& p : pe->particles)
			p.pos = Vec3::Lerp(p.pos, center, t / 1.5f);

		return false;
	}
	else
	{
		pe->time = 0.3f;
		pe->manualDelete = 0;
		return true;
	}
}

//=================================================================================================
void Drain::Save(GameWriter& f)
{
	f << target;
	f << pe->id;
	f << t;
}

//=================================================================================================
void Drain::Load(GameReader& f)
{
	if(LOAD_VERSION < V_0_13)
		f.Skip<int>(); // old from
	f >> target;
	pe = ParticleEmitter::GetById(f.Read<int>());
	f >> t;
}
