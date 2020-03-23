#pragma once

//-----------------------------------------------------------------------------
#include "Camera.h"

//-----------------------------------------------------------------------------
struct GameCamera : public Camera
{
	GameCamera(float springiness = 40.f);
	void Reset(bool full = false);
	void Update(float dt);
	void RotateTo(float dt, float dest_rot);
	void UpdateFreeRot(float dt);
	void SetZoom(const Vec3* zoom_pos);

	Unit* target;
	Vec3 real_from, real_to, zoom_pos;
	Vec2 rot, real_rot;
	float dist, shift, h, springiness, drunk_anim;
	FrustumPlanes frustum;
	Key free_rot_key;
	bool free_rot, zoom;

private:
	float HandleCollisions(const Vec3& pos, const Vec3& dir);

	float tmp_dist, tmp_shift, tmp_h, tmp_springiness, prev_dist;
	bool reset;
};
