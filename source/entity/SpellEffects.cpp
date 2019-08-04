// efekty czarów, eksplozje, electro, drain
#include "Pch.h"
#include "GameCore.h"
#include "SpellEffects.h"
#include "Unit.h"
#include "Spell.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"

int Electro::netid_counter;

//=================================================================================================
void Explo::Save(FileWriter& f)
{
	f << pos;
	f << size;
	f << sizemax;
	f << dmg;
	f << hitted.size();
	for(Unit* unit : hitted)
		f << unit->refid;
	f << (owner ? owner->refid : -1);
	f << tex->filename;
}

//=================================================================================================
void Explo::Load(FileReader& f)
{
	f >> pos;
	f >> size;
	f >> sizemax;
	f >> dmg;
	hitted.resize(f.Read<uint>());
	for(uint i = 0; i < hitted.size(); ++i)
		hitted[i] = Unit::GetByRefid(f.Read<int>());
	owner = Unit::GetByRefid(f.Read<int>());
	tex = ResourceManager::Get().Load<Texture>(f.ReadString1());
}

//=================================================================================================
void Electro::AddLine(const Vec3& from, const Vec3& to)
{
	ElectroLine& line = Add1(lines);

	line.t = 0.f;
	line.pts.push_back(from);

	int steps = int(Vec3::Distance(from, to) * 10);

	Vec3 dir = to - from;
	const Vec3 step = dir / float(steps);
	Vec3 prev_off(0.f, 0.f, 0.f);

	for(int i = 1; i < steps; ++i)
	{
		Vec3 p = from + step*(float(i) + Random(-0.25f, 0.25f));
		Vec3 r = Vec3::Random(Vec3(-0.3f, -0.3f, -0.3f), Vec3(0.3f, 0.3f, 0.3f));
		prev_off = (r + prev_off) / 2;
		line.pts.push_back(p + prev_off);
	}

	line.pts.push_back(to);
}

//=================================================================================================
void Electro::Save(FileWriter& f)
{
	f << lines.size();
	for(ElectroLine& line : lines)
	{
		f.WriteVector4(line.pts);
		f << line.t;
	}
	f << hitted.size();
	for(Unit* unit : hitted)
		f << unit->refid;
	f << dmg;
	f << (owner ? owner->refid : -1);
	f << spell->id;
	f << valid;
	f << hitsome;
	f << start_pos;
	f << netid;
}

//=================================================================================================
void Electro::Load(FileReader& f)
{
	lines.resize(f.Read<uint>());
	for(ElectroLine& line : lines)
	{
		f.ReadVector4(line.pts);
		f >> line.t;
	}
	hitted.resize(f.Read<uint>());
	for(Unit*& unit : hitted)
		unit = Unit::GetByRefid(f.Read<int>());
	f >> dmg;
	owner = Unit::GetByRefid(f.Read<int>());
	const string& spell_id = f.ReadString1();
	spell = Spell::TryGet(spell_id);
	if(!spell)
		throw Format("Missing spell '%s' for electro.", spell_id.c_str());
	f >> valid;
	f >> hitsome;
	f >> start_pos;
	f >> netid;
}

//=================================================================================================
void Drain::Save(FileWriter& f)
{
	f << from->refid;
	f << to->refid;
	f << pe->refid;
	f << t;
}

//=================================================================================================
void Drain::Load(FileReader& f)
{
	from = Unit::GetByRefid(f.Read<int>());
	to = Unit::GetByRefid(f.Read<int>());
	pe = ParticleEmitter::GetByRefid(f.Read<int>());
	f >> t;
}
