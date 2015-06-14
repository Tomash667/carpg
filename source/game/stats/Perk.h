#pragma once

//-----------------------------------------------------------------------------
struct CreatedCharacter;
struct PlayerController;

//-----------------------------------------------------------------------------
enum class Perk
{
	Weakness,
	Strength,
	Skilled,
	SkillFocus,
	Talent,
	CraftingTradition,
	Max
};

//-----------------------------------------------------------------------------
struct PerkInfo
{
	enum Flags
	{
		Flaw = 1 << 0,
		History = 1 << 1,
		Free = 1 << 2,
		Multiple = 1 << 3,
		Validate = 1 << 4,
	};

	Perk perk_id;
	cstring id;
	string name, desc;
	int flags;

	inline PerkInfo(Perk perk_id, cstring id, int flags) : perk_id(perk_id), id(id), flags(flags)
	{

	}

	static PerkInfo* Find(const string& id);	
};

//-----------------------------------------------------------------------------
struct TakenPerk
{
	Perk perk;
	int value;

	inline TakenPerk()
	{

	}

	inline TakenPerk(Perk perk, int value = -1) : perk(perk), value(value)
	{

	}

	void GetDesc(string& s) const;
	int Apply(CreatedCharacter& cc, bool validate=false) const;
	void Apply(PlayerController& pc) const;
	void Remove(CreatedCharacter& cc) const;

	static void LoadText();

	static cstring txIncrasedAttrib, txIncrasedSkill, txDecrasedAttrib, txDecrasedSkill, txDecrasedSkills;
};

//-----------------------------------------------------------------------------
extern PerkInfo g_perks[(int)Perk::Max];

//-----------------------------------------------------------------------------
inline bool SortPerks(Perk p1, Perk p2)
{
	return strcoll(g_perks[(int)p1].name.c_str(), g_perks[(int)p2].name.c_str()) < 0;
}
inline bool SortTakenPerks(const std::pair<Perk, int>& tp1, const std::pair<Perk, int>& tp2)
{
	return SortPerks(tp1.first, tp2.first);
}
