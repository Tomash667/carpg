#include "Pch.h"
#include "SpellEffects.h"

#include "Ability.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "GameResources.h"
#include "Level.h"
#include "LevelArea.h"
#include "Net.h"
#include "SaveState.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <ResourceManager.h>
#include <SoundManager.h>

EntityType<Electro>::Impl EntityType<Electro>::impl;

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
bool Electro::Update(float dt)
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

	if(!Net::IsLocal())
	{
		if(lines.back().t >= 0.5f)
		{
			delete this;
			return true;
		}
	}
	else if(valid)
	{
		if(lines.back().t >= 0.25f)
		{
			Unit* target = hitted.back();
			Unit* owner = this->owner;
			if(!target || !owner)
			{
				valid = false;
				return false;
			}

			const Vec3 target_pos = lines.back().to;

			// deal damage
			if(!owner->IsFriend(*target, true))
			{
				if(target->IsAI() && owner->IsAlive())
					target->ai->HitReaction(startPos);
				target->GiveDmg(dmg, owner, nullptr, Unit::DMG_NO_BLOOD | Unit::DMG_MAGICAL);
			}

			// play sound
			if(ability->sound_hit)
				sound_mgr->PlaySound3d(ability->sound_hit, target_pos, ability->sound_hit_dist);

			// add particles
			if(ability->tex_particle)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = ability->tex_particle;
				pe->emision_interval = 0.01f;
				pe->life = 0.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 8;
				pe->spawn_max = 12;
				pe->max_particles = 12;
				pe->pos = target_pos;
				pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
				pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
				pe->pos_min = Vec3(-ability->size, -ability->size, -ability->size);
				pe->pos_max = Vec3(ability->size, ability->size, ability->size);
				pe->size = ability->size_particle;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				area->tmp->pes.push_back(pe);
			}

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::ELECTRO_HIT;
				c.pos = target_pos;
			}

			if(dmg >= 10.f)
			{
				static vector<pair<Unit*, float>> targets;

				// hit another target
				for(Unit* unit : area->units)
				{
					if(!unit->IsAlive() || IsInside(hitted, unit))
						continue;

					float dist = Vec3::Distance(unit->pos, target->pos);
					if(dist <= 5.f)
						targets.push_back(pair<Unit*, float>(unit, dist));
				}

				if(!targets.empty())
				{
					if(targets.size() > 1)
					{
						std::sort(targets.begin(), targets.end(), [](const pair<Unit*, float>& target1, const pair<Unit*, float>& target2)
						{
							return target1.second < target2.second;
						});
					}

					Unit* newTarget = nullptr;
					float dist;
					for(vector<pair<Unit*, float>>::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
					{
						Vec3 hitpoint;
						Unit* new_hitted;
						if(game_level->RayTest(target_pos, it2->first->GetCenter(), target, hitpoint, new_hitted))
						{
							if(new_hitted == it2->first)
							{
								newTarget = it2->first;
								dist = it2->second;
								break;
							}
						}
					}

					if(newTarget)
					{
						// found another target
						dmg = min(dmg / 2, Lerp(dmg, dmg / 5, dist / 5));
						valid = true;
						hitted.push_back(newTarget);
						Vec3 from = lines.back().to;
						Vec3 to = newTarget->GetCenter();
						AddLine(from, to);

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::UPDATE_ELECTRO;
							c.e_id = id;
							c.pos = to;
						}
					}
					else
					{
						// noone to hit
						valid = false;
					}

					targets.clear();
				}
				else
					valid = false;
			}
			else
			{
				// already hit enough targets
				valid = false;
			}
		}
	}
	else
	{
		if(hitsome && lines.back().t >= 0.25f)
		{
			const Vec3 target_pos = lines.back().to;
			hitsome = false;

			if(ability->sound_hit)
				sound_mgr->PlaySound3d(ability->sound_hit, target_pos, ability->sound_hit_dist);

			// particles
			if(ability->tex_particle)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = ability->tex_particle;
				pe->emision_interval = 0.01f;
				pe->life = 0.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 8;
				pe->spawn_max = 12;
				pe->max_particles = 12;
				pe->pos = target_pos;
				pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
				pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
				pe->pos_min = Vec3(-ability->size, -ability->size, -ability->size);
				pe->pos_max = Vec3(ability->size, ability->size, ability->size);
				pe->size = ability->size_particle;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				area->tmp->pes.push_back(pe);
			}

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::ELECTRO_HIT;
				c.pos = target_pos;
			}
		}
		if(lines.back().t >= 0.5f)
		{
			delete this;
			return true;
		}
	}

	return false;
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
	f << startPos;
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
	f >> startPos;
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
bool Drain::Update(float dt)
{
	t += dt;

	if(pe->manual_delete == 2)
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
		pe->manual_delete = 0;
		return true;
	}
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
