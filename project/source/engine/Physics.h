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
inline VEC2 ToVEC2(const btVector3& v)
{
	return VEC2(v.x(), v.z());
}

inline VEC3 ToVEC3(const btVector3& v)
{
	return VEC3(v.x(), v.y(), v.z());
}

inline btVector3 ToVector3(const VEC2& v)
{
	return btVector3(v.x, 0, v.y);
}

inline btVector3 ToVector3(const VEC3& v)
{
	return btVector3(v.x, v.y, v.z);
}
