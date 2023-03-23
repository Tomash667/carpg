#pragma once

//-----------------------------------------------------------------------------
namespace old
{
	// pre V_0_14 compatibility
	enum class Perk
	{
		None = -1,

		// negative starting perks
		BadBack, // -5 str, -25% carry
		ChronicDisease, // -5 end, 50% natural healing
		Sluggish, // -5 dex, -25 mobility
		SlowLearner, // -5 int, -1 apt to all skills
		Asocial, // -5 cha, worse prices
		Poor, // no gold, worse items

		// positive starting perks
		Talent, // +5 attrib, +1 apt
		Skilled, // +3 skill points
		SkillFocus, // +5 skill, +1 apt (only for skills with value >= 0)
		AlchemistApprentice, // +10 alchemy, potions & ladle
		Wealthy, // +2.5k gold
		VeryWealthy, // [C] +50k gold
		FamilyHeirloom, // good starting item
		Leader, // start with npc

		// normal perks
		StrongBack, // (60 str) +25% carry
		Aggressive, // (60 str) +10 melee attack
		Mobility, // (60 dex) +20 mobility
		Finesse, // (60 dex) +10 ranged attack
		Tough, // (60 end) +hp
		HardSkin, // (60 end) +10 def
		Adaptation, // (75 end) poison resistance
		PerfectHealth, // (90 end) +5 hp reg
		Energetic, // (60 dex/end) +100 stamina
		StrongAura, // (60 wis) +X mp
		ManaHarmony, // (90 wis) +X mp reg
		MagicAdept, // (60 int) +magic power

		// mixed - perks are saved by id so can't reorder currently
		TravelingMerchant, // +10 persuasion, more gold

		Max // max 256, saved as byte
	};

	::Perk* Convert(Perk perk);
}

//-----------------------------------------------------------------------------
struct Perk
{
	enum Flags
	{
		Flaw = 1 << 0,
		Start = 1 << 1,
		History = 1 << 2
	};

	enum ValueType
	{
		None,
		Attribute,
		Skill
	};

	struct Required
	{
		enum Type
		{
			HAVE_PERK,
			HAVE_NO_PERK,
			HAVE_ATTRIBUTE,
			HAVE_SKILL,
			CAN_MOD
		};
		Type type;
		int subtype;
		int value;
	};

	struct Effect
	{
		enum Type
		{
			ATTRIBUTE,
			SKILL,
			EFFECT,
			APTITUDE,
			SKILL_POINT,
			GOLD
		};
		Type type;
		int subtype;
		union
		{
			int value;
			float valuef;
		};
	};

	int hash;
	string id, name, desc, details;
	int parent, flags, cost;
	ValueType valueType;
	vector<Required> required;
	vector<Effect> effects;
	bool defined;

	Perk() : parent(0), flags(0), cost(0), valueType(None), defined(false) {}

	// hardcoded
	static Ptr<Perk> alchemistApprentice, asocial, heirloom, leader, poor;

	static vector<Perk*> perks;
	static std::map<int, Perk*> hashPerks;
	static Perk* Get(int hash);
	static Perk* Get(Cstring id) { return Get(Hash(id)); }
	static void Validate(uint& err);
};

//-----------------------------------------------------------------------------
struct PerkContext
{
	CreatedCharacter* cc;
	PlayerController* pc;
	bool startup, validate, checkRemove;

	PerkContext(CreatedCharacter* cc, PlayerController* pc, bool startup)
		: cc(cc), pc(pc), startup(startup), validate(false), checkRemove(false) {}
	PerkContext(CreatedCharacter* cc) : PerkContext(cc, nullptr, true) {}
	PerkContext(PlayerController* pc, bool startup) : PerkContext(nullptr, pc, startup) {}
	bool CanMod(AttributeId a);
	bool HavePerk(Perk* perk);
	bool Have(AttributeId a, int value);
	bool Have(SkillId s, int value);
	void AddEffect(Perk* perk, EffectId effect, float power = 0);
	void RemoveEffect(Perk* perk);
	void Mod(AttributeId a, int value, bool mod);
	void Mod(SkillId s, int value, bool mod);
	void AddPerk(Perk* perk);
	void RemovePerk(Perk* perk);
};

//-----------------------------------------------------------------------------
struct TakenPerk
{
	Perk* perk;
	int value;

	TakenPerk() {}
	TakenPerk(Perk* perk, int value = -1) : perk(perk), value(value) {}
	void GetDetails(string& str) const;
	void Apply(PerkContext& ctx);
	void Remove(PerkContext& ctx);
	cstring FormatName();
	bool CanTake(PerkContext& ctx);

	static cstring txIncreasedAttrib, txIncreasedSkill, txDecreasedAttrib, txDecreasedSkill;
};

//-----------------------------------------------------------------------------
inline bool SortPerks(Perk* p1, Perk* p2)
{
	return strcoll(p1->name.c_str(), p2->name.c_str()) < 0;
}
inline bool SortTakenPerks(const pair<cstring, int>& tp1, const pair<cstring, int>& tp2)
{
	return strcoll(tp1.first, tp2.first) < 0;
}
