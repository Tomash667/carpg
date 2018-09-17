#pragma once

namespace ItemHelper
{
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void GenerateMerchantItems(vector<ItemSlot>& items, int price_limit);
	void GenerateBlacksmithItems(vector<ItemSlot>& items, int price_limit, int count_mod, bool is_city);
	void GenerateAlchemistItems(vector<ItemSlot>& items, int count_mod);
	void GenerateInnkeeperItems(vector<ItemSlot>& items, int count_mod, bool is_city);
	void GenerateFoodSellerItems(vector<ItemSlot>& items, bool is_city);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);
	const Item* GetRandomItem(int max_value);
}
