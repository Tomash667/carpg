// efekty czarów, eksplozje, electro, drain
#include "Pch.h"
#include "Base.h"
#include "SpellEffects.h"
#include "Unit.h"
#include "Game.h"

//=================================================================================================
void Explo::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &size, sizeof(size), &tmp, NULL);
	WriteFile(file, &sizemax, sizeof(sizemax), &tmp, NULL);
	WriteFile(file, &dmg, sizeof(dmg), &tmp, NULL);
	uint count = hitted.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	int refid = (owner ? owner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	byte len = (byte)tex.res->filename.size();
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, tex.res->filename.c_str(), len, &tmp, NULL);
}

//=================================================================================================
void Explo::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &size, sizeof(size), &tmp, NULL);
	ReadFile(file, &sizemax, sizeof(sizemax), &tmp, NULL);
	ReadFile(file, &dmg, sizeof(dmg), &tmp, NULL);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	hitted.resize(count);
	int refid;
	for(uint i=0; i<count; ++i)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		hitted[i] = Unit::GetByRefid(refid);
	}
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	owner = Unit::GetByRefid(refid);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	tex = Game::_game->LoadTex2(BUF);
}

//=================================================================================================
void Electro::AddLine(const VEC3& from, const VEC3& to)
{
	ElectroLine& line = Add1(lines);

	line.t = 0.f;
	line.pts.push_back(from);

	int steps = int(distance(from, to)*10);

	VEC3 dir = to - from;
	const VEC3 step = dir/float(steps);
	VEC3 prev_off(0.f,0.f,0.f);

	for(int i=1; i<steps; ++i)
	{
		VEC3 p = from + step*(float(i)+random(-0.25f,0.25f));
		VEC3 r = random(VEC3(-0.3f,-0.3f,-0.3f), VEC3(0.3f,0.3f,0.3f));
		prev_off = (r + prev_off)/2;
		line.pts.push_back(p + prev_off);
	}

	line.pts.push_back(to);
}

//=================================================================================================
void Electro::Save(HANDLE file)
{
	uint count = lines.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<ElectroLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
	{
		count = it->pts.size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		if(count)
			WriteFile(file, &it->pts[0], sizeof(VEC3)*count, &tmp, NULL);
		WriteFile(file, &it->t, sizeof(it->t), &tmp, NULL);
	}
	count = hitted.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	WriteFile(file, &dmg, sizeof(dmg), &tmp, NULL);
	int refid = (owner ? owner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	byte len = (byte)strlen(spell->name);
	WriteFile(file, &len, sizeof(len), &tmp, NULL);
	WriteFile(file, spell->name, len, &tmp, NULL);
	WriteFile(file, &valid, sizeof(valid), &tmp, NULL);
	WriteFile(file, &hitsome, sizeof(hitsome), &tmp, NULL);
	WriteFile(file, &start_pos, sizeof(start_pos), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);
}

//=================================================================================================
void Electro::Load(HANDLE file)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	lines.resize(count);
	for(vector<ElectroLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
	{
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		it->pts.resize(count);
		if(count)
			ReadFile(file, &it->pts[0], sizeof(VEC3)*count, &tmp, NULL);
		ReadFile(file, &it->t, sizeof(it->t), &tmp, NULL);
	}
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	hitted.resize(count);
	int refid;
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		*it = Unit::GetByRefid(refid);
	}
	ReadFile(file, &dmg, sizeof(dmg), &tmp, NULL);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	owner = Unit::GetByRefid(refid);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, NULL);
	spell = FindSpell(BUF);
	ReadFile(file, &valid, sizeof(valid), &tmp, NULL);
	ReadFile(file, &hitsome, sizeof(hitsome), &tmp, NULL);
	ReadFile(file, &start_pos, sizeof(start_pos), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
}

//=================================================================================================
void Drain::Save(HANDLE file)
{
	WriteFile(file, &from->refid, sizeof(from->refid), &tmp, NULL);
	WriteFile(file, &to->refid, sizeof(from->refid), &tmp, NULL);
	WriteFile(file, &pe->refid, sizeof(pe->refid), &tmp, NULL);
	WriteFile(file, &t, sizeof(t), &tmp, NULL);
}

//=================================================================================================
void Drain::Load(HANDLE file)
{
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	from = Unit::GetByRefid(refid);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	to = Unit::GetByRefid(refid);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	pe = ParticleEmitter::GetByRefid(refid);
	ReadFile(file, &t, sizeof(t), &tmp, NULL);
}
