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
	SE_SAME_MULTIPLE,
	SE_SAME_RANDOM
};

//-----------------------------------------------------------------------------
struct Stock
{
	string id;
	vector<int> code;
	asIScriptFunction* script;

	Stock() : script(nullptr) {}
	~Stock();
	void Parse(vector<ItemSlot>& items);

private:
	void AddItems(vector<ItemSlot>& items, StockEntry type, int code, uint count, bool same);
	void ParseInternal(vector<ItemSlot>& items);

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
