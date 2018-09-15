#pragma once

namespace ItemHelper
{
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold, bool extra);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
}
