#pragma once

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
	PS_LEVELED_LIST_MOD
};

//-----------------------------------------------------------------------------
struct ItemScript
{
	string id;
	vector<int> code;

	void Test(string& errors, uint& count);
	void CheckItem(const int*& ps, string& errors, uint& count);

	static vector<ItemScript*> scripts;
	static ItemScript* TryGet(Cstring id);
};
