#pragma once

class ContactCallback : public btCollisionWorld::ContactResultCallback
{
	const btCollisionObject* other;
	const btCollisionObject* ignore;
	const int flag;
	bool hit;

	inline bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		return proxy0->m_collisionFilterGroup == flag && proxy0->m_collisionFilterMask == flag;
	}

	inline btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0,
		const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		if(colObj1Wrap->getCollisionObject() == ignore)
			return 1;
		other = colObj1Wrap->getCollisionObject();
		hit = true;
		return 0;
	}

public:
	ContactCallback(btCollisionObject* ignore, int flag) : hit(false), other(nullptr), ignore(ignore), flag(flag) {}

	inline bool HasHit() const { return hit; }
};
