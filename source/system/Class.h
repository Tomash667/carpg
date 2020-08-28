#pragma once

//-----------------------------------------------------------------------------
struct Class
{
	enum Flags
	{
		F_MP_BAR = 1 << 0,
		F_MAGE_ITEMS = 1 << 1,
		F_FROM_FOREST = 1 << 2,
		F_AUTOHEAL = 1 << 3
	};

	struct LevelEntry
	{
		enum Type
		{
			T_ATTRIBUTE,
			T_SKILL,
			T_SKILL_SPECIAL
		} type;
		enum Special
		{
			S_WEAPON,
			S_ARMOR
		};
		int value;
		float priority;
		bool required;
	};

	struct PotionEntry
	{
		int level;
		vector<pair<const Item*, int>> items;
	};

	string id, player_id, hero_id, crazy_id, name, desc, about;
	vector<string> names, nicknames;
	vector<LevelEntry> level;
	Texture* icon;
	UnitData* player, *hero, *crazy;
	Ability* ability;
	vector<PotionEntry> potions;
	int flags;

	Class() : icon(nullptr), player(nullptr), hero(nullptr), crazy(nullptr), ability(nullptr), flags(0)
	{
	}
	bool IsPickable() const { return player != nullptr; }
	const PotionEntry& GetPotionEntry(int level) const;

	static vector<Class*> classes;
	static Class* TryGet(Cstring id);
	static Class* GetRandomPlayer();
	static Class* GetRandomHero(bool evil = false);
	static UnitData* GetRandomHeroData(bool evil = false);
	static Class* GetRandomCrazy();
	static UnitData* GetRandomCrazyData();
	static int InitLists();
private:
	static int VerifyGroup(UnitGroup* group, bool crazy);
};

//-----------------------------------------------------------------------------
// pre V_0_12
namespace old
{
	enum class Class
	{
		BARBARIAN,
		BARD,
		CLERIC,
		DRUID,
		HUNTER,
		MAGE,
		MONK,
		PALADIN,
		ROGUE,
		WARRIOR,

		MAX,
		INVALID,
		RANDOM
	};

	::Class* ConvertOldClass(int clas);
}
