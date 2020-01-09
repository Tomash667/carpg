#pragma once

//-----------------------------------------------------------------------------
struct RaytestWithIgnoredCallback : public btCollisionWorld::RayResultCallback
{
	RaytestWithIgnoredCallback(const void* ignore1, const void* ignore2) : ignore1(ignore1), ignore2(ignore2), hit(false), fraction(1.01f)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		void* ptr = rayResult.m_collisionObject->getUserPointer();
		if(ptr && Any(ptr, ignore1, ignore2))
			return 1.f;
		if(rayResult.m_hitFraction < fraction)
		{
			fraction = rayResult.m_hitFraction;
			hit = true;
		}
		return 0.f;
	}

	bool clear;
	const void* ignore1, *ignore2;
	float fraction;
	bool hit;
};

//-----------------------------------------------------------------------------
struct RaytestAnyUnitCallback : public btCollisionWorld::RayResultCallback
{
	RaytestAnyUnitCallback() : clear(true)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(IsSet(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
			return 1.f;
		clear = false;
		return 0.f;
	}

	bool clear;
};

//-----------------------------------------------------------------------------
struct RaytestClosestUnitCallback : public btCollisionWorld::RayResultCallback
{
	explicit RaytestClosestUnitCallback(Unit* ignore) : ignore(ignore), hitted(nullptr), fraction(1.01f)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(!IsSet(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
			return 1.f;

		Unit* unit = reinterpret_cast<Unit*>(rayResult.m_collisionObject->getUserPointer());
		if(unit == ignore)
			return 1.f;

		if(rayResult.m_hitFraction < fraction)
		{
			hitted = unit;
			fraction = rayResult.m_hitFraction;
		}

		return 0.f;
	}

	Unit* ignore, *hitted;
	float fraction;
};

//-----------------------------------------------------------------------------
struct RaytestClosestUnitDeadOrAliveCallback : public btCollisionWorld::RayResultCallback
{
	RaytestClosestUnitDeadOrAliveCallback(Unit* me) : me(me), hit(nullptr)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		void* ptr = rayResult.m_collisionObject->getUserPointer();
		if(me == ptr)
			return 1.f;
		if(m_closestHitFraction > rayResult.m_hitFraction)
		{
			m_closestHitFraction = rayResult.m_hitFraction;
			if(IsSet(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT | CG_UNIT_DEAD))
				hit = reinterpret_cast<Unit*>(ptr);
			else
				hit = nullptr;
		}
		return 0.f;
	}

	Unit* me;
	Unit* hit;
};

//-----------------------------------------------------------------------------
struct ContactTestCallback : public btCollisionWorld::ContactResultCallback
{
	btCollisionObject* obj;
	delegate<bool(btCollisionObject*, bool)> clbk;
	bool use_clbk2, hit;

	ContactTestCallback(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2) : obj(obj), clbk(clbk), use_clbk2(use_clbk2), hit(false)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk((btCollisionObject*)proxy0->m_clientObject, true);
	}

	float addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap,
		int partId1, int index1)
	{
		if(use_clbk2)
		{
			const btCollisionObject* cobj;
			if(colObj0Wrap->m_collisionObject == obj)
				cobj = colObj1Wrap->m_collisionObject;
			else
				cobj = colObj0Wrap->m_collisionObject;
			if(!clbk((btCollisionObject*)cobj, false))
				return 1.f;
		}
		hit = true;
		return 0.f;
	}
};

//-----------------------------------------------------------------------------
struct BulletCallback : public btCollisionWorld::ConvexResultCallback
{
	BulletCallback(btCollisionObject* ignore) : target(nullptr), ignore(ignore)
	{
		ClearBit(m_collisionFilterMask, CG_BARRIER);
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
	{
		assert(m_closestHitFraction >= convexResult.m_hitFraction);

		if(convexResult.m_hitCollisionObject == ignore)
			return 1.f;

		m_closestHitFraction = convexResult.m_hitFraction;
		hitpoint = convexResult.m_hitPointLocal;
		target = (btCollisionObject*)convexResult.m_hitCollisionObject;
		return 0.f;
	}

	btCollisionObject* ignore, *target;
	btVector3 hitpoint;
};

//-----------------------------------------------------------------------------
struct ConvexCallback : public btCollisionWorld::ConvexResultCallback
{
	delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk;
	vector<float>* t_list;
	float end_t, closest;
	bool use_clbk2, end = false;

	ConvexCallback(delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, vector<float>* t_list, bool use_clbk2) : clbk(clbk), t_list(t_list), use_clbk2(use_clbk2),
		end(false), end_t(1.f), closest(1.1f)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk((btCollisionObject*)proxy0->m_clientObject, true) == LT_COLLIDE;
	}

	float addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if(use_clbk2)
		{
			auto result = clbk((btCollisionObject*)convexResult.m_hitCollisionObject, false);
			if(result == LT_IGNORE)
				return m_closestHitFraction;
			else if(result == LT_END)
			{
				if(end_t > convexResult.m_hitFraction)
				{
					closest = min(closest, convexResult.m_hitFraction);
					m_closestHitFraction = convexResult.m_hitFraction;
					end_t = convexResult.m_hitFraction;
				}
				end = true;
				return 1.f;
			}
			else if(end && convexResult.m_hitFraction > end_t)
				return 1.f;
		}
		closest = min(closest, convexResult.m_hitFraction);
		if(t_list)
			t_list->push_back(convexResult.m_hitFraction);
		// return value is unused
		return 1.f;
	}
};
