#pragma once

//-----------------------------------------------------------------------------
struct Bullet
{
	Unit* owner;
	Spell* spell;
	Vec3 pos, rot, start_pos;
	Mesh* mesh;
	float speed, timer, attack, tex_size, yspeed, poison_attack, backstab;
	int level;
	TexturePtr tex;
	TrailParticleEmitter* trail;
	ParticleEmitter* pe;
	bool remove;

	static const int MIN_SIZE = 41;

	void Save(FileWriter& f);
	void Load(FileReader& f);
};
