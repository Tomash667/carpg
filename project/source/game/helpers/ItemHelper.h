#pragma once

//-----------------------------------------------------------------------------
namespace ItemHelper
{
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);
	const Item* GetRandomItem(int max_value);
	const Item* GetBetterItem(const Item* item);
	void SkipStock(FileReader& f);
	void AddRandomItem(vector<ItemSlot>& items, ITEM_TYPE type, int price_limit, int flags, uint count);
	int CalculateReward(int level, const Int2& level_range, const Int2& price_range);
}
