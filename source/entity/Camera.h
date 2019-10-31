#pragma once

//-----------------------------------------------------------------------------
#include "CameraBase.h"

//-----------------------------------------------------------------------------
struct Camera : public CameraBase
{
	Vec3 real_from, real_to;
	Vec2 rot, real_rot;
	float dist, tmp_dist, draw_range, springiness, d;
	FrustumPlanes frustum;
	Key free_rot_key;
	bool free_rot;

private:
	int reset;

public:
	Camera(float springiness = 40.f);

	void Reset();
	void UpdateRot(float dt, const Vec2& new_rot);
	void Update(float dt, const Vec3& new_from, const Vec3& new_to);
};
