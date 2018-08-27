#pragma once

//-----------------------------------------------------------------------------
enum StockEntry
{
	SE_ADD,
	SE_MULTIPLE,
	SE_ITEM,
	SE_CHANCE,
	SE_RANDOM,
	SE_CITY,
	SE_NOT_CITY,
	SE_ANY_CITY,
	SE_START_SET,
	SE_END_SET,
	SE_LIST,
	SE_LEVELED_LIST,
	SE_SAME_MULTIPLE,
	SE_SAME_RANDOM
};

//-----------------------------------------------------------------------------
struct Stock
{
	string id;
	vector<int> code;

	void Parse(int level, bool city, vector<ItemSlot>& items);

private:
	void AddItems(vector<ItemSlot>& items, StockEntry type, int code, int level, uint count, bool same);

public:
	static vector<Stock*> stocks;
	static Stock* TryGet(Cstring id);
	static Stock* Get(Cstring id)
	{
		auto stock = TryGet(id);
		assert(stock);
		return stock;
	}
};
