#pragma once

//-----------------------------------------------------------------------------
#include "Unit.h"

//-----------------------------------------------------------------------------
enum LineTestResult
{
	LT_IGNORE,
	LT_COLLIDE,
	LT_END
};

//-----------------------------------------------------------------------------
struct RaytestWithIgnoredCallback : public btCollisionWorld::RayResultCallback
{
	RaytestWithIgnoredCallback(const void* ignore1, const void* ignore2) : ignore1(ignore1), ignore2(ignore2), hit(false), fraction(1.01f)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		void* ptr = rayResult.m_collisionObject->getUserPointer();
		// ignore selected colliders
		if(ptr && Any(ptr, ignore1, ignore2))
			return 1.f;
		// ignore dead units
		if(IsSet(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
		{
			Unit* unit = reinterpret_cast<Unit*>(ptr);
			if(!unit->IsAlive())
				return 1.f;
		}
		if(rayResult.m_hitFraction < fraction)
		{
			fraction = rayResult.m_hitFraction;
			hit = true;
		}
		return 0.f;
	}

	const void* ignore1, *ignore2;
	float fraction;
	bool hit;
};

//-----------------------------------------------------------------------------
struct RaytestAnyUnitCallback : public btCollisionWorld::RayResultCallback
{
	RaytestAnyUnitCallback() : clear(true)
	{
		m_collisionFilterMask = ~CG_UNIT;
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		clear = false;
		return 0.f;
	}

	bool clear;
};

//-----------------------------------------------------------------------------
struct RaytestClosestUnitCallback : public btCollisionWorld::RayResultCallback
{
	explicit RaytestClosestUnitCallback(delegate<bool(Unit*)> clbk) : clbk(clbk), hit(nullptr)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		Unit* unit;
		if(IsSet(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
		{
			unit = reinterpret_cast<Unit*>(rayResult.m_collisionObject->getUserPointer());
			if(!clbk(unit))
				return 1.f;
		}
		else
			unit = nullptr;

		if(m_closestHitFraction > rayResult.m_hitFraction)
		{
			m_closestHitFraction = rayResult.m_hitFraction;
			hit = unit;
		}

		return 0.f;
	}

	bool hasHit() const { return m_closestHitFraction < 1.f; }
	float getFraction() const { return m_closestHitFraction; }

	delegate<bool(Unit*)> clbk;
	Unit* hit;
};

//-----------------------------------------------------------------------------
struct RaytestTerrainCallback : public btCollisionWorld::RayResultCallback
{
	RaytestTerrainCallback()
	{
		m_collisionFilterMask = CG_TERRAIN;
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(m_closestHitFraction > rayResult.m_hitFraction)
			m_closestHitFraction = rayResult.m_hitFraction;
		return 0.f;
	}

	bool hasHit() const { return m_closestHitFraction < 1.f; }
	float getFraction() const { return m_closestHitFraction; }
};

//-----------------------------------------------------------------------------
struct ContactTestCallback : public btCollisionWorld::ContactResultCallback
{
	ContactTestCallback(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool useClbk2) : obj(obj), clbk(clbk), useClbk2(useClbk2), hit(false)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk((btCollisionObject*)proxy0->m_clientObject, true);
	}

	float addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap,
		int partId1, int index1)
	{
		if(useClbk2)
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

	btCollisionObject* obj;
	delegate<bool(btCollisionObject*, bool)> clbk;
	bool useClbk2, hit;
};

//-----------------------------------------------------------------------------
struct BulletCallback : public btCollisionWorld::ConvexResultCallback
{
	explicit BulletCallback(btCollisionObject* ignore) : target(nullptr), ignore(ignore)
	{
		m_collisionFilterMask = ~CG_BARRIER;
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
	{
		assert(m_closestHitFraction >= convexResult.m_hitFraction);

		btCollisionObject* cobj = const_cast<btCollisionObject*>(convexResult.m_hitCollisionObject);
		if(cobj == ignore)
			return 1.f;

		// ignore dead units
		if(IsSet(cobj->getCollisionFlags(), CG_UNIT))
		{
			Unit* unit = reinterpret_cast<Unit*>(cobj->getUserPointer());
			if(!unit->IsAlive())
				return 1.f;
		}

		m_closestHitFraction = convexResult.m_hitFraction;
		hitpoint = convexResult.m_hitPointLocal;
		target = cobj;
		return 0.f;
	}

	btCollisionObject* ignore, *target;
	btVector3 hitpoint;
};

//-----------------------------------------------------------------------------
struct ConvexCallback : public btCollisionWorld::ConvexResultCallback
{
	typedef delegate<LineTestResult(const btCollisionObject*, btCollisionWorld::LocalConvexResult*)> Callback;

	ConvexCallback(Callback clbk, vector<float>* tList, bool useClbk2) : clbk(clbk), useClbk2(useClbk2), tList(nullptr), end(false), endT(1.f), closest(1.1f)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk(reinterpret_cast<btCollisionObject*>(proxy0->m_clientObject), nullptr) == LT_COLLIDE;
	}

	float addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if(useClbk2)
		{
			auto result = clbk(convexResult.m_hitCollisionObject, &convexResult);
			if(result == LT_IGNORE)
				return m_closestHitFraction;
			else if(result == LT_END)
			{
				if(endT > convexResult.m_hitFraction)
				{
					closest = min(closest, convexResult.m_hitFraction);
					m_closestHitFraction = convexResult.m_hitFraction;
					endT = convexResult.m_hitFraction;
				}
				end = true;
				return 1.f;
			}
			else if(end && convexResult.m_hitFraction > endT)
				return 1.f;
		}
		closest = min(closest, convexResult.m_hitFraction);
		if(tList)
			tList->push_back(convexResult.m_hitFraction);
		// return value is unused
		return 1.f;
	}

	bool hasHit() const
	{
		return closest <= 1.f;
	}

	Callback clbk;
	vector<float>* tList;
	float endT, closest;
	bool useClbk2, end;
};
