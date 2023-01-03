#pragma once

//-----------------------------------------------------------------------------
namespace ItemHelper
{
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void GenerateTreasure(vector<Chest*>& chests, int level, int count);
	int GetItemPrice(const Item* item, Unit& unit, bool buy);
	const Item* GetRandomItem(int maxValue);
	const Item* GetBetterItem(const Item* item);
	void SkipStock(GameReader& f);
	void AddRandomItem(vector<ItemSlot>& items, ITEM_TYPE type, int priceLimit, int flags, uint count);
	int CalculateReward(int level, const Int2& levelRange, const Int2& priceRange);
	int GetRestCost(int days);
}
