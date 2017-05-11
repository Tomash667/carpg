#include "Pch.h"
#include "Base.h"
#include "Physics.h"

//=================================================================================================
void CustomCollisionWorld::Reset()
{
	btOverlappingPairCache* cache = m_broadphasePairCache->getOverlappingPairCache();
	
	for(int i=0, count=m_collisionObjects.size(); i<count; ++i)
	{
		btCollisionObject* obj = m_collisionObjects[i];
		btBroadphaseProxy* bp = obj->getBroadphaseHandle();
		if (bp)
		{
			cache->cleanProxyFromPairs(bp, m_dispatcher1);
			m_broadphasePairCache->destroyProxy(bp, m_dispatcher1);
		}
		delete obj;
	}

	m_collisionObjects.clear();
	m_broadphasePairCache->resetPool(m_dispatcher1);
}
