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
