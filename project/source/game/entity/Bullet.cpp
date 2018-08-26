#include "Pch.h"
#include "GameCore.h"
#include "Bullet.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Unit.h"
#include "Spell.h"
#include "ParticleSystem.h"

//=================================================================================================
void Bullet::Save(FileWriter& f)
{
	f << pos;
	f << rot;
	f << mesh->filename;
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
	f >> pos;
	f >> rot;
	if(LOAD_VERSION < V_0_3)
		f.Skip<float>();
	mesh = ResourceManager::Get<Mesh>().GetLoaded(f.ReadString1());
	f >> speed;
	f >> timer;
	f >> attack;
	f >> tex_size;
	f >> yspeed;
	f >> poison_attack;
	int refid;
	f >> refid;
	owner = Unit::GetByRefid(refid);
	const string& spell_id = f.ReadString1();
	if(!spell_id.empty())
	{
		spell = FindSpell(spell_id.c_str());
		if(!spell)
			throw Format("Missing spell '%s' for bullet.", spell_id.c_str());
	}
	else
		spell = nullptr;
	const string& tex_name = f.ReadString1();
	if(!tex_name.empty())
		tex = ResourceManager::Get<Texture>().GetLoaded(tex_name);
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
