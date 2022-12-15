#pragma once

//-----------------------------------------------------------------------------
#include "Skill.h"

//-----------------------------------------------------------------------------
struct Ability
{
	enum Type
	{
		Point,
		Ray,
		Target,
		Ball,
		Charge,
		Summon,
		Aggro,
		SummonAway,
		RangedAttack,
		Trap
	};

	enum Effect
	{
		None,
		// for Target type
		Heal,
		Raise,
		// for Ray type
		Drain,
		Electro,
		// for Charge type
		Stun,
		// for RangedAttack type
		Rooted
	};

	enum Flags
	{
		Explode = 1 << 0, // bullets will explode
		Poison = 1 << 1, // bullets will poison target
		Triple = 1 << 2, // spawn 3 bullets, only for type Point/Ball
		NonCombat = 1 << 3, // ai can use this outside of combat
		Cleric = 1 << 4, // uses charisma/gods magic, train gods magic skill
		IgnoreUnits = 1 << 5, // for type Charge
		PickDir = 1 << 6, // for type Charge
		UseCast = 1 << 7, // use cast spell animation
		Mage = 1 << 8, // uses int/mystic magic, train mystic magic skill, require wand to cast
		Strength = 1 << 9, // use Strength to calculate damage
		Boss50Hp = 1 << 10, // can be used when hp below 50%
		DefaultAttack = 1 << 11, // don't use golden crosshair when casting
		UseKneel = 1 << 12, // use kneel animation
	};

	int hash;
	string id, animation;
	SoundPtr soundCast, soundHit;
	TexturePtr tex, texParticle, texIcon;
	ParticleEffect* effect1; FIXME; // better name
	TexOverride texExplode;
	Vec2 cooldown;
	Type type;
	Effect effect;
	int flags, dmg, dmgBonus, charges, learningPoints, skill, level, count;
	float range, moveRange, size, sizeParticle, speed, explodeRange, soundCastDist, soundHitDist, mana, stamina, recharge, width, time, castTime;
	btCollisionShape* shape;
	Mesh* mesh;
	string name, desc, unitId;
	union
	{
		UnitData* unit;
		BaseTrap* trap;
	};
	Color color; // RangedAttack trail color

	Ability() : soundCast(nullptr), soundHit(nullptr), tex(nullptr), texParticle(nullptr), texIcon(nullptr), shape(nullptr), mesh(nullptr), type(Point),
		cooldown(0, 0), flags(0), dmg(0), dmgBonus(0), range(10.f), moveRange(10.f), size(0.f), sizeParticle(0.f), speed(0.f), explodeRange(0.f),
		soundCastDist(1.f), soundHitDist(2.f), mana(0), stamina(0), charges(1), recharge(0), width(0), effect(None), learningPoints(0), skill(999),
		level(0), count(1), time(1), color(Color::White), castTime(0) {}
	~Ability()
	{
		delete shape;
	}

	bool IsTargeted() const { return !Any(type, Aggro, SummonAway); }
	bool RequireList() const { return IsSet(flags, IgnoreUnits); }
	SkillId GetSkill() const;

	static vector<Ability*> abilities;
	static std::map<int, Ability*> hashAbilities;
	static Ability* Get(int hash);
	static Ability* Get(Cstring id) { return Get(Hash(id)); }
	static Ability* GetS(const string& id) { return Get(id); }
};
