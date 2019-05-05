#pragma once

//-----------------------------------------------------------------------------
#include "Item.h"

//-----------------------------------------------------------------------------
enum ParseScript
{
	PS_END,
	PS_ONE,
	PS_CHANCE,
	PS_ONE_OF_MANY,
	PS_IF_LEVEL,
	PS_ELSE,
	PS_END_IF,
	PS_CHANCE2,
	PS_IF_CHANCE,
	PS_MANY,
	PS_RANDOM,
	PS_ITEM,
	PS_LIST,
	PS_LEVELED_LIST,
	PS_LEVELED_LIST_MOD,
	PS_SPECIAL_ITEM,
	PS_SPECIAL_ITEM_MOD
};

//-----------------------------------------------------------------------------
enum SpecialItem
{
	SPECIAL_WEAPON,
	SPECIAL_ARMOR
};

//-----------------------------------------------------------------------------
struct ItemScript
{
	string id;
	vector<int> code;
	bool is_subprofile;

	void Test(string& errors, uint& count);
	void CheckItem(const int*& ps, string& errors, uint& count);
	void Parse(Unit& unit) const;
private:
	void GiveItem(Unit& unit, const int*& ps, int count) const;
	void SkipItem(const int*& ps, int count) const;

public:
	static vector<ItemScript*> scripts;
	static const LeveledItemList* weapon_list[WT_MAX], *armor_list[AT_MAX];
	static ItemScript* TryGet(Cstring id);
	static void Init();
	static const LeveledItemList& GetSpecial(int special, int sub);
};
