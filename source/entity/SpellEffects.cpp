// efekty czarów, eksplozje, electro, drain
#include "Pch.h"
#include "GameCore.h"
#include "SpellEffects.h"
#include "Unit.h"
#include "Ability.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "GameResources.h"
#include "LevelArea.h"

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
	f << ability->hash;
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
	if(LOAD_VERSION >= V_0_13)
		ability = Ability::Get(f.Read<int>());
	else
	{
		const string& tex = f.ReadString1();
		for(Ability* ability : Ability::abilities)
		{
			if(ability->tex_explode.diffuse && ability->tex_explode.diffuse->filename == tex)
			{
				this->ability = ability;
				break;
			}
		}
	}
}

//=================================================================================================
void Explo::Write(BitStreamWriter& f)
{
	f << ability->hash;
	f << pos;
	f << size;
	f << sizemax;
}

//=================================================================================================
bool Explo::Read(BitStreamReader& f)
{
	ability = Ability::Get(f.Read<int>());
	f >> pos;
	f >> size;
	f >> sizemax;
	return f.IsOk();
}

//=================================================================================================
Electro::~Electro()
{
	for(Line& line : lines)
	{
		if(line.trail)
			line.trail->destroy = true;
	}
}

//=================================================================================================
void Electro::AddLine(const Vec3& from, const Vec3& to, float t)
{
	Line& line = Add1(lines);
	line.from = from;
	line.to = to;
	line.t = t;
	line.trail = nullptr;

	if(t < 0.5f)
	{
		const int steps = int(Vec3::Distance(line.from, line.to) * 10);

		TrailParticleEmitter* trail = new TrailParticleEmitter;
		trail->Init(steps + 1);
		trail->manual = true;
		trail->first = 0;
		trail->last = steps + 1;
		trail->alive = steps + 1;
		trail->tex = game_res->tLightingLine;
		trail->fade = 0.25f;
		trail->color1 = Vec4(0.2f, 0.2f, 1.f, 0.5f);
		trail->color2 = Vec4(0.2f, 0.2f, 1.f, 0);

		const Vec3 dir = line.to - line.from;
		const Vec3 step = dir / float(steps);
		Vec3 prev_off(0.f, 0.f, 0.f);

		trail->parts[0].exists = true;
		trail->parts[0].next = 1;
		trail->parts[0].pt = line.from;
		trail->parts[0].t = 0;

		for(int i = 1; i < steps; ++i)
		{
			Vec3 p = line.from + step * (float(i) + Random(-0.25f, 0.25f));
			Vec3 r = Vec3::Random(Vec3(-0.3f, -0.3f, -0.3f), Vec3(0.3f, 0.3f, 0.3f));
			prev_off = (r + prev_off) / 2;
			trail->parts[i].exists = true;
			trail->parts[i].next = i + 1;
			trail->parts[i].pt = p + prev_off;
			trail->parts[i].t = 0;
		}

		trail->parts[steps].exists = true;
		trail->parts[steps].next = -1;
		trail->parts[steps].pt = line.to;
		trail->parts[steps].t = 0;

		area->tmp->tpes.push_back(trail);
		line.trail = trail;
		UpdateColor(line);
	}
}

//=================================================================================================
void Electro::Update(float dt)
{
	for(Line& line : lines)
	{
		if(!line.trail)
			continue;
		line.t += dt;
		if(line.t >= 0.5f)
		{
			line.trail->destroy = true;
			line.trail = nullptr;
		}
		else
			UpdateColor(line);
	}
}

//=================================================================================================
void Electro::UpdateColor(Line& line)
{
	const float t = line.t * 4;
	const float r = 0.5f;
	for(int i = 0, count = (int)line.trail->parts.size(); i < count; ++i)
	{
		const float x = float(i) / count;
		const float d = abs(x - t);
		float a;
		if(d <= r / 2)
			a = 1.f;
		else if(d < r)
			a = 1.f - (d - r / 2) / (r / 2);
		else
			a = 0.f;
		line.trail->parts[i].t = a;
	}
}

//=================================================================================================
void Electro::Save(FileWriter& f)
{
	f << id;
	f << lines.size();
	for(Line& line : lines)
	{
		f << line.from;
		f << line.to;
		f << line.t;
		f << (line.trail ? line.trail->id : -1);
	}
	f << hitted.size();
	for(Entity<Unit> unit : hitted)
		f << unit;
	f << dmg;
	f << owner;
	f << ability->hash;
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
	if(LOAD_VERSION >= V_0_13)
	{
		lines.resize(f.Read<uint>());
		for(Line& line : lines)
		{
			f >> line.from;
			f >> line.to;
			f >> line.t;
			line.trail = TrailParticleEmitter::GetById(f.Read<int>());
		}
	}
	else
	{
		vector<Vec3> pts;
		float t;
		uint count;
		f >> count;
		for(uint i = 0; i < count; ++i)
		{
			f.ReadVector4(pts);
			f >> t;
			AddLine(pts.front(), pts.back(), t);
		}
	}
	hitted.resize(f.Read<uint>());
	for(Entity<Unit>& unit : hitted)
		f >> unit;
	f >> dmg;
	f >> owner;
	if(LOAD_VERSION >= V_0_13)
	{
		int ability_hash = f.Read<int>();
		ability = Ability::Get(ability_hash);
		if(!ability)
			throw Format("Missing ability %u for electro.", ability_hash);
	}
	else
	{
		const string& ability_id = f.ReadString1();
		ability = Ability::Get(ability_id);
		if(!ability)
			throw Format("Missing ability '%s' for electro.", ability_id.c_str());
	}
	f >> valid;
	f >> hitsome;
	f >> start_pos;
}

//=================================================================================================
void Electro::Write(BitStreamWriter& f)
{
	f << id;
	f << ability->hash;
	f.WriteCasted<byte>(lines.size());
	for(Line& line : lines)
	{
		f << line.from;
		f << line.to;
		f << line.t;
	}
}

//=================================================================================================
bool Electro::Read(BitStreamReader& f)
{
	f >> id;
	ability = Ability::Get(f.Read<int>());

	byte count;
	f >> count;
	if(!f.Ensure(count * Line::SIZE))
		return false;
	lines.reserve(count);

	Vec3 from, to;
	float t;
	for(byte i = 0; i < count; ++i)
	{
		f >> from;
		f >> to;
		f >> t;
		AddLine(from, to, t);
	}

	valid = true;
	Register();
	return true;
}

//=================================================================================================
void Drain::Save(FileWriter& f)
{
	f << target;
	f << pe->id;
	f << t;
}

//=================================================================================================
void Drain::Load(FileReader& f)
{
	if(LOAD_VERSION < V_0_13)
		f.Skip<int>(); // old from
	f >> target;
	pe = ParticleEmitter::GetById(f.Read<int>());
	f >> t;
}
