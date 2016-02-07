// kontekst poziomu
#include "Pch.h"
#include "Base.h"
#include "LevelContext.h"
#include "Game.h"

//=================================================================================================
void LevelContext::RemoveDeadUnits()
{
	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		if((*it)->live_state == Unit::DEAD && (*it)->IsAI())
		{
			(*it)->to_remove = true;
			Game::Get().to_remove.push_back(*it);
			if(it+1 == end)
			{
				units->pop_back();
				return;
			}
			else
			{
				std::iter_swap(it, end-1);
				units->pop_back();
				end = units->end();
			}
		}
	}
}

//=================================================================================================
bool LevelContext::RemoveItemFromWorld(const Item* item)
{
	assert(item);

	// szukaj w zw≥okach
	{
		Unit* unit;
		int slot;
		if(FindItemInCorpse(item, &unit, &slot))
		{
			unit->items.erase(unit->items.begin()+slot);
			return true;
		}
	}

	// szukaj na ziemi
	if(RemoveGroundItem(item))
		return true;

	// szukaj w skrzyni
	if(chests)
	{
		Chest* chest;
		int slot;
		if(FindItemInChest(item, &chest, &slot))
		{
			chest->items.erase(chest->items.begin()+slot);
			return true;
		}
	}

	return false;
}

//=================================================================================================
bool LevelContext::FindItemInCorpse(const Item* item, Unit** unit, int* slot)
{
	assert(item);

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		if(!(*it)->IsAlive())
		{
			int item_slot = (*it)->FindItem(item);
			if(item_slot != -1)
			{
				if(unit)
					*unit = *it;
				if(slot)
					*slot = item_slot;
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
bool LevelContext::RemoveGroundItem(const Item* item)
{
	assert(item);

	for(vector<GroundItem*>::iterator it = items->begin(), end = items->end(); it != end; ++it)
	{
		if((*it)->item == item)
		{
			delete *it;
			if(it+1 != end)
				std::iter_swap(it, end-1);
			items->pop_back();
			return true;
		}
	}

	return false;
}

//=================================================================================================
bool LevelContext::FindItemInChest(const Item* item, Chest** chest, int* slot)
{
	assert(item);

	for(vector<Chest*>::iterator it = chests->begin(), end = chests->end(); it != end; ++it)
	{
		int item_slot = (*it)->FindItem(item);
		if(item_slot != -1)
		{
			if(chest)
				*chest = *it;
			if(slot)
				*slot = item_slot;
			return true;
		}
	}

	return false;
}

//=================================================================================================
Object* LevelContext::FindObject(Obj* obj)
{
	assert(obj);

	for(vector<Object>::iterator it = objects->begin(), end = objects->end(); it != end; ++it)
	{
		if(it->base == obj)
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
Chest* LevelContext::FindChestInRoom(const Room& p)
{
	for(vector<Chest*>::iterator it = chests->begin(), end = chests->end(); it != end; ++it)
	{
		if(p.IsInside((*it)->pos))
			return *it;
	}

	return nullptr;
}

//=================================================================================================
Chest* LevelContext::GetRandomFarChest(const INT2& pt)
{
	vector<std::pair<Chest*, float> > far_chests;
	float close_dist = -1.f;
	VEC3 pos = pt_to_pos(pt);

	// znajdü 5 najdalszych skrzyni
	for(vector<Chest*>::iterator it = chests->begin(), end = chests->end(); it != end; ++it)
	{
		float dist = distance2d(pos, (*it)->pos);
		if(dist > close_dist)
		{
			if(far_chests.empty())
				far_chests.push_back(std::pair<Chest*, float>(*it, dist));
			else
			{
				for(vector<std::pair<Chest*, float> >::iterator it2 = far_chests.begin(), end2 = far_chests.end(); it2 != end2; ++it2)
				{
					if(dist > it2->second)
					{
						far_chests.insert(it2, std::pair<Chest*, float>(*it, dist));
						break;
					}
				}
			}
			if(far_chests.size() > 5u)
			{
				far_chests.pop_back();
				close_dist = far_chests.back().second;
			}
		}
	}

	int index;
	if(far_chests.size() != 5u)
	{
		// jeøeli skrzyni jest ma≥o wybierz najdalszπ
		index = 0;
	}
	else if(chests->size() < 10u)
	{
		// jeøeli skrzyni by≥o mniej niø 10 to czÍúÊ moøe byÊ za blisko
		index = rand2()%(chests->size()-5);
	}
	else
		index = rand2()%5;

	return far_chests[index].first;
}
