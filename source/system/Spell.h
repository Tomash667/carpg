#pragma once

//-----------------------------------------------------------------------------
// Czar
struct Spell
{
	enum Type
	{
		Point,
		Ray,
		Target,
		Ball
	};

	enum Flags
	{
		Explode = 1 << 0,
		Poison = 1 << 1,
		Raise = 1 << 2,
		Jump = 1 << 3,
		Drain = 1 << 4,
		Hold = 1 << 5,
		Triple = 1 << 6, // tylko dla Point i Ball
		Heal = 1 << 7,
		NonCombat = 1 << 8,
		Cleric = 1 << 9, // uses charisma/gods magic instead of magic power/level
	};

	string id;
	SoundPtr sound_cast, sound_hit;
	TexturePtr tex, tex_particle;
	TexOverride tex_explode;
	Vec2 cooldown;
	Type type;
	int flags, dmg, dmg_bonus;
	float range, move_range, size, size_particle, speed, explode_range, sound_cast_dist, sound_hit_dist, mana;
	btCollisionShape* shape;
	Mesh* mesh;

	Spell() : sound_cast(nullptr), sound_hit(nullptr), tex(nullptr), tex_particle(nullptr), shape(nullptr), mesh(nullptr), type(Point), cooldown(0, 0),
		flags(0), dmg(0), dmg_bonus(0), range(10.f), move_range(10.f), size(0.f), size_particle(0.f), speed(0.f), explode_range(0.f), sound_cast_dist(1.f),
		sound_hit_dist(2.f), mana(0) {}
	~Spell()
	{
		delete shape;
	}

	static vector<Spell*> spells;
	static vector<pair<string, Spell*>> aliases;
	static Spell* TryGet(Cstring id);
};
