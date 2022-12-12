#include "Pch.h"
#include "LocationPart.h"

#include "Ability.h"
#include "AITeam.h"
#include "BitStreamFunc.h"
#include "Chest.h"
#include "Door.h"
#include "Electro.h"
#include "Explo.h"
#include "GameCommon.h"
#include "GameResources.h"
#include "GroundItem.h"
#include "Level.h"
#include "LevelPart.h"
#include "Net.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "Room.h"
#include "Trap.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <Scene.h>
#include <SceneNode.h>
#include <SoundManager.h>
#include <Terrain.h>

//=================================================================================================
LocationPart::~LocationPart()
{
	DeleteElements(units);
	DeleteElements(objects);
	DeleteElements(usables);
	DeleteElements(doors);
	DeleteElements(chests);
	DeleteElements(groundItems);
	DeleteElements(traps);
}

//=================================================================================================
void LocationPart::BuildScene()
{
	assert(lvlPart);

	Scene* scene = lvlPart->scene;

	// ground items
	for(GroundItem* groundItem : groundItems)
	{
		groundItem->CreateSceneNode();
		scene->Add(groundItem->node);
	}
}

//=================================================================================================
void LocationPart::Update(float dt)
{
	// update units
	// new units can be added inside this loop - so no iterators!
	for(uint i = 0, count = units.size(); i < count; ++i)
		units[i]->Update(dt);

	// update flickering lights
	lvlPart->lightsDt += dt;
	if(lvlPart->lightsDt >= 1.f / 20)
	{
		for(GameLight& light : lights)
		{
			light.pos = light.startPos + Vec3::Random(Vec3(-0.05f, -0.05f, -0.05f), Vec3(0.05f, 0.05f, 0.05f));
			light.color = Vec4((light.startColor + Vec3::Random(Vec3(-0.1f, -0.1f, -0.1f), Vec3(0.1f, 0.1f, 0.1f))).Clamped(), 1);
		}
		lvlPart->lightsDt = 0;
	}

	// update chests
	for(Chest* chest : chests)
		chest->meshInst->Update(dt);

	// update doors
	for(Door* door : doors)
		door->Update(dt, *this);

	LoopAndRemove(traps, [&](Trap* trap) { return trap->Update(dt, *this); });
	LoopAndRemove(lvlPart->bullets, [&](Bullet* bullet) { return bullet->Update(dt, *this); });
	LoopAndRemove(lvlPart->electros, [dt](Electro* electro) { return electro->Update(dt); });
	LoopAndRemove(lvlPart->explos, [&](Explo* explo) { return explo->Update(dt, *this); });
	LoopAndRemove(lvlPart->pes, [dt](ParticleEmitter* pe) { return pe->Update(dt); });
	LoopAndRemove(lvlPart->tpes, [dt](TrailParticleEmitter* tpe) { return tpe->Update(dt); });
	LoopAndRemove(lvlPart->drains, [dt](Drain& drain) { return drain.Update(dt); });

	// update blood spatters
	for(Blood& blood : bloods)
	{
		blood.size += dt;
		if(blood.size >= 1.f)
			blood.size = 1.f;
	}

	// update objects
	for(Object* obj : objects)
	{
		if(obj->meshInst)
			obj->meshInst->Update(dt);
	}
}

//=================================================================================================
void LocationPart::Save(GameWriter& f)
{
	f << units.size();
	for(Unit* unit : units)
		unit->Save(f);

	f << objects.size();
	for(Object* object : objects)
		object->Save(f);

	f << usables.size();
	for(Usable* usable : usables)
		usable->Save(f);

	f << doors.size();
	for(Door* door : doors)
		door->Save(f);

	f << chests.size();
	for(Chest* chest : chests)
		chest->Save(f);

	f << groundItems.size();
	for(GroundItem* groundItem : groundItems)
		groundItem->Save(f);

	f << traps.size();
	for(Trap* trap : traps)
		trap->Save(f);

	f << bloods.size();
	for(Blood& blood : bloods)
		blood.Save(f);

	f << lights.size();
	for(GameLight& light : lights)
		light.Save(f);

	if(lvlPart)
		lvlPart->Save(f);
}

//=================================================================================================
void LocationPart::Load(GameReader& f, old::LoadCompatibility compatibility)
{
	if(f.isLocal && !lvlPart)
		lvlPart = new LevelPart(this);

	switch(compatibility)
	{
	case old::LoadCompatibility::None:
		{
			units.resize(f.Read<uint>());
			for(Unit*& unit : units)
			{
				unit = new Unit;
				unit->Load(f);
				unit->locPart = this;
			}

			objects.resize(f.Read<uint>());
			for(Object*& object : objects)
			{
				object = new Object;
				object->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			doors.resize(f.Read<uint>());
			for(Door*& door : doors)
			{
				door = new Door;
				door->Load(f);
			}

			chests.resize(f.Read<uint>());
			for(Chest*& chest : chests)
			{
				chest = new Chest;
				chest->Load(f);
			}

			groundItems.resize(f.Read<uint>());
			for(GroundItem*& groundItem : groundItems)
			{
				groundItem = new GroundItem;
				groundItem->Load(f);
			}

			traps.resize(f.Read<uint>());
			for(Trap*& trap : traps)
			{
				trap = new Trap;
				trap->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood& blood : bloods)
				blood.Load(f);

			lights.resize(f.Read<uint>());
			for(GameLight& light : lights)
				light.Load(f);
		}
		break;
	case old::LoadCompatibility::InsideBuilding:
		{
			units.resize(f.Read<uint>());
			for(Unit*& unit : units)
			{
				unit = new Unit;
				unit->Load(f);
				unit->locPart = this;
			}

			doors.resize(f.Read<uint>());
			for(Door*& door : doors)
			{
				door = new Door;
				door->Load(f);
			}

			objects.resize(f.Read<uint>());
			for(Object*& object : objects)
			{
				object = new Object;
				object->Load(f);
			}

			groundItems.resize(f.Read<uint>());
			for(GroundItem*& groundItem : groundItems)
			{
				groundItem = new GroundItem;
				groundItem->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood& blood : bloods)
				blood.Load(f);

			lights.resize(f.Read<uint>());
			for(GameLight& light : lights)
				light.Load(f);
		}
		break;
	case old::LoadCompatibility::InsideLocationLevel:
		{
			units.resize(f.Read<uint>());
			for(Unit*& unit : units)
			{
				unit = new Unit;
				unit->Load(f);
				unit->locPart = this;
			}

			chests.resize(f.Read<uint>());
			for(Chest*& chest : chests)
			{
				chest = new Chest;
				chest->Load(f);
			}

			objects.resize(f.Read<uint>());
			for(Object*& object : objects)
			{
				object = new Object;
				object->Load(f);
			}

			doors.resize(f.Read<uint>());
			for(Door*& door : doors)
			{
				door = new Door;
				door->Load(f);
			}

			groundItems.resize(f.Read<int>());
			for(GroundItem*& groundItem : groundItems)
			{
				groundItem = new GroundItem;
				groundItem->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood& blood : bloods)
				blood.Load(f);

			lights.resize(f.Read<uint>());
			for(GameLight& light : lights)
				light.Load(f);
		}
		break;
	case old::LoadCompatibility::InsideLocationLevelTraps:
		{
			traps.resize(f.Read<uint>());
			for(Trap*& trap : traps)
			{
				trap = new Trap;
				trap->Load(f);
			}
		}
		break;
	case old::LoadCompatibility::OutsideLocation:
		{
			units.resize(f.Read<uint>());
			for(Unit*& unit : units)
			{
				unit = new Unit;
				unit->Load(f);
				unit->locPart = this;
			}

			objects.resize(f.Read<uint>());
			for(Object*& object : objects)
			{
				object = new Object;
				object->Load(f);
			}

			chests.resize(f.Read<uint>());
			for(Chest*& chest : chests)
			{
				chest = new Chest;
				chest->Load(f);
			}

			groundItems.resize(f.Read<uint>());
			for(GroundItem*& groundItem : groundItems)
			{
				groundItem = new GroundItem;
				groundItem->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood& blood : bloods)
				blood.Load(f);
		}
		break;
	}

	if(lvlPart && Any(compatibility, old::LoadCompatibility::None, old::LoadCompatibility::InsideBuilding))
		lvlPart->Load(f);
}

//=================================================================================================
void LocationPart::Write(BitStreamWriter& f)
{
	// units
	f.Write(units.size());
	for(Unit* unit : units)
		unit->Write(f);
	// objects
	f.Write(objects.size());
	for(Object* object : objects)
		object->Write(f);
	// usable objects
	f.Write(usables.size());
	for(Usable* usable : usables)
		usable->Write(f);
	// doors
	f.Write(doors.size());
	for(Door* door : doors)
		door->Write(f);
	// chests
	f.Write(chests.size());
	for(Chest* chest : chests)
		chest->Write(f);
	// ground items
	f.Write(groundItems.size());
	for(GroundItem* groundItem : groundItems)
		groundItem->Write(f);
	// traps
	f.Write(traps.size());
	for(Trap* trap : traps)
		trap->Write(f);
	// bloods
	f.Write(bloods.size());
	for(Blood& blood : bloods)
		blood.Write(f);
	// lights
	f.Write(lights.size());
	for(GameLight& light : lights)
		light.Write(f);
}

//=================================================================================================
bool LocationPart::Read(BitStreamReader& f)
{
	if(!lvlPart)
		lvlPart = new LevelPart(this);

	// units
	uint count;
	f >> count;
	if(!f.Ensure(Unit::MIN_SIZE * count))
	{
		Error("Read location part: Invalid unit count.");
		return false;
	}
	units.resize(count);
	for(Unit*& unit : units)
	{
		unit = new Unit;
		if(!unit->Read(f))
		{
			Error("Read location part: Broken unit.");
			return false;
		}
		unit->locPart = this;
	}

	// objects
	f >> count;
	if(!f.Ensure(count * Object::MIN_SIZE))
	{
		Error("Read location part: Invalid object count.");
		return false;
	}
	objects.resize(count);
	for(Object*& object : objects)
	{
		object = new Object;
		if(!object->Read(f))
		{
			Error("Read location part: Broken object.");
			return false;
		}
	}

	// usable objects
	f >> count;
	if(!f.Ensure(Usable::MIN_SIZE * count))
	{
		Error("Read location part: Invalid usable object count.");
		return false;
	}
	usables.resize(count);
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		if(!usable->Read(f))
		{
			Error("Read location part: Broken usable objects.");
			return false;
		}
	}

	// doors
	f >> count;
	if(!f.Ensure(count * Door::MIN_SIZE))
	{
		Error("Read location part: Invalid door count.");
		return false;
	}
	doors.resize(count);
	for(Door*& door : doors)
	{
		door = new Door;
		if(!door->Read(f))
		{
			Error("Read location part: Broken door.");
			return false;
		}
	}

	// chests
	f >> count;
	if(!f.Ensure(count * Chest::MIN_SIZE))
	{
		Error("Read location part: Invalid chest count.");
		return false;
	}
	chests.resize(count);
	for(Chest*& chest : chests)
	{
		chest = new Chest;
		if(!chest->Read(f))
		{
			Error("Read location part: Broken chest.");
			return false;
		}
	}

	// ground items
	f >> count;
	if(!f.Ensure(count * GroundItem::MIN_SIZE))
	{
		Error("Read location part: Invalid ground item count.");
		return false;
	}
	groundItems.resize(count);
	for(GroundItem*& groundItem : groundItems)
	{
		groundItem = new GroundItem;
		if(!groundItem->Read(f))
		{
			Error("Read location part: Broken ground item.");
			return false;
		}
	}

	// traps
	f >> count;
	if(!f.Ensure(count * Trap::MIN_SIZE))
	{
		Error("Read location part: Invalid trap count.");
		return false;
	}
	traps.resize(count);
	for(Trap*& trap : traps)
	{
		trap = new Trap;
		if(!trap->Read(f))
		{
			Error("Read location part: Broken trap.");
			return false;
		}
	}

	// bloods
	f >> count;
	if(!f.Ensure(count * Blood::MIN_SIZE))
	{
		Error("Read location part: Invalid blood count.");
		return false;
	}
	bloods.resize(count);
	for(Blood& blood : bloods)
		blood.Read(f);
	if(!f)
	{
		Error("Read location part: Broken blood.");
		return false;
	}

	// lights
	f >> count;
	if(!f.Ensure(count * GameLight::MIN_SIZE))
	{
		Error("Read location part: Invalid light count.");
		return false;
	}
	lights.resize(count);
	for(GameLight& light : lights)
		light.Read(f);
	if(!f)
	{
		Error("Read location part: Broken light.");
		return false;
	}

	return true;
}

//=================================================================================================
void LocationPart::Clear()
{
	bloods.clear();
	DeleteElements(objects);
	DeleteElements(chests);
	DeleteElements(groundItems);
	for(Unit* unit : units)
	{
		if(unit->IsAlive() && unit->IsHero() && unit->hero->otherTeam)
			unit->hero->otherTeam->Remove();
		delete unit;
	}
	units.clear();
}

//=================================================================================================
Unit* LocationPart::FindUnit(UnitData* ud)
{
	assert(ud);

	for(Unit* unit : units)
	{
		if(unit->data == ud)
			return unit;
	}

	return nullptr;
}

//=================================================================================================
Usable* LocationPart::FindUsable(BaseUsable* base)
{
	assert(base);

	for(Usable* use : usables)
	{
		if(use->base == base)
			return use;
	}

	return nullptr;
}

//=================================================================================================
bool LocationPart::RemoveItem(const Item* item)
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
	if(RemoveItemFromChest(item))
		return true;

	return false;
}

//=================================================================================================
bool LocationPart::FindItemInCorpse(const Item* item, Unit** unit, int* slot)
{
	assert(item);

	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
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
void LocationPart::AddGroundItem(GroundItem* groundItem, bool adjustY)
{
	assert(groundItem);

	if(gameLevel->ready && lvlPart)
	{
		gameRes->PreloadItem(groundItem->item);
		groundItem->CreateSceneNode();
		lvlPart->scene->Add(groundItem->node);

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPAWN_ITEM;
			c.item = groundItem;
		}
	}

	if(adjustY && partType == LocationPart::Type::Outside)
		gameLevel->terrain->SetY(groundItem->pos);
	groundItems.push_back(groundItem);
}

//=================================================================================================
bool LocationPart::RemoveGroundItem(const Item* item)
{
	assert(item);

	for(GroundItem* groundItem : groundItems)
	{
		if(groundItem->item == item)
		{
			RemoveGroundItem(groundItem);
			return true;
		}
	}

	return false;
}

//=================================================================================================
void LocationPart::RemoveGroundItem(int questId)
{
	assert(questId != -1);

	for(GroundItem* groundItem : groundItems)
	{
		if(groundItem->item->questId == questId)
		{
			RemoveGroundItem(groundItem);
			break;
		}
	}
}

//=================================================================================================
void LocationPart::RemoveGroundItem(GroundItem* groundItem)
{
	assert(groundItem);

	if(gameLevel->ready && lvlPart)
	{
		lvlPart->scene->Remove(groundItem->node);
		groundItem->node->Free();

		if(PlayerController::data.beforePlayer == BP_ITEM && PlayerController::data.beforePlayerPtr.item == groundItem)
			PlayerController::data.beforePlayer = BP_NONE;

		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::REMOVE_ITEM;
			c.id = groundItem->id;
		}
	}

	RemoveElement(groundItems, groundItem);
	delete groundItem;
}

//=================================================================================================
Object* LocationPart::FindObject(BaseObject* baseObj)
{
	assert(baseObj);

	for(Object* obj : objects)
	{
		if(obj->base == baseObj)
			return obj;
	}

	return nullptr;
}

//=================================================================================================
Object* LocationPart::FindNearestObject(BaseObject* baseObj, const Vec3& pos)
{
	Object* found_obj = nullptr;
	float best_dist = 9999.f;
	for(vector<Object*>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == baseObj)
		{
			float dist = Vec3::Distance(pos, obj.pos);
			if(dist < best_dist)
			{
				best_dist = dist;
				found_obj = &obj;
			}
		}
	}
	return found_obj;
}

//=================================================================================================
Chest* LocationPart::FindChestInRoom(const Room& p)
{
	for(Chest* chest : chests)
	{
		if(p.IsInside(chest->pos))
			return chest;
	}

	return nullptr;
}

//=================================================================================================
Chest* LocationPart::GetRandomFarChest(const Int2& pt)
{
	vector<pair<Chest*, float>> far_chests;
	float close_dist = -1.f;
	Vec3 pos = PtToPos(pt);

	// znajdü 5 najdalszych skrzyni
	for(vector<Chest*>::iterator it = chests.begin(), end = chests.end(); it != end; ++it)
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
	else if(chests.size() < 10u)
	{
		// jeøeli skrzyni by≥o mniej niø 10 to czÍúÊ moøe byÊ za blisko
		index = Rand() % (chests.size() - 5);
	}
	else
		index = Rand() % 5;

	return far_chests[index].first;
}

//=================================================================================================
bool LocationPart::HaveUnit(Unit* unit)
{
	assert(unit);

	for(Unit* u : units)
	{
		if(u == unit)
			return true;
	}

	return false;
}

//=================================================================================================
Chest* LocationPart::FindChestWithItem(const Item* item, int* index)
{
	assert(item);

	for(Chest* chest : chests)
	{
		int idx = chest->FindItem(item);
		if(idx != -1)
		{
			if(index)
				*index = idx;
			return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* LocationPart::FindChestWithQuestItem(int questId, int* index)
{
	for(Chest* chest : chests)
	{
		int idx = chest->FindQuestItem(questId);
		if(idx != -1)
		{
			if(index)
				*index = idx;
			return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
bool LocationPart::RemoveItemFromChest(const Item* item)
{
	int index;
	Chest* chest = FindChestWithItem(item, &index);
	if(chest)
	{
		chest->items.erase(chest->items.begin() + index);
		return true;
	}
	return false;
}

//=================================================================================================
// Check only alive units, ignore team
bool LocationPart::RemoveItemFromUnit(const Item* item)
{
	for(Unit* unit : units)
	{
		if(!unit->IsAlive() || unit->IsTeamMember())
			continue;

		if(unit->HaveItem(item))
		{
			unit->RemoveItem(item, 1u);
			return true;
		}
	}

	return false;
}

//=================================================================================================
Door* LocationPart::FindDoor(const Int2& pt)
{
	for(Door* door : doors)
	{
		if(door->pt == pt)
			return door;
	}

	return nullptr;
}

//=================================================================================================
void LocationPart::SpellHitEffect(Bullet& bullet, const Vec3& pos, Unit* hitted)
{
	Ability& ability = *bullet.ability;

	// sound
	if(ability.soundHit)
	{
		soundMgr->PlaySound3d(ability.soundHit, pos, ability.soundHitDist);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPELL_SOUND;
			c.extraId = 1;
			c.ability = &ability;
			c.pos = pos;
		}
	}

	if(IsSet(ability.flags, Ability::Explode))
	{
		// explosion
		if(Net::IsLocal())
		{
			Explo* explo = CreateExplo(&ability, pos);
			explo->dmg = (float)ability.dmg;
			if(bullet.owner)
				explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * ability.dmgBonus);
			explo->owner = bullet.owner;
			if(hitted)
				explo->hitted.push_back(hitted);
		}
	}
	else
	{
		// particles
		if(ability.texParticle && ability.type == Ability::Ball)
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = ability.texParticle;
			pe->emissionInterval = 0.01f;
			pe->life = 0.f;
			pe->particleLife = 0.5f;
			pe->emissions = 1;
			pe->spawnMin = 8;
			pe->spawnMax = 12;
			pe->maxParticles = 12;
			pe->pos = pos;
			pe->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
			pe->speedMax = Vec3(1.5f, 1.5f, 1.5f);
			pe->posMin = Vec3(-ability.size, -ability.size, -ability.size);
			pe->posMax = Vec3(ability.size, ability.size, ability.size);
			pe->size = ability.size / 2;
			pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->alpha = 1.f;
			pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->mode = 1;
			pe->Init();
			lvlPart->pes.push_back(pe);
		}
	}
}

//=================================================================================================
bool LocationPart::CheckForHit(Unit& unit, Unit*& hitted, Vec3& hitpoint)
{
	// attack with weapon or unarmed

	Mesh::Point* hitbox, *point;

	if(unit.meshInst->mesh->head.nGroups > 1)
	{
		Mesh* mesh = unit.GetWeapon().mesh;
		if(!mesh)
			return false;
		hitbox = mesh->FindPoint("hit");
		point = unit.meshInst->mesh->GetPoint(NAMES::pointWeapon);
		assert(point);
	}
	else
	{
		point = nullptr;
		hitbox = unit.meshInst->mesh->GetPoint(Format("hitbox%d", unit.act.attack.index + 1));
		if(!hitbox)
			hitbox = unit.meshInst->mesh->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(unit, hitted, *hitbox, point, hitpoint);
}

//=================================================================================================
// Check collision against hitbox and unit
// If returned true but hitted is nullptr then object was hit
// There are two allowed inputs:
// - bone is "bron"(weapon) attachment point, hitbox is from weapon
// - bone is nullptr, hitbox is from unit attack
bool LocationPart::CheckForHit(Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint)
{
	assert(hitted && hitbox.IsBox());

	unit.meshInst->SetupBones();

	// calculate hitbox matrix
	Matrix m1 = Matrix::Scale(unit.data->scale) * Matrix::RotationY(unit.rot) * Matrix::Translation(unit.pos); // m1 (World) = Rot * Pos
	if(bone)
	{
		// m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitScale * UnitRot * UnitPos
		m1 = hitbox.mat * (bone->mat * unit.meshInst->matBones[bone->bone] * m1);
	}
	else
		m1 = hitbox.mat * unit.meshInst->matBones[hitbox.bone] * m1;

	// a - weapon hitbox, b - unit hitbox
	Oob a, b;
	m1._11 = m1._22 = m1._33 = 1.f;
	a.c = Vec3::TransformZero(m1);
	a.e = hitbox.size;
	a.u[0] = Vec3(m1._11, m1._12, m1._13);
	a.u[1] = Vec3(m1._21, m1._22, m1._23);
	a.u[2] = Vec3(m1._31, m1._32, m1._33);
	b.u[0] = Vec3(1, 0, 0);
	b.u[1] = Vec3(0, 1, 0);
	b.u[2] = Vec3(0, 0, 1);

	// search for collision
	for(vector<Unit*>::iterator it = units.begin(), end = units.end(); it != end; ++it)
	{
		if(*it == &unit || !(*it)->IsAlive() || Vec3::Distance((*it)->pos, unit.pos) > 5.f || unit.IsFriend(**it, true))
			continue;

		Box box2;
		(*it)->GetBox(box2);
		b.c = box2.Midpoint();
		b.e = box2.Size() / 2;

		if(OOBToOOB(b, a))
		{
			hitpoint = a.c;
			hitted = *it;
			return true;
		}
	}

	// collisions with melee_target
	b.e = Vec3(0.6f, 2.f, 0.6f);
	for(Object* obj : objects)
	{
		if(obj->base && obj->base->id == "melee_target")
		{
			b.c = obj->pos;
			b.c.y += 1.f;

			if(OOBToOOB(b, a))
			{
				hitpoint = a.c;
				hitted = nullptr;

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = gameRes->tSpark;
				pe->emissionInterval = 0.01f;
				pe->life = 5.f;
				pe->particleLife = 0.5f;
				pe->emissions = 1;
				pe->spawnMin = 10;
				pe->spawnMax = 15;
				pe->maxParticles = 15;
				pe->pos = hitpoint;
				pe->speedMin = Vec3(-1, 0, -1);
				pe->speedMax = Vec3(1, 1, 1);
				pe->posMin = Vec3(-0.1f, -0.1f, -0.1f);
				pe->posMax = Vec3(0.1f, 0.1f, 0.1f);
				pe->size = 0.3f;
				pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				lvlPart->pes.push_back(pe);

				soundMgr->PlaySound3d(gameRes->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, HIT_SOUND_DIST);

				if(Net::IsLocal() && unit.IsPlayer())
				{
					if(questMgr->questTutorial->inTutorial)
						questMgr->questTutorial->HandleMeleeAttackCollision();
					unit.player->Train(TrainWhat::AttackNoDamage, 0.f, 1);
				}

				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
Explo* LocationPart::CreateExplo(Ability* ability, const Vec3& pos)
{
	assert(ability);

	Explo* explo = new Explo;
	explo->ability = ability;
	explo->pos = pos;
	explo->size = 0;
	explo->sizemax = ability->explodeRange;
	lvlPart->explos.push_back(explo);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CREATE_EXPLOSION;
		c.ability = ability;
		c.pos = explo->pos;
	}

	return explo;
}
