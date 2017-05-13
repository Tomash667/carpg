// efekty czarów, eksplozje, electro, drain
#include "Pch.h"
#include "Base.h"
#include "SpellEffects.h"
#include "Unit.h"
#include "Spell.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"

//=================================================================================================
void Explo::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &size, sizeof(size), &tmp, nullptr);
	WriteFile(file, &sizemax, sizeof(sizemax), &tmp, nullptr);
	WriteFile(file, &dmg, sizeof(dmg), &tmp, nullptr);
	uint count = hitted.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, nullptr);
	int refid = (owner ? owner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	byte len = (byte)strlen(tex->filename);
	WriteFile(file, &len, sizeof(len), &tmp, nullptr);
	WriteFile(file, tex->filename, len, &tmp, nullptr);
}

//=================================================================================================
void Explo::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &size, sizeof(size), &tmp, nullptr);
	ReadFile(file, &sizemax, sizeof(sizemax), &tmp, nullptr);
	ReadFile(file, &dmg, sizeof(dmg), &tmp, nullptr);
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	hitted.resize(count);
	int refid;
	for(uint i=0; i<count; ++i)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		hitted[i] = Unit::GetByRefid(refid);
	}
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	owner = Unit::GetByRefid(refid);
	ReadString1(file);
	tex = ResourceManager::Get().GetLoadedTexture(BUF);
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
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<ElectroLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
	{
		count = it->pts.size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		if(count)
			WriteFile(file, &it->pts[0], sizeof(VEC3)*count, &tmp, nullptr);
		WriteFile(file, &it->t, sizeof(it->t), &tmp, nullptr);
	}
	count = hitted.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, nullptr);
	WriteFile(file, &dmg, sizeof(dmg), &tmp, nullptr);
	int refid = (owner ? owner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteString1(file, spell->id);
	WriteFile(file, &valid, sizeof(valid), &tmp, nullptr);
	WriteFile(file, &hitsome, sizeof(hitsome), &tmp, nullptr);
	WriteFile(file, &start_pos, sizeof(start_pos), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
}

//=================================================================================================
void Electro::Load(HANDLE file)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	lines.resize(count);
	for(vector<ElectroLine>::iterator it = lines.begin(), end = lines.end(); it != end; ++it)
	{
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		it->pts.resize(count);
		if(count)
			ReadFile(file, &it->pts[0], sizeof(VEC3)*count, &tmp, nullptr);
		ReadFile(file, &it->t, sizeof(it->t), &tmp, nullptr);
	}
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	hitted.resize(count);
	int refid;
	for(vector<Unit*>::iterator it = hitted.begin(), end = hitted.end(); it != end; ++it)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
		*it = Unit::GetByRefid(refid);
	}
	ReadFile(file, &dmg, sizeof(dmg), &tmp, nullptr);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	owner = Unit::GetByRefid(refid);
	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);
	BUF[len] = 0;
	ReadFile(file, BUF, len, &tmp, nullptr);
	spell = FindSpell(BUF);
	if(!spell)
		throw Format("Missing spell '%s' for electro.", BUF);
	ReadFile(file, &valid, sizeof(valid), &tmp, nullptr);
	ReadFile(file, &hitsome, sizeof(hitsome), &tmp, nullptr);
	ReadFile(file, &start_pos, sizeof(start_pos), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
}

//=================================================================================================
void Drain::Save(HANDLE file)
{
	WriteFile(file, &from->refid, sizeof(from->refid), &tmp, nullptr);
	WriteFile(file, &to->refid, sizeof(from->refid), &tmp, nullptr);
	WriteFile(file, &pe->refid, sizeof(pe->refid), &tmp, nullptr);
	WriteFile(file, &t, sizeof(t), &tmp, nullptr);
}

//=================================================================================================
void Drain::Load(HANDLE file)
{
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	from = Unit::GetByRefid(refid);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	to = Unit::GetByRefid(refid);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	pe = ParticleEmitter::GetByRefid(refid);
	ReadFile(file, &t, sizeof(t), &tmp, nullptr);
}
