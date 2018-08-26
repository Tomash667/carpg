#include "Pch.h"
#include "EngineCore.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "ParticleSystem.h"

//=================================================================================================
float drop_range(float v, float t)
{
	if(v > 0)
	{
		float t_wznoszenia = v / G;
		if(t_wznoszenia >= t)
			return (v*v) / (2 * G);
		else
			return v*t - (G*(t*t)) / 2;
	}
	else
		return v*t - G*(t*t) / 2;
}

//=================================================================================================
void ParticleEmitter::Init()
{
	particles.resize(max_particles);
	time = 0.f;
	alive = 0;
	destroy = false;
	for(int i = 0; i < max_particles; ++i)
		particles[i].exists = false;

	// oblicz promie�
	float t;
	if(life > 0)
		t = min(particle_life, life);
	else
		t = particle_life;
	float r = 0.f;

	// lewa
	float r2 = abs(pos_min.x + speed_min.x * t);
	if(r2 > r)
		r = r2;

	// prawa
	r2 = abs(pos_max.x + speed_max.x * t);
	if(r2 > r)
		r = r2;

	// ty�
	r2 = abs(pos_min.z + speed_min.z * t);
	if(r2 > r)
		r = r2;

	// prz�d
	r2 = abs(pos_max.z + speed_max.z * t);
	if(r2 > r)
		r = r2;

	// g�ra
	r2 = abs(pos_max.y + drop_range(speed_max.y, t));
	if(r2 > r)
		r = r2;

	// d�
	r2 = abs(pos_min.y + drop_range(speed_min.y, t));
	if(r2 > r)
		r = r2;

	radius = sqrt(2 * r*r);

	// nowe
	//gravity = false;
	manual_delete = 0;
}

//=================================================================================================
void ParticleEmitter::Save(FileWriter& f)
{
	f << tex->filename;
	f << emision_interval;
	f << life;
	f << particle_life;
	f << alpha;
	f << size;
	f << emisions;
	f << spawn_min;
	f << spawn_max;
	f << max_particles;
	f << mode;
	f << pos;
	f << speed_min;
	f << speed_max;
	f << pos_min;
	f << pos_max;
	f << op_size;
	f << op_alpha;
	f << manual_delete;
	f << time;
	f << radius;
	f << particles;
	f << alive;
	f << destroy;
}

//=================================================================================================
void ParticleEmitter::Load(FileReader& f)
{
	tex = ResourceManager::Get<Texture>().GetLoaded(f.ReadString1());
	f >> emision_interval;
	f >> life;
	f >> particle_life;
	f >> alpha;
	f >> size;
	f >> emisions;
	f >> spawn_min;
	f >> spawn_max;
	f >> max_particles;
	f >> mode;
	f >> pos;
	f >> speed_min;
	f >> speed_max;
	f >> pos_min;
	f >> pos_max;
	f >> op_size;
	f >> op_alpha;
	f >> manual_delete;
	f >> time;
	f >> radius;
	f >> particles;
	f >> alive;
	f >> destroy;
}

//=================================================================================================
void TrailParticleEmitter::Init(int maxp)
{
	parts.resize(maxp);
	for(vector<TrailParticle>::iterator it = parts.begin(), end = parts.end(); it != end; ++it)
		it->exists = false;

	first = -1;
	last = -1;
	alive = 0;
	destroy = false;
	timer = 0.f;
}

//=================================================================================================
bool TrailParticleEmitter::Update(float dt, Vec3* pt1, Vec3* pt2)
{
	if(first != -1 && dt > 0.f)
	{
		int id = first;

		while(id != -1)
		{
			TrailParticle& tp = parts[id];
			tp.t -= dt;
			if(tp.t < 0)
			{
				tp.exists = false;
				first = tp.next;
				--alive;
				if(id == last)
					last = -1;
				id = first;
			}
			else
				id = tp.next;
		}
	}

	timer += dt;

	if(pt1 && pt2 && timer >= 1.f / parts.size())
	{
		timer = 0.f;

		if(last == -1)
		{
			TrailParticle& tp = parts[0];
			tp.t = fade;
			tp.exists = true;
			tp.next = -1;
			tp.pt1 = *pt1;
			tp.pt2 = *pt2;
			first = 0;
			last = 0;
			++alive;
		}
		else
		{
			if(alive == (int)parts.size())
				return false;

			int id = 0;
			while(parts[id].exists)
				++id;

			TrailParticle& tp = parts[id];
			tp.t = fade;
			tp.exists = true;
			tp.next = -1;
			tp.pt1 = *pt1;
			tp.pt2 = *pt2;

			parts[last].next = id;
			last = id;
			++alive;
		}
	}

	if(destroy && alive == 0)
		return true;
	else
		return false;
}

//=================================================================================================
void TrailParticleEmitter::Save(FileWriter& f)
{
	f << fade;
	f << color1;
	f << color2;
	f << parts;
	f << first;
	f << last;
	f << destroy;
	f << alive;
	f << timer;
}

//=================================================================================================
void TrailParticleEmitter::Load(FileReader& f)
{
	f >> fade;
	f >> color1;
	f >> color2;
	f >> parts;
	f >> first;
	f >> last;
	f >> destroy;
	f >> alive;
	if(LOAD_VERSION >= V_0_3)
		f >> timer;
	else
		timer = 0.f;
}
