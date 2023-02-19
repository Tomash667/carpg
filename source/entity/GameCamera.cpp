#include "Pch.h"
#include "GameCamera.h"

#include <Engine.h> // GetWindowAspect
#include <Input.h>
#include <SoundManager.h>

#include "Game.h"
#include "GameKeys.h"
#include "GameResources.h"
#include "InsideBuilding.h"
#include "InsideLocation.h"
#include "InsideLocationLevel.h"
#include "Level.h"
#include "LevelPart.h"
#include "LocationPart.h"
#include "OutsideLocation.h"
#include "PhysicCallbacks.h"
#include "Unit.h"

const float SPRINGINESS_NORMAL = 40.f;
const float SPRINGINESS_SLOW = 5.f;
const float DIST_MIN = 0.5f;
const float DIST_DEF = 3.5f;
const float DIST_MAX = 6.f;

//=================================================================================================
GameCamera::GameCamera() : springiness(SPRINGINESS_NORMAL), reset(true), freeRot(false)
{
}

//=================================================================================================
void GameCamera::Reset(bool full)
{
	if(full)
	{
		realRot = Vec2(0, 4.2875104f);
		dist = DIST_DEF;
		drunkAnim = 0.f;
	}
	shift = 0.f;
	h = 0.2f;
	zoom = false;
	reset = true;
	freeRot = false;
	from = realFrom;
	to = realTo;
	tmpDist = dist;
	tmpShift = shift;
	springiness = SPRINGINESS_NORMAL;
	springinessTimer = 0.f;
}

//=================================================================================================
void GameCamera::Update(float dt)
{
	drunkAnim = Clip(drunkAnim + dt);

	if(!zoom && springinessTimer > 0.f)
	{
		springinessTimer -= dt;
		if(springinessTimer <= 0.f)
		{
			springiness += 5;
			if(springiness >= SPRINGINESS_NORMAL)
				springiness = SPRINGINESS_NORMAL;
			else
				springinessTimer = 0.5f;
		}
	}

	const float d = reset
		? 1.0f
		: 1.0f - exp(log(0.5f) * springiness * dt);

	if(!freeRot)
		realRot.x = target->rot;

	// update rotation, distance & shift
	if(reset)
	{
		rot = realRot;
		tmpDist = dist;
		tmpShift = shift;
		tmpH = h;
	}
	else
	{
		rot = Vec2::Slerp(rot, realRot, d).Clip();
		tmpDist += (dist - tmpDist) * d;
		tmpShift += (shift - tmpShift) * d;
		tmpH += (h - tmpH) * d;
	}

	// calculate new from & to, handle collisions
	const Vec3 forwardDir = Vec3::Transform(Vec3::UnitY, Matrix::Rotation(rot.y, rot.x, 0));
	const Vec3 backwardDir = Vec3::Transform(Vec3::UnitY, Matrix::Rotation(rot.y, rot.x + shift, 0));

	Vec3 pos = target->pos;
	pos.y += target->GetHeight() + tmpH;

	if(zoom)
		realTo = zoomPos;
	else
		realTo = pos + forwardDir * 20;

	// camera goes from character head backwards (to => from)
	float t = HandleCollisions(pos, -tmpDist * backwardDir);

	float realDist = tmpDist * t - 0.1f;
	if(realDist < 0.01f)
		realDist = 0.01f;
	realFrom = pos + backwardDir * -realDist;

	// update from & to
	if(reset)
	{
		from = realFrom;
		to = realTo;
		reset = false;
	}
	else
	{
		from += (realFrom - from) * d;
		to += (realTo - to) * d;
	}

	// calculate matrices, frustum
	const float drunk = target->alcohol / target->hpmax;
	const float drunkMod = (drunk > 0.1f ? (drunk - 0.1f) / 0.9f : 0.f);
	fov = PI / 4 + sin(drunkAnim) * (PI / 16) * drunkMod;
	aspect = engine->GetWindowAspect() * (1.f + sin(drunkAnim) / 10 * drunkMod);
	UpdateMatrix();
	frustum.Set(matViewProj);

	// 3d source listener
	soundMgr->SetListenerPosition(target->GetHeadSoundPos(), Vec3(sin(target->rot + PI), 0, cos(target->rot + PI)));
}

//=================================================================================================
void GameCamera::RotateTo(float dt, float destRot)
{
	freeRot = false;
	if(realRot.y > destRot)
	{
		realRot.y -= dt;
		if(realRot.y < destRot)
			realRot.y = destRot;
	}
	else if(realRot.y < destRot)
	{
		realRot.y += dt;
		if(realRot.y > destRot)
			realRot.y = destRot;
	}
}

//=================================================================================================
void GameCamera::UpdateFreeRot(float dt)
{
	if(!Any(GKey.allowInput, GameKeys::ALLOW_INPUT, GameKeys::ALLOW_MOUSE))
		return;

	const float angleMin = PI + 0.1f;
	const float angleMax = PI * 1.8f - 0.1f;
	const float sensitivity = game->settings.GetMouseSensitivity();

	int div = (target->action == A_SHOOT ? 800 : 400);
	realRot.y += -float(input->GetMouseDif().y) * sensitivity / div;
	if(realRot.y > angleMax)
		realRot.y = angleMax;
	if(realRot.y < angleMin)
		realRot.y = angleMin;

	if(!target->IsStanding())
	{
		realRot.x = Clip(realRot.x + float(input->GetMouseDif().x) * sensitivity / 400);
		freeRot = true;
		freeRotKey = Key::None;
	}
	else if(!freeRot)
	{
		freeRotKey = GKey.KeyDoReturn(GK_ROTATE_CAMERA, &Input::Pressed);
		if(freeRotKey != Key::None)
		{
			realRot.x = Clip(target->rot + PI);
			freeRot = true;
		}
	}
	else
	{
		if(freeRotKey == Key::None || input->Up(freeRotKey))
			freeRot = false;
		else
			realRot.x = Clip(realRot.x + float(input->GetMouseDif().x) * sensitivity / 400);
	}
}

//=================================================================================================
void GameCamera::UpdateDistance()
{
	if(GKey.AllowMouse())
	{
		dist = Clamp(dist - input->GetMouseWheel(), DIST_MIN, DIST_MAX);
		if(input->PressedRelease(Key::MiddleButton))
			dist = DIST_DEF;
	}
}

//=================================================================================================
void GameCamera::SetZoom(const Vec3* zoomPos)
{
	bool newZoom = (zoomPos != nullptr);
	if(zoom == newZoom)
	{
		if(zoomPos)
			this->zoomPos = *zoomPos;
		return;
	}
	zoom = newZoom;
	if(zoom)
	{
		h = -0.15f;
		shift = 0.483023137f;
		prevDist = dist;
		dist = 2.f;
		springiness = SPRINGINESS_SLOW;
		this->zoomPos = *zoomPos;
	}
	else
	{
		h = 0.2f;
		shift = 0.f;
		dist = prevDist;
		springinessTimer = 0.5f;
	}
}

//=================================================================================================
float GameCamera::HandleCollisions(const Vec3& pos, const Vec3& dir)
{
	LocationPart& locPart = *target->locPart;
	float t, minT = 2.f;

	const int tx = int(target->pos.x / 2),
		tz = int(target->pos.z / 2);

	if(locPart.partType == LocationPart::Type::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)gameLevel->location;

		// terrain
		RaytestTerrainCallback callback;
		phyWorld->rayTest(ToVector3(pos), ToVector3(pos + dir), callback);
		float t = callback.getFraction();
		if(t < minT && t > 0.f)
			minT = t;

		// buildings
		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(OutsideLocation::size - 1, tx + 3),
			maxz = min(OutsideLocation::size - 1, tz + 3);
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				if(outside->tiles[x + z * OutsideLocation::size].IsBlocking())
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 8.f, float(z + 1) * 2);
					if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
						minT = t;
				}
			}
		}
	}
	else if(locPart.partType == LocationPart::Type::Inside)
	{
		InsideLocation* inside = (InsideLocation*)gameLevel->location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(lvl.w - 1, tx + 3),
			maxz = min(lvl.h - 1, tz + 3);

		// ceil
		const Plane ceil(0, -1, 0, 4);
		if(RayToPlane(pos, dir, ceil, &t) && t < minT && t > 0.f)
			minT = t;

		// floor
		const Plane floor(0, 1, 0, 0);
		if(RayToPlane(pos, dir, floor, &t) && t < minT && t > 0.f)
			minT = t;

		// dungeon
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				Tile& p = lvl.map[x + z * lvl.w];
				if(IsBlocking(p.type))
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
						minT = t;
				}
				else if(IsSet(p.flags, Tile::F_LOW_CEILING))
				{
					const Box box(float(x) * 2, 3.f, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
						minT = t;
				}

				if(p.type == ENTRY_PREV || p.type == ENTRY_NEXT)
				{
					EntryType type;
					Int2 pt;
					GameDirection entryDir;
					if(p.type == ENTRY_PREV)
					{
						type = lvl.prevEntryType;
						pt = lvl.prevEntryPt;
						entryDir = lvl.prevEntryDir;
					}
					else
					{
						type = lvl.nextEntryType;
						pt = lvl.nextEntryPt;
						entryDir = lvl.nextEntryDir;
					}

					if(type == ENTRY_STAIRS_UP)
					{
						if(gameRes->vdStairsUp->RayToMesh(pos, dir, PtToPos(pt), DirToRot(entryDir), t) && t < minT)
							minT = t;
					}
					else if(type == ENTRY_STAIRS_DOWN)
					{
						if(gameRes->vdStairsDown->RayToMesh(pos, dir, PtToPos(pt), DirToRot(entryDir), t) && t < minT)
							minT = t;
					}
				}
				else if(p.type == DOORS || p.type == HOLE_FOR_DOORS)
				{
					Vec3 doorPos(float(x * 2) + 1, 0, float(z * 2) + 1);
					float rot;

					if(IsBlocking(lvl.map[x - 1 + z * lvl.w].type))
					{
						rot = 0;
						int mov = 0;
						if(lvl.rooms[lvl.map[x + (z - 1) * lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + (z + 1) * lvl.w].room]->IsCorridor())
							--mov;
						if(mov == 1)
							doorPos.z += 0.8229f;
						else if(mov == -1)
							doorPos.z -= 0.8229f;
					}
					else
					{
						rot = PI / 2;
						int mov = 0;
						if(lvl.rooms[lvl.map[x - 1 + z * lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + 1 + z * lvl.w].room]->IsCorridor())
							--mov;
						if(mov == 1)
							doorPos.x += 0.8229f;
						else if(mov == -1)
							doorPos.x -= 0.8229f;
					}

					if(gameRes->vdDoorHole->RayToMesh(pos, dir, doorPos, rot, t) && t < minT)
						minT = t;

					Door* door = locPart.FindDoor(Int2(x, z));
					if(door && door->IsBlocking())
					{
						Box box(doorPos.x, 0.f, doorPos.z);
						box.v2.y = Door::HEIGHT * 2;
						if(rot == 0.f)
						{
							box.v1.x -= Door::WIDTH;
							box.v2.x += Door::WIDTH;
							box.v1.z -= Door::THICKNESS;
							box.v2.z += Door::THICKNESS;
						}
						else
						{
							box.v1.z -= Door::WIDTH;
							box.v2.z += Door::WIDTH;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
							minT = t;
					}
				}
			}
		}
	}
	else
	{
		// building
		InsideBuilding& building = static_cast<InsideBuilding&>(locPart);

		// ceil
		if(building.top > 0.f)
		{
			const Plane ceil(0, -1, 0, building.top);
			if(RayToPlane(pos, dir, ceil, &t) && t < minT && t > 0.f)
				minT = t;
		}

		// floor
		if(!building.navmesh)
		{
			const Plane floor(0, 1, 0, 0);
			if(RayToPlane(pos, dir, floor, &t) && t < minT && t > 0.f)
				minT = t;
		}
		else
		{
			RaytestTerrainCallback callback(CG_TERRAIN | CG_BUILDING);
			phyWorld->rayTest(ToVector3(pos), ToVector3(pos + dir), callback);
			float t = callback.getFraction();
			if(t < minT && t > 0.f)
				minT = t;
		}

		// xsphere
		if(building.xsphereRadius > 0.f)
		{
			Vec3 from = pos + dir;
			if(RayToSphere(from, -dir, building.xspherePos, building.xsphereRadius, t) && t > 0.f)
			{
				t = 1.f - t;
				if(t < minT)
					minT = t;
			}
		}

		// doors
		for(vector<Door*>::iterator it = locPart.doors.begin(), end = locPart.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box box(door.pos);
				box.v2.y = Door::HEIGHT * 2;
				if(door.rot == 0.f)
				{
					box.v1.x -= Door::WIDTH;
					box.v2.x += Door::WIDTH;
					box.v1.z -= Door::THICKNESS;
					box.v2.z += Door::THICKNESS;
				}
				else
				{
					box.v1.z -= Door::WIDTH;
					box.v2.z += Door::WIDTH;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
					minT = t;
			}

			if(gameRes->vdDoorHole->RayToMesh(pos, dir, door.pos, door.rot, t) && t < minT)
				minT = t;
		}
	}

	// colliders
	for(vector<CollisionObject>::iterator it = locPart.lvlPart->colliders.begin(), end = locPart.lvlPart->colliders.end(); it != end; ++it)
	{
		if(!it->camCollider)
			continue;

		if(it->type == CollisionObject::SPHERE)
		{
			if(RayToCylinder(pos, pos + dir, Vec3(it->pos.x, -32.f, it->pos.z), Vec3(it->pos.x, 32.f, it->pos.z), it->radius, t) && t < minT && t > 0.f)
				minT = t;
		}
		else if(it->type == CollisionObject::RECTANGLE)
		{
			Box box(it->pos.x - it->w, -32.f, it->pos.z - it->h, it->pos.x + it->w, 32.f, it->pos.z + it->h);
			if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
				minT = t;
		}
		else
		{
			float w, h;
			if(Equal(it->rot, PI / 2) || Equal(it->rot, PI * 3 / 2))
			{
				w = it->h;
				h = it->w;
			}
			else
			{
				w = it->w;
				h = it->h;
			}

			Box box(it->pos.x - w, -32.f, it->pos.z - h, it->pos.x + w, 32.f, it->pos.z + h);
			if(RayToBox(pos, dir, box, &t) && t < minT && t > 0.f)
				minT = t;
		}
	}

	// camera colliders
	for(vector<CameraCollider>::iterator it = gameLevel->camColliders.begin(), end = gameLevel->camColliders.end(); it != end; ++it)
	{
		if(RayToBox(pos, dir, it->box, &t) && t < minT && t > 0.f)
			minT = t;
	}

	// handle znear
	if(minT > 1.f || target->player->noclip)
		minT = 1.f;
	else if(minT < 0.1f)
		minT = 0.1f;

	return minT;
}
