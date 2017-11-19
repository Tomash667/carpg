// efekty czarów, eksplozje, electro, drain
#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Unit;
struct Spell;
struct ParticleEmitter;

//-----------------------------------------------------------------------------
struct Explo
{
	Vec3 pos;
	float size, sizemax, dmg;
	TexturePtr tex;
	vector<Unit*> hitted;
	Unit* owner;
	bool stun;

	static const int MIN_SIZE = 21;

	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct ElectroLine
{
	vector<Vec3> pts;
	float t;
};

//-----------------------------------------------------------------------------
struct Electro
{
	vector<ElectroLine> lines;
	vector<Unit*> hitted;
	float dmg;
	Unit* owner;
	Spell* spell;
	bool valid, hitsome;
	Vec3 start_pos;
	int netid;

	static const int MIN_SIZE = 5;
	static const int LINE_MIN_SIZE = 28;

	void AddLine(const Vec3& from, const Vec3& to);
	void Save(HANDLE file);
	void Load(HANDLE file);
};

//-----------------------------------------------------------------------------
struct Drain
{
	Unit* from, *to;
	ParticleEmitter* pe;
	float t;

	void Save(HANDLE file);
	void Load(HANDLE file);
};
