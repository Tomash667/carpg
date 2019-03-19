#include "Pch.h"
#include "GameCore.h"
#include "LevelContext.h"
#include "Game.h"
#include "Chest.h"
#include "Level.h"
#include "City.h"
#include "Room.h"
#include "ParticleSystem.h"
#include "GroundItem.h"
#include "Unit.h"

//=================================================================================================
void TmpLevelContext::Clear()
{
	bullets.clear();
	DeleteElements(pes);
	DeleteElements(tpes);
	DeleteElements(explos);
	DeleteElements(electros);
	drains.clear();
	colliders.clear();
}

//=================================================================================================
void LevelContext::BuildRefidTables()
{
	for(vector<ParticleEmitter*>::iterator it2 = pes->begin(), end2 = pes->end(); it2 != end2; ++it2)
	{
		(*it2)->refid = (int)ParticleEmitter::refid_table.size();
		ParticleEmitter::refid_table.push_back(*it2);
	}
	for(vector<TrailParticleEmitter*>::iterator it2 = tpes->begin(), end2 = tpes->end(); it2 != end2; ++it2)
	{
		(*it2)->refid = (int)TrailParticleEmitter::refid_table.size();
		TrailParticleEmitter::refid_table.push_back(*it2);
	}
}

//=================================================================================================
void LevelContext::RemoveDeadUnits()
{
	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		if((*it)->live_state == Unit::DEAD && (*it)->IsAI())
		{
			(*it)->to_remove = true;
			L.to_remove.push_back(*it);
			if(it + 1 == end)
			{
				units->pop_back();
				return;
			}
			else
			{
				std::iter_swap(it, end - 1);
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
			unit->items.erase(unit->items.begin() + slot);
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
			chest->items.erase(chest->items.begin() + slot);
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
			if(item_slot != Unit::INVALID_IINDEX)
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
			if(it + 1 != end)
				std::iter_swap(it, end - 1);
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
Object* LevelContext::FindObject(BaseObject* base_obj)
{
	assert(base_obj);

	for(Object* obj : *objects)
	{
		if(obj->base == base_obj)
			return obj;
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
Chest* LevelContext::GetRandomFarChest(const Int2& pt)
{
	vector<pair<Chest*, float>> far_chests;
	float close_dist = -1.f;
	Vec3 pos = PtToPos(pt);

	// znajdü 5 najdalszych skrzyni
	for(vector<Chest*>::iterator it = chests->begin(), end = chests->end(); it != end; ++it)
	{
		float dist = Vec3::Distance2d(pos, (*it)->pos);
		if(dist > close_dist)
		{
			if(far_chests.empty())
				far_chests.push_back(pair<Chest*, float>(*it, dist));
			else
			{
				for(vector<pair<Chest*, float>>::iterator it2 = far_chests.begin(), end2 = far_chests.end(); it2 != end2; ++it2)
				{
					if(dist > it2->second)
					{
						far_chests.insert(it2, pair<Chest*, float>(*it, dist));
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
		index = Rand() % (chests->size() - 5);
	}
	else
		index = Rand() % 5;

	return far_chests[index].first;
}

//=================================================================================================
cstring LevelContext::GetName()
{
	switch(type)
	{
	case Inside:
		return "Inside";
	case Outside:
		return "Outside";
	default:
		return Format("Building%d", building_id);
	}
}

//=================================================================================================
void LevelContext::SetTmpCtx(TmpLevelContext* ctx)
{
	assert(ctx);
	tmp_ctx = ctx;
	bullets = &ctx->bullets;
	pes = &ctx->pes;
	tpes = &ctx->tpes;
	explos = &ctx->explos;
	electros = &ctx->electros;
	drains = &ctx->drains;
	colliders = &ctx->colliders;
	ctx->Clear();
}

//=================================================================================================
Unit* LevelContext::FindUnit(UnitData* ud)
{
	assert(ud);

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
			return *it;
	}

	return nullptr;
}

//=================================================================================================
Usable* LevelContext::FindUsable(BaseUsable* base)
{
	assert(base);

	for(Usable* use : *usables)
	{
		if(use->base == base)
			return use;
	}

	return nullptr;
}


//=================================================================================================
LevelContext& LevelContextEnumerator::Iterator::operator * () const
{
	assert(index != -2);
	if(index == -1)
		return L.local_ctx;
	else
		return static_cast<City*>(loc)->inside_buildings[index]->ctx;
}

//=================================================================================================
LevelContextEnumerator::Iterator& LevelContextEnumerator::Iterator::operator ++ ()
{
	++index;
	if(loc->type != L_CITY || index >= (int)static_cast<City*>(loc)->inside_buildings.size())
		index = -2;
	return *this;
}
