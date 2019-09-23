// efekty czarów, eksplozje, electro, drain
#include "Pch.h"
#include "GameCore.h"
#include "SpellEffects.h"
#include "Unit.h"
#include "Spell.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

EntityType<Electro>::Impl EntityType<Electro>::impl;

//=================================================================================================
void Explo::Save(FileWriter& f)
{
	f << pos;
	f << size;
	f << sizemax;
	f << dmg;
	f << hitted;
	f << owner;
	f << tex->filename;
}

//=================================================================================================
void Explo::Load(FileReader& f)
{
	f >> pos;
	f >> size;
	f >> sizemax;
	f >> dmg;
	f >> hitted;
	f >> owner;
	tex = res_mgr->Load<Texture>(f.ReadString1());
}

//=================================================================================================
void Explo::Write(BitStreamWriter& f)
{
	f << tex->filename;
	f << pos;
	f << size;
	f << sizemax;
}

//=================================================================================================
bool Explo::Read(BitStreamReader& f)
{
	const string& tex_id = f.ReadString1();
	f >> pos;
	f >> size;
	f >> sizemax;
	if(!f)
		return false;
	tex = res_mgr->Load<Texture>(tex_id);
	return true;
}

//=================================================================================================
void Electro::AddLine(const Vec3& from, const Vec3& to)
{
	Line& line = Add1(lines);

	line.t = 0.f;
	line.pts.push_back(from);

	int steps = int(Vec3::Distance(from, to) * 10);

	Vec3 dir = to - from;
	const Vec3 step = dir / float(steps);
	Vec3 prev_off(0.f, 0.f, 0.f);

	for(int i = 1; i < steps; ++i)
	{
		Vec3 p = from + step * (float(i) + Random(-0.25f, 0.25f));
		Vec3 r = Vec3::Random(Vec3(-0.3f, -0.3f, -0.3f), Vec3(0.3f, 0.3f, 0.3f));
		prev_off = (r + prev_off) / 2;
		line.pts.push_back(p + prev_off);
	}

	line.pts.push_back(to);
}

//=================================================================================================
void Electro::Save(FileWriter& f)
{
	f << id;
	f << lines.size();
	for(Line& line : lines)
	{
		f.WriteVector4(line.pts);
		f << line.t;
	}
	f << hitted.size();
	for(Entity<Unit> unit : hitted)
		f << unit;
	f << dmg;
	f << owner;
	f << spell->id;
	f << valid;
	f << hitsome;
	f << start_pos;
}

//=================================================================================================
void Electro::Load(FileReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();
	lines.resize(f.Read<uint>());
	for(Line& line : lines)
	{
		f.ReadVector4(line.pts);
		f >> line.t;
	}
	hitted.resize(f.Read<uint>());
	for(Entity<Unit>& unit : hitted)
		f >> unit;
	f >> dmg;
	f >> owner;
	const string& spell_id = f.ReadString1();
	spell = Spell::TryGet(spell_id);
	if(!spell)
		throw Format("Missing spell '%s' for electro.", spell_id.c_str());
	f >> valid;
	f >> hitsome;
	f >> start_pos;
}

//=================================================================================================
void Electro::Write(BitStreamWriter& f)
{
	f << id;
	f << spell->id;
	f.WriteCasted<byte>(lines.size());
	for(Line& line : lines)
	{
		f << line.pts.front();
		f << line.pts.back();
		f << line.t;
	}
}

//=================================================================================================
bool Electro::Read(BitStreamReader& f)
{
	f >> id;
	spell = Spell::TryGet(f.ReadString1());

	byte count;
	f >> count;
	if(!f.Ensure(count * LINE_MIN_SIZE))
		return false;
	lines.reserve(count);
	Vec3 from, to;
	float t;
	for(byte i = 0; i < count; ++i)
	{
		f >> from;
		f >> to;
		f >> t;
		AddLine(from, to);
		lines.back().t = t;
	}

	valid = true;
	Register();
	return true;
}

//=================================================================================================
void Drain::Save(FileWriter& f)
{
	f << from->id;
	f << to->id;
	f << pe->id;
	f << t;
}

//=================================================================================================
void Drain::Load(FileReader& f)
{
	from = Unit::GetById(f.Read<int>());
	to = Unit::GetById(f.Read<int>());
	pe = ParticleEmitter::GetById(f.Read<int>());
	f >> t;
}
