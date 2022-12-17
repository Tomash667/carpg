#include "Pch.h"
#include "Level.h"

#include "Ability.h"
#include "AIController.h"
#include "AITeam.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "Camp.h"
#include "Chest.h"
#include "City.h"
#include "Collision.h"
#include "Door.h"
#include "Electro.h"
#include "EntityInterpolator.h"
#include "FOV.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "InsideBuilding.h"
#include "InsideLocationLevel.h"
#include "Language.h"
#include "LevelGui.h"
#include "LevelPart.h"
#include "LocationGeneratorFactory.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "Pathfinding.h"
#include "PhysicCallbacks.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Scripted.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "ScriptManager.h"
#include "Stock.h"
#include "Team.h"
#include "Trap.h"
#include "UnitEventHandler.h"
#include "UnitGroup.h"
#include "World.h"

#include <angelscript.h>
#include <ParticleSystem.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <scriptarray/scriptarray.h>
#include <SoundManager.h>
#include <Texture.h>
#include <Terrain.h>

Level* gameLevel;

//=================================================================================================
Level::Level() : localPart(nullptr), terrain(nullptr), terrainShape(nullptr), dungeonShape(nullptr), dungeonShapeData(nullptr), shapeWall(nullptr),
shapeStairs(nullptr), shapeStairsPart(), shapeBlock(nullptr), shapeBarrier(nullptr), shapeDoor(nullptr), shapeArrow(nullptr), shapeSummon(nullptr),
shapeFloor(nullptr), dungeonMesh(nullptr), ready(false)
{
	camera.zfar = 80.f;
}

//=================================================================================================
Level::~Level()
{
	DeleteElements(bowInstances);

	delete terrain;
	delete terrainShape;
	delete dungeonMesh;
	delete dungeonShape;
	delete dungeonShapeData;
	delete shapeWall;
	delete shapeStairsPart[0];
	delete shapeStairsPart[1];
	delete shapeStairs;
	delete shapeBlock;
	delete shapeBarrier;
	delete shapeDoor;
	delete shapeArrow;
	delete shapeSummon;
	delete shapeFloor;
}

//=================================================================================================
void Level::LoadLanguage()
{
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txWorldMap = Str("worldMap");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
}

//=================================================================================================
void Level::Init()
{
	terrain = new Terrain;
	Terrain::Options terrainOptions;
	terrainOptions.nParts = 8;
	terrainOptions.texSize = 256;
	terrainOptions.tileSize = 2.f;
	terrainOptions.tilesPerPart = 16;
	terrain->Init(terrainOptions);
	terrain->Build();
	terrain->RemoveHeightMap(true);

	// collision shapes
	const float size = 256.f;
	const float border = 32.f;

	shapeWall = new btBoxShape(btVector3(1.f, 2.f, 1.f));
	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f, 2.f, 0.1f));
	shapeStairsPart[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f, 2.f, 0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f, 2.f, 1.f));
	shapeStairsPart[1] = b;
	tr.setOrigin(btVector3(-0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	shapeStairs = s;
	shapeDoor = new btBoxShape(btVector3(Door::WIDTH, Door::HEIGHT, Door::THICKNESS));
	shapeBlock = new btBoxShape(btVector3(1.f, 4.f, 1.f));
	shapeBarrier = new btBoxShape(btVector3(size / 2, 40.f, border / 2));
	shapeSummon = new btCylinderShape(btVector3(1.5f / 2, 0.75f, 1.5f / 2));
	shapeFloor = new btBoxShape(btVector3(20.f, 0.01f, 20.f));

	Mesh::Point* point = gameRes->aArrow->FindPoint("Empty");
	assert(point && point->IsBox());
	shapeArrow = new btBoxShape(ToVector3(point->size));

	dungeonMesh = new SimpleMesh;
}

//=================================================================================================
void Level::Reset()
{
	unitWarpData.clear();
	toRemove.clear();
	minimapReveal.clear();
	minimapRevealMp.clear();
}

//=================================================================================================
void Level::ProcessUnitWarps()
{
	bool warpedToArena = false;

	for(UnitWarpData& warp : unitWarpData)
	{
		if(warp.where == warp.unit->locPart->partId)
		{
			// unit left location
			if(warp.unit->eventHandler)
				warp.unit->eventHandler->HandleUnitEvent(UnitEventHandler::LEAVE, warp.unit);
			RemoveUnit(warp.unit);
		}
		else if(warp.where == WARP_OUTSIDE)
		{
			// exit from building
			InsideBuilding& building = *static_cast<InsideBuilding*>(warp.unit->locPart);
			RemoveElement(building.units, warp.unit);
			warp.unit->locPart = localPart;
			if(warp.building == -1)
			{
				warp.unit->rot = building.outsideRot;
				WarpUnit(*warp.unit, building.outsideSpawn);
			}
			else
			{
				CityBuilding& cityBuilding = cityCtx->buildings[warp.building];
				WarpUnit(*warp.unit, cityBuilding.walkPt);
				warp.unit->RotateTo(PtToPos(cityBuilding.pt));
			}
			localPart->units.push_back(warp.unit);
		}
		else if(warp.where == WARP_ARENA)
		{
			// warp to arena
			InsideBuilding& building = *GetArena();
			RemoveElement(warp.unit->locPart->units, warp.unit);
			warp.unit->locPart = &building;
			Vec3 pos;
			if(!WarpToRegion(building, (warp.unit->inArena == 0 ? building.region1 : building.region2), warp.unit->GetUnitRadius(), pos, 20))
			{
				// failed to warp to arena, spawn outside of arena
				warp.unit->locPart = localPart;
				warp.unit->rot = building.outsideRot;
				WarpUnit(*warp.unit, building.outsideSpawn);
				localPart->units.push_back(warp.unit);
				RemoveElement(game->arena->units, warp.unit);
			}
			else
			{
				warp.unit->rot = (warp.unit->inArena == 0 ? PI : 0);
				WarpUnit(*warp.unit, pos);
				building.units.push_back(warp.unit);
				warpedToArena = true;
			}
		}
		else
		{
			// enter building
			InsideBuilding& building = *cityCtx->insideBuildings[warp.where];
			RemoveElement(warp.unit->locPart->units, warp.unit);
			warp.unit->locPart = &building;
			warp.unit->rot = PI;
			WarpUnit(*warp.unit, building.insideSpawn);
			building.units.push_back(warp.unit);
		}

		if(warp.unit == game->pc->unit)
		{
			camera.Reset();
			game->pc->data.rotBuf = 0.f;

			if(game->fallbackType == FALLBACK::ARENA)
			{
				game->pc->unit->frozen = FROZEN::ROTATE;
				game->fallbackType = FALLBACK::ARENA2;
			}
			else if(game->fallbackType == FALLBACK::ARENA_EXIT)
			{
				game->pc->unit->frozen = FROZEN::NO;
				game->fallbackType = FALLBACK::NONE;
			}
		}
	}

	unitWarpData.clear();

	if(warpedToArena)
	{
		Vec3 pt1(0, 0, 0), pt2(0, 0, 0);
		int count1 = 0, count2 = 0;

		for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
		{
			if((*it)->inArena == 0)
			{
				pt1 += (*it)->pos;
				++count1;
			}
			else
			{
				pt2 += (*it)->pos;
				++count2;
			}
		}

		if(count1 > 0)
			pt1 /= (float)count1;
		else
		{
			InsideBuilding& building = *GetArena();
			pt1 = ((building.region1.Midpoint() + building.region2.Midpoint()) / 2).XZ();
		}
		if(count2 > 0)
			pt2 /= (float)count2;
		else
		{
			InsideBuilding& building = *GetArena();
			pt2 = ((building.region1.Midpoint() + building.region2.Midpoint()) / 2).XZ();
		}

		for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
			(*it)->rot = Vec3::LookAtAngle((*it)->pos, (*it)->inArena == 0 ? pt2 : pt1);
	}
}

//=================================================================================================
void Level::ProcessRemoveUnits(bool leave)
{
	if(leave)
	{
		for(Unit* unit : toRemove)
		{
			RemoveElement(unit->locPart->units, unit);
			delete unit;
		}
	}
	else
	{
		for(Unit* unit : toRemove)
			game->DeleteUnit(unit);
	}
	toRemove.clear();
}

//=================================================================================================
void Level::Apply()
{
	cityCtx = (location->type == L_CITY ? static_cast<City*>(location) : nullptr);
	locParts.clear();
	location->Apply(locParts);
	localPart = &locParts[0].get();
	for(LocationPart& locPart : locParts)
	{
		if(!locPart.lvlPart)
			locPart.lvlPart = new LevelPart(&locPart);
	}
}

//=================================================================================================
LocationPart& Level::GetLocationPart(const Vec3& pos)
{
	if(!cityCtx)
		return *localPart;
	else
	{
		Int2 offset(int((pos.x - 256.f) / 256.f), int((pos.z - 256.f) / 256.f));
		if(offset.x % 2 == 1)
			++offset.x;
		if(offset.y % 2 == 1)
			++offset.y;
		offset /= 2;
		for(InsideBuilding* inside : cityCtx->insideBuildings)
		{
			if(inside->levelShift == offset)
				return *inside;
		}
		return *localPart;
	}
}

//=================================================================================================
LocationPart* Level::GetLocationPartById(int partId)
{
	if(localPart->partId == partId)
		return localPart;
	if(cityCtx)
	{
		if(partId >= 0 && partId < (int)cityCtx->insideBuildings.size())
			return cityCtx->insideBuildings[partId];
	}
	else if(LocationHelper::IsMultiLevel(location))
	{
		MultiInsideLocation* loc = static_cast<MultiInsideLocation*>(location);
		if(partId >= 0 && partId < (int)loc->levels.size())
			return loc->levels[partId];
	}
	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(int id)
{
	if(id == -1)
		return nullptr;

	for(LocationPart& locPart : locParts)
	{
		for(Unit* unit : locPart.units)
		{
			if(unit->id == id)
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(delegate<bool(Unit*)> pred)
{
	for(LocationPart& locPart : locParts)
	{
		for(Unit* unit : locPart.units)
		{
			if(pred(unit))
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(UnitData* ud)
{
	return FindUnit([ud](Unit* unit)
	{
		return unit->data == ud;
	});
}

//=================================================================================================
Usable* Level::FindUsable(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(Usable* usable : locPart.usables)
		{
			if(usable->id == id)
				return usable;
		}
	}
	return nullptr;
}

//=================================================================================================
Door* Level::FindDoor(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(Door* door : locPart.doors)
		{
			if(door->id == id)
				return door;
		}
	}

	return nullptr;
}

//=================================================================================================
Trap* Level::FindTrap(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(Trap* trap : locPart.traps)
		{
			if(trap->id == id)
				return trap;
		}
	}

	return nullptr;
}

//=================================================================================================
Trap* Level::FindTrap(BaseTrap* base, const Vec3& pos)
{
	assert(base);
	LocationPart& locPart = GetLocationPart(pos);
	Trap* best = nullptr;
	float bestDist = std::numeric_limits<float>::max();
	for(Trap* trap : locPart.traps)
	{
		if(trap->base != base)
			continue;
		const float dist = Vec3::Distance(trap->pos, pos);
		if(dist < bestDist)
		{
			best = trap;
			bestDist = dist;
		}
	}
	return best;
}

//=================================================================================================
Chest* Level::FindChest(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(Chest* chest : locPart.chests)
		{
			if(chest->id == id)
				return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* Level::GetRandomChest(Room& room)
{
	LocalVector<Chest*> chests;
	for(Chest* chest : localPart->chests)
	{
		if(room.IsInside(chest->pos))
			chests.push_back(chest);
	}
	if(chests.empty())
		return nullptr;
	return chests[Rand() % chests.size()];
}

//=================================================================================================
Chest* Level::GetTreasureChest()
{
	assert(lvl);

	Room* room;
	if(location->target == LABYRINTH)
		room = lvl->rooms[0];
	else
		room = lvl->rooms[static_cast<InsideLocation*>(location)->specialRoom];

	Chest* bestChest = nullptr;
	const Vec3 center = room->Center();
	float bestDist = 999.f;
	for(Chest* chest : lvl->chests)
	{
		if(room->IsInside(chest->pos))
		{
			float dist = Vec3::Distance(chest->pos, center);
			if(dist < bestDist)
			{
				bestDist = dist;
				bestChest = chest;
			}
		}
	}

	return bestChest;
}

//=================================================================================================
Electro* Level::FindElectro(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(Electro* electro : locPart.lvlPart->electros)
		{
			if(electro->id == id)
				return electro;
		}
	}

	return nullptr;
}

//=================================================================================================
bool Level::RemoveTrap(int id)
{
	for(LocationPart& locPart : locParts)
	{
		for(vector<Trap*>::iterator it = locPart.traps.begin(), end = locPart.traps.end(); it != end; ++it)
		{
			if((*it)->id == id)
			{
				delete *it;
				locPart.traps.erase(it);
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
void Level::RemoveOldTrap(BaseTrap* baseTrap, Unit* owner, uint maxAllowed)
{
	assert(owner);

	uint count = 0;
	Trap* bestTrap = nullptr;
	LocationPart* bestPart = nullptr;
	for(LocationPart& locPart : locParts)
	{
		for(vector<Trap*>::iterator it = locPart.traps.begin(), end = locPart.traps.end(); it != end; ++it)
		{
			Trap* trap = *it;
			if(trap->base == baseTrap && trap->owner == owner && trap->state == 0)
			{
				if(!bestTrap || trap->id < bestTrap->id)
				{
					bestTrap = trap;
					bestPart = &locPart;
				}
				++count;
			}
		}
	}

	if(count >= maxAllowed)
	{
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::REMOVE_TRAP;
			c.id = bestTrap->id;
		}

		RemoveElement(bestPart->traps, bestTrap);
		delete bestTrap;
	}
}

//=================================================================================================
void Level::RemoveUnit(Unit* unit, bool notify)
{
	assert(unit);
	if(unit->action == A_DESPAWN || (Net::IsClient() && unit->summoner))
		SpawnUnitEffect(*unit);
	unit->RemoveAllEventHandlers();
	unit->toRemove = true;
	toRemove.push_back(unit);
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->id;
	}
}

//=================================================================================================
void Level::RemoveUnit(UnitData* ud, bool onLeave)
{
	assert(ud);

	Unit* unit = FindUnit([=](Unit* unit)
	{
		return unit->data == ud && unit->IsAlive();
	});

	if(unit)
		RemoveUnit(unit, !onLeave);
}

//=================================================================================================
ObjectEntity Level::SpawnObjectEntity(LocationPart& locPart, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* outPoint, int variant)
{
	if(IsSet(base->flags, OBJ_TABLE_SPAWNER))
	{
		// table & stools
		BaseObject* table = BaseObject::Get(Rand() % 2 == 0 ? "table" : "table2");
		BaseUsable* stool = BaseUsable::Get("stool");

		// table
		Object* o = new Object;
		o->mesh = table->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = 1;
		o->base = table;
		locPart.objects.push_back(o);
		SpawnObjectExtras(locPart, table, pos, rot, o);

		// stools
		int count = Random(2, 4);
		int d[4] = { 0,1,2,3 };
		for(int i = 0; i < 4; ++i)
			std::swap(d[Rand() % 4], d[Rand() % 4]);

		for(int i = 0; i < count; ++i)
		{
			float sdir, slen;
			switch(d[i])
			{
			case 0:
				sdir = 0.f;
				slen = table->size.y + 0.3f;
				break;
			case 1:
				sdir = PI / 2;
				slen = table->size.x + 0.3f;
				break;
			case 2:
				sdir = PI;
				slen = table->size.y + 0.3f;
				break;
			case 3:
				sdir = PI * 3 / 2;
				slen = table->size.x + 0.3f;
				break;
			default:
				assert(0);
				break;
			}

			sdir += rot;

			Usable* u = new Usable;
			u->Register();
			u->base = stool;
			u->pos = pos + Vec3(sin(sdir) * slen, 0, cos(sdir) * slen);
			u->rot = sdir;
			locPart.usables.push_back(u);

			SpawnObjectExtras(locPart, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IsSet(base->flags, OBJ_BUILDING))
	{
		// building
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		locPart.objects.push_back(o);

		const GameDirection dir = RotToDir(rot);
		ProcessBuildingObjects(locPart, nullptr, nullptr, o->mesh, nullptr, rot, dir, pos, nullptr, nullptr, false, outPoint);

		return o;
	}
	else if(IsSet(base->flags, OBJ_USABLE))
	{
		// usable object
		BaseUsable* baseUsable = (BaseUsable*)base;

		Usable* u = new Usable;
		u->Register();
		u->base = baseUsable;
		u->pos = pos;
		u->rot = rot;

		if(IsSet(baseUsable->useFlags, BaseUsable::CONTAINER))
		{
			u->container = new ItemContainer;
			const Item* item = Book::GetRandom();
			if(item)
				u->container->items.push_back({ item, 1, 1 });
			if(Rand() % 2 == 0)
			{
				uint level;
				if(cityCtx)
					level = 1;
				else
					level = GetChestDifficultyLevel();
				uint gold = Random(level * 5, level * 10);
				u->container->items.push_back({ Item::gold, gold , gold });
			}
		}

		if(variant == -1)
		{
			if(base->variants)
			{
				// extra code for bench
				if(IsSet(baseUsable->useFlags, BaseUsable::IS_BENCH))
				{
					switch(location->type)
					{
					case L_CITY:
						variant = 0;
						break;
					case L_DUNGEON:
						variant = Rand() % 2;
						break;
					default:
						variant = Rand() % 2 + 2;
						break;
					}
				}
				else
					variant = Random<int>(base->variants->meshes.size() - 1);
			}
		}
		u->variant = variant;

		locPart.usables.push_back(u);

		SpawnObjectExtras(locPart, base, pos, rot, u, scale, flags);

		return u;
	}
	else if(IsSet(base->flags, OBJ_IS_CHEST))
	{
		// chest
		Chest* chest = new Chest;
		chest->Register();
		chest->base = base;
		chest->rot = rot;
		chest->pos = pos;
		locPart.chests.push_back(chest);

		SpawnObjectExtras(locPart, base, pos, rot, nullptr, scale, flags);

		return chest;
	}
	else
	{
		// normal object
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		locPart.objects.push_back(o);

		SpawnObjectExtras(locPart, base, pos, rot, o, scale, flags);

		return o;
	}
}

//=================================================================================================
void Level::SpawnObjectExtras(LocationPart& locPart, BaseObject* obj, const Vec3& pos, float rot, void* userPtr, float scale, int flags)
{
	assert(obj);

	// ogieñ pochodni
	if(!IsSet(flags, SOE_DONT_SPAWN_PARTICLES))
	{
		if(IsSet(obj->flags, OBJ_LIGHT))
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emissionInterval = 0.1f;
			pe->emissions = -1;
			pe->life = -1;
			pe->maxParticles = 50;
			pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particleLife = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->posMin = Vec3(0, 0, 0);
			pe->posMax = Vec3(0, 0, 0);
			pe->spawnMin = 1;
			pe->spawnMax = 3;
			pe->speedMin = Vec3(-1, 3, -1);
			pe->speedMax = Vec3(1, 4, 1);
			pe->mode = 1;
			pe->Init();
			locPart.lvlPart->pes.push_back(pe);

			pe->tex = gameRes->tFlare;
			if(IsSet(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					pe->tex = gameRes->tFlare2;
			}

			// œwiat³o
			if(!IsSet(flags, SOE_DONT_CREATE_LIGHT))
			{
				GameLight& light = Add1(locPart.lights);
				light.startPos = pe->pos;
				light.range = 5;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					light.startColor = Vec3(0.8f, 0.8f, 1.f);
				else
					light.startColor = Vec3(1.f, 0.9f, 0.9f);
			}
		}
		else if(IsSet(obj->flags, OBJ_BLOOD_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emissionInterval = 0.1f;
			pe->emissions = -1;
			pe->life = -1;
			pe->maxParticles = 50;
			pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particleLife = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->posMin = Vec3(0, 0, 0);
			pe->posMax = Vec3(0, 0, 0);
			pe->spawnMin = 1;
			pe->spawnMax = 3;
			pe->speedMin = Vec3(-1, 4, -1);
			pe->speedMax = Vec3(1, 6, 1);
			pe->mode = 0;
			pe->tex = gameRes->tBlood[BLOOD_RED];
			pe->size = 0.5f;
			pe->Init();
			locPart.lvlPart->pes.push_back(pe);
		}
		else if(IsSet(obj->flags, OBJ_WATER_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emissionInterval = 0.1f;
			pe->emissions = -1;
			pe->life = -1;
			pe->maxParticles = 500;
			pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particleLife = 3.f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->posMin = Vec3(0, 0, 0);
			pe->posMax = Vec3(0, 0, 0);
			pe->spawnMin = 4;
			pe->spawnMax = 8;
			pe->speedMin = Vec3(-0.6f, 4, -0.6f);
			pe->speedMax = Vec3(0.6f, 7, 0.6f);
			pe->mode = 0;
			pe->tex = gameRes->tWater;
			pe->size = 0.05f;
			pe->Init();
			locPart.lvlPart->pes.push_back(pe);
		}
	}

	// fizyka
	if(obj->shape)
	{
		CollisionObject& c = Add1(locPart.lvlPart->colliders);
		c.owner = userPtr;
		c.camCollider = IsSet(obj->flags, OBJ_PHY_BLOCKS_CAM);

		int group = CG_OBJECT;
		if(IsSet(obj->flags, OBJ_PHY_BLOCKS_CAM))
			group |= CG_CAMERA_COLLIDER;

		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(obj->shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);

		if(obj->type == OBJ_CYLINDER)
		{
			cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2, pos.z));
			c.type = CollisionObject::SPHERE;
			c.pos = pos;
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			btTransform& tr = cobj->getWorldTransform();
			const Matrix mat = *obj->matrix * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(mat);
			pos2 += pos;
			tr.setOrigin(ToVector3(pos2));
			tr.setRotation(btQuaternion(rot, 0, 0));

			c.pos = pos2;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
		else
		{
			const Matrix m_rot = Matrix::RotationY(rot);
			const Matrix m_pos = Matrix::Translation(pos);
			// skalowanie jakimœ sposobem przechodzi do btWorldTransform i przy rysowaniu jest z³a skala (dwukrotnie u¿yta)
			const Matrix m_scale = Matrix::Scale(1.f / obj->size.x, 1.f, 1.f / obj->size.y);
			const Matrix m = m_scale * *obj->matrix * m_rot * m_pos;
			cobj->getWorldTransform().setFromOpenGLMatrix(&m._11);
			Vec3 out_pos = Vec3::TransformZero(m);
			Quat q = Quat::CreateFromRotationMatrix(m);

			float yaw = asin(-2 * (q.x * q.z - q.w * q.y));
			c.pos = out_pos;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(yaw))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = yaw;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}

		phyWorld->addCollisionObject(cobj, group);

		if(IsSet(obj->flags, OBJ_PHYSICS_PTR))
		{
			assert(userPtr);
			cobj->setUserPointer(userPtr);
		}

		if(IsSet(obj->flags, OBJ_DOUBLE_PHYSICS))
			SpawnObjectExtras(locPart, obj->nextObj, pos, rot, userPtr, scale, flags);
		else if(IsSet(obj->flags, OBJ_MULTI_PHYSICS))
		{
			for(int i = 0;; ++i)
			{
				if(obj->nextObj[i].shape)
					SpawnObjectExtras(locPart, &obj->nextObj[i], pos, rot, userPtr, scale, flags);
				else
					break;
			}
		}
	}
	else if(IsSet(obj->flags, OBJ_SCALEABLE))
	{
		CollisionObject& c = Add1(locPart.lvlPart->colliders);
		c.type = CollisionObject::SPHERE;
		c.pos = pos;
		c.radius = obj->r * scale;

		btCollisionObject* cobj = new btCollisionObject;
		btCylinderShape* shape = new btCylinderShape(btVector3(obj->r * scale, obj->h * scale, obj->r * scale));
		shapes.push_back(shape);
		cobj->setCollisionShape(shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);
		cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2 * scale, pos.z));
		phyWorld->addCollisionObject(cobj, CG_OBJECT);
	}
	else if(IsSet(obj->flags, OBJ_TMP_PHYSICS))
	{
		CollisionObject& c = Add1(locPart.lvlPart->colliders);
		c.owner = CollisionObject::TMP;
		c.camCollider = false;
		if(obj->type == OBJ_CYLINDER)
		{
			c.type = CollisionObject::SPHERE;
			c.pos = pos;
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			const Matrix mat = *obj->matrix * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(mat);
			pos2 += pos;

			c.pos = pos2;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
	}

	if(IsSet(obj->flags, OBJ_CAM_COLLIDERS))
	{
		int roti = (int)round((rot / (PI / 2)));
		for(vector<Mesh::Point>::const_iterator it = obj->mesh->attachPoints.begin(), end = obj->mesh->attachPoints.end(); it != end; ++it)
		{
			const Mesh::Point& pt = *it;
			if(strncmp(pt.name.c_str(), "camcol", 6) != 0)
				continue;

			const Matrix m = pt.mat * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(m) + pos;

			btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
			shapes.push_back(shape);
			btCollisionObject* co = new btCollisionObject;
			co->setCollisionShape(shape);
			co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
			co->getWorldTransform().setOrigin(ToVector3(pos2));
			phyWorld->addCollisionObject(co, CG_CAMERA_COLLIDER);
			if(roti != 0)
				co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

			float w = pt.size.x, h = pt.size.z;
			if(roti == 1 || roti == 3)
				std::swap(w, h);

			CameraCollider& cc = Add1(camColliders);
			cc.box.v1.x = pos2.x - w;
			cc.box.v2.x = pos2.x + w;
			cc.box.v1.z = pos2.z - h;
			cc.box.v2.z = pos2.z + h;
			cc.box.v1.y = pos2.y - pt.size.y;
			cc.box.v2.y = pos2.y + pt.size.y;
		}
	}
}

//=================================================================================================
void Level::ProcessBuildingObjects(LocationPart& locPart, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* insideMesh, float rot, GameDirection dir,
	const Vec3& shift, Building* building, CityBuilding* cityBuilding, bool recreate, Vec3* outPoint)
{
	if(mesh->attachPoints.empty())
	{
		cityBuilding->walkPt = shift;
		assert(!inside);
		return;
	}

	// https://github.com/Tomash667/carpg/wiki/%5BPL%5D-Buildings-export
	// o_x_[!N!]nazwa_???
	// x (o - obiekt, r - obrócony obiekt, p - fizyka, s - strefa, c - postaæ, m - maska œwiat³a, d - detal wokó³ obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	string token;
	bool have_exit = false, have_spawn = false;
	bool is_inside = (inside != nullptr);
	Vec3 spawn_point;
	static vector<const Mesh::Point*> details;

	for(vector<Mesh::Point>::const_iterator it2 = mesh->attachPoints.begin(), end2 = mesh->attachPoints.end(); it2 != end2; ++it2)
	{
		const Mesh::Point& pt = *it2;
		if(pt.name.length() < 5 || pt.name[0] != 'o')
			continue;

		char c = pt.name[2];
		if(c == 'o' || c == 'r' || c == 'p' || c == 's' || c == 'c' || c == 'l' || c == 'e')
		{
			uint poss = pt.name.find_first_of('_', 4);
			if(poss == string::npos)
				poss = pt.name.length();
			token = pt.name.substr(4, poss - 4);
			for(uint k = 0, len = token.length(); k < len; ++k)
			{
				if(token[k] == '#')
					token[k] = '_';
			}
		}
		else if(c == 'm')
		{
			// light mask
			// nothing to do here
		}
		else if(c == 'd')
		{
			if(!recreate)
			{
				assert(!is_inside);
				details.push_back(&pt);
			}
			continue;
		}
		else
			continue;

		Vec3 pos;
		if(dir != GDIR_DOWN)
		{
			const Matrix m = pt.mat * Matrix::RotationY(rot);
			pos = Vec3::TransformZero(m);
		}
		else
			pos = Vec3::TransformZero(pt.mat);
		const float height = pos.y;
		pos += shift;

		switch(c)
		{
		case 'o': // object
		case 'r': // rotated object
		case 'l': // random rotation (90* angle) object
			if(!recreate)
			{
				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* base = BaseObject::TryGet(name);
				assert(base);

				if(base)
				{
					if(locPart.partType == LocationPart::Type::Outside)
						pos.y = terrain->GetH(pos) + height;

					float r;
					switch(c)
					{
					case 'o':
					default:
						r = Random(MAX_ANGLE);
						break;
					case 'r':
						r = Clip(pt.rot.y + rot);
						break;
					case 'l':
						r = PI / 2 * (Rand() % 4);
						break;
					}
					SpawnObjectEntity(locPart, base, pos, r, 1.f, 0, nullptr, variant);
				}
			}
			break;
		case 'p': // physics
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(locPart.lvlPart->colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pos = pos;
				cobj.owner = nullptr;
				cobj.camCollider = is_wall;

				if(locPart.partType == LocationPart::Type::Outside)
				{
					terrain->SetY(pos);
					pos.y += 2.f;
				}

				btCylinderShape* shape = new btCylinderShape(btVector3(pt.size.x, 4.f, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phyWorld->addCollisionObject(co, group);
			}
			else if(token == "square" || token == "squarev" || token == "squarevn" || token == "squarevp")
			{
				bool is_wall = (token == "square" || token == "squarevn");
				bool block_camera = (token == "square");

				CollisionObject& cobj = Add1(locPart.lvlPart->colliders);
				cobj.type = CollisionObject::RECTANGLE;
				cobj.pos = pos;
				cobj.w = pt.size.x;
				cobj.h = pt.size.z;
				cobj.owner = nullptr;
				cobj.camCollider = block_camera;

				btBoxShape* shape;
				if(token != "squarevp")
				{
					shape = new btBoxShape(btVector3(pt.size.x, 16.f, pt.size.z));
					if(locPart.partType == LocationPart::Type::Outside)
					{
						terrain->SetY(pos);
						pos.y += 8.f;
					}
					else
						pos.y = 0.f;
				}
				else
				{
					shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
					if(locPart.partType == LocationPart::Type::Outside)
						pos.y += terrain->GetH(pos);
				}
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phyWorld->addCollisionObject(co, group);

				if(dir != GDIR_DOWN)
				{
					cobj.type = CollisionObject::RECTANGLE_ROT;
					cobj.rot = rot;
					cobj.radius = sqrt(cobj.w * cobj.w + cobj.h * cobj.h);
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
				}
			}
			else if(token == "squarevpa")
			{
				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				if(locPart.partType == LocationPart::Type::Outside)
					pos.y += terrain->GetH(pos);
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = CG_COLLIDER;
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phyWorld->addCollisionObject(co, group);

				if(dir != GDIR_DOWN)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
			}
			else if(token == "squarecam")
			{
				if(locPart.partType == LocationPart::Type::Outside)
					pos.y += terrain->GetH(pos);

				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phyWorld->addCollisionObject(co, CG_CAMERA_COLLIDER);
				if(dir != GDIR_DOWN)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

				float w = pt.size.x, h = pt.size.z;
				if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
					std::swap(w, h);

				CameraCollider& cc = Add1(camColliders);
				cc.box.v1.x = pos.x - w;
				cc.box.v2.x = pos.x + w;
				cc.box.v1.z = pos.z - h;
				cc.box.v2.z = pos.z + h;
				cc.box.v1.y = pos.y - pt.size.y;
				cc.box.v2.y = pos.y + pt.size.y;
			}
			else if(token == "xsphere")
			{
				inside->xspherePos = pos;
				inside->xsphereRadius = pt.size.x;
			}
			else
				assert(0);
			break;
		case 's': // zone
			if(!recreate)
			{
				if(token == "enter")
				{
					assert(!inside);

					inside = new InsideBuilding((int)city->insideBuildings.size());
					inside->lvlPart = new LevelPart(inside);
					inside->levelShift = city->insideOffset;
					inside->offset = Vec2(512.f * city->insideOffset.x + 256.f, 512.f * city->insideOffset.y + 256.f);
					if(city->insideOffset.x > city->insideOffset.y)
					{
						--city->insideOffset.x;
						++city->insideOffset.y;
					}
					else
					{
						city->insideOffset.x += 2;
						city->insideOffset.y = 0;
					}
					float w, h;
					if(dir == GDIR_DOWN || dir == GDIR_UP)
					{
						w = pt.size.x;
						h = pt.size.z;
					}
					else
					{
						w = pt.size.z;
						h = pt.size.x;
					}
					inside->enterRegion.v1.x = pos.x - w;
					inside->enterRegion.v1.y = pos.z - h;
					inside->enterRegion.v2.x = pos.x + w;
					inside->enterRegion.v2.y = pos.z + h;
					Vec2 mid = inside->enterRegion.Midpoint();
					inside->enterY = terrain->GetH(mid.x, mid.y) + 0.1f;
					inside->building = building;
					inside->outsideRot = rot;
					inside->top = -1.f;
					inside->xsphereRadius = -1.f;
					city->insideBuildings.push_back(inside);
					locParts.push_back(*inside);

					assert(insideMesh);

					if(insideMesh)
					{
						Vec3 o_pos = Vec3(inside->offset.x, 0.f, inside->offset.y);

						Object* o = new Object;
						o->base = nullptr;
						o->mesh = insideMesh;
						o->pos = o_pos;
						o->rot = Vec3(0, 0, 0);
						o->scale = 1.f;
						o->requireSplit = true;
						inside->objects.push_back(o);

						ProcessBuildingObjects(*inside, city, inside, insideMesh, nullptr, 0.f, GDIR_DOWN, o->pos, nullptr, nullptr);
					}

					have_exit = true;
				}
				else if(token == "exit")
				{
					assert(inside);

					inside->exitRegion.v1.x = pos.x - pt.size.x;
					inside->exitRegion.v1.y = pos.z - pt.size.z;
					inside->exitRegion.v2.x = pos.x + pt.size.x;
					inside->exitRegion.v2.y = pos.z + pt.size.z;

					have_exit = true;
				}
				else if(token == "spawn")
				{
					if(is_inside)
						inside->insideSpawn = pos;
					else
					{
						spawn_point = pos;
						terrain->SetY(spawn_point);
					}

					have_spawn = true;
				}
				else if(token == "top")
				{
					assert(is_inside);
					inside->top = pos.y;
				}
				else if(token == "door" || token == "doorc" || token == "doorl" || token == "door2")
				{
					Door* door = new Door;
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->locked = LOCK_NONE;
					if(token != "door") // door2 are closed now, this is intended
					{
						door->state = Door::Closed;
						if(token == "doorl")
							door->locked = LOCK_UNLOCKABLE;
					}
					else
						door->state = Door::Opened;
					door->door2 = (token == "door2");
					door->Init();
					locPart.doors.push_back(door);
				}
				else if(token == "arena")
				{
					assert(inside);

					inside->region1.v1.x = pos.x - pt.size.x;
					inside->region1.v1.y = pos.z - pt.size.z;
					inside->region1.v2.x = pos.x + pt.size.x;
					inside->region1.v2.y = pos.z + pt.size.z;
				}
				else if(token == "arena2")
				{
					assert(inside);

					inside->region2.v1.x = pos.x - pt.size.x;
					inside->region2.v1.y = pos.z - pt.size.z;
					inside->region2.v2.x = pos.x + pt.size.x;
					inside->region2.v2.y = pos.z + pt.size.z;
				}
				else if(token == "viewer")
				{
					// ten punkt jest u¿ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(cityBuilding)
					{
						cityBuilding->walkPt = pos;
						terrain->SetY(cityBuilding->walkPt);
					}
					else if(outPoint)
						*outPoint = pos;
				}
				else
					assert(0);
			}
			break;
		case 'c': // unit
			if(!recreate && city->citizens > 0)
			{
				UnitData* ud = UnitData::TryGet(token.c_str());
				assert(ud);
				if(ud)
				{
					Unit* u = SpawnUnitNearLocation(locPart, pos, *ud, nullptr, -2);
					if(u)
					{
						u->rot = Clip(pt.rot.y + rot);
						u->ai->startRot = u->rot;
					}
				}
			}
			break;
		case 'm': // light mask
			{
				LightMask& mask = Add1(inside->masks);
				mask.size = Vec2(pt.size.x, pt.size.z);
				mask.pos = Vec2(pos.x, pos.z);
			}
			break;
		case 'e': // effect
			if(token == "magicfire")
			{
				if(!game->inLoad)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = gameRes->tFlare2;
					pe->alpha = 1.0f;
					pe->size = 1.0f;
					pe->emissionInterval = 0.1f;
					pe->emissions = -1;
					pe->life = -1;
					pe->maxParticles = 50;
					pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->particleLife = 0.5f;
					pe->pos = pos;
					if(locPart.partType == LocationPart::Type::Outside)
						pe->pos.y += terrain->GetH(pos);
					pe->posMin = Vec3(0, 0, 0);
					pe->posMax = Vec3(0, 0, 0);
					pe->spawnMin = 2;
					pe->spawnMax = 4;
					pe->speedMin = Vec3(-1, 3, -1);
					pe->speedMax = Vec3(1, 4, 1);
					pe->mode = 1;
					pe->Init();
					locPart.lvlPart->pes.push_back(pe);
				}
			}
			else
				assert(0);
			break;
		}
	}

	if(is_inside)
	{
		// floor
		btCollisionObject* co = new btCollisionObject;
		co->setCollisionShape(shapeFloor);
		co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_COLLIDER);
		co->getWorldTransform().setOrigin(ToVector3(shift));
		phyWorld->addCollisionObject(co, CG_COLLIDER);

		// ceiling
		co = new btCollisionObject;
		co->setCollisionShape(shapeFloor);
		co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_COLLIDER);
		co->getWorldTransform().setOrigin(ToVector3(shift + Vec3(0, 4, 0)));
		phyWorld->addCollisionObject(co, CG_COLLIDER);
	}

	if(!details.empty() && !is_inside)
	{
		int c = Rand() % 80 + details.size() * 2, count;
		if(c < 10)
			count = 0;
		else if(c < 30)
			count = 1;
		else if(c < 60)
			count = 2;
		else if(c < 90)
			count = 3;
		else
			count = 4;
		if(count > 0)
		{
			Shuffle(details.begin(), details.end());
			while(count && !details.empty())
			{
				const Mesh::Point& pt = *details.back();
				details.pop_back();
				--count;

				uint poss = pt.name.find_first_of('_', 4);
				if(poss == string::npos)
				{
					assert(0);
					continue;
				}
				token = pt.name.substr(4, poss - 4);
				for(uint k = 0, len = token.length(); k < len; ++k)
				{
					if(token[k] == '#')
						token[k] = '_';
				}

				Vec3 pos;
				if(dir != GDIR_DOWN)
				{
					const Matrix m = pt.mat * Matrix::RotationY(rot);
					pos = Vec3::TransformZero(m);
				}
				else
					pos = Vec3::TransformZero(pt.mat);
				pos += shift;

				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* obj = BaseObject::TryGet(name);
				assert(obj);

				if(obj)
				{
					if(locPart.partType == LocationPart::Type::Outside)
						terrain->SetY(pos);
					SpawnObjectEntity(locPart, obj, pos, Clip(pt.rot.y + rot), 1.f, 0, nullptr, variant);
				}
			}
		}
		details.clear();
	}

	if(!recreate)
	{
		if(is_inside || inside)
			assert(have_exit && have_spawn);

		if(!is_inside && inside)
			inside->outsideSpawn = spawn_point;
	}
}

//=================================================================================================
void Level::RecreateObjects(bool spawnParticles)
{
	static BaseObject* chest = BaseObject::Get("chest");

	for(LocationPart& locPart : locParts)
	{
		int flags = Level::SOE_DONT_CREATE_LIGHT;
		if(!spawnParticles)
			flags |= Level::SOE_DONT_SPAWN_PARTICLES;

		// dotyczy tylko pochodni
		if(locPart.partType == LocationPart::Type::Inside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			BaseLocation& base = gBaseLocations[inside->target];
			if(IsSet(base.options, BLO_MAGIC_LIGHT))
				flags |= Level::SOE_MAGIC_LIGHT;
		}

		for(vector<Object*>::iterator it = locPart.objects.begin(), end = locPart.objects.end(); it != end; ++it)
		{
			Object& obj = **it;
			BaseObject* base_obj = obj.base;

			if(!base_obj)
				continue;

			if(IsSet(base_obj->flags, OBJ_BUILDING))
			{
				const float rot = obj.rot.y;
				const GameDirection dir = RotToDir(rot);
				ProcessBuildingObjects(locPart, nullptr, nullptr, base_obj->mesh, nullptr, rot, dir, obj.pos, nullptr, nullptr, true);
			}
			else
				SpawnObjectExtras(locPart, base_obj, obj.pos, obj.rot.y, &obj, obj.scale, flags);
		}

		for(vector<Chest*>::iterator it = locPart.chests.begin(), end = locPart.chests.end(); it != end; ++it)
			SpawnObjectExtras(locPart, chest, (*it)->pos, (*it)->rot, nullptr, 1.f, flags);

		for(vector<Usable*>::iterator it = locPart.usables.begin(), end = locPart.usables.end(); it != end; ++it)
			SpawnObjectExtras(locPart, (*it)->base, (*it)->pos, (*it)->rot, *it, 1.f, flags);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LocationPart& locPart, BaseObject* obj, const Vec2& pos, float rot, float range, float margin, float scale)
{
	bool ok = false;
	if(obj->type == OBJ_CYLINDER)
	{
		globalCol.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(locPart, globalCol, pt, obj->r + margin + range);
		float extraRadius = range / 20;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(globalCol, pt, obj->r + margin))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;
			extraRadius += range / 20;
		}

		if(!ok)
			return nullptr;

		if(locPart.partType == LocationPart::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(locPart, obj, pt, rot, scale);
	}
	else
	{
		globalCol.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(locPart, globalCol, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extraRadius = range / 20;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(globalCol, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;
			extraRadius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(locPart.partType == LocationPart::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(locPart, obj, pt, rot, scale);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LocationPart& locPart, BaseObject* obj, const Vec2& pos, const Vec2& rotTarget, float range, float margin,
	float scale)
{
	if(obj->type == OBJ_CYLINDER)
		return SpawnObjectNearLocation(locPart, obj, pos, Vec2::LookAtAngle(pos, rotTarget), range, margin, scale);
	else
	{
		globalCol.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(locPart, globalCol, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extraRadius = range / 20, rot;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		bool ok = false;
		for(int i = 0; i < 20; ++i)
		{
			rot = Vec2::LookAtAngle(Vec2(pt.x, pt.z), rotTarget);
			if(!Collide(globalCol, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;
			extraRadius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(locPart.partType == LocationPart::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(locPart, obj, pt, rot, scale);
	}
}

//=================================================================================================
void Level::PickableItemBegin(LocationPart& locPart, Object& o)
{
	assert(o.base);

	pickableLocPart = &locPart;
	pickableObj = &o;
	pickableSpawns.clear();
	pickableItems.clear();

	for(vector<Mesh::Point>::iterator it = o.base->mesh->attachPoints.begin(), end = o.base->mesh->attachPoints.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			assert(it->type == Mesh::Point::BOX);
			Box box(it->mat._41, it->mat._42, it->mat._43);
			box.v1.x -= it->size.x - 0.05f;
			box.v1.z -= it->size.z - 0.05f;
			box.v2.x += it->size.x - 0.05f;
			box.v2.z += it->size.z - 0.05f;
			box.v1.y = box.v2.y = box.v1.y - it->size.y;
			pickableSpawns.push_back(box);
		}
	}

	assert(!pickableSpawns.empty());
}

//=================================================================================================
bool Level::PickableItemAdd(const Item* item)
{
	assert(item);

	for(int i = 0; i < 20; ++i)
	{
		// pobierz punkt
		uint spawn = Rand() % pickableSpawns.size();
		Box& box = pickableSpawns[spawn];
		// ustal pozycjê
		Vec3 pos(Random(box.v1.x, box.v2.x), box.v1.y, Random(box.v1.z, box.v2.z));
		// sprawdŸ kolizjê
		bool ok = true;
		for(vector<PickableItem>::iterator it = pickableItems.begin(), end = pickableItems.end(); it != end; ++it)
		{
			if(it->spawn == spawn)
			{
				if(CircleToCircle(pos.x, pos.z, 0.15f, it->pos.x, it->pos.z, 0.15f))
				{
					ok = false;
					break;
				}
			}
		}
		// dodaj
		if(ok)
		{
			PickableItem& i = Add1(pickableItems);
			i.spawn = spawn;
			i.pos = pos;

			GroundItem* groundItem = new GroundItem;
			groundItem->Register();
			groundItem->count = 1;
			groundItem->teamCount = 1;
			groundItem->item = item;
			groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
			float rot = pickableObj->rot.y,
				s = sin(rot),
				c = cos(rot);
			groundItem->pos = Vec3(pos.x * c + pos.z * s, pos.y, -pos.x * s + pos.z * c) + pickableObj->pos;
			pickableLocPart->AddGroundItem(groundItem, false);

			return true;
		}
	}

	return false;
}

//=================================================================================================
void Level::PickableItemsFromStock(LocationPart& locPart, Object& o, Stock& stock)
{
	pickableTmpStock.clear();
	stock.Parse(pickableTmpStock);

	if(!pickableTmpStock.empty())
	{
		PickableItemBegin(locPart, o);
		for(ItemSlot& slot : pickableTmpStock)
		{
			for(uint i = 0; i < slot.count; ++i)
			{
				if(!PickableItemAdd(slot.item))
					return;
			}
		}
	}
}

//=================================================================================================
GroundItem* Level::FindGroundItem(int id, LocationPart** locPartResult)
{
	for(LocationPart& locPart : locParts)
	{
		for(GroundItem* groundItem : locPart.GetGroundItems())
		{
			if(groundItem->id == id)
			{
				if(locPartResult)
					*locPartResult = &locPart;
				return groundItem;
			}
		}
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideAnyRoom(const Item* item)
{
	assert(lvl && item);
	while(true)
	{
		int id = Rand() % lvl->rooms.size();
		if(!lvl->rooms[id]->IsCorridor())
		{
			GroundItem* groundItem = SpawnGroundItemInsideRoom(*lvl->rooms[id], item);
			if(groundItem)
				return groundItem;
		}
	}
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRoom(Room& room, const Item* item)
{
	assert(item);

	for(int i = 0; i < 50; ++i)
	{
		Vec3 pos = room.GetRandomPos(0.5f);
		globalCol.clear();
		GatherCollisionObjects(*localPart, globalCol, pos, 0.25f);
		if(!Collide(globalCol, pos, 0.25f))
			return SpawnItem(item, pos);
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool tryExact)
{
	assert(item);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!tryExact)
		{
			float a = Random(), b = Random();
			if(b < a)
				std::swap(a, b);
			pt = Vec3(pos.x + b * radius * cos(2 * PI * a / b), 0, pos.y + b * radius * sin(2 * PI * a / b));
		}
		else
		{
			tryExact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		globalCol.clear();
		GatherCollisionObjects(*localPart, globalCol, pt, 0.25f);
		if(!Collide(globalCol, pt, 0.25f))
			return SpawnItem(item, pt);
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& regionSize, bool tryExact)
{
	assert(item);
	assert(regionSize.x > 0 && regionSize.y > 0);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!tryExact)
			pt = Vec3(pos.x + Random(regionSize.x), 0, pos.y + Random(regionSize.y));
		else
		{
			tryExact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		globalCol.clear();
		GatherCollisionObjects(*localPart, globalCol, pt, 0.25f);
		if(!Collide(globalCol, pt, 0.25f))
			return SpawnItem(item, pt);
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::CreateUnit(UnitData& base, int level, bool createPhysics)
{
	Unit* unit = new Unit;
	unit->Init(base, level);

	// preload items
	if(base.group != G_PLAYER && base.itemScript && !resMgr->IsLoadScreen())
	{
		array<const Item*, SLOT_MAX>& equipped = unit->GetEquippedItems();
		for(const Item* item : equipped)
		{
			if(item)
				gameRes->PreloadItem(item);
		}
		for(ItemSlot& slot : unit->items)
			gameRes->PreloadItem(slot.item);
	}
	if(base.trader && ready)
	{
		for(ItemSlot& slot : unit->stock->items)
			gameRes->PreloadItem(slot.item);
	}

	// mesh
	unit->CreateMesh(Unit::CREATE_MESH::NORMAL);

	// physics
	if(createPhysics)
		unit->CreatePhysics();
	else
		unit->cobj = nullptr;

	if(Net::IsServer() && ready)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
	}

	return unit;
}

//=================================================================================================
Unit* Level::CreateUnitWithAI(LocationPart& locPart, UnitData& unit, int level, const Vec3* pos, const float* rot)
{
	Unit* u = CreateUnit(unit, level);
	u->locPart = &locPart;
	locPart.units.push_back(u);

	if(pos)
	{
		if(locPart.partType == LocationPart::Type::Outside)
		{
			Vec3 pt = *pos;
			gameLevel->terrain->SetY(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		u->UpdatePhysics();
		u->visualPos = u->pos;
	}

	if(rot)
		u->rot = *rot;

	AIController* ai = new AIController;
	ai->Init(u);
	game->ais.push_back(ai);

	return u;
}

//=================================================================================================
Vec3 Level::FindSpawnPos(Room* room, Unit* unit)
{
	assert(room && unit);

	const float radius = unit->data->GetRadius();

	for(int i = 0; i < 10; ++i)
	{
		Vec3 pos = room->GetRandomPos(radius);
		Int2 pt = PosToPt(pos);

		Tile& tile = lvl->map[pt(lvl->w)];
		if(Any(tile.type, ENTRY_PREV, ENTRY_NEXT))
			continue;

		globalCol.clear();
		GatherCollisionObjects(*localPart, globalCol, pos, radius, nullptr);
		if(!Collide(globalCol, pos, radius))
			return pos;
	}

	return room->Center();
}

//=================================================================================================
Vec3 Level::FindSpawnPos(LocationPart& locPart, Unit* unit)
{
	assert(locPart.partType == LocationPart::Type::Building); // not implemented

	InsideBuilding& inside = static_cast<InsideBuilding&>(locPart);

	Vec3 pos;
	WarpToRegion(locPart, inside.region1, unit->GetUnitRadius(), pos, 20);
	return pos;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoom(Room& room, UnitData& unit, int level, const Int2& awayPt, const Int2& excludedPt)
{
	const float radius = unit.GetRadius();
	const Vec3 awayPos(2.f * awayPt.x + 1.f, 0.f, 2.f * awayPt.y + 1.f);

	for(int i = 0; i < 10; ++i)
	{
		const Vec3 pos = room.GetRandomPos(radius);
		if(Vec3::Distance(awayPos, pos) < ALERT_SPAWN_RANGE)
			continue;

		const Int2 pt = Int2(int(pos.x / 2), int(pos.y / 2));
		if(pt == excludedPt)
			continue;

		globalCol.clear();
		GatherCollisionObjects(*localPart, globalCol, pos, radius, nullptr);

		if(!Collide(globalCol, pos, radius))
		{
			float rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(*localPart, unit, level, &pos, &rot);
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoomOrNear(Room& room, UnitData& ud, int level, const Int2& awayPt, const Int2& excludedPt)
{
	Unit* u = SpawnUnitInsideRoom(room, ud, level, awayPt, excludedPt);
	if(u)
		return u;

	LocalVector<Room*> connected(room.connected);
	connected.Shuffle();

	for(Room* room : connected)
	{
		u = SpawnUnitInsideRoom(*room, ud, level, awayPt, excludedPt);
		if(u)
			return u;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocation(LocationPart& locPart, const Vec3& pos, UnitData& unit, const Vec3* lookAt, int level, float extraRadius)
{
	const float radius = unit.GetRadius();

	globalCol.clear();
	GatherCollisionObjects(locPart, globalCol, pos, extraRadius + radius, nullptr);

	Vec3 tmpPos = pos;

	for(int i = 0; i < 10; ++i)
	{
		if(!Collide(globalCol, tmpPos, radius))
		{
			float rot;
			if(lookAt)
				rot = Vec3::LookAtAngle(tmpPos, *lookAt);
			else
				rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(locPart, unit, level, &tmpPos, &rot);
		}

		tmpPos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRegion(LocationPart& locPart, const Box2d& region, UnitData& unit, int level)
{
	Vec3 pos;
	if(!WarpToRegion(locPart, region, unit.GetRadius(), pos))
		return nullptr;

	return CreateUnitWithAI(locPart, unit, level, &pos);
}

//=================================================================================================
Unit* Level::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn, int flags)
{
	if(!inn)
		inn = cityCtx->FindInn();

	Vec3 pos;
	bool ok = false;
	if(IsSet(flags, SU_MAIN_ROOM) || Rand() % 5 != 0)
	{
		if(WarpToRegion(*inn, inn->region1, ud.GetRadius(), pos, 20)
			|| WarpToRegion(*inn, inn->region2, ud.GetRadius(), pos, 10))
			ok = true;
	}
	else
	{
		if(WarpToRegion(*inn, inn->region2, ud.GetRadius(), pos, 10)
			|| WarpToRegion(*inn, inn->region1, ud.GetRadius(), pos, 20))
			ok = true;
	}

	if(ok)
	{
		float rot = Random(MAX_ANGLE);
		Unit* u = CreateUnitWithAI(*inn, ud, level, &pos, &rot);
		if(u && IsSet(flags, SU_TEMPORARY))
			u->temporary = true;
		return u;
	}
	else
		return nullptr;
}

//=================================================================================================
void Level::SpawnUnitsGroup(LocationPart& locPart, const Vec3& pos, const Vec3* lookAt, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback)
{
	Pooled<TmpUnitGroup> tgroup;
	tgroup->Fill(group, level);

	for(TmpUnitGroup::Spawn& spawn : tgroup->Roll(level, count))
	{
		Unit* u = SpawnUnitNearLocation(locPart, pos, *spawn.first, lookAt, spawn.second, 4.f);
		if(u && callback)
			callback(u);
	}
}

//=================================================================================================
Unit* Level::SpawnUnit(LocationPart& locPart, TmpSpawn spawn)
{
	return SpawnUnit(locPart, *spawn.first, spawn.second);
}

//=================================================================================================
Unit* Level::SpawnUnit(LocationPart& locPart, UnitData& data, int level)
{
	assert(locPart.partType == LocationPart::Type::Building); // not implemented

	InsideBuilding& building = static_cast<InsideBuilding&>(locPart);
	Vec3 pos;
	if(!WarpToRegion(locPart, building.region1, data.GetRadius(), pos))
		return nullptr;

	float rot = Random(MAX_ANGLE);
	return CreateUnitWithAI(locPart, data, level, &pos, &rot);
}

//=================================================================================================
void Level::SpawnUnits(UnitGroup* group, int level)
{
	LocationGenerator* locGen = game->locGenFactory->Get(location);
	locGen->SpawnUnits(group, level);
}

//=================================================================================================
void Level::GatherCollisionObjects(LocationPart& locPart, vector<CollisionObject>& objects, const Vec3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x - _radius) / 2)),
		maxx = int((_pos.x + _radius) / 2),
		minz = max(0, int((_pos.z - _radius) / 2)),
		maxz = int((_pos.z + _radius) / 2);

	if((!ignore || !ignore->ignoreBlocks) && locPart.partType != LocationPart::Type::Building)
	{
		if(location->outside)
		{
			City* city = static_cast<City*>(location);
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].IsBlocking())
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			maxx = min(maxx, lvl->w);
			maxz = min(maxz, lvl->h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl->map[x + z * lvl->w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == ENTRY_NEXT || type == ENTRY_PREV)
					{
						EntryType entryType = type == ENTRY_NEXT ? lvl->nextEntryType : lvl->prevEntryType;
						if(entryType == ENTRY_STAIRS_UP || entryType == ENTRY_STAIRS_DOWN)
						{
							CollisionObject& co = Add1(objects);
							co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.checkRect = &Level::CollideWithStairsRect;
							co.extra = (type == ENTRY_PREV);
							co.type = CollisionObject::CUSTOM;
						}
					}
				}
			}
		}
	}

	// units
	if(!(ignore && ignore->ignoreUnits))
	{
		float radius;
		Vec3 pos;
		if(ignore && ignore->ignoredUnits)
		{
			for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
			{
				if(!*it || !(*it)->IsStanding())
					continue;

				const Unit** u = ignore->ignoredUnits;
				do
				{
					if(!*u)
						break;
					if(*u == *it)
						goto ignore_unit;
					++u;
				}
				while(1);

				radius = (*it)->GetUnitRadius();
				pos = (*it)->GetColliderPos();
				if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
				{
					CollisionObject& co = Add1(objects);
					co.pos = pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}

			ignore_unit:
				;
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
			{
				if(!*it || !(*it)->IsStanding())
					continue;

				radius = (*it)->GetUnitRadius();
				Vec3 pos = (*it)->GetColliderPos();
				if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
				{
					CollisionObject& co = Add1(objects);
					co.pos = pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}
			}
		}
	}

	// object colliders
	if(!(ignore && ignore->ignoreObjects))
	{
		if(!ignore || !ignore->ignoredObjects)
		{
			for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
			{
				if(it->type == CollisionObject::RECTANGLE)
				{
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->w, it->h))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->radius))
						objects.push_back(*it);
				}
			}
		}
		else
		{
			for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
			{
				if(it->owner)
				{
					const void** objs = ignore->ignoredObjects;
					do
					{
						if(it->owner == *objs)
							goto ignore;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->w, it->h))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->radius))
						objects.push_back(*it);
				}

			ignore:
				;
			}
		}
	}

	// doors
	if(!(ignore && ignore->ignoreDoors))
	{
		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			if((*it)->IsBlocking() && CircleToRotatedRectangle(_pos.x, _pos.z, _radius, (*it)->pos.x, (*it)->pos.z, Door::WIDTH, Door::THICKNESS, (*it)->rot))
			{
				CollisionObject& co = Add1(objects);
				co.pos = (*it)->pos;
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = Door::WIDTH;
				co.h = Door::THICKNESS;
				co.rot = (*it)->rot;
			}
		}
	}
}

//=================================================================================================
void Level::GatherCollisionObjects(LocationPart& locPart, vector<CollisionObject>& objects, const Box2d& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x / 2)),
		maxx = int(_box.v2.x / 2),
		minz = max(0, int(_box.v1.y / 2)),
		maxz = int(_box.v2.y / 2);

	if((!ignore || !ignore->ignoreBlocks) && locPart.partType != LocationPart::Type::Building)
	{
		if(location->outside)
		{
			City* city = static_cast<City*>(location);
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].IsBlocking())
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			maxx = min(maxx, lvl->w);
			maxz = min(maxz, lvl->h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl->map[x + z * lvl->w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == ENTRY_NEXT || type == ENTRY_PREV)
					{
						EntryType entryType = type == ENTRY_NEXT ? lvl->nextEntryType : lvl->prevEntryType;
						if(entryType == ENTRY_STAIRS_UP || entryType == ENTRY_STAIRS_DOWN)
						{
							CollisionObject& co = Add1(objects);
							co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.checkRect = &Level::CollideWithStairsRect;
							co.extra = (type == ENTRY_PREV);
							co.type = CollisionObject::CUSTOM;
						}
					}
				}
			}
		}
	}

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size();

	// units
	if(!(ignore && ignore->ignoreUnits))
	{
		float radius;
		if(ignore && ignore->ignoredUnits)
		{
			for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
			{
				if(!(*it)->IsStanding())
					continue;

				const Unit** u = ignore->ignoredUnits;
				do
				{
					if(!*u)
						break;
					if(*u == *it)
						goto ignore_unit;
					++u;
				}
				while(1);

				radius = (*it)->GetUnitRadius();
				if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				{
					CollisionObject& co = Add1(objects);
					co.pos = (*it)->pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}

			ignore_unit:
				;
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
			{
				if(!(*it)->IsStanding())
					continue;

				radius = (*it)->GetUnitRadius();
				if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				{
					CollisionObject& co = Add1(objects);
					co.pos = (*it)->pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}
			}
		}
	}

	// object colliders
	if(!(ignore && ignore->ignoreObjects))
	{
		if(!ignore || !ignore->ignoredObjects)
		{
			for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
			{
				if(it->type == CollisionObject::RECTANGLE)
				{
					if(RectangleToRectangle(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						objects.push_back(*it);
				}
			}
		}
		else
		{
			for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
			{
				if(it->owner)
				{
					const void** objs = ignore->ignoredObjects;
					do
					{
						if(it->owner == *objs)
							goto ignore;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(RectangleToRectangle(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						objects.push_back(*it);
				}

			ignore:
				;
			}
		}
	}
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius)
{
	assert(radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(Distance(pos.x, pos.z, it->pos.x, it->pos.z) <= it->radius + radius)
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(CircleToRectangle(pos.x, pos.z, radius, it->pos.x, it->pos.z, it->w, it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			if(CircleToRotatedRectangle(pos.x, pos.z, radius, it->pos.x, it->pos.z, it->w, it->h, it->rot))
				return true;
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check))(*it, pos, radius))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Box2d& _box, float _margin)
{
	Box2d box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pos.x, it->pos.z, it->radius + _margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w + _margin;
				r1.size.y = it->h + _margin;
				r1.rot = -it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = 0.f;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->checkRect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Box2d& _box, float margin, float _rot)
{
	if(!NotZero(_rot))
		return Collide(objects, _box, margin);

	Box2d box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	Vec2 rectpos = box.Midpoint(),
		rectsize = box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRotatedRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y, _rot))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = 0.f;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->checkRect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::CollideWithStairs(const CollisionObject& cobj, const Vec3& pos, float radius) const
{
	assert(cobj.type == CollisionObject::CUSTOM && cobj.check == &Level::CollideWithStairs && !location->outside && radius > 0.f);

	GameDirection dir;
	if(cobj.extra == 0)
		dir = lvl->nextEntryDir;
	else
		dir = lvl->prevEntryDir;

	if(dir != GDIR_DOWN)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x, cobj.pos.z - 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x - 0.95f, cobj.pos.z, 0.05f, 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x, cobj.pos.z + 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x + 0.95f, cobj.pos.z, 0.05f, 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
bool Level::CollideWithStairsRect(const CollisionObject& cobj, const Box2d& box) const
{
	assert(cobj.type == CollisionObject::CUSTOM && cobj.checkRect == &Level::CollideWithStairsRect && !location->outside);

	GameDirection dir;
	if(cobj.extra == 0)
		dir = lvl->nextEntryDir;
	else
		dir = lvl->prevEntryDir;

	if(dir != GDIR_DOWN)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z - 1.f, cobj.pos.x + 1.f, cobj.pos.z - 0.9f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z - 1.f, cobj.pos.x - 0.9f, cobj.pos.z + 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z + 0.9f, cobj.pos.x + 1.f, cobj.pos.z + 1.f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x + 0.9f, cobj.pos.z - 1.f, cobj.pos.x + 1.f, cobj.pos.z + 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
void Level::CreateBlood(LocationPart& locPart, const Unit& u, bool fullyCreated)
{
	if(!gameRes->tBloodSplat[u.data->blood] || IsSet(u.data->flags2, F2_BLOODLESS))
		return;

	Blood& b = Add1(locPart.bloods);
	u.meshInst->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = Random(MAX_ANGLE);
	b.size = (fullyCreated ? 1.f : 0.f);
	b.scale = u.data->bloodSize;

	if(locPart.haveTerrain)
	{
		b.pos.y = terrain->GetH(b.pos) + 0.05f;
		terrain->GetAngle(b.pos.x, b.pos.z, b.normal);
	}
	else
	{
		b.pos.y = u.pos.y + 0.05f;
		b.normal = Vec3(0, 1, 0);
	}
}

//=================================================================================================
void Level::SpawnBlood()
{
	for(Unit* unit : bloodToSpawn)
		CreateBlood(*unit->locPart, *unit, true);
	bloodToSpawn.clear();
}

//=================================================================================================
void Level::WarpUnit(Unit& unit, const Vec3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	unit.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, false, true);

	globalCol.clear();
	LocationPart& locPart = *unit.locPart;
	Level::IgnoreObjects ignore = { 0 };
	const Unit* ignoreUnits[2] = { &unit, nullptr };
	ignore.ignoredUnits = ignoreUnits;
	GatherCollisionObjects(locPart, globalCol, pos, 2.f + unit_radius, &ignore);

	Vec3 tmpPos = pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!Collide(globalCol, tmpPos, unit_radius))
		{
			unit.pos = tmpPos;
			ok = true;
			break;
		}

		tmpPos = pos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

		if(i < 10)
			radius += 0.25f;
	}

	assert(ok);

	if(locPart.haveTerrain && terrain->IsInside(unit.pos))
		terrain->SetY(unit.pos);

	if(unit.cobj)
		unit.UpdatePhysics();

	unit.visualPos = unit.pos;

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->playerInfo->warping = true;
	}
}

//=================================================================================================
bool Level::WarpToRegion(LocationPart& locPart, const Box2d& region, float radius, Vec3& pos, int tries)
{
	for(int i = 0; i < tries; ++i)
	{
		pos = region.GetRandomPos3();

		globalCol.clear();
		GatherCollisionObjects(locPart, globalCol, pos, radius, nullptr);

		if(!Collide(globalCol, pos, radius))
			return true;
	}

	return false;
}

//=================================================================================================
void Level::WarpNearLocation(LocationPart& locPart, Unit& unit, const Vec3& pos, float extraRadius, bool allowExact, int tries)
{
	const float radius = unit.GetUnitRadius();

	globalCol.clear();
	IgnoreObjects ignore = { 0 };
	const Unit* ignoreUnits[2] = { &unit, nullptr };
	ignore.ignoredUnits = ignoreUnits;
	GatherCollisionObjects(locPart, globalCol, pos, extraRadius + radius, &ignore);

	Vec3 tmpPos = pos;
	if(!allowExact)
		tmpPos += Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;

	for(int i = 0; i < tries; ++i)
	{
		if(!Collide(globalCol, tmpPos, radius))
			break;

		tmpPos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extraRadius;
	}

	unit.pos = tmpPos;
	unit.Moved(true);
	unit.visualPos = unit.pos;

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->playerInfo->warping = true;
	}

	if(unit.cobj)
		unit.UpdatePhysics();
}

//=================================================================================================
Trap* Level::TryCreateTrap(Int2 pt, TRAP_TYPE type)
{
	assert(lvl);

	BaseTrap& base = BaseTrap::traps[type];
	const Vec3 pos = Vec3(2.f * pt.x + Random(base.rw, 2.f - base.rw), 0.f, 2.f * pt.y + Random(base.h, 2.f - base.h));

	// check for collisions
	Level::IgnoreObjects ignore{};
	ignore.ignoreBlocks = true;
	globalCol.clear();
	const Box2d box(pos.x - base.rw / 2 - 0.1f, pos.z - base.h / 2 - 0.1f, pos.x + base.rw / 2 + 0.1f, pos.y + base.h / 2 + 0.1f);
	GatherCollisionObjects(*localPart, globalCol, box, &ignore);
	if(!globalCol.empty() && Collide(globalCol, box))
		return nullptr;

	// check if can shoot at this position
	Int2 shootTile;
	GameDirection shootDir;
	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		struct TrapLocation
		{
			Int2 pt;
			int dist;
			GameDirection dir;
		};

		static vector<TrapLocation> possible;

		for(int i = 0; i < 4; ++i)
		{
			GameDirection dir = (GameDirection)i;
			bool ok = false;
			int j;

			for(j = 1; j <= 10; ++j)
			{
				if(IsBlocking(lvl->map[pt.x + DirToPos(dir).x * j + (pt.y + DirToPos(dir).y * j) * lvl->w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				const Int2 tile = pt + DirToPos(dir) * j;
				if(CanShootAtLocation(Vec3(pos.x + (2.f * j - 1.2f) * DirToPos(dir).x, 1.f, pos.z + (2.f * j - 1.2f) * DirToPos(dir).y),
					Vec3(pos.x, 1.f, pos.z)))
				{
					TrapLocation& tr = Add1(possible);
					tr.pt = tile;
					tr.dist = j;
					tr.dir = dir;
				}
			}
		}

		if(!possible.empty())
		{
			if(possible.size() > 1)
			{
				std::sort(possible.begin(), possible.end(), [](TrapLocation& pt1, TrapLocation& pt2)
				{
					return abs(pt1.dist - 5) < abs(pt2.dist - 5);
				});
			}

			shootTile = possible[0].pt;
			shootDir = possible[0].dir;

			possible.clear();
		}
		else
			return nullptr;
	}

	// create trap
	Trap* trap = new Trap;
	localPart->traps.push_back(trap);

	trap->base = &base;
	trap->meshInst = nullptr;
	trap->hitted = nullptr;
	trap->state = 0;
	trap->attack = 0;
	trap->pos = pos;
	switch(type)
	{
	case TRAP_ARROW:
	case TRAP_POISON:
		trap->rot = 0;
		trap->tile = shootTile;
		trap->dir = shootDir;
		break;
	case TRAP_SPEAR:
		trap->rot = Random(MAX_ANGLE);
		trap->hitted = new vector<Unit*>;
		break;
	case TRAP_FIREBALL:
		trap->rot = PI / 2 * (Rand() % 4);
		break;
	case TRAP_BEAR:
		trap->rot = Random(MAX_ANGLE);
		break;
	}

	trap->Register();
	return trap;
}

//=================================================================================================
Trap* Level::CreateTrap(const Vec3& pos, TRAP_TYPE type, int id)
{
	Trap* trap = new Trap;
	GetLocationPart(pos).traps.push_back(trap);

	BaseTrap& base = BaseTrap::traps[type];
	trap->id = id;
	trap->Register();
	trap->base = &base;
	trap->meshInst = nullptr;
	trap->hitted = nullptr;
	trap->state = 0;
	trap->attack = 0;
	trap->pos = pos;
	trap->mpTrigger = false;

	switch(type)
	{
	case TRAP_ARROW:
	case TRAP_POISON:
		assert(0); // TODO
		break;
	case TRAP_SPEAR:
		trap->rot = Random(MAX_ANGLE);
		trap->hitted = new vector<Unit*>;
		break;
	case TRAP_FIREBALL:
		trap->rot = PI / 2 * (Rand() % 4);
		break;
	case TRAP_BEAR:
		trap->rot = Random(MAX_ANGLE);
		break;
	}

	gameRes->LoadTrap(trap->base);
	if(trap->base->mesh->IsAnimated())
		trap->meshInst = new MeshInstance(trap->base->mesh);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CREATE_TRAP;
		c.extraId = trap->id;
		c.id = type;
		c.pos = pos;
	}

	return trap;
}

//=================================================================================================
void Level::UpdateLocation(int days, int openChance, bool reset)
{
	for(LocationPart& locPart : locParts)
	{
		// if reset remove all units
		// otherwise all (>10 days) or some
		if(reset)
		{
			for(Unit* unit : locPart.units)
			{
				if(unit->IsAlive() && unit->IsHero() && unit->hero->otherTeam)
					unit->hero->otherTeam->Remove();
				delete unit;
			}
			locPart.units.clear();
		}
		else if(days > 10)
			DeleteElements(locPart.units, [](Unit* unit) { return !unit->IsAlive(); });
		else
			DeleteElements(locPart.units, [=](Unit* unit) { return !unit->IsAlive() && Random(4, 10) < days; });

		// remove all ground items (>10 days) or some
		if(days > 10)
			DeleteElements(locPart.GetGroundItems());
		else
			DeleteElements(locPart.GetGroundItems(), RemoveRandomPred<GroundItem*>(days, 0, 10));

		// heal units
		if(!reset)
		{
			for(Unit* unit : locPart.units)
			{
				if(unit->IsAlive())
					unit->NaturalHealing(days);
			}
		}

		// remove all blood (>30 days) or some (>5 days)
		if(days > 30)
			locPart.bloods.clear();
		else if(days > 5)
			RemoveElements(locPart.bloods, RemoveRandomPred<Blood>(days, 4, 30));

		// remove all temporary traps (>30 days) or some (>5 days)
		if(days > 30)
		{
			for(vector<Trap*>::iterator it = locPart.traps.begin(), end = locPart.traps.end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL)
				{
					delete *it;
					if(it + 1 == end)
					{
						locPart.traps.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						locPart.traps.pop_back();
						end = locPart.traps.end();
					}
				}
				else
					++it;
			}
		}
		else if(days >= 5)
		{
			for(vector<Trap*>::iterator it = locPart.traps.begin(), end = locPart.traps.end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL && Rand() % 30 < days)
				{
					delete *it;
					if(it + 1 == end)
					{
						locPart.traps.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						locPart.traps.pop_back();
						end = locPart.traps.end();
					}
				}
				else
					++it;
			}
		}

		// randomly open/close doors
		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.locked == 0)
			{
				if(Rand() % 100 < openChance)
					door.state = Door::Opened;
				else
					door.state = Door::Closed;
			}
		}
	}
}

//=================================================================================================
int Level::GetDifficultyLevel() const
{
	if(location->outside)
		return location->st;
	else
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		if(inside->IsMultilevel())
		{
			float max_st = (float)inside->st;
			float min_st = max(3.f, max_st * 2 / 3);
			uint levels = static_cast<MultiInsideLocation*>(inside)->levels.size() - 1;
			return (int)Lerp(min_st, max_st, float(dungeonLevel) / levels);
		}
		else
			return inside->st;
	}
}

//=================================================================================================
int Level::GetChestDifficultyLevel() const
{
	int st = GetDifficultyLevel();
	if(location->group->IsEmpty())
	{
		if(st > 10)
			return 3;
		else if(st > 5)
			return 2;
		else
			return 1;
	}
	else
		return st;
}

//=================================================================================================
void Level::OnRevisitLevel()
{
	for(LocationPart& locPart : locParts)
	{
		// recreate doors
		for(Door* door : locPart.doors)
			door->Recreate();
	}
}

//=================================================================================================
bool Level::HaveArena()
{
	if(cityCtx)
		return IsSet(cityCtx->flags, City::HaveArena);
	return false;
}

//=================================================================================================
InsideBuilding* Level::GetArena()
{
	assert(cityCtx);
	for(InsideBuilding* b : cityCtx->insideBuildings)
	{
		if(b->building->group == BuildingGroup::BG_ARENA)
			return b;
	}
	assert(0);
	return nullptr;
}

//=================================================================================================
cstring Level::GetCurrentLocationText()
{
	if(isOpen)
	{
		if(location->outside)
			return location->name.c_str();
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
				return Format(txLocationText, location->name.c_str(), dungeonLevel + 1);
			else
				return location->name.c_str();
		}
	}
	else if(location)
		return Format(txLocationTextMap, location->name.c_str());
	else
		return txWorldMap;
}

//=================================================================================================
void Level::CheckIfLocationCleared()
{
	if(location->state == LS_CLEARED)
		return;

	Unit& playerUnit = *game->pc->unit;

	// is current level cleared?
	bool isClear = true;
	for(Unit* unit : localPart->units)
	{
		if(unit->IsAlive() && playerUnit.IsEnemy(*unit, true))
		{
			isClear = false;
			break;
		}
	}

	// is all levels cleared?
	if(isClear)
	{
		if(cityCtx)
		{
			for(InsideBuilding* insideBuilding : cityCtx->insideBuildings)
			{
				for(Unit* unit : insideBuilding->units)
				{
					if(unit->IsAlive() && playerUnit.IsEnemy(*unit, true))
					{
						isClear = false;
						goto superbreak;
					}
				}
			}
		superbreak:;
		}
		else if(!location->outside)
		{
			InsideLocation* inside = static_cast<InsideLocation*>(location);
			if(inside->IsMultilevel())
				isClear = static_cast<MultiInsideLocation*>(inside)->LevelCleared();
		}
	}

	if(isClear)
	{
		if(location->state != LS_HIDDEN)
			location->state = LS_CLEARED;

		// events v1
		bool prevent = false;
		if(eventHandler)
			prevent = eventHandler->HandleLocationEvent(LocationEventHandler::CLEARED);

		// events v2
		for(Event& e : location->events)
		{
			if(e.type == EVENT_CLEARED)
			{
				ScriptEvent event(EVENT_CLEARED);
				event.location = location;
				e.quest->FireEvent(event);
			}
		}

		// remove camp in 4-8 days
		if(location->type == L_CAMP)
			static_cast<Camp*>(location)->createTime = world->GetWorldtime() - 30 + Random(4, 8);

		// add news
		if(!prevent && !location->group->IsEmpty())
		{
			if(location->type == L_CAMP)
				world->AddNews(Format(txNewsCampCleared, world->GetNearestSettlement(location->pos)->name.c_str()));
			else
				world->AddNews(Format(txNewsLocCleared, location->name.c_str()));
		}
	}
}

//=================================================================================================
// usuwa podany przedmiot ze œwiata
// u¿ywane w queœcie z kamieniem szaleñców
bool Level::RemoveItemFromWorld(const Item* item)
{
	assert(item);
	for(LocationPart& locPart : locParts)
	{
		if(locPart.RemoveItem(item))
			return true;
	}
	return false;
}

//=================================================================================================
void Level::SpawnTerrainCollider()
{
	if(terrainShape)
		delete terrainShape;

	terrainShape = new btHeightfieldTerrainShape(OutsideLocation::size + 1, OutsideLocation::size + 1, terrain->GetHeightMap(), 1.f, 0.f, 10.f, 1, PHY_FLOAT, false);
	terrainShape->setLocalScaling(btVector3(2.f, 1.f, 2.f));

	objTerrain = new btCollisionObject;
	objTerrain->setCollisionShape(terrainShape);
	objTerrain->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_TERRAIN);
	objTerrain->getWorldTransform().setOrigin(btVector3(float(OutsideLocation::size), 5.f, float(OutsideLocation::size)));
	phyWorld->addCollisionObject(objTerrain, CG_TERRAIN);
}

//=================================================================================================
void Level::SpawnDungeonColliders()
{
	assert(lvl);

	InsideLocation* inside = static_cast<InsideLocation*>(location);
	Tile* m = lvl->map;
	int w = lvl->w,
		h = lvl->h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(IsBlocking(m[x + y * w]) && (!IsBlocking(m[x - 1 + (y - 1) * w]) || !IsBlocking(m[x + (y - 1) * w]) || !IsBlocking(m[x + 1 + (y - 1) * w])
				|| !IsBlocking(m[x - 1 + y * w]) || !IsBlocking(m[x + 1 + y * w])
				|| !IsBlocking(m[x - 1 + (y + 1) * w]) || !IsBlocking(m[x + (y + 1) * w]) || !IsBlocking(m[x + 1 + (y + 1) * w])))
			{
				SpawnDungeonCollider(Vec3(2.f * x + 1.f, 2.f, 2.f * y + 1.f));
			}
		}
	}

	// left/right wall
	for(int i = 1; i < h - 1; ++i)
	{
		// left
		if(IsBlocking(m[i * w]) && !IsBlocking(m[1 + i * w]))
			SpawnDungeonCollider(Vec3(1.f, 2.f, 2.f * i + 1.f));

		// right
		if(IsBlocking(m[i * w + w - 1]) && !IsBlocking(m[w - 2 + i * w]))
			SpawnDungeonCollider(Vec3(2.f * (w - 1) + 1.f, 2.f, 2.f * i + 1.f));
	}

	// front/back wall
	for(int i = 1; i < lvl->w - 1; ++i)
	{
		// front
		if(IsBlocking(m[i + (h - 1) * w]) && !IsBlocking(m[i + (h - 2) * w]))
			SpawnDungeonCollider(Vec3(2.f * i + 1.f, 2.f, 2.f * (h - 1) + 1.f));

		// back
		if(IsBlocking(m[i]) && !IsBlocking(m[i + w]))
			SpawnDungeonCollider(Vec3(2.f * i + 1.f, 2.f, 1.f));
	}

	// up stairs
	if(inside->HavePrevEntry())
	{
		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(shapeStairs);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
		cobj->getWorldTransform().setOrigin(btVector3(2.f * lvl->prevEntryPt.x + 1.f, 0.f, 2.f * lvl->prevEntryPt.y + 1.f));
		cobj->getWorldTransform().setRotation(btQuaternion(DirToRot(lvl->prevEntryDir), 0, 0));
		phyWorld->addCollisionObject(cobj, CG_BUILDING);
	}

	// room floors/ceilings
	dungeonMesh->Clear();
	int index = 0;

	if((inside->type == L_DUNGEON && inside->target == LABYRINTH) || inside->type == L_CAVE)
	{
		const float h = Room::HEIGHT;
		for(int x = 0; x < 16; ++x)
		{
			for(int y = 0; y < 16; ++y)
			{
				// floor
				dungeonMesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, 0, 2.f * y * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, 0, 2.f * y * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, 0, 2.f * (y + 1) * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, 0, 2.f * (y + 1) * lvl->h / 16));
				dungeonMesh->indices.push_back(index);
				dungeonMesh->indices.push_back(index + 1);
				dungeonMesh->indices.push_back(index + 2);
				dungeonMesh->indices.push_back(index + 2);
				dungeonMesh->indices.push_back(index + 1);
				dungeonMesh->indices.push_back(index + 3);
				index += 4;

				// ceil
				dungeonMesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, h, 2.f * y * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, h, 2.f * y * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, h, 2.f * (y + 1) * lvl->h / 16));
				dungeonMesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, h, 2.f * (y + 1) * lvl->h / 16));
				dungeonMesh->indices.push_back(index);
				dungeonMesh->indices.push_back(index + 2);
				dungeonMesh->indices.push_back(index + 1);
				dungeonMesh->indices.push_back(index + 2);
				dungeonMesh->indices.push_back(index + 3);
				dungeonMesh->indices.push_back(index + 1);
				index += 4;
			}
		}
	}

	for(Room* room : lvl->rooms)
	{
		// floor
		dungeonMesh->vertices.push_back(Vec3(2.f * room->pos.x, 0, 2.f * room->pos.y));
		dungeonMesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * room->pos.y));
		dungeonMesh->vertices.push_back(Vec3(2.f * room->pos.x, 0, 2.f * (room->pos.y + room->size.y)));
		dungeonMesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * (room->pos.y + room->size.y)));
		dungeonMesh->indices.push_back(index);
		dungeonMesh->indices.push_back(index + 1);
		dungeonMesh->indices.push_back(index + 2);
		dungeonMesh->indices.push_back(index + 2);
		dungeonMesh->indices.push_back(index + 1);
		dungeonMesh->indices.push_back(index + 3);
		index += 4;

		// ceil
		const float h = (room->IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);
		dungeonMesh->vertices.push_back(Vec3(2.f * room->pos.x, h, 2.f * room->pos.y));
		dungeonMesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * room->pos.y));
		dungeonMesh->vertices.push_back(Vec3(2.f * room->pos.x, h, 2.f * (room->pos.y + room->size.y)));
		dungeonMesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * (room->pos.y + room->size.y)));
		dungeonMesh->indices.push_back(index);
		dungeonMesh->indices.push_back(index + 2);
		dungeonMesh->indices.push_back(index + 1);
		dungeonMesh->indices.push_back(index + 2);
		dungeonMesh->indices.push_back(index + 3);
		dungeonMesh->indices.push_back(index + 1);
		index += 4;
	}

	delete dungeonShape;
	delete dungeonShapeData;

	btIndexedMesh mesh;
	mesh.m_numTriangles = dungeonMesh->indices.size() / 3;
	mesh.m_triangleIndexBase = (byte*)dungeonMesh->indices.data();
	mesh.m_triangleIndexStride = sizeof(word) * 3;
	mesh.m_numVertices = dungeonMesh->vertices.size();
	mesh.m_vertexBase = (byte*)dungeonMesh->vertices.data();
	mesh.m_vertexStride = sizeof(Vec3);

	dungeonShapeData = new btTriangleIndexVertexArray();
	dungeonShapeData->addIndexedMesh(mesh, PHY_SHORT);
	dungeonShape = new btBvhTriangleMeshShape(dungeonShapeData, true);
	dungeonShape->setUserPointer(dungeonMesh);

	objDungeon = new btCollisionObject;
	objDungeon->setCollisionShape(dungeonShape);
	objDungeon->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	phyWorld->addCollisionObject(objDungeon, CG_BUILDING);
}

//=================================================================================================
void Level::SpawnDungeonCollider(const Vec3& pos)
{
	auto cobj = new btCollisionObject;
	cobj->setCollisionShape(shapeWall);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	cobj->getWorldTransform().setOrigin(ToVector3(pos));
	phyWorld->addCollisionObject(cobj, CG_BUILDING);
}

//=================================================================================================
void Level::RemoveColliders()
{
	physics->Reset();

	DeleteElements(shapes);
	camColliders.clear();
}

//=================================================================================================
Int2 Level::GetSpawnPoint()
{
	InsideLocation* inside = static_cast<InsideLocation*>(location);
	if(enterFrom >= ENTER_FROM_PORTAL)
		return PosToPt(inside->GetPortal(enterFrom)->GetSpawnPos());
	else if(enterFrom == ENTER_FROM_NEXT_LEVEL)
		return lvl->GetNextEntryFrontTile();
	else
		return lvl->GetPrevEntryFrontTile();
}

//=================================================================================================
// Get nearest pos for unit to exit level
Vec3 Level::GetExitPos(Unit& u, bool forceBorder)
{
	const Vec3& pos = u.pos;

	if(location->outside)
	{
		if(u.locPart->partType == LocationPart::Type::Building)
			return static_cast<InsideBuilding*>(u.locPart)->exitRegion.Midpoint().XZ();
		else if(cityCtx && !forceBorder)
		{
			float best_dist, dist;
			int best_index = -1, index = 0;

			for(vector<EntryPoint>::const_iterator it = cityCtx->entryPoints.begin(), end = cityCtx->entryPoints.end(); it != end; ++it, ++index)
			{
				if(it->exitRegion.IsInside(u.pos))
				{
					// unit is already inside exitable location part, go to outside exit
					best_index = -1;
					break;
				}
				else
				{
					dist = Vec2::Distance(Vec2(u.pos.x, u.pos.z), it->exitRegion.Midpoint());
					if(best_index == -1 || dist < best_dist)
					{
						best_dist = dist;
						best_index = index;
					}
				}
			}

			if(best_index != -1)
				return cityCtx->entryPoints[best_index].exitRegion.Midpoint().XZ();
		}

		int best = 0;
		float dist, best_dist;

		// lewa krawêdŸ
		best_dist = abs(pos.x - 32.f);

		// prawa krawêdŸ
		dist = abs(pos.x - (256.f - 32.f));
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 1;
		}

		// górna krawêdŸ
		dist = abs(pos.z - 32.f);
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 2;
		}

		// dolna krawêdŸ
		dist = abs(pos.z - (256.f - 32.f));
		if(dist < best_dist)
			best = 3;

		switch(best)
		{
		default:
			assert(0);
		case 0:
			return Vec3(32.f, 0, pos.z);
		case 1:
			return Vec3(256.f - 32.f, 0, pos.z);
		case 2:
			return Vec3(pos.x, 0, 32.f);
		case 3:
			return Vec3(pos.x, 0, 256.f - 32.f);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(dungeonLevel == 0 && inside->fromPortal)
			return inside->portal->pos;
		const Int2& pt = inside->GetLevelData().prevEntryPt;
		return PtToPos(pt);
	}
}

//=================================================================================================
bool Level::CanSee(Unit& u1, Unit& u2)
{
	if(u1.locPart != u2.locPart)
		return false;

	LocationPart& locPart = *u1.locPart;
	Int2 tile1(int(u1.pos.x / 2), int(u1.pos.z / 2)),
		tile2(int(u2.pos.x / 2), int(u2.pos.z / 2));

	if(locPart.partType == LocationPart::Type::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].IsBlocking()
					&& LineToRectangle(u1.pos, u2.pos, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
			}
		}
	}
	else if(locPart.partType == LocationPart::Type::Inside)
	{
		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl->w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl->h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl->map[x + y * lvl->w].type) && LineToRectangle(u1.pos, u2.pos, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
				if(lvl->map[x + y * lvl->w].type == DOORS)
				{
					Door* door = locPart.FindDoor(Int2(x, y));
					if(door && door->IsBlocking())
					{
						Box2d box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= Door::THICKNESS;
							box.v2.y += Door::THICKNESS;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
		{
			if(!it->camCollider || it->type != CollisionObject::RECTANGLE)
				continue;

			Box2d box(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box2d box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= Door::THICKNESS;
					box.v2.y += Door::THICKNESS;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

//=================================================================================================
bool Level::CanSee(LocationPart& locPart, const Vec3& v1, const Vec3& v2, bool isDoor, void* ignore)
{
	if(v1.XZ().Equal(v2.XZ()))
		return true;

	Int2 tile1(int(v1.x / 2), int(v1.z / 2)),
		tile2(int(v2.x / 2), int(v2.z / 2));

	if(locPart.partType == LocationPart::Type::Outside)
	{
		OutsideLocation* outside = static_cast<OutsideLocation*>(location);

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].IsBlocking() && LineToRectangle(v1, v2, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
			}
		}
	}
	else if(locPart.partType == LocationPart::Type::Inside)
	{
		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl->w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl->h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl->map[x + y * lvl->w].type) && LineToRectangle(v1, v2, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
				if(lvl->map[x + y * lvl->w].type == DOORS)
				{
					Int2 pt(x, y);
					Door* door = locPart.FindDoor(pt);
					if(door && door->IsBlocking()
						&& (!isDoor || tile2 != pt)) // ignore target door
					{
						Box2d box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= Door::THICKNESS;
							box.v2.y += Door::THICKNESS;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(LineToRectangle(v1, v2, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
		{
			if(!it->camCollider || it->type != CollisionObject::RECTANGLE || (ignore && ignore == it->owner))
				continue;

			Box2d box(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h);
			if(LineToRectangle(v1, v2, box.v1, box.v2))
				return false;
		}

		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking()
				&& (!isDoor || !v2.Equal(door.pos))) // ignore target door
			{
				Box2d box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= Door::THICKNESS;
					box.v2.y += Door::THICKNESS;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(LineToRectangle(v1, v2, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

//=================================================================================================
void Level::KillAll(bool friendly, Unit& unit, Unit* ignore)
{
	if(friendly)
	{
		// kill all except player/ignore
		for(LocationPart& locPart : locParts)
		{
			for(Unit* u : locPart.units)
			{
				if(u->IsAlive() && !u->IsPlayer() && u != ignore)
					u->GiveDmg(u->hp);
			}
		}
	}
	else
	{
		// kill enemies
		for(LocationPart& locPart : locParts)
		{
			for(Unit* u : locPart.units)
			{
				if(u->IsAlive() && u->IsEnemy(unit) && u != ignore)
					u->GiveDmg(u->hp);
			}
		}
	}
}

//=================================================================================================
void Level::AddPlayerTeam(const Vec3& pos, float rot)
{
	const bool hide_weapon = enterFrom == ENTER_FROM_OUTSIDE;

	for(Unit& unit : team->members)
	{
		localPart->units.push_back(&unit);
		unit.CreatePhysics();
		if(unit.IsHero())
			game->ais.push_back(unit.ai);

		unit.SetTakeHideWeaponAnimationToEnd(hide_weapon, false);
		unit.rot = rot;
		unit.animation = unit.currentAnimation = ANI_STAND;
		unit.meshInst->Play(NAMES::aniStand, PLAY_PRIO1, 0);
		unit.BreakAction();
		unit.SetAnimationAtEnd();
		unit.locPart = localPart;

		if(unit.IsAI())
		{
			unit.ai->state = AIController::Idle;
			unit.ai->st.idle.action = AIController::Idle_None;
			unit.ai->target = nullptr;
			unit.ai->alertTarget = nullptr;
			unit.ai->timer = Random(2.f, 5.f);
		}

		WarpNearLocation(*localPart, unit, pos, cityCtx ? 4.f : 2.f, true, 20);
		unit.visualPos = unit.pos;

		if(!location->outside)
			FOV::DungeonReveal(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)), minimapReveal);

		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
	}
}

//=================================================================================================
void Level::UpdateDungeonMinimap(bool inLevel)
{
	if(minimapOpenedDoors)
	{
		for(Unit& unit : team->activeMembers)
		{
			if(unit.IsPlayer())
				FOV::DungeonReveal(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)), minimapReveal);
		}
	}

	if(minimapReveal.empty())
		return;

	DynamicTexture& tex = *game->tMinimap;
	tex.Lock();
	for(vector<Int2>::iterator it = minimapReveal.begin(), end = minimapReveal.end(); it != end; ++it)
	{
		Tile& p = lvl->map[it->x + (lvl->w - it->y - 1) * lvl->w];
		SetBit(p.flags, Tile::F_REVEALED);
		uint* pix = tex[it->y] + it->x;
		if(Any(p.type, WALL, BLOCKADE_WALL))
			*pix = Color(100, 100, 100);
		else if(p.type == DOORS)
			*pix = Color(127, 51, 0);
		else
			*pix = Color(220, 220, 240);
	}
	tex.Unlock();

	if(Net::IsLocal())
	{
		if(inLevel)
		{
			if(Net::IsOnline())
				minimapRevealMp.insert(minimapRevealMp.end(), minimapReveal.begin(), minimapReveal.end());
		}
		else
			minimapRevealMp.clear();
	}

	minimapReveal.clear();
}

//=================================================================================================
void Level::RevealMinimap()
{
	if(location->outside)
		return;

	for(int y = 0; y < lvl->h; ++y)
	{
		for(int x = 0; x < lvl->w; ++x)
			minimapReveal.push_back(Int2(x, y));
	}

	UpdateDungeonMinimap(false);

	if(Net::IsServer())
		Net::PushChange(NetChange::CHEAT_REVEAL_MINIMAP);
}

//=================================================================================================
bool Level::IsCity()
{
	return location->type == L_CITY && static_cast<City*>(location)->IsCity();
}

//=================================================================================================
bool Level::IsVillage()
{
	return location->type == L_CITY && static_cast<City*>(location)->IsVillage();
}

//=================================================================================================
bool Level::IsTutorial()
{
	return location->type == L_DUNGEON && location->target == TUTORIAL_FORT;
}

//=================================================================================================
bool Level::IsOutside()
{
	return location->outside;
}

//=================================================================================================
void Level::Update()
{
	for(LocationPart& locPart : locParts)
	{
		for(Unit* unit : locPart.units)
		{
			if(unit->data->trader)
				unit->RefreshStock();
		}
	}
}

//=================================================================================================
void Level::Write(BitStreamWriter& f)
{
	location->Write(f);
	f.WriteCasted<byte>(GetLocationMusic());
	f << boss;

	if(net->mpLoad)
	{
		for(LocationPart& locPart : locParts)
			locPart.lvlPart->Write(f);
	}
}

//=================================================================================================
bool Level::Read(BitStreamReader& f, bool loadedResources)
{
	// location
	if(!location->Read(f))
	{
		Error("Read level: Failed to read location.");
		return false;
	}
	isOpen = true;
	Apply();
	game->locGenFactory->Get(location)->OnLoad();
	location->RequireLoadingResources(&loadedResources);

	// apply usable users
	Usable::ApplyRequests();

	// music
	MusicType music;
	f.ReadCasted<byte>(music);
	if(!f)
	{
		Error("Read level: Broken music.");
		return false;
	}
	gameRes->LoadMusic(music, false, true);
	f >> boss;
	if(boss)
	{
		gameRes->LoadMusic(MusicType::Boss, false, true);
		game->SetMusic(MusicType::Boss);
	}
	else
		game->SetMusic(music);

	if(net->mpLoad)
	{
		for(LocationPart& locPart : locParts)
		{
			if(!locPart.lvlPart->Read(f))
				return false;
		}
	}

	return true;
}

//=================================================================================================
MusicType Level::GetLocationMusic()
{
	switch(location->type)
	{
	case L_CITY:
		if(location->target == VILLAGE_EMPTY)
			return MusicType::EmptyCity;
		else
			return MusicType::City;
	case L_DUNGEON:
	case L_CAVE:
		if(Any(location->target, HERO_CRYPT, MONSTER_CRYPT))
			return MusicType::Crypt;
		else
			return MusicType::Dungeon;
	case L_OUTSIDE:
		if(locationIndex == questMgr->questSecret->where2 || Any(location->target, MOONWELL, ACADEMY))
			return MusicType::Moonwell;
		else
			return MusicType::Forest;
	case L_CAMP:
		return MusicType::Forest;
	case L_ENCOUNTER:
		return MusicType::Travel;
	default:
		assert(0);
		return MusicType::Dungeon;
	}
}

//=================================================================================================
// -1 - outside
// -2 - all
void Level::CleanLevel(int buildingId)
{
	for(LocationPart& locPart : locParts)
	{
		if((locPart.partType != LocationPart::Type::Building && (buildingId == -2 || buildingId == -1))
			|| (locPart.partType == LocationPart::Type::Building && (buildingId == -2 || buildingId == locPart.partId)))
		{
			locPart.bloods.clear();
			for(Unit* unit : locPart.units)
			{
				if(!unit->IsAlive() && !unit->IsTeamMember() && !unit->toRemove)
					RemoveUnit(unit, false);
			}
		}
	}

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CLEAN_LEVEL;
		c.id = buildingId;
	}
}

//=================================================================================================
GroundItem* Level::SpawnItem(const Item* item, const Vec3& pos)
{
	GroundItem* groundItem = new GroundItem;
	groundItem->Register();
	groundItem->count = 1;
	groundItem->teamCount = 1;
	groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
	groundItem->pos = pos;
	groundItem->item = item;
	localPart->AddGroundItem(groundItem);
	return groundItem;
}

//=================================================================================================
GroundItem* Level::SpawnItemNearLocation(LocationPart& locPart, const Item* item)
{
	assert(locPart.partType == LocationPart::Type::Building); // not implemented

	InsideBuilding& building = static_cast<InsideBuilding&>(locPart);
	Vec3 pos;
	if(!WarpToRegion(locPart, building.region1, 0.3f, pos))
		return nullptr;

	GroundItem* groundItem = new GroundItem;
	groundItem->Register();
	groundItem->count = 1;
	groundItem->teamCount = 1;
	groundItem->rot = Quat::RotY(Random(MAX_ANGLE));
	groundItem->pos = pos;
	groundItem->item = item;
	locPart.AddGroundItem(groundItem);
	return groundItem;
}

//=================================================================================================
GroundItem* Level::SpawnItemAtObject(const Item* item, Object* obj)
{
	assert(item && obj);
	Mesh* mesh = obj->base->mesh;
	GroundItem* groundItem = SpawnItem(item, obj->pos);
	for(vector<Mesh::Point>::iterator it = mesh->attachPoints.begin(), end = mesh->attachPoints.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			Matrix rot = Matrix::RotationY(obj->rot.y + PI);
			Vec3 offset = Vec3::TransformZero(rot * it->mat);
			groundItem->pos += offset;
			groundItem->rot = Quat::CreateFromRotationMatrix(it->mat) * Quat::RotY(obj->rot.y);
			break;
		}
	}
	return groundItem;
}

//=================================================================================================
void Level::SpawnItemRandomly(const Item* item, uint count)
{
	assert(location->outside);
	for(uint i = 0; i < count; ++i)
	{
		const float s = (float)OutsideLocation::size;
		Vec2 pos = Vec2(s, s) + Vec2::RandomPoissonDiscPoint() * (s - 30.f);
		SpawnGroundItemInsideRadius(item, pos, 3.f, true);
	}
}

//=================================================================================================
Unit* Level::GetNearestEnemy(Unit* unit)
{
	Unit* best = nullptr;
	float best_dist;
	for(Unit* u : unit->locPart->units)
	{
		if(u->IsAlive() && unit->IsEnemy(*u))
		{
			float dist = Vec3::DistanceSquared(unit->pos, u->pos);
			if(!best || dist < best_dist)
			{
				best = u;
				best_dist = dist;
			}
		}
	}
	return best;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocationS(UnitData* ud, const Vec3& pos, float range, int level)
{
	return SpawnUnitNearLocation(GetLocationPart(pos), pos, *ud, nullptr, level, range);
}

//=================================================================================================
GroundItem* Level::FindNearestItem(const Item* item, const Vec3& pos)
{
	assert(item);

	LocationPart& locPart = GetLocationPart(pos);
	float best_dist = 999.f;
	GroundItem* best_item = nullptr;
	for(GroundItem* groundItem : locPart.GetGroundItems())
	{
		if(groundItem->item == item)
		{
			float dist = Vec3::Distance(pos, groundItem->pos);
			if(dist < best_dist || !best_item)
			{
				best_dist = dist;
				best_item = groundItem;
			}
		}
	}
	return best_item;
}

//=================================================================================================
GroundItem* Level::FindItem(const Item* item)
{
	assert(item);

	for(LocationPart& locPart : locParts)
	{
		for(GroundItem* groundItem : locPart.GetGroundItems())
		{
			if(groundItem->item == item)
				return groundItem;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::GetMayor()
{
	if(!cityCtx)
		return nullptr;
	cstring id;
	if(cityCtx->target == VILLAGE)
		id = "soltys";
	else
		id = "mayor";
	return FindUnit(UnitData::Get(id));
}

//=================================================================================================
bool Level::IsSafe()
{
	if(cityCtx)
		return true;
	else if(location->outside)
		return location->state == LS_CLEARED || location->type == L_ENCOUNTER;
	else
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
			if(multi->IsLevelClear())
			{
				if(dungeonLevel == 0)
				{
					if(!multi->fromPortal)
						return true;
				}
				else
					return true;
			}
			return false;
		}
		else
			return (location->state == LS_CLEARED);
	}
}

//=================================================================================================
bool Level::CanFastTravel()
{
	if(Net::IsLocal())
	{
		if(!location->outside
			|| !IsSafe()
			|| game->arena->mode != Arena::NONE
			|| questMgr->questTutorial->inTutorial
			|| questMgr->questContest->state >= Quest_Contest::CONTEST_STARTING
			|| questMgr->questTournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			return false;

		CanLeaveLocationResult result = CanLeaveLocation(*team->leader, false);
		return result == CanLeaveLocationResult::Yes;
	}
	else
		return canFastTravel;
}

//=================================================================================================
CanLeaveLocationResult Level::CanLeaveLocation(Unit& unit, bool checkDist)
{
	if(questMgr->questSecret->state == Quest_Secret::SECRET_FIGHT)
		return CanLeaveLocationResult::InCombat;

	if(cityCtx)
	{
		for(Unit& u : team->members)
		{
			if(u.summoner)
				continue;

			if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
				return CanLeaveLocationResult::TeamTooFar;

			if(u.IsPlayer() && checkDist)
			{
				if(u.locPart->partType == LocationPart::Type::Building || Vec3::Distance2d(unit.pos, u.pos) > 8.f)
					return CanLeaveLocationResult::TeamTooFar;
			}

			for(vector<Unit*>::iterator it2 = localPart->units.begin(), end2 = localPart->units.end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && unit.IsEnemy(u2) && u2.IsAI() && u2.ai->inCombat
					&& Vec3::Distance2d(unit.pos, u2.pos) < ALERT_RANGE && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}
	else
	{
		for(Unit& u : team->members)
		{
			if(u.summoner)
				continue;

			if(u.busy != Unit::Busy_No || (checkDist && Vec3::Distance2d(unit.pos, u.pos) > 8.f))
				return CanLeaveLocationResult::TeamTooFar;

			for(vector<Unit*>::iterator it2 = localPart->units.begin(), end2 = localPart->units.end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && u.IsEnemy(u2) && u2.IsAI() && u2.ai->inCombat
					&& Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}

	return CanLeaveLocationResult::Yes;
}

//=================================================================================================
bool Level::CanShootAtLocation(const Vec3& from, const Vec3& to) const
{
	RaytestAnyUnitCallback callback;
	phyWorld->rayTest(ToVector3(from), ToVector3(to), callback);
	return callback.clear;
}

//=================================================================================================
bool Level::CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const
{
	RaytestWithIgnoredCallback callback(&me, ptr);
	phyWorld->rayTest(btVector3(me.pos.x, me.pos.y + 1.f, me.pos.z), btVector3(to.x, to.y + 1.f, to.z), callback);
	return !callback.hit;
}

//=================================================================================================
bool Level::RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted)
{
	RaytestClosestUnitCallback callback([=](Unit* unit)
	{
		if(unit == ignore || !unit->IsAlive())
			return false;
		return true;
	});
	phyWorld->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hasHit())
	{
		hitpoint = from + (to - from) * callback.getFraction();
		hitted = callback.hit;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool Level::LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
	vector<float>* tList, bool useClbk2, float* endT)
{
	assert(shape->isConvex());

	btTransform t_from, t_to;
	t_from.setIdentity();
	t_from.setOrigin(ToVector3(from));
	t_to.setIdentity();
	t_to.setOrigin(ToVector3(dir) + t_from.getOrigin());

	ConvexCallback callback(clbk, tList, useClbk2);

	phyWorld->convexSweepTest((btConvexShape*)shape, t_from, t_to, callback);

	bool has_hit = (callback.closest <= 1.f);
	t = min(callback.closest, 1.f);
	if(endT)
	{
		if(callback.end)
			*endT = callback.endT;
		else
			*endT = 1.f;
	}
	return has_hit;
}

//=================================================================================================
bool Level::ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool useClbk2)
{
	ContactTestCallback callback(obj, clbk, useClbk2);
	phyWorld->contactTest(obj, callback);
	return callback.hit;
}

//=================================================================================================
int Level::CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* isSmall)
{
	assert(radius > 0.f && me);

	constexpr float SMALL_DISTANCE = 0.001f;
	Vec3 new_pos = pos + dir;
	Vec3 gather_pos = pos + dir / 2;
	float gather_radius = dir.Length() + radius;
	globalCol.clear();

	Level::IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { me, nullptr };
	ignore.ignoredUnits = (const Unit**)ignored;
	GatherCollisionObjects(*me->locPart, globalCol, gather_pos, gather_radius, &ignore);

	if(globalCol.empty())
	{
		if(isSmall)
			*isSmall = (Vec3::Distance(pos, new_pos) < SMALL_DISTANCE);
		pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!Collide(globalCol, new_pos, radius))
	{
		if(isSmall)
			*isSmall = (Vec3::Distance(pos, new_pos) < SMALL_DISTANCE);
		pos = new_pos;
		return 3;
	}

	// idŸ po x
	Vec3 new_pos2 = me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(globalCol, new_pos2, radius))
	{
		if(isSmall)
			*isSmall = (Vec3::Distance(pos, new_pos2) < SMALL_DISTANCE);
		pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(globalCol, new_pos2, radius))
	{
		if(isSmall)
			*isSmall = (Vec3::Distance(pos, new_pos2) < SMALL_DISTANCE);
		pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
}

//=================================================================================================
void Level::SpawnUnitEffect(Unit& unit)
{
	Vec3 real_pos = unit.pos;
	real_pos.y += 1.f;
	soundMgr->PlaySound3d(gameRes->sSummon, real_pos, SPAWN_SOUND_DIST);

	ParticleEmitter* pe = new ParticleEmitter;
	pe->tex = gameRes->tSpawn;
	pe->emissionInterval = 0.1f;
	pe->life = 5.f;
	pe->particleLife = 0.5f;
	pe->emissions = 5;
	pe->spawnMin = 10;
	pe->spawnMax = 15;
	pe->maxParticles = 15 * 5;
	pe->pos = unit.pos;
	pe->speedMin = Vec3(-1, 0, -1);
	pe->speedMax = Vec3(1, 1, 1);
	pe->posMin = Vec3(-0.75f, 0, -0.75f);
	pe->posMax = Vec3(0.75f, 1.f, 0.75f);
	pe->size = 0.3f;
	pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->alpha = 0.5f;
	pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->mode = 0;
	pe->Init();
	unit.locPart->lvlPart->pes.push_back(pe);
}

//=================================================================================================
MeshInstance* Level::GetBowInstance(Mesh* mesh)
{
	if(bowInstances.empty())
	{
		if(!mesh->IsLoaded())
			resMgr->LoadInstant(mesh);
		return new MeshInstance(mesh);
	}
	else
	{
		MeshInstance* instance = bowInstances.back();
		bowInstances.pop_back();
		instance->mesh = mesh;
		return instance;
	}
}

//=================================================================================================
CityBuilding* Level::GetRandomBuilding(BuildingGroup* group)
{
	assert(group);
	if(!cityCtx)
		return nullptr;
	LocalVector<CityBuilding*> available;
	for(CityBuilding& building : cityCtx->buildings)
	{
		if(building.building->group == group)
			available.push_back(&building);
	}
	if(available.empty())
		return nullptr;
	return available.RandomItem();
}

//=================================================================================================
Room* Level::GetRoom(RoomTarget target)
{
	if(location->outside)
		return nullptr;
	for(Room* room : lvl->rooms)
	{
		if(room->target == target)
			return room;
	}
	return nullptr;
}

//=================================================================================================
Room* Level::GetFarRoom()
{
	if(!lvl)
		return nullptr;
	InsideLocation* inside = static_cast<InsideLocation*>(location);
	return &lvl->GetFarRoom(inside->HaveNextEntry(), true);
}

//=================================================================================================
Object* Level::FindObjectInRoom(Room& room, BaseObject* base)
{
	for(Object* obj : localPart->objects)
	{
		if(obj->base == base && room.IsInside(obj->pos))
			return obj;
	}
	return nullptr;
}

//=================================================================================================
CScriptArray* Level::FindPath(Room& from, Room& to)
{
	assert(lvl);

	asITypeInfo* type = scriptMgr->GetEngine()->GetTypeInfoByDecl("array<Room@>");
	CScriptArray* array = CScriptArray::Create(type);

	Int2 from_pt = from.CenterTile();
	Int2 to_pt = to.CenterTile();

	vector<Int2> path;
	pathfinding->FindPath(*localPart, from_pt, to_pt, path);
	std::reverse(path.begin(), path.end());

	word prev_room_index = 0xFFFF;
	for(const Int2& pt : path)
	{
		word room_index = lvl->map[pt.x + pt.y * lvl->w].room;
		if(room_index != prev_room_index)
		{
			Room* room = lvl->rooms[room_index];
			array->InsertLast(&room);
			prev_room_index = room_index;
		}
	}

	return array;
}

//=================================================================================================
CScriptArray* Level::GetUnits(Room& room)
{
	assert(lvl);

	asITypeInfo* type = scriptMgr->GetEngine()->GetTypeInfoByDecl("array<Unit@>");
	CScriptArray* array = CScriptArray::Create(type);

	for(Unit* unit : localPart->units)
	{
		if(room.IsInside(unit->pos))
			array->InsertLast(&unit);
	}

	return array;
}

//=================================================================================================
bool Level::FindPlaceNearWall(BaseObject& obj, SpawnPoint& point)
{
	assert(lvl);

	Mesh::Point* pt = obj.mesh->FindPoint("hit");
	assert(pt);
	const Vec2 size = pt->size.XZ() * 2;

	const int width = 2 + (int)ceil(size.x / 2);
	const int width_a = (width - 1) / 2;
	const int width_b = width - width_a - 1;
	const int start_x = Random(width_a, lvl->h - width_b);
	const int start_y = Random(width_a, lvl->h - width_b);
	int x = start_x, y = start_y;

	while(true)
	{
		// __#
		// _?#
		// __#
		bool ok = true;
		for(int y1 = -width_a; y1 <= width_b; ++y1)
		{
			if(IsBlocking2(lvl->map[x - 1 + (y + y1) * lvl->w])
				|| IsBlocking2(lvl->map[x + (y + y1) * lvl->w])
				|| lvl->map[x + 1 + (y + y1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x + 1), 0, 2.f * (y - width_a) + (float)width);
			point.rot = DirToRot(GDIR_LEFT);
			return true;
		}

		// #__
		// #?_
		// #__
		ok = true;
		for(int y1 = -width_a; y1 <= width_b; ++y1)
		{
			if(IsBlocking2(lvl->map[x + 1 + (y + y1) * lvl->w])
				|| IsBlocking2(lvl->map[x + (y + y1) * lvl->w])
				|| lvl->map[x - 1 + (y + y1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * x, 0, 2.f * (y - width_a) + (float)width);
			point.rot = DirToRot(GDIR_RIGHT);
			return true;
		}

		// ###
		// _?_
		// ___
		ok = true;
		for(int x1 = -width_a; x1 <= width_b; ++x1)
		{
			if(IsBlocking2(lvl->map[x + x1 + (y - 1) * lvl->w])
				|| IsBlocking2(lvl->map[x + x1 + y * lvl->w])
				|| lvl->map[x + x1 + (y + 1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x - width_a) + (float)width, 0, 2.f * (y + 1));
			point.rot = DirToRot(GDIR_DOWN);
			return true;
		}

		// ___
		// _?_
		// ###
		ok = true;
		for(int x1 = -width_a; x1 <= width_b; ++x1)
		{
			if(IsBlocking2(lvl->map[x + x1 + (y + 1) * lvl->w])
				|| IsBlocking2(lvl->map[x + x1 + y * lvl->w])
				|| lvl->map[x + x1 + (y - 1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x - width_a) + (float)width, 0, 2.f * y);
			point.rot = DirToRot(GDIR_UP);
			return true;
		}

		++x;
		if(x == lvl->w - width_b)
		{
			x = width_a;
			++y;
			if(y == lvl->h - width_b)
				y = width_a;
		}
		if(x == start_x && y == start_y)
			return false;
	}
}

//=================================================================================================
void Level::CreateObjectsMeshInstance()
{
	const bool isLoading = (game->inLoad || net->mpLoad);
	for(LocationPart& locPart : locParts)
	{
		for(Object* obj : locPart.objects)
		{
			if(obj->mesh->IsAnimated())
			{
				float time = 0;
				if(isLoading)
					time = obj->time;
				obj->meshInst = new MeshInstance(obj->mesh);
				obj->meshInst->Play(&obj->mesh->anims[0], PLAY_NO_BLEND, 0);
				if(time != 0)
					obj->meshInst->groups[0].time = time;
			}
		}

		for(Chest* chest : locPart.chests)
			chest->Recreate();

		for(Trap* trap : locPart.traps)
		{
			if(trap->base->mesh->IsAnimated())
			{
				if(trap->meshInst)
					trap->meshInst->ApplyPreload(trap->base->mesh);
				else
					trap->meshInst = new MeshInstance(trap->base->mesh);
			}
		}
	}
}

//=================================================================================================
void Level::RemoveTmpObjectPhysics()
{
	for(LocationPart& locPart : locParts)
	{
		LoopAndRemove(locPart.lvlPart->colliders, [](const CollisionObject& cobj)
		{
			return cobj.owner == CollisionObject::TMP;
		});
	}
}

//=================================================================================================
void Level::RecreateTmpObjectPhysics()
{
	for(LocationPart& locPart : locParts)
	{
		for(Object* obj : locPart.objects)
		{
			if(!obj->base || !IsSet(obj->base->flags, OBJ_TMP_PHYSICS))
				continue;
			CollisionObject& c = Add1(locPart.lvlPart->colliders);
			c.owner = CollisionObject::TMP;
			c.camCollider = false;
			if(obj->base->type == OBJ_CYLINDER)
			{
				c.type = CollisionObject::SPHERE;
				c.pos = obj->pos;
				c.radius = obj->base->r;
			}
			else if(obj->base->type == OBJ_HITBOX)
			{
				const Matrix mat = *obj->base->matrix * Matrix::RotationY(obj->rot.y);
				Vec3 pos2 = Vec3::TransformZero(mat);
				pos2 += obj->pos;

				c.pos = pos2;
				c.w = obj->base->size.x;
				c.h = obj->base->size.y;
				if(NotZero(obj->rot.y))
				{
					c.type = CollisionObject::RECTANGLE_ROT;
					c.rot = obj->rot.y;
					c.radius = max(c.w, c.h) * SQRT_2;
				}
				else
					c.type = CollisionObject::RECTANGLE;
			}
		}
	}
}

//=================================================================================================
Vec3 Level::GetSpawnCenter()
{
	Vec3 pos(Vec3::Zero);
	int count = 0;
	for(Unit* unit : localPart->units)
	{
		pos += unit->pos;
		++count;
	}
	pos /= (float)count;
	return pos;
}

//=================================================================================================
void Level::StartBossFight(Unit& unit)
{
	boss = &unit;
	gameGui->levelGui->SetBoss(&unit, false);
	game->SetMusic(MusicType::Boss);
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::BOSS_START;
		c.unit = &unit;
	}
}

//=================================================================================================
void Level::EndBossFight()
{
	boss = nullptr;
	gameGui->levelGui->SetBoss(nullptr, false);
	game->SetMusic();
	if(Net::IsServer())
		Net::PushChange(NetChange::BOSS_END);
}

//=================================================================================================
void Level::CreateSpellParticleEffect(LocationPart* locPart, Ability* ability, const Vec3& pos, const Vec2& bounds)
{
	assert(ability);

	if(!locPart)
		locPart = &GetLocationPart(pos);

	ParticleEmitter* pe = new ParticleEmitter;
	pe->tex = ability->texParticle;
	pe->emissionInterval = 0.01f;
	pe->life = 0.f;
	pe->particleLife = 0.5f;
	pe->emissions = 1;
	pe->pos = pos;
	switch(ability->effect)
	{
	case Ability::Raise:
		pe->spawnMin = 16;
		pe->spawnMax = 25;
		pe->maxParticles = 25;
		pe->speedMin = Vec3(0, 4, 0);
		pe->speedMax = Vec3(0, 5, 0);
		pe->posMin = Vec3(-bounds.x, -bounds.y / 2, -bounds.x);
		pe->posMax = Vec3(bounds.x, bounds.y / 2, bounds.x);
		pe->size = ability->sizeParticle;
		pe->particleLife = 1.f;
		pe->alpha = 1.f;
		pe->mode = 0;
		break;
	case Ability::Heal:
		pe->spawnMin = 16;
		pe->spawnMax = 25;
		pe->maxParticles = 25;
		pe->speedMin = Vec3(-1.5f, -1.5f, -1.5f);
		pe->speedMax = Vec3(1.5f, 1.5f, 1.5f);
		pe->posMin = Vec3(-bounds.x, -bounds.y / 2, -bounds.x);
		pe->posMax = Vec3(bounds.x, bounds.y / 2, bounds.x);
		pe->size = ability->sizeParticle;
		pe->alpha = 0.9f;
		pe->mode = 1;
		break;
	default:
		pe->spawnMin = 12;
		pe->spawnMax = 12;
		pe->maxParticles = 12;
		pe->speedMin = Vec3(-0.5f, 1.5f, -0.5f);
		pe->speedMax = Vec3(0.5f, 3.0f, 0.5f);
		pe->posMin = Vec3(-0.5f, 0, -0.5f);
		pe->posMax = Vec3(0.5f, 0, 0.5f);
		pe->size = ability->sizeParticle / 2;
		pe->alpha = 1.f;
		pe->mode = 1;
		break;
	}
	pe->opSize = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->opAlpha = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->Init();
	locPart->lvlPart->pes.push_back(pe);

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PARTICLE_EFFECT;
		c.ability = ability;
		c.pos = pos;
		c.extraFloats[0] = bounds.x;
		c.extraFloats[1] = bounds.y;
	}
}
