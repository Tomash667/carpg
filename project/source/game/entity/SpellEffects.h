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
	Unit* owner;
	Vec3 pos;
	float size, sizemax, dmg;
	TexturePtr tex;
	vector<Unit*> hitted;

	static const int MIN_SIZE = 21;

	void Save(HANDLE file);
	void Load(FileReader& f);
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
	int netid;
	Spell* spell;
	Unit* owner;
	vector<ElectroLine> lines;
	vector<Unit*> hitted;
	float dmg;
	bool valid, hitsome;
	Vec3 start_pos;

	static const int MIN_SIZE = 5;
	static const int LINE_MIN_SIZE = 28;
	static int netid_counter;

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
