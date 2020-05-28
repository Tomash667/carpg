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
#include "LevelArea.h"
#include "OutsideLocation.h"
#include "PhysicCallbacks.h"
#include "Unit.h"

const float SPRINGINESS_NORMAL = 40.f;
const float SPRINGINESS_SLOW = 5.f;
const float DIST_MIN = 0.5f;
const float DIST_DEF = 3.5f;
const float DIST_MAX = 6.f;

//=================================================================================================
GameCamera::GameCamera() : springiness(SPRINGINESS_NORMAL), reset(true), free_rot(false)
{
}

//=================================================================================================
void GameCamera::Reset(bool full)
{
	if(full)
	{
		real_rot = Vec2(0, 4.2875104f);
		dist = DIST_DEF;
		drunk_anim = 0.f;
	}
	shift = 0.f;
	h = 0.2f;
	zoom = false;
	reset = true;
	free_rot = false;
	from = real_from;
	to = real_to;
	tmp_dist = dist;
	tmp_shift = shift;
	springiness = SPRINGINESS_NORMAL;
	springiness_timer = 0.f;
}

//=================================================================================================
void GameCamera::Update(float dt)
{
	drunk_anim = Clip(drunk_anim + dt);

	if(!zoom && springiness_timer > 0.f)
	{
		springiness_timer -= dt;
		if(springiness_timer <= 0.f)
		{
			springiness += 5;
			if(springiness >= SPRINGINESS_NORMAL)
				springiness = SPRINGINESS_NORMAL;
			else
				springiness_timer = 0.5f;
		}
	}

	const float d = reset
		? 1.0f
		: 1.0f - exp(log(0.5f) * springiness * dt);

	if(!free_rot)
		real_rot.x = target->rot;

	// update rotation, distance & shift
	if(reset)
	{
		rot = real_rot;
		tmp_dist = dist;
		tmp_shift = shift;
		tmp_h = h;
	}
	else
	{
		rot = Vec2::Slerp(rot, real_rot, d).Clip();
		tmp_dist += (dist - tmp_dist) * d;
		tmp_shift += (shift - tmp_shift) * d;
		tmp_h += (h - tmp_h) * d;
	}

	// calculate new from & to, handle collisions
	const Vec3 forward_dir = Vec3::Transform(Vec3::UnitY, Matrix::Rotation(rot.y, rot.x, 0));
	const Vec3 backward_dir = Vec3::Transform(Vec3::UnitY, Matrix::Rotation(rot.y, rot.x + shift, 0));

	Vec3 pos = target->pos;
	pos.y += target->GetUnitHeight() + tmp_h;

	if(zoom)
		real_to = zoom_pos;
	else
		real_to = pos + forward_dir * 20;

	// camera goes from character head backwards (to => from)
	float t = HandleCollisions(pos, -tmp_dist * backward_dir);

	float real_dist = tmp_dist * t - 0.1f;
	if(real_dist < 0.01f)
		real_dist = 0.01f;
	real_from = pos + backward_dir * -real_dist;

	// update from & to
	if(reset)
	{
		from = real_from;
		to = real_to;
		reset = false;
	}
	else
	{
		from += (real_from - from) * d;
		to += (real_to - to) * d;
	}

	// calculate matrices, frustum
	float drunk = target->alcohol / target->hpmax;
	float drunk_mod = (drunk > 0.1f ? (drunk - 0.1f) / 0.9f : 0.f);

	Matrix mat_view = Matrix::CreateLookAt(from, to);
	Matrix mat_proj = Matrix::CreatePerspectiveFieldOfView(PI / 4 + sin(drunk_anim) * (PI / 16) * drunk_mod,
		engine->GetWindowAspect() * (1.f + sin(drunk_anim) / 10 * drunk_mod), znear, zfar);
	mat_view_proj = mat_view * mat_proj;
	mat_view_inv = mat_view.Inverse();
	frustum.Set(mat_view_proj);

	// 3d source listener
	sound_mgr->SetListenerPosition(target->GetHeadSoundPos(), Vec3(sin(target->rot + PI), 0, cos(target->rot + PI)));
}

//=================================================================================================
void GameCamera::RotateTo(float dt, float dest_rot)
{
	free_rot = false;
	if(real_rot.y > dest_rot)
	{
		real_rot.y -= dt;
		if(real_rot.y < dest_rot)
			real_rot.y = dest_rot;
	}
	else if(real_rot.y < dest_rot)
	{
		real_rot.y += dt;
		if(real_rot.y > dest_rot)
			real_rot.y = dest_rot;
	}
}

//=================================================================================================
void GameCamera::UpdateFreeRot(float dt)
{
	if(!Any(GKey.allow_input, GameKeys::ALLOW_INPUT, GameKeys::ALLOW_MOUSE))
		return;

	const float c_cam_angle_min = PI + 0.1f;
	const float c_cam_angle_max = PI * 1.8f - 0.1f;

	int div = (target->action == A_SHOOT ? 800 : 400);
	real_rot.y += -float(input->GetMouseDif().y) * game->settings.mouse_sensitivity_f / div;
	if(real_rot.y > c_cam_angle_max)
		real_rot.y = c_cam_angle_max;
	if(real_rot.y < c_cam_angle_min)
		real_rot.y = c_cam_angle_min;

	if(!target->IsStanding())
	{
		real_rot.x = Clip(real_rot.x + float(input->GetMouseDif().x) * game->settings.mouse_sensitivity_f / 400);
		free_rot = true;
		free_rot_key = Key::None;
	}
	else if(!free_rot)
	{
		free_rot_key = GKey.KeyDoReturn(GK_ROTATE_CAMERA, &Input::Pressed);
		if(free_rot_key != Key::None)
		{
			real_rot.x = Clip(target->rot + PI);
			free_rot = true;
		}
	}
	else
	{
		if(free_rot_key == Key::None || input->Up(free_rot_key))
			free_rot = false;
		else
			real_rot.x = Clip(real_rot.x + float(input->GetMouseDif().x) * game->settings.mouse_sensitivity_f / 400);
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
void GameCamera::SetZoom(const Vec3* zoom_pos)
{
	bool new_zoom = (zoom_pos != nullptr);
	if(zoom == new_zoom)
	{
		if(zoom_pos)
			this->zoom_pos = *zoom_pos;
		return;
	}
	zoom = new_zoom;
	if(zoom)
	{
		h = -0.15f;
		shift = 0.483023137f;
		prev_dist = dist;
		dist = 2.f;
		springiness = SPRINGINESS_SLOW;
		this->zoom_pos = *zoom_pos;
	}
	else
	{
		h = 0.2f;
		shift = 0.f;
		dist = prev_dist;
		springiness_timer = 0.5f;
	}
}

//=================================================================================================
float GameCamera::HandleCollisions(const Vec3& pos, const Vec3& dir)
{
	LevelArea& area = *target->area;
	float t, min_t = 2.f;

	const int tx = int(target->pos.x / 2),
		      tz = int(target->pos.z / 2);

	if(area.area_type == LevelArea::Type::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)game_level->location;

		// terrain
		RaytestTerrainCallback callback;
		phy_world->rayTest(ToVector3(pos), ToVector3(pos + dir), callback);
		float t = callback.getFraction();
		if(t < min_t && t > 0.f)
			min_t = t;

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
					if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
						min_t = t;
				}
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		InsideLocation* inside = (InsideLocation*)game_level->location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(lvl.w - 1, tx + 3),
			maxz = min(lvl.h - 1, tz + 3);

		// ceil
		const Plane sufit(0, -1, 0, 4);
		if(RayToPlane(pos, dir, sufit, &t) && t < min_t && t > 0.f)
			min_t = t;

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(pos, dir, podloga, &t) && t < min_t && t > 0.f)
			min_t = t;

		// dungeon
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				Tile& p = lvl.map[x + z * lvl.w];
				if(IsBlocking(p.type))
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
						min_t = t;
				}
				else if(IsSet(p.flags, Tile::F_LOW_CEILING))
				{
					const Box box(float(x) * 2, 3.f, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
						min_t = t;
				}
				if(p.type == STAIRS_UP)
				{
					if(game_res->vdStairsUp->RayToMesh(pos, dir, PtToPos(lvl.staircase_up), DirToRot(lvl.staircase_up_dir), t) && t < min_t)
						min_t = t;
				}
				else if(p.type == STAIRS_DOWN)
				{
					if(!lvl.staircase_down_in_wall
						&& game_res->vdStairsDown->RayToMesh(pos, dir, PtToPos(lvl.staircase_down), DirToRot(lvl.staircase_down_dir), t) && t < min_t)
						min_t = t;
				}
				else if(p.type == DOORS || p.type == HOLE_FOR_DOORS)
				{
					Vec3 door_pos(float(x * 2) + 1, 0, float(z * 2) + 1);
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
							door_pos.z += 0.8229f;
						else if(mov == -1)
							door_pos.z -= 0.8229f;
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
							door_pos.x += 0.8229f;
						else if(mov == -1)
							door_pos.x -= 0.8229f;
					}

					if(game_res->vdDoorHole->RayToMesh(pos, dir, door_pos, rot, t) && t < min_t)
						min_t = t;

					Door* door = area.FindDoor(Int2(x, z));
					if(door && door->IsBlocking())
					{
						Box box(door_pos.x, 0.f, door_pos.z);
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

						if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
							min_t = t;
					}
				}
			}
		}
	}
	else
	{
		// building
		InsideBuilding& building = static_cast<InsideBuilding&>(area);

		// ceil
		if(building.top > 0.f)
		{
			const Plane sufit(0, -1, 0, 4);
			if(RayToPlane(pos, dir, sufit, &t) && t < min_t && t > 0.f)
				min_t = t;
		}

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(pos, dir, podloga, &t) && t < min_t && t > 0.f)
			min_t = t;

		// xsphere
		if(building.xsphere_radius > 0.f)
		{
			Vec3 from = pos + dir;
			if(RayToSphere(from, -dir, building.xsphere_pos, building.xsphere_radius, t) && t > 0.f)
			{
				t = 1.f - t;
				if(t < min_t)
					min_t = t;
			}
		}

		// doors
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
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

				if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
					min_t = t;
			}

			if(game_res->vdDoorHole->RayToMesh(pos, dir, door.pos, door.rot, t) && t < min_t)
				min_t = t;
		}
	}

	// objects
	for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
	{
		if(!it->cam_collider)
			continue;

		if(it->type == CollisionObject::SPHERE)
		{
			if(RayToCylinder(pos, pos + dir, Vec3(it->pt.x, 0, it->pt.y), Vec3(it->pt.x, 32.f, it->pt.y), it->radius, t) && t < min_t && t > 0.f)
				min_t = t;
		}
		else if(it->type == CollisionObject::RECTANGLE)
		{
			Box box(it->pt.x - it->w, 0.f, it->pt.y - it->h, it->pt.x + it->w, 32.f, it->pt.y + it->h);
			if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
				min_t = t;
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

			Box box(it->pt.x - w, 0.f, it->pt.y - h, it->pt.x + w, 32.f, it->pt.y + h);
			if(RayToBox(pos, dir, box, &t) && t < min_t && t > 0.f)
				min_t = t;
		}
	}

	// camera colliders
	for(vector<CameraCollider>::iterator it = game_level->cam_colliders.begin(), end = game_level->cam_colliders.end(); it != end; ++it)
	{
		if(RayToBox(pos, dir, it->box, &t) && t < min_t && t > 0.f)
			min_t = t;
	}

	// uwzglêdnienie znear
	if(min_t > 1.f || target->player->noclip)
		min_t = 1.f;
	else if(min_t < 0.1f)
		min_t = 0.1f;

	return min_t;
}
