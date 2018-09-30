#include "Pch.h"
#include "EngineCore.h"
#include "Physics.h"

//=================================================================================================
CustomCollisionWorld::CustomCollisionWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphase, btCollisionConfiguration* config) :
	btCollisionWorld(dispatcher, broadphase, config), config(config), dispatcher(dispatcher), broadphase(broadphase)
{
}

//=================================================================================================
CustomCollisionWorld* CustomCollisionWorld::Init()
{
	btDefaultCollisionConfiguration* config = new btDefaultCollisionConfiguration;
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(config);
	btDbvtBroadphase* broadphase = new btDbvtBroadphase;
	CustomCollisionWorld* world = new CustomCollisionWorld(dispatcher, broadphase, config);
	Info("Engine: Bullet physics system created.");
	return world;
}

//=================================================================================================
void CustomCollisionWorld::Cleanup(CustomCollisionWorld* world)
{
	if(!world)
		return;
	btCollisionConfiguration* config = world->config;
	btDispatcher* dispatcher = world->dispatcher;
	btBroadphaseInterface* broadphase = world->broadphase;
	delete world;
	delete broadphase;
	delete dispatcher;
	delete config;
}

//=================================================================================================
void CustomCollisionWorld::Reset()
{
	btOverlappingPairCache* cache = m_broadphasePairCache->getOverlappingPairCache();

	for(int i = 0, count = m_collisionObjects.size(); i < count; ++i)
	{
		btCollisionObject* obj = m_collisionObjects[i];
		btBroadphaseProxy* bp = obj->getBroadphaseHandle();
		if(bp)
		{
			cache->cleanProxyFromPairs(bp, m_dispatcher1);
			m_broadphasePairCache->destroyProxy(bp, m_dispatcher1);
		}
		delete obj;
	}

	m_collisionObjects.clear();
	m_broadphasePairCache->resetPool(m_dispatcher1);
}

//=================================================================================================
void CustomCollisionWorld::UpdateAabb(btCollisionObject* cobj)
{
	btVector3 a_min, a_max;
	cobj->getCollisionShape()->getAabb(cobj->getWorldTransform(), a_min, a_max);
	broadphase->setAabb(cobj->getBroadphaseHandle(), a_min, a_max, dispatcher);
}
