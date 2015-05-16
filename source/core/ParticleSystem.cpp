#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "SaveState.h"

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
	byte len = (byte)tex.res->filename.length();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, tex.res->filename.c_str(), len, &tmp, NULL);
	WriteFile(file, &emision_interval, sizeof(emision_interval), &tmp, NULL);
	WriteFile(file, &life, sizeof(life), &tmp, NULL);
	WriteFile(file, &particle_life, sizeof(particle_life), &tmp, NULL);
	WriteFile(file, &alpha, sizeof(alpha), &tmp, NULL);
	WriteFile(file, &size, sizeof(size), &tmp, NULL);
	WriteFile(file, &emisions, sizeof(emisions), &tmp, NULL);
	WriteFile(file, &spawn_min, sizeof(spawn_min), &tmp, NULL);
	WriteFile(file, &spawn_max, sizeof(spawn_max), &tmp, NULL);
	WriteFile(file, &max_particles, sizeof(max_particles), &tmp, NULL);
	WriteFile(file, &mode, sizeof(mode), &tmp, NULL);
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &speed_min, sizeof(speed_min), &tmp, NULL);
	WriteFile(file, &speed_max, sizeof(speed_max), &tmp, NULL);
	WriteFile(file, &pos_min, sizeof(pos_min), &tmp, NULL);
	WriteFile(file, &pos_max, sizeof(pos_max), &tmp, NULL);
	WriteFile(file, &op_size, sizeof(op_size), &tmp, NULL);
	WriteFile(file, &op_alpha, sizeof(op_alpha), &tmp, NULL);
	WriteFile(file, &manual_delete, sizeof(manual_delete), &tmp, NULL);
	WriteFile(file, &time, sizeof(time), &tmp, NULL);
	WriteFile(file, &radius, sizeof(radius), &tmp, NULL);
	uint count = particles.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	if(count)
		WriteFile(file, &particles[0], sizeof(particles[0])*count, &tmp, NULL);
	WriteFile(file, &alive, sizeof(alive), &tmp, NULL);
	WriteFile(file, &destroy, sizeof(destroy), &tmp, NULL);
}

//=================================================================================================
void ParticleEmitter::Load(HANDLE file)
{
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	tex = Game::_game->LoadTex2(BUF);
	ReadFile(file, &emision_interval, sizeof(emision_interval), &tmp, NULL);
	ReadFile(file, &life, sizeof(life), &tmp, NULL);
	ReadFile(file, &particle_life, sizeof(particle_life), &tmp, NULL);
	ReadFile(file, &alpha, sizeof(alpha), &tmp, NULL);
	ReadFile(file, &size, sizeof(size), &tmp, NULL);
	ReadFile(file, &emisions, sizeof(emisions), &tmp, NULL);
	ReadFile(file, &spawn_min, sizeof(spawn_min), &tmp, NULL);
	ReadFile(file, &spawn_max, sizeof(spawn_max), &tmp, NULL);
	ReadFile(file, &max_particles, sizeof(max_particles), &tmp, NULL);
	ReadFile(file, &mode, sizeof(mode), &tmp, NULL);
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &speed_min, sizeof(speed_min), &tmp, NULL);
	ReadFile(file, &speed_max, sizeof(speed_max), &tmp, NULL);
	ReadFile(file, &pos_min, sizeof(pos_min), &tmp, NULL);
	ReadFile(file, &pos_max, sizeof(pos_max), &tmp, NULL);
	ReadFile(file, &op_size, sizeof(op_size), &tmp, NULL);
	ReadFile(file, &op_alpha, sizeof(op_alpha), &tmp, NULL);
	ReadFile(file, &manual_delete, sizeof(manual_delete), &tmp, NULL);
	ReadFile(file, &time, sizeof(time), &tmp, NULL);
	ReadFile(file, &radius, sizeof(radius), &tmp, NULL);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	particles.resize(count);
	if(count)
		ReadFile(file, &particles[0], sizeof(particles[0])*count, &tmp, NULL);
	ReadFile(file, &alive, sizeof(alive), &tmp, NULL);
	ReadFile(file, &destroy, sizeof(destroy), &tmp, NULL);
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
	WriteFile(file, &fade, sizeof(fade), &tmp, NULL);
	WriteFile(file, &color1, sizeof(color1), &tmp, NULL);
	WriteFile(file, &color2, sizeof(color2), &tmp, NULL);
	uint count = parts.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	if(count)
		WriteFile(file, &parts[0], sizeof(parts[0])*count, &tmp, NULL);
	WriteFile(file, &first, sizeof(first), &tmp, NULL);
	WriteFile(file, &last, sizeof(last), &tmp, NULL);
	WriteFile(file, &destroy, sizeof(destroy), &tmp, NULL);
	WriteFile(file, &alive, sizeof(alive), &tmp, NULL);
	WriteFile(file, &timer, sizeof(timer), &tmp, NULL);
}

//=================================================================================================
void TrailParticleEmitter::Load(HANDLE file)
{
	ReadFile(file, &fade, sizeof(fade), &tmp, NULL);
	ReadFile(file, &color1, sizeof(color1), &tmp, NULL);
	ReadFile(file, &color2, sizeof(color2), &tmp, NULL);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	parts.resize(count);
	if(count)
		ReadFile(file, &parts[0], sizeof(parts[0])*count, &tmp, NULL);
	ReadFile(file, &first, sizeof(first), &tmp, NULL);
	ReadFile(file, &last, sizeof(last), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_10)
	{
		int old_destroy;
		ReadFile(file, &old_destroy, sizeof(old_destroy), &tmp, NULL);
		destroy = (old_destroy != 0);
	}
	else
		ReadFile(file, &destroy, sizeof(destroy), &tmp, NULL);
	ReadFile(file, &alive, sizeof(alive), &tmp, NULL);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &timer, sizeof(timer), &tmp, NULL);
	else
		timer = 0.f;
}
