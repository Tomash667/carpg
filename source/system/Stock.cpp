#include "Pch.h"
#include "GameCore.h"
#include "Stock.h"
#include "ItemSlot.h"
#include "Item.h"
#include "Level.h"
#include "ScriptManager.h"
#include <angelscript.h>

//-----------------------------------------------------------------------------
vector<Stock*> Stock::stocks;

//-----------------------------------------------------------------------------
enum class CityBlock
{
	IN,
	OUT,
	ANY
};

//=================================================================================================
inline bool CheckCity(CityBlock in_city, bool city)
{
	if(in_city == CityBlock::IN)
		return city;
	else if(in_city == CityBlock::OUT)
		return !city;
	else
		return true;
}

//=================================================================================================
Stock::~Stock()
{
	if(script)
		script->Release();
}

//=================================================================================================
void Stock::Parse(vector<ItemSlot>& items)
{
	if(script)
	{
		ScriptContext& ctx = script_mgr->GetContext();
		ctx.stock = &items;
		script_mgr->RunScript(script);
		ctx.stock = nullptr;
	}

	ParseInternal(items);
	SortItems(items);
}

//=================================================================================================
void Stock::ParseInternal(vector<ItemSlot>& items)
{
	CityBlock in_city = CityBlock::ANY;
	LocalVector<int> sets;
	bool in_set = false;
	bool city = (game_level->location && game_level->IsCity());
	uint i = 0;

	do
	{
		for(; i < code.size(); ++i)
		{
			StockEntry action = (StockEntry)code[i];
			switch(action)
			{
			case SE_ADD:
				if(CheckCity(in_city, city))
				{
					++i;
					StockEntry type = (StockEntry)code[i];
					++i;
					AddItems(items, type, code[i], 1, true);
				}
				else
					i += 2;
				break;
			case SE_MULTIPLE:
			case SE_SAME_MULTIPLE:
				if(CheckCity(in_city, city))
				{
					++i;
					int count = code[i];
					++i;
					StockEntry type = (StockEntry)code[i];
					++i;
					AddItems(items, type, code[i], (uint)count, action == SE_SAME_MULTIPLE);
				}
				else
					i += 3;
				break;
			case SE_CHANCE:
				if(CheckCity(in_city, city))
				{
					++i;
					int count = code[i];
					++i;
					int chance = code[i];
					int ch = Rand() % chance;
					int total = 0;
					bool done = false;
					for(int j = 0; j < count; ++j)
					{
						++i;
						StockEntry type = (StockEntry)code[i];
						++i;
						int c = code[i];
						++i;
						total += code[i];
						if(ch < total && !done)
						{
							done = true;
							AddItems(items, type, c, 1, true);
						}
					}
				}
				else
				{
					++i;
					int count = code[i];
					i += 1 + 3 * count;
				}
				break;
			case SE_RANDOM:
			case SE_SAME_RANDOM:
				if(CheckCity(in_city, city))
				{
					++i;
					int a = code[i];
					++i;
					int b = code[i];
					++i;
					StockEntry type = (StockEntry)code[i];
					++i;
					uint count = (uint)Random(a, b);
					if(count > 0)
						AddItems(items, type, code[i], count, action == SE_SAME_RANDOM);
				}
				else
					i += 4;
				break;
			case SE_CITY:
				in_city = CityBlock::IN;
				break;
			case SE_NOT_CITY:
				in_city = CityBlock::OUT;
				break;
			case SE_ANY_CITY:
				in_city = CityBlock::ANY;
				break;
			case SE_START_SET:
				assert(!in_set);
				sets.push_back(i + 1);
				while(code[i] != SE_END_SET)
					++i;
				break;
			case SE_END_SET:
				assert(in_set);
				return;
			default:
				assert(0);
				break;
			}
		}

		if(sets.size() > 0)
		{
			i = sets[Rand() % sets.size()];
			in_set = true;
		}
		else
			break;
	}
	while(true);
}

//=================================================================================================
void Stock::AddItems(vector<ItemSlot>& items, StockEntry type, int code, uint count, bool same)
{
	switch(type)
	{
	case SE_ITEM:
		InsertItemBare(items, reinterpret_cast<const Item*>(code), count);
		break;
	case SE_LIST:
		{
			ItemList* lis = reinterpret_cast<ItemList*>(code);
			if(same)
				InsertItemBare(items, lis->Get(), count);
			else
			{
				for(uint i = 0; i < count; ++i)
					InsertItemBare(items, lis->Get());
			}
		}
		break;
	case SE_LEVELED_LIST:
		{
			LeveledItemList* llis = reinterpret_cast<LeveledItemList*>(code);
			if(same)
				InsertItemBare(items, llis->Get(), count);
			else
			{
				for(uint i = 0; i < count; ++i)
					InsertItemBare(items, llis->Get());
			}
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
Stock* Stock::TryGet(Cstring id)
{
	for(auto stock : stocks)
	{
		if(stock->id == id)
			return stock;
	}

	return nullptr;
}
