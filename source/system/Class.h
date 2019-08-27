#pragma once

//-----------------------------------------------------------------------------
struct Class
{
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

	string id, player_id, hero_id, crazy_id, name, desc, about;
	vector<string> names, nicknames;
	vector<LevelEntry> level;
	Texture* icon;
	UnitData* player, *hero, *crazy;
	int player_weight, hero_weight, crazy_weight;
	Action* action;
	bool mp_bar;

	Class() : icon(nullptr), player(nullptr), hero(nullptr), crazy(nullptr), action(nullptr), mp_bar(false)
	{
	}
	bool IsPickable() const { return player != nullptr; }

	static vector<Class*> classes;
	static Class* TryGet(Cstring id);
	static Class* GetRandomPlayer();
	static Class* GetRandomHero();
	static Class* GetRandomCrazy();
	static void InitLists();
};
