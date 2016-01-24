#include "Pch.h"
#include "Base.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "ParticleSystem.h"

//=================================================================================================
float drop_range(float v, float t)
{
	if(v > 0)
	{
		float t_wznoszenia = v/G;
		if(t_wznoszenia >= t)
			return (v*v)/(2*G);
		else
			return v*t - (G*(t*t))/2;
	}
	else
		return v*t - G*(t*t)/2;
}

//=================================================================================================
void ParticleEmitter::Init()
{
	particles.resize(max_particles);
	time = 0.f;
	alive = 0;
	destroy = false;
	for(int i=0; i<max_particles; ++i)
		particles[i].exists = false;

	// oblicz promieñ
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

	// ty³
	r2 = abs(pos_min.z + speed_min.z * t);
	if(r2 > r)
		r = r2;

	// przód
	r2 = abs(pos_max.z + speed_max.z * t);
	if(r2 > r)
		r = r2;

	// góra
	r2 = abs(pos_max.y + drop_range(speed_max.y, t));
	if(r2 > r)
		r = r2;

	// dó³
	r2 = abs(pos_min.y + drop_range(speed_min.y, t));
	if(r2 > r)
		r = r2;

	radius = sqrt(2*r*r);


	// nowe
	//gravity = false;
	manual_delete = 0;
}

//=================================================================================================
void ParticleEmitter::Save(HANDLE file)
{
	WriteString1(file, tex->filename);
	WriteFile(file, &emision_interval, sizeof(emision_interval), &tmp, nullptr);
	WriteFile(file, &life, sizeof(life), &tmp, nullptr);
	WriteFile(file, &particle_life, sizeof(particle_life), &tmp, nullptr);
	WriteFile(file, &alpha, sizeof(alpha), &tmp, nullptr);
	WriteFile(file, &size, sizeof(size), &tmp, nullptr);
	WriteFile(file, &emisions, sizeof(emisions), &tmp, nullptr);
	WriteFile(file, &spawn_min, sizeof(spawn_min), &tmp, nullptr);
	WriteFile(file, &spawn_max, sizeof(spawn_max), &tmp, nullptr);
	WriteFile(file, &max_particles, sizeof(max_particles), &tmp, nullptr);
	WriteFile(file, &mode, sizeof(mode), &tmp, nullptr);
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &speed_min, sizeof(speed_min), &tmp, nullptr);
	WriteFile(file, &speed_max, sizeof(speed_max), &tmp, nullptr);
	WriteFile(file, &pos_min, sizeof(pos_min), &tmp, nullptr);
	WriteFile(file, &pos_max, sizeof(pos_max), &tmp, nullptr);
	WriteFile(file, &op_size, sizeof(op_size), &tmp, nullptr);
	WriteFile(file, &op_alpha, sizeof(op_alpha), &tmp, nullptr);
	WriteFile(file, &manual_delete, sizeof(manual_delete), &tmp, nullptr);
	WriteFile(file, &time, sizeof(time), &tmp, nullptr);
	WriteFile(file, &radius, sizeof(radius), &tmp, nullptr);
	uint count = particles.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	if(count)
		WriteFile(file, &particles[0], sizeof(particles[0])*count, &tmp, nullptr);
	WriteFile(file, &alive, sizeof(alive), &tmp, nullptr);
	WriteFile(file, &destroy, sizeof(destroy), &tmp, nullptr);
}

//=================================================================================================
void ParticleEmitter::Load(HANDLE file)
{
	ReadString1(file);
	tex = ResourceManager::Get().GetLoadedTexture(BUF);
	ReadFile(file, &emision_interval, sizeof(emision_interval), &tmp, nullptr);
	ReadFile(file, &life, sizeof(life), &tmp, nullptr);
	ReadFile(file, &particle_life, sizeof(particle_life), &tmp, nullptr);
	ReadFile(file, &alpha, sizeof(alpha), &tmp, nullptr);
	ReadFile(file, &size, sizeof(size), &tmp, nullptr);
	ReadFile(file, &emisions, sizeof(emisions), &tmp, nullptr);
	ReadFile(file, &spawn_min, sizeof(spawn_min), &tmp, nullptr);
	ReadFile(file, &spawn_max, sizeof(spawn_max), &tmp, nullptr);
	ReadFile(file, &max_particles, sizeof(max_particles), &tmp, nullptr);
	ReadFile(file, &mode, sizeof(mode), &tmp, nullptr);
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &speed_min, sizeof(speed_min), &tmp, nullptr);
	ReadFile(file, &speed_max, sizeof(speed_max), &tmp, nullptr);
	ReadFile(file, &pos_min, sizeof(pos_min), &tmp, nullptr);
	ReadFile(file, &pos_max, sizeof(pos_max), &tmp, nullptr);
	ReadFile(file, &op_size, sizeof(op_size), &tmp, nullptr);
	ReadFile(file, &op_alpha, sizeof(op_alpha), &tmp, nullptr);
	ReadFile(file, &manual_delete, sizeof(manual_delete), &tmp, nullptr);
	ReadFile(file, &time, sizeof(time), &tmp, nullptr);
	ReadFile(file, &radius, sizeof(radius), &tmp, nullptr);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	particles.resize(count);
	if(count)
		ReadFile(file, &particles[0], sizeof(particles[0])*count, &tmp, nullptr);
	ReadFile(file, &alive, sizeof(alive), &tmp, nullptr);
	ReadFile(file, &destroy, sizeof(destroy), &tmp, nullptr);
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
bool TrailParticleEmitter::Update(float dt, VEC3* pt1, VEC3* pt2)
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

	if(pt1 && pt2 && timer >= 1.f/parts.size())
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
void TrailParticleEmitter::Save(HANDLE file)
{
	WriteFile(file, &fade, sizeof(fade), &tmp, nullptr);
	WriteFile(file, &color1, sizeof(color1), &tmp, nullptr);
	WriteFile(file, &color2, sizeof(color2), &tmp, nullptr);
	uint count = parts.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	if(count)
		WriteFile(file, &parts[0], sizeof(parts[0])*count, &tmp, nullptr);
	WriteFile(file, &first, sizeof(first), &tmp, nullptr);
	WriteFile(file, &last, sizeof(last), &tmp, nullptr);
	WriteFile(file, &destroy, sizeof(destroy), &tmp, nullptr);
	WriteFile(file, &alive, sizeof(alive), &tmp, nullptr);
	WriteFile(file, &timer, sizeof(timer), &tmp, nullptr);
}

//=================================================================================================
void TrailParticleEmitter::Load(HANDLE file)
{
	ReadFile(file, &fade, sizeof(fade), &tmp, nullptr);
	ReadFile(file, &color1, sizeof(color1), &tmp, nullptr);
	ReadFile(file, &color2, sizeof(color2), &tmp, nullptr);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	parts.resize(count);
	if(count)
		ReadFile(file, &parts[0], sizeof(parts[0])*count, &tmp, nullptr);
	ReadFile(file, &first, sizeof(first), &tmp, nullptr);
	ReadFile(file, &last, sizeof(last), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_10)
	{
		int old_destroy;
		ReadFile(file, &old_destroy, sizeof(old_destroy), &tmp, nullptr);
		destroy = (old_destroy != 0);
	}
	else
		ReadFile(file, &destroy, sizeof(destroy), &tmp, nullptr);
	ReadFile(file, &alive, sizeof(alive), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &timer, sizeof(timer), &tmp, nullptr);
	else
		timer = 0.f;
}
