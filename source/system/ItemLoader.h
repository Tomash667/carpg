#pragma once

//-----------------------------------------------------------------------------
#include "ContentLoader.h"

//-----------------------------------------------------------------------------
class ItemLoader : public ContentLoader
{
	friend class Content;
private:
	void DoLoading() override;
	static void Cleanup();
	void InitTokenizer() override;
	void LoadEntity(int top, const string& id) override;
	void Finalize() override;
	void ParseItem(ITEM_TYPE type, const string& id);
	void ParseItemList(const string& id);
	void ParseLeveledItemList(const string& id);
	void ParseStock(const string& id);
	void ParseItemOrList(Stock* stock);
	void ParseBookScheme(const string& id);
	void ParseStartItems();
	void ParseBetterItems();
	void ParseAlias(const string& id);
	void CalculateCrc();
};
