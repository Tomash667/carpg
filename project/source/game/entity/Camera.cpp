#include "Pch.h"
#include "Base.h"
#include "Camera.h"

Camera::Camera(float springiness) : springiness(springiness), reset(2), free_rot(false)
{

}

void Camera::Reset()
{
	reset = 2;
	free_rot = false;
	from = real_from;
	to = real_to;
	tmp_dist = dist;
}

void Camera::UpdateRot(float dt, const VEC2& new_rot)
{
	if(reset == 0)
		d = 1.0f - exp(log(0.5f) * springiness * dt);
	else
	{
		d = 1.0f;
		reset = 1;
	}

	real_rot = new_rot;
	rot = clip(slerp(rot, real_rot, d));
	tmp_dist += (dist - tmp_dist) * d;
}

void Camera::Update(float dt, const VEC3& new_from, const VEC3& new_to)
{
	real_from = new_from;
	real_to = new_to;

	if(reset == 0)
	{
		from += (real_from - from) * d;
		to += (real_to - to) * d;
	}
	else
	{
		from = real_from;
		to = real_to;
		reset = 0;
	}
}
