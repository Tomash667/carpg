#include "Pch.h"
#include "Door.h"

#include "BitStreamFunc.h"
#include "Collision.h"
#include "GameResources.h"
#include "Level.h"
#include "Location.h"
#include "Net.h"
#include "Unit.h"

#include <SoundManager.h>

EntityType<Door>::Impl EntityType<Door>::impl;
const float Door::WIDTH = 0.842f;
const float Door::THICKNESS = 0.181f;
const float Door::HEIGHT = 1.319f;
const float Door::SOUND_DIST = 1.f;
const float Door::UNLOCK_SOUND_DIST = 0.5f;
const float Door::BLOCKED_SOUND_DIST = 1.f;

//=================================================================================================
Door::~Door()
{
	delete meshInst;
}

//=================================================================================================
void Door::Init()
{
	Register();
	Recreate();
}

//=================================================================================================
void Door::Recreate()
{
	// mesh
	meshInst = new MeshInstance(door2 ? game_res->aDoor2 : game_res->aDoor);
	meshInst->base_speed = 2.f;

	// physics
	phy = new btCollisionObject;
	phy->setCollisionShape(game_level->shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
	btTransform& tr = phy->getWorldTransform();
	Vec3 phyPos = pos;
	phyPos.y += HEIGHT;
	tr.setOrigin(ToVector3(phyPos));
	tr.setRotation(btQuaternion(rot, 0, 0));
	phy_world->addCollisionObject(phy, CG_DOOR);

	// is open
	if(state == Opened)
	{
		btVector3& phyPos = phy->getWorldTransform().getOrigin();
		phyPos.setY(phyPos.y() - 100.f);
		meshInst->SetToEnd(&meshInst->mesh->anims[0]);
	}
}

//=================================================================================================
void Door::Cleanup()
{
	if(state == Closing || state == Closing2)
		state = Closed;
	else if(state == Opening || state == Opening2)
		state = Opened;
	delete meshInst;
	meshInst = nullptr;
}

//=================================================================================================
void Door::Update(float dt, LocationPart& locPart)
{
	meshInst->Update(dt);
	if(state == Opening || state == Opening2)
	{
		bool done = meshInst->IsEnded();
		if(state == Opening)
		{
			if(done || meshInst->GetProgress() >= 0.25f)
			{
				state = Opening2;
				btVector3& phyPos = phy->getWorldTransform().getOrigin();
				phyPos.setY(phyPos.y() - 100.f);
			}
		}
		if(done)
			state = Opened;
	}
	else if(state == Closing || state == Closing2)
	{
		bool done = meshInst->IsEnded();
		if(state == Closing)
		{
			if(done || meshInst->GetProgress() <= 0.25f)
			{
				bool blocking = false;

				for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
				{
					if((*it)->IsAlive() && CircleToRotatedRectangle((*it)->pos.x, (*it)->pos.z, (*it)->GetUnitRadius(), pos.x, pos.z, WIDTH, THICKNESS, rot))
					{
						blocking = true;
						break;
					}
				}

				if(!blocking)
				{
					state = Closing2;
					btVector3& phyPos = phy->getWorldTransform().getOrigin();
					phyPos.setY(phyPos.y() + 100.f);
				}
			}
		}
		if(done)
		{
			if(state == Closing2)
				state = Closed;
			else
			{
				// can't close doors, somone is blocking it
				state = Opening2;
				meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_NO_BLEND | PLAY_STOP_AT_END, 0);
				sound_mgr->PlaySound3d(game_res->sDoorBudge, pos, BLOCKED_SOUND_DIST);
			}
		}
	}
}

//=================================================================================================
void Door::Save(GameWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << pt;
	f << locked;
	f << state;
	f << door2;

	if(f.isLocal)
		meshInst->Save(f);
}

//=================================================================================================
void Door::Load(GameReader& f)
{
	if(LOAD_VERSION >= V_0_12)
		f >> id;
	Register();

	f >> pos;
	f >> rot;
	f >> pt;
	f >> locked;
	f >> state;
	if(LOAD_VERSION < V_0_12)
		f.Skip<int>(); // old netid
	f >> door2;

	if(f.isLocal)
	{
		meshInst = new MeshInstance(door2 ? game_res->aDoor2 : game_res->aDoor);
		meshInst->Load(f, LOAD_VERSION >= V_0_13 ? 1 : 0);
		meshInst->base_speed = 2.f;

		phy = new btCollisionObject;
		phy->setCollisionShape(game_level->shape_door);
		phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

		btTransform& tr = phy->getWorldTransform();
		Vec3 phyPos = pos;
		phyPos.y += HEIGHT;
		tr.setOrigin(ToVector3(phyPos));
		tr.setRotation(btQuaternion(rot, 0, 0));
		phy_world->addCollisionObject(phy, CG_DOOR);

		if(!IsBlocking())
		{
			btVector3& phyPos = phy->getWorldTransform().getOrigin();
			phyPos.setY(phyPos.y() - 100.f);
		}
	}
	else
		meshInst = nullptr;
}

//=================================================================================================
void Door::Write(BitStreamWriter& f)
{
	f << id;
	f << pos;
	f << rot;
	f << pt;
	f.WriteCasted<byte>(locked);
	f.WriteCasted<byte>(state);
	f << door2;
	if(net->mp_load)
		meshInst->Write(f);
}

//=================================================================================================
bool Door::Read(BitStreamReader& f)
{
	f >> id;
	f >> pos;
	f >> rot;
	f >> pt;
	f.ReadCasted<byte>(locked);
	f.ReadCasted<byte>(state);
	f >> door2;
	if(!f)
		return false;

	if(state >= Max)
	{
		Error("Invalid door state %d.", state);
		return false;
	}

	meshInst = new MeshInstance(door2 ? game_res->aDoor2 : game_res->aDoor);
	if(net->mp_load)
		meshInst->Read(f);
	meshInst->base_speed = 2.f;
	phy = new btCollisionObject;
	phy->setCollisionShape(game_level->shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

	btTransform& tr = phy->getWorldTransform();
	Vec3 phyPos = pos;
	phyPos.y += HEIGHT;
	tr.setOrigin(ToVector3(phyPos));
	tr.setRotation(btQuaternion(rot, 0, 0));
	phy_world->addCollisionObject(phy, CG_DOOR);

	if(!net->mp_load && state == Opened)
		meshInst->SetToEnd(&meshInst->mesh->anims[0]);
	if(!IsBlocking())
	{
		btVector3& phyPos = phy->getWorldTransform().getOrigin();
		phyPos.setY(phyPos.y() - 100.f);
	}

	Register();
	return true;
}

//=================================================================================================
void Door::Open()
{
	if(!game_level->location->outside)
		game_level->minimap_opened_doors = true;
	state = Opening;
	locked = LOCK_NONE;
	meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);

	if(Rand() % 2 == 0)
		sound_mgr->PlaySound3d(game_res->sDoor[Rand() % 3], GetCenter(), SOUND_DIST);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_DOOR;
		c.id = id;
		c.count = 0;
	}
}

//=================================================================================================
void Door::OpenInstant()
{
	state = Door::Opened;
	btVector3& phyPos = phy->getWorldTransform().getOrigin();
	phyPos.setY(phyPos.y() - 100.f);
	meshInst->SetToEnd(&meshInst->mesh->anims[0]);
}

//=================================================================================================
void Door::Close()
{
	state = Closing;
	meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);

	if(Rand() % 2 == 0)
	{
		Sound* sound;
		if(Rand() % 2 == 0)
			sound = game_res->sDoorClose;
		else
			sound = game_res->sDoor[Rand() % 3];
		sound_mgr->PlaySound3d(sound, GetCenter(), SOUND_DIST);
	}

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_DOOR;
		c.id = id;
		c.count = 1;
	}
}

//=================================================================================================
void Door::SetState(bool closing)
{
	bool ok = true;
	if(closing)
	{
		// closing door
		if(state == Opened)
		{
			state = Closing;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
		}
		else if(state == Opening)
		{
			state = Closing2;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
		}
		else if(state == Opening2)
		{
			state = Closing;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
		}
		else
			ok = false;
	}
	else
	{
		// opening door
		if(state == Closed)
		{
			locked = LOCK_NONE;
			state = Opening;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
		}
		else if(state == Closing)
		{
			locked = LOCK_NONE;
			state = Opening2;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
		}
		else if(state == Closing2)
		{
			locked = LOCK_NONE;
			state = Opening;
			meshInst->Play(&meshInst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
		}
		else
			ok = false;
	}

	if(ok && Rand() % 2 == 0)
	{
		Sound* sound;
		if(closing && Rand() % 2 == 0)
			sound = game_res->sDoorClose;
		else
			sound = game_res->sDoor[Rand() % 3];
		sound_mgr->PlaySound3d(sound, GetCenter(), SOUND_DIST);
	}

	if(Net::IsServer())
	{
		// send to other players
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_DOOR;
		c.id = id;
		c.count = (closing ? 1 : 0);
	}
}
