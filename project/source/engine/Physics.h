#pragma once

//-----------------------------------------------------------------------------
class CustomCollisionWorld : public btCollisionWorld
{
public:
	CustomCollisionWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphasePairCache, btCollisionConfiguration* collisionConfiguration) :
		btCollisionWorld(dispatcher, broadphasePairCache, collisionConfiguration)
	{
	}

	void Reset();
};

//-----------------------------------------------------------------------------
inline Vec2 ToVEC2(const btVector3& v)
{
	return Vec2(v.x(), v.z());
}

inline Vec3 ToVEC3(const btVector3& v)
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
