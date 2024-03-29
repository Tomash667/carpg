#include "Pch.h"
#include "Electro.h"

#include "Ability.h"
#include "AIController.h"
#include "BitStreamFunc.h"
#include "GameResources.h"
#include "Level.h"
#include "LevelPart.h"
#include "LocationPart.h"
#include "Net.h"
#include "ParticleEffect.h"
#include "Unit.h"

#include <ParticleSystem.h>
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
		trail->tex = gameRes->tLightingLine;
		trail->fade = 0.25f;
		trail->color1 = Vec4(0.2f, 0.2f, 1.f, 0.5f);
		trail->color2 = Vec4(0.2f, 0.2f, 1.f, 0);

		const Vec3 dir = line.to - line.from;
		const Vec3 step = dir / float(steps);
		Vec3 prevOff(0.f, 0.f, 0.f);

		trail->parts[0].exists = true;
		trail->parts[0].next = 1;
		trail->parts[0].pt = line.from;
		trail->parts[0].t = 0;

		for(int i = 1; i < steps; ++i)
		{
			Vec3 p = line.from + step * (float(i) + Random(-0.25f, 0.25f));
			Vec3 r = Vec3::Random(Vec3(-0.3f, -0.3f, -0.3f), Vec3(0.3f, 0.3f, 0.3f));
			prevOff = (r + prevOff) / 2;
			trail->parts[i].exists = true;
			trail->parts[i].next = i + 1;
			trail->parts[i].pt = p + prevOff;
			trail->parts[i].t = 0;
		}

		trail->parts[steps].exists = true;
		trail->parts[steps].next = -1;
		trail->parts[steps].pt = line.to;
		trail->parts[steps].t = 0;

		locPart->lvlPart->tpes.push_back(trail);
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

	if(Net::IsClient())
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

			const Vec3 targetPos = lines.back().to;

			// deal damage
			if(!owner->IsFriend(*target, true))
			{
				if(target->IsAI() && owner->IsAlive())
					target->ai->HitReaction(startPos);
				target->GiveDmg(dmg, owner, nullptr, Unit::DMG_NO_BLOOD | Unit::DMG_MAGICAL);
			}

			// play sound
			if(ability->soundHit)
				soundMgr->PlaySound3d(ability->soundHit, targetPos, ability->soundHitDist);

			// add particles
			if(ability->particleEffect)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				ability->particleEffect->Apply(pe);
				pe->pos = targetPos;
				pe->Init();
				locPart->lvlPart->pes.push_back(pe);
			}

			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::ELECTRO_HIT);
				c.pos = targetPos;
			}

			Unit* newTarget = FindNextTarget();
			if(newTarget)
			{
				// found another target
				const float dist = Vec3::Distance(target->pos, newTarget->pos);
				dmg = min(dmg / 2, Lerp(dmg, dmg / 5, dist / 5));
				valid = true;
				hitted.push_back(newTarget);
				Vec3 from = lines.back().to;
				Vec3 to = newTarget->GetCenter();
				AddLine(from, to);

				if(Net::IsOnline())
				{
					NetChange& c = Net::PushChange(NetChange::UPDATE_ELECTRO);
					c.extraId = id;
					c.pos = to;
				}
			}
			else
			{
				// hit enough/no target
				valid = false;
				hitsome = false;
			}
		}
	}
	else
	{
		if(hitsome && lines.back().t >= 0.25f)
		{
			const Vec3 targetPos = lines.back().to;
			hitsome = false;

			if(ability->soundHit)
				soundMgr->PlaySound3d(ability->soundHit, targetPos, ability->soundHitDist);

			// particles
			if(ability->particleEffect)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				ability->particleEffect->Apply(pe);
				pe->pos = targetPos;
				pe->Init();
				locPart->lvlPart->pes.push_back(pe);
			}

			if(Net::IsOnline())
			{
				NetChange& c = Net::PushChange(NetChange::ELECTRO_HIT);
				c.pos = targetPos;
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
Unit* Electro::FindNextTarget()
{
	if(dmg < 10.f)
	{
		// already hit enough targets
		return nullptr;
	}

	// hit another target
	Unit* target = hitted.back();
	static vector<pair<Unit*, float>> targets;
	for(Unit* unit : locPart->units)
	{
		if(!unit->IsAlive() || IsInside(hitted, unit))
			continue;

		float dist = Vec3::Distance(unit->pos, target->pos);
		if(dist <= 5.f)
			targets.push_back(pair<Unit*, float>(unit, dist));
	}
	if(targets.empty())
		return nullptr;

	if(targets.size() > 1)
	{
		std::sort(targets.begin(), targets.end(), [](const pair<Unit*, float>& target1, const pair<Unit*, float>& target2)
		{
			return target1.second < target2.second;
		});
	}

	const Vec3 targetPos = lines.back().to;
	Unit* newTarget = nullptr;
	for(vector<pair<Unit*, float>>::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
	{
		Vec3 hitpoint;
		Unit* newHitted;
		if(gameLevel->RayTest(targetPos, it2->first->GetCenter(), target, hitpoint, newHitted))
		{
			if(newHitted == it2->first)
			{
				newTarget = it2->first;
				break;
			}
		}
	}

	targets.clear();
	return newTarget;
}

//=================================================================================================
void Electro::Save(GameWriter& f)
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
void Electro::Load(GameReader& f)
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
		int abilityHash = f.Read<int>();
		ability = Ability::Get(abilityHash);
		if(!ability)
			throw Format("Missing ability %u for electro.", abilityHash);
	}
	else
	{
		const string& abilityId = f.ReadString1();
		ability = Ability::Get(abilityId);
		if(!ability)
			throw Format("Missing ability '%s' for electro.", abilityId.c_str());
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
