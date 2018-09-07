#include "Pch.h"
#include "GameCore.h"
#include "Level.h"
#include "City.h"
#include "InsideBuilding.h"
#include "Game.h"

Level L;

void Level::Reset()
{
	unit_warp_data.clear();
	to_remove.clear();
}

void Level::ProcessUnitWarps()
{
	Game& game = Game::Get();
	bool warped_to_arena = false;

	for(auto& warp : unit_warp_data)
	{
		if(warp.where == WARP_OUTSIDE)
		{
			if(game.city_ctx && warp.unit->in_building != -1)
			{
				// exit from building
				InsideBuilding& building = *game.city_ctx->inside_buildings[warp.unit->in_building];
				RemoveElement(building.units, warp.unit);
				warp.unit->in_building = -1;
				warp.unit->rot = building.outside_rot;
				game.WarpUnit(*warp.unit, building.outside_spawn);
				game.local_ctx.units->push_back(warp.unit);
			}
			else
			{
				// unit left location
				if(warp.unit->event_handler)
					warp.unit->event_handler->HandleUnitEvent(UnitEventHandler::LEAVE, warp.unit);
				game.RemoveUnit(warp.unit);
			}
		}
		else if(warp.where == WARP_ARENA)
		{
			// warp to arena
			InsideBuilding& building = *game.GetArena();
			RemoveElement(game.GetContext(*warp.unit).units, warp.unit);
			warp.unit->in_building = building.ctx.building_id;
			Vec3 pos;
			if(!game.WarpToArea(building.ctx, (warp.unit->in_arena == 0 ? building.arena1 : building.arena2), warp.unit->GetUnitRadius(), pos, 20))
			{
				// failed to exit from arena
				warp.unit->in_building = -1;
				warp.unit->rot = building.outside_rot;
				game.WarpUnit(*warp.unit, building.outside_spawn);
				game.local_ctx.units->push_back(warp.unit);
				RemoveElement(game.at_arena, warp.unit);
			}
			else
			{
				warp.unit->rot = (warp.unit->in_arena == 0 ? PI : 0);
				game.WarpUnit(*warp.unit, pos);
				building.units.push_back(warp.unit);
				warped_to_arena = true;
			}
		}
		else
		{
			// enter building
			InsideBuilding& building = *game.city_ctx->inside_buildings[warp.where];
			if(warp.unit->in_building == -1)
				RemoveElement(game.local_ctx.units, warp.unit);
			else
				RemoveElement(game.city_ctx->inside_buildings[warp.unit->in_building]->units, warp.unit);
			warp.unit->in_building = warp.where;
			warp.unit->rot = PI;
			game.WarpUnit(*warp.unit, building.inside_spawn);
			building.units.push_back(warp.unit);
		}

		if(warp.unit == game.pc->unit)
		{
			game.cam.Reset();
			game.pc_data.rot_buf = 0.f;

			if(game.fallback_type == FALLBACK::ARENA)
			{
				game.pc->unit->frozen = FROZEN::ROTATE;
				game.fallback_type = FALLBACK::ARENA2;
			}
			else if(game.fallback_type == FALLBACK::ARENA_EXIT)
			{
				game.pc->unit->frozen = FROZEN::NO;
				game.fallback_type = FALLBACK::NONE;
			}
		}
	}

	unit_warp_data.clear();

	if(warped_to_arena)
	{
		Vec3 pt1(0, 0, 0), pt2(0, 0, 0);
		int count1 = 0, count2 = 0;

		for(vector<Unit*>::iterator it = game.at_arena.begin(), end = game.at_arena.end(); it != end; ++it)
		{
			if((*it)->in_arena == 0)
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
			InsideBuilding& building = *game.GetArena();
			pt1 = ((building.arena1.Midpoint() + building.arena2.Midpoint()) / 2).XZ();
		}
		if(count2 > 0)
			pt2 /= (float)count2;
		else
		{
			InsideBuilding& building = *game.GetArena();
			pt2 = ((building.arena1.Midpoint() + building.arena2.Midpoint()) / 2).XZ();
		}

		for(vector<Unit*>::iterator it = game.at_arena.begin(), end = game.at_arena.end(); it != end; ++it)
			(*it)->rot = Vec3::LookAtAngle((*it)->pos, (*it)->in_arena == 0 ? pt2 : pt1);
	}
}

void Level::ProcessRemoveUnits(bool clear)
{
	Game& game = Game::Get();
	if(clear)
	{
		for(Unit* unit : to_remove)
			game.DeleteUnit(unit);
	}
	else
	{
		for(Unit* unit : to_remove)
		{
			RemoveElement(game.GetContext(*unit).units, unit);
			delete unit;
		}
	}
	to_remove.clear();
}

void Level::SpawnOutsideBariers()
{
	Game& game = Game::Get();

	const float size = 256.f;
	const float size2 = size / 2;
	const float border = 32.f;
	const float border2 = border / 2;

	// top
	{
		CollisionObject& cobj = Add1(game.local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = Vec2(size2, border2);
		cobj.w = size2;
		cobj.h = border2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game.shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(size2, 40.f, border2));
		obj->setWorldTransform(tr);
		game.phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// bottom
	{
		CollisionObject& cobj = Add1(game.local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = Vec2(size2, size - border2);
		cobj.w = size2;
		cobj.h = border2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game.shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(size2, 40.f, size - border2));
		obj->setWorldTransform(tr);
		game.phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// left
	{
		CollisionObject& cobj = Add1(game.local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = Vec2(border2, size2);
		cobj.w = border2;
		cobj.h = size2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game.shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setOrigin(btVector3(border2, 40.f, size2));
		tr.setRotation(btQuaternion(PI / 2, 0, 0));
		obj->setWorldTransform(tr);
		game.phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// right
	{
		CollisionObject& cobj = Add1(game.local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = Vec2(size - border2, size2);
		cobj.w = border2;
		cobj.h = size2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game.shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setOrigin(btVector3(size - border2, 40.f, size2));
		tr.setRotation(btQuaternion(PI / 2, 0, 0));
		obj->setWorldTransform(tr);
		game.phy_world->addCollisionObject(obj, CG_BARRIER);
	}
}
