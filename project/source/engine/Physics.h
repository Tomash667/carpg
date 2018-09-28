#pragma once

//-----------------------------------------------------------------------------
#define NULL nullptr
#include <btBulletCollisionCommon.h>
#include <BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h>
#undef NULL

//-----------------------------------------------------------------------------
class CustomCollisionWorld : public btCollisionWorld
{
public:
	CustomCollisionWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphase, btCollisionConfiguration* config);
	static CustomCollisionWorld* Init();
	static void Cleanup(CustomCollisionWorld* world);
	void Reset();

private:
	btCollisionConfiguration* config;
	btDispatcher* dispatcher;
	btBroadphaseInterface* broadphase;
};

//-----------------------------------------------------------------------------
inline Vec2 ToVec2(const btVector3& v)
{
	return Vec2(v.x(), v.z());
}

inline Vec3 ToVec3(const btVector3& v)
{
	return Vec3(v.x(), v.y(), v.z());
}

inline btVector3 ToVector3(const Vec2& v)
{
	return btVector3(v.x, 0, v.y);
}

inline btVector3 ToVector3(const Vec3& v)
{
	return btVector3(v.x, v.y, v.z);
}

inline btQuaternion ToQuaternion(const Quat& q)
{
	return btQuaternion(q.x, q.y, q.z, q.w);
}

inline Quat ToQuat(const btQuaternion& q)
{
	return Quat(q.x(), q.y(), q.z(), q.w());
}
