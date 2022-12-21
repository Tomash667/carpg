#include "Pch.h"
#include "Stock.h"

#include "Item.h"
#include "ItemSlot.h"
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
inline bool CheckCity(CityBlock inCity, bool city)
{
	if(inCity == CityBlock::IN)
		return city;
	else if(inCity == CityBlock::OUT)
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
		ScriptContext& ctx = scriptMgr->GetContext();
		ctx.stock = &items;
		scriptMgr->RunScript(script);
		ctx.stock = nullptr;
	}

	ParseInternal(items);
	SortItems(items);
}

//=================================================================================================
void Stock::ParseInternal(vector<ItemSlot>& items)
{
	CityBlock inCity = CityBlock::ANY;
	LocalVector<int> sets;
	bool inSet = false;
	bool city = (gameLevel->location && gameLevel->IsCity());
	uint i = 0;

	do
	{
		for(; i < code.size(); ++i)
		{
			StockEntry action = (StockEntry)code[i];
			switch(action)
			{
			case SE_ADD:
				if(CheckCity(inCity, city))
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
				if(CheckCity(inCity, city))
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
				if(CheckCity(inCity, city))
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
				if(CheckCity(inCity, city))
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
				inCity = CityBlock::IN;
				break;
			case SE_NOT_CITY:
				inCity = CityBlock::OUT;
				break;
			case SE_ANY_CITY:
				inCity = CityBlock::ANY;
				break;
			case SE_START_SET:
				assert(!inSet);
				sets.push_back(i + 1);
				while(code[i] != SE_END_SET)
					++i;
				break;
			case SE_END_SET:
				assert(inSet);
				return;
			default:
				assert(0);
				break;
			}
		}

		if(sets.size() > 0)
		{
			i = sets[Rand() % sets.size()];
			inSet = true;
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
