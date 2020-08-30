#include "Pch.h"
#include "LevelArea.h"

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
#include "Net.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "Room.h"
#include "Trap.h"
#include "Unit.h"

#include <ParticleSystem.h>
#include <Profiler.h>
#include <Scene.h>
#include <SceneNode.h>
#include <SoundManager.h>

//=================================================================================================
LevelArea::~LevelArea()
{
	DeleteElements(units);
	DeleteElements(objects);
	DeleteElements(usables);
	DeleteElements(doors);
	DeleteElements(chests);
	DeleteElements(items);
	DeleteElements(traps);
}

//=================================================================================================
void LevelArea::Update(float dt)
{
	ProfilerBlock profiler_block([&] { return Format("UpdateArea %s", GetName()); });

	// update units
	// new units can be added inside this loop - so no iterators!
	for(uint i = 0, count = units.size(); i < count; ++i)
		units[i]->Update(dt);

	// update flickering lights
	tmp->lights_dt += dt;
	if(tmp->lights_dt >= 1.f / 20)
	{
		for(GameLight& light : lights)
		{
			light.pos = light.start_pos + Vec3::Random(Vec3(-0.05f, -0.05f, -0.05f), Vec3(0.05f, 0.05f, 0.05f));
			light.color = Vec4((light.start_color + Vec3::Random(Vec3(-0.1f, -0.1f, -0.1f), Vec3(0.1f, 0.1f, 0.1f))).Clamped(), 1);
		}
		tmp->lights_dt = 0;
	}

	// update chests
	for(Chest* chest : chests)
		chest->meshInst->Update(dt);

	// update doors
	for(Door* door : doors)
		door->Update(dt, *this);

	LoopAndRemove(traps, [&](Trap* trap) { return trap->Update(dt, *this); });
	LoopAndRemove(tmp->bullets, [&](Bullet* bullet) { return bullet->Update(dt, *this); });
	LoopAndRemove(tmp->electros, [dt](Electro* electro) { return electro->Update(dt); });
	LoopAndRemove(tmp->explos, [&](Explo* explo) { return explo->Update(dt, *this); });
	LoopAndRemove(tmp->pes, [dt](ParticleEmitter* pe) { return pe->Update(dt); });
	LoopAndRemove(tmp->tpes, [dt](TrailParticleEmitter* tpe) { return tpe->Update(dt); });
	LoopAndRemove(tmp->drains, [dt](Drain& drain) { return drain.Update(dt); });

	// update blood spatters
	for(Blood* blood : bloods)
	{
		blood->size += dt;
		if(blood->size >= 1.f)
			blood->size = 1.f;
	}

	// update objects
	for(Object* obj : objects)
	{
		if(obj->meshInst)
			obj->meshInst->Update(dt);
	}
}

//=================================================================================================
void LevelArea::Save(GameWriter& f)
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

	f << items.size();
	for(GroundItem* item : items)
		item->Save(f);

	f << traps.size();
	for(Trap* trap : traps)
		trap->Save(f);

	f << bloods.size();
	for(Blood* blood : bloods)
		blood->Save(f);

	f << lights.size();
	for(GameLight& light : lights)
		light.Save(f);

	if(tmp)
		tmp->Save(f);
}

//=================================================================================================
void LevelArea::Load(GameReader& f, old::LoadCompatibility compatibility)
{
	if(f.isLocal && !tmp)
	{
		tmp = TmpLevelArea::Get();
		tmp->area = this;
		tmp->lights_dt = 1.f;
	}

	switch(compatibility)
	{
	case old::LoadCompatibility::None:
		{
			units.resize(f.Read<uint>());
			for(Unit*& unit : units)
			{
				unit = new Unit;
				unit->Load(f);
				unit->area = this;
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

			items.resize(f.Read<uint>());
			for(GroundItem*& item : items)
			{
				item = new GroundItem;
				item->Load(f);
			}

			traps.resize(f.Read<uint>());
			for(Trap*& trap : traps)
			{
				trap = new Trap;
				trap->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood*& blood : bloods)
			{
				blood = new Blood;
				blood->Load(f);
			}

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
				unit->area = this;
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

			items.resize(f.Read<uint>());
			for(GroundItem*& item : items)
			{
				item = new GroundItem;
				item->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood*& blood : bloods)
			{
				blood = new Blood;
				blood->Load(f);
			}

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
				unit->area = this;
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

			items.resize(f.Read<int>());
			for(GroundItem*& item : items)
			{
				item = new GroundItem;
				item->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood*& blood : bloods)
			{
				blood = new Blood;
				blood->Load(f);
			}

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
				unit->area = this;
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

			items.resize(f.Read<uint>());
			for(GroundItem*& item : items)
			{
				item = new GroundItem;
				item->Load(f);
			}

			usables.resize(f.Read<uint>());
			for(Usable*& usable : usables)
			{
				usable = new Usable;
				usable->Load(f);
			}

			bloods.resize(f.Read<uint>());
			for(Blood*& blood : bloods)
			{
				blood = new Blood;
				blood->Load(f);
			}
		}
		break;
	}

	if(tmp && Any(compatibility, old::LoadCompatibility::None, old::LoadCompatibility::InsideBuilding))
		tmp->Load(f);
}

//=================================================================================================
void LevelArea::Write(BitStreamWriter& f)
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
	f.Write(items.size());
	for(GroundItem* item : items)
		item->Write(f);
	// traps
	f.Write(traps.size());
	for(Trap* trap : traps)
		trap->Write(f);
	// bloods
	f.Write(bloods.size());
	for(Blood* blood : bloods)
		blood->Write(f);
	// lights
	f.Write(lights.size());
	for(GameLight& light : lights)
		light.Write(f);
}

//=================================================================================================
bool LevelArea::Read(BitStreamReader& f)
{
	if(!tmp)
	{
		tmp = TmpLevelArea::Get();
		tmp->area = this;
		tmp->lights_dt = 1.f;
	}

	// units
	uint count;
	f >> count;
	if(!f.Ensure(Unit::MIN_SIZE * count))
	{
		Error("Read area: Invalid unit count.");
		return false;
	}
	units.resize(count);
	for(Unit*& unit : units)
	{
		unit = new Unit;
		if(!unit->Read(f))
		{
			Error("Read area: Broken unit.");
			return false;
		}
		unit->area = this;
	}

	// objects
	f >> count;
	if(!f.Ensure(count * Object::MIN_SIZE))
	{
		Error("Read area: Invalid object count.");
		return false;
	}
	objects.resize(count);
	for(Object*& object : objects)
	{
		object = new Object;
		if(!object->Read(f))
		{
			Error("Read area: Broken object.");
			return false;
		}
	}

	// usable objects
	f >> count;
	if(!f.Ensure(Usable::MIN_SIZE * count))
	{
		Error("Read area: Invalid usable object count.");
		return false;
	}
	usables.resize(count);
	for(Usable*& usable : usables)
	{
		usable = new Usable;
		if(!usable->Read(f))
		{
			Error("Read area: Broken usable objects.");
			return false;
		}
	}

	// doors
	f >> count;
	if(!f.Ensure(count * Door::MIN_SIZE))
	{
		Error("Read area: Invalid door count.");
		return false;
	}
	doors.resize(count);
	for(Door*& door : doors)
	{
		door = new Door;
		if(!door->Read(f))
		{
			Error("Read area: Broken door.");
			return false;
		}
	}

	// chests
	f >> count;
	if(!f.Ensure(count * Chest::MIN_SIZE))
	{
		Error("Read area: Invalid chest count.");
		return false;
	}
	chests.resize(count);
	for(Chest*& chest : chests)
	{
		chest = new Chest;
		if(!chest->Read(f))
		{
			Error("Read area: Broken chest.");
			return false;
		}
	}

	// ground items
	f >> count;
	if(!f.Ensure(count * GroundItem::MIN_SIZE))
	{
		Error("Read area: Invalid ground item count.");
		return false;
	}
	items.resize(count);
	for(GroundItem*& item : items)
	{
		item = new GroundItem;
		if(!item->Read(f))
		{
			Error("Read area: Broken ground item.");
			return false;
		}
	}

	// traps
	f >> count;
	if(!f.Ensure(count * Trap::MIN_SIZE))
	{
		Error("Read area: Invalid trap count.");
		return false;
	}
	traps.resize(count);
	for(Trap*& trap : traps)
	{
		trap = new Trap;
		if(!trap->Read(f))
		{
			Error("Read area: Broken trap.");
			return false;
		}
	}

	// bloods
	f >> count;
	if(!f.Ensure(count * Blood::MIN_SIZE))
	{
		Error("Read area: Invalid blood count.");
		return false;
	}
	bloods.resize(count);
	for(Blood*& blood : bloods)
	{
		blood = new Blood;
		blood->Read(f);
	}
	if(!f)
	{
		Error("Read area: Broken blood.");
		return false;
	}

	// lights
	f >> count;
	if(!f.Ensure(count * GameLight::MIN_SIZE))
	{
		Error("Read area: Invalid light count.");
		return false;
	}
	lights.resize(count);
	for(GameLight& light : lights)
		light.Read(f);
	if(!f)
	{
		Error("Read area: Broken light.");
		return false;
	}

	return true;
}

//=================================================================================================
void LevelArea::Clear()
{
	DeleteElements(bloods);
	DeleteElements(objects);
	DeleteElements(chests);
	DeleteElements(items);
	for(Unit* unit : units)
	{
		if(unit->IsAlive() && unit->IsHero() && unit->hero->otherTeam)
			unit->hero->otherTeam->Remove();
		delete unit;
	}
	units.clear();
}

//=================================================================================================
cstring LevelArea::GetName()
{
	switch(area_type)
	{
	case Type::Inside:
		return "Inside";
	case Type::Outside:
		return "Outside";
	default:
		return Format("Building%d", area_id);
	}
}

//=================================================================================================
Unit* LevelArea::FindUnit(UnitData* ud)
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
Usable* LevelArea::FindUsable(BaseUsable* base)
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
bool LevelArea::RemoveItem(const Item* item)
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
bool LevelArea::FindItemInCorpse(const Item* item, Unit** unit, int* slot)
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
bool LevelArea::RemoveGroundItem(const Item* item)
{
	assert(item);

	for(vector<GroundItem*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
	{
		if((*it)->item == item)
		{
			delete *it;
			if(it + 1 != end)
				std::iter_swap(it, end - 1);
			items.pop_back();
			return true;
		}
	}

	return false;
}

//=================================================================================================
Object* LevelArea::FindObject(BaseObject* base_obj)
{
	assert(base_obj);

	for(Object* obj : objects)
	{
		if(obj->base == base_obj)
			return obj;
	}

	return nullptr;
}

//=================================================================================================
Object* LevelArea::FindNearestObject(BaseObject* base_obj, const Vec3& pos)
{
	Object* found_obj = nullptr;
	float best_dist = 9999.f;
	for(vector<Object*>::iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == base_obj)
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
Chest* LevelArea::FindChestInRoom(const Room& p)
{
	for(Chest* chest : chests)
	{
		if(p.IsInside(chest->pos))
			return chest;
	}

	return nullptr;
}

//=================================================================================================
Chest* LevelArea::GetRandomFarChest(const Int2& pt)
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
bool LevelArea::HaveUnit(Unit* unit)
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
Chest* LevelArea::FindChestWithItem(const Item* item, int* index)
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
Chest* LevelArea::FindChestWithQuestItem(int quest_id, int* index)
{
	for(Chest* chest : chests)
	{
		int idx = chest->FindQuestItem(quest_id);
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
bool LevelArea::RemoveItemFromChest(const Item* item)
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
Door* LevelArea::FindDoor(const Int2& pt)
{
	for(Door* door : doors)
	{
		if(door->pt == pt)
			return door;
	}

	return nullptr;
}

//=================================================================================================
void LevelArea::SpellHitEffect(Bullet& bullet, const Vec3& pos, Unit* hitted)
{
	Ability& ability = *bullet.ability;

	if(IsSet(ability.flags, Ability::Explode))
	{
		// explosion
		if(Net::IsLocal())
		{
			Explo* explo = CreateExplo(&ability, pos);
			explo->dmg = (float)ability.dmg;
			if(bullet.owner)
				explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * ability.dmg_bonus);
			explo->owner = bullet.owner;
			if(hitted)
				explo->hitted.push_back(hitted);
		}
	}
	else
	{
		// sound
		if(ability.sound_hit)
			sound_mgr->PlaySound3d(ability.sound_hit, pos, ability.sound_hit_dist);

		// particles
		if(ability.tex_particle && ability.type == Ability::Ball)
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->tex = ability.tex_particle;
			pe->emision_interval = 0.01f;
			pe->life = 0.f;
			pe->particle_life = 0.5f;
			pe->emisions = 1;
			pe->spawn_min = 8;
			pe->spawn_max = 12;
			pe->max_particles = 12;
			pe->pos = pos;
			pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
			pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
			pe->pos_min = Vec3(-ability.size, -ability.size, -ability.size);
			pe->pos_max = Vec3(ability.size, ability.size, ability.size);
			pe->size = ability.size / 2;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->alpha = 1.f;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->mode = 1;
			pe->Init();
			tmp->pes.push_back(pe);
		}
	}
}

//=================================================================================================
bool LevelArea::CheckForHit(Unit& unit, Unit*& hitted, Vec3& hitpoint)
{
	// attack with weapon or unarmed

	Mesh::Point* hitbox, *point;

	if(unit.mesh_inst->mesh->head.n_groups > 1)
	{
		Mesh* mesh = unit.GetWeapon().mesh;
		if(!mesh)
			return false;
		hitbox = mesh->FindPoint("hit");
		point = unit.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);
	}
	else
	{
		point = nullptr;
		hitbox = unit.mesh_inst->mesh->GetPoint(Format("hitbox%d", unit.act.attack.index + 1));
		if(!hitbox)
			hitbox = unit.mesh_inst->mesh->FindPoint("hitbox");
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
bool LevelArea::CheckForHit(Unit& unit, Unit*& hitted, Mesh::Point& hitbox, Mesh::Point* bone, Vec3& hitpoint)
{
	assert(hitted && hitbox.IsBox());

	unit.mesh_inst->SetupBones();

	// calculate hitbox matrix
	Matrix m1 = Matrix::Scale(unit.data->scale) * Matrix::RotationY(unit.rot) * Matrix::Translation(unit.pos); // m1 (World) = Rot * Pos
	if(bone)
	{
		// m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitScale * UnitRot * UnitPos
		m1 = hitbox.mat * (bone->mat * unit.mesh_inst->mat_bones[bone->bone] * m1);
	}
	else
		m1 = hitbox.mat * unit.mesh_inst->mat_bones[hitbox.bone] * m1;

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
				pe->tex = game_res->tSpark;
				pe->emision_interval = 0.01f;
				pe->life = 5.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 10;
				pe->spawn_max = 15;
				pe->max_particles = 15;
				pe->pos = hitpoint;
				pe->speed_min = Vec3(-1, 0, -1);
				pe->speed_max = Vec3(1, 1, 1);
				pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
				pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
				pe->size = 0.3f;
				pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				tmp->pes.push_back(pe);

				sound_mgr->PlaySound3d(game_res->GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, HIT_SOUND_DIST);

				if(Net::IsLocal() && unit.IsPlayer())
				{
					if(quest_mgr->quest_tutorial->in_tutorial)
						quest_mgr->quest_tutorial->HandleMeleeAttackCollision();
					unit.player->Train(TrainWhat::AttackNoDamage, 0.f, 1);
				}

				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
Explo* LevelArea::CreateExplo(Ability* ability, const Vec3& pos)
{
	assert(ability);

	// create entity
	Explo* explo = new Explo;
	explo->ability = ability;
	explo->pos = pos;
	explo->size = 0.01f;
	explo->sizemax = ability->explode_range;
	tmp->explos.push_back(explo);

	// create scene node
	SceneNode* node = SceneNode::Get();
	node->tmp = false;
	node->SetMesh(game_res->aSpellball);
	node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
	node->center = explo->pos;
	node->radius *= explo->size;
	node->mat = Matrix::Scale(explo->size) * Matrix::Translation(explo->pos);
	node->tex_override = &ability->tex_explode;
	node->tint.w = 1.f - explo->size / explo->sizemax;
	explo->node = node;
	tmp->scene->Add(node);

	// play sound
	sound_mgr->PlaySound3d(ability->sound_hit, explo->pos, ability->sound_hit_dist);

	// notify
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CREATE_EXPLOSION;
		c.ability = ability;
		c.pos = explo->pos;
	}

	return explo;
}

//=================================================================================================
void LevelArea::RecreateScene()
{
	Scene* scene = tmp->scene;

	for(Usable* usable : usables)
	{
		SceneNode* node = SceneNode::Get();
		node->tmp = false;
		node->SetMesh(usable->base->mesh);
		node->center = usable->pos;
		node->mat = Matrix::RotationY(usable->rot) * Matrix::Translation(usable->pos);
		usable->node = node;
		scene->Add(node);
	}

	for(Explo* explo : tmp->explos)
	{
		SceneNode* node = SceneNode::Get();
		node->tmp = false;
		node->SetMesh(game_res->aSpellball);
		node->flags |= SceneNode::F_NO_LIGHTING | SceneNode::F_ALPHA_BLEND | SceneNode::F_NO_ZWRITE;
		node->center = explo->pos;
		node->radius *= explo->size;
		node->mat = Matrix::Scale(explo->size) * Matrix::Translation(explo->pos);
		node->tex_override = &explo->ability->tex_explode;
		node->tint.w = 1.f - explo->size / explo->sizemax;
		explo->node = node;
		scene->Add(node);
	}
}
