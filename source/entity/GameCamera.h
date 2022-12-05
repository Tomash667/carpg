#pragma once

//-----------------------------------------------------------------------------
#include "Camera.h"

//-----------------------------------------------------------------------------
struct GameCamera : public Camera
{
	GameCamera();
	void Reset(bool full = false);
	void Update(float dt);
	void RotateTo(float dt, float destRot);
	void UpdateFreeRot(float dt);
	void UpdateDistance();
	void SetZoom(const Vec3* zoomPos);

	Unit* target;
	Vec3 realFrom, realTo, zoomPos;
	Vec2 rot, realRot;
	float dist, shift, h, springiness, drunkAnim;
	FrustumPlanes frustum;
	Key freeRotKey;
	bool freeRot, zoom;

private:
	float HandleCollisions(const Vec3& pos, const Vec3& dir);

	float tmpDist, tmpShift, tmpH, springinessTimer, prevDist;
	bool reset;
};
