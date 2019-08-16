#pragma once

//-----------------------------------------------------------------------------
struct Class
{
	string id, player_id, hero_id, crazy_id, name, desc, about;
	vector<string> names, nicknames;
	Texture* icon;
	UnitData* player, *hero, *crazy;
	int player_weight, hero_weight, crazy_weight;
	Action* action;

	Class() : icon(nullptr), player(nullptr), hero(nullptr), crazy(nullptr), action(nullptr)
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
