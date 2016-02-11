// pocisk (strza³a, czar)
#include "Pch.h"
#include "Base.h"
#include "Bullet.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "Animesh.h"
#include "Unit.h"
#include "Spell.h"
#include "ParticleSystem.h"

//=================================================================================================
void Bullet::Save(FileWriter& f)
{
	f << pos;
	f << rot;
	f << mesh->res->filename;
	f << speed;
	f << timer;
	f << attack;
	f << tex_size;
	f << yspeed;
	f << poison_attack;
	int refid = (owner ? owner->refid : -1);
	f << refid;
	if(spell)
		f << spell->id;
	else
		f.Write0();
	if(tex)
		f << tex->filename;
	else
		f.Write0();
	refid = (trail ? trail->refid : -1);
	f << refid;
	refid = (trail2 ? trail2->refid : -1);
	f << refid;
	refid = (pe ? pe->refid : -1);
	f << refid;
	f << remove;
	f << backstab;
	f << level;
	f << start_pos;
}

//=================================================================================================
void Bullet::Load(FileReader& f)
{
	ResourceManager& resMgr = ResourceManager::Get();

	f >> pos;
	f >> rot;
	if(LOAD_VERSION < V_0_3)
		f.Skip<float>();
	f.ReadStringBUF();
	resMgr.GetLoadedMesh(BUF, mesh);
	f >> speed;
	f >> timer;
	f >> attack;
	f >> tex_size;
	f >> yspeed;
	f >> poison_attack;
	int refid;
	f >> refid;
	owner = Unit::GetByRefid(refid);
	f.ReadStringBUF();
	if(BUF[0])
	{
		spell = FindSpell(BUF);
		if(!spell)
			throw Format("Missing spell '%s' for bullet.", BUF);
	}
	else
		spell = nullptr;
	f.ReadStringBUF();
	if(BUF[0])
		tex = resMgr.GetLoadedTexture(BUF);
	else
		tex = nullptr;
	f >> refid;
	trail = TrailParticleEmitter::GetByRefid(refid);
	f >> refid;
	trail2 = TrailParticleEmitter::GetByRefid(refid);
	f >> refid;
	pe = ParticleEmitter::GetByRefid(refid);
	f >> remove;
	if(LOAD_VERSION >= V_0_4)
	{
		f >> level;
		f >> backstab;
	}
	else
	{
		level = (int)round(f.Read<float>());
		backstab = 0;
	}
	f >> start_pos;
}
