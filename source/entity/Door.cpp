#include "Pch.h"
#include "Door.h"

#include "BitStreamFunc.h"
#include "Collision.h"
#include "GameResources.h"
#include "Level.h"
#include "Location.h"
#include "Net.h"
#include "SceneNodeHelper.h"
#include "Unit.h"

#include <SceneNode.h>
#include <SoundManager.h>

EntityType<Door>::Impl EntityType<Door>::impl;
const float Door::WIDTH = 0.842f;
const float Door::THICKNESS = 0.181f;
const float Door::HEIGHT = 1.319f;
const float Door::SOUND_DIST = 1.f;
const float Door::UNLOCK_SOUND_DIST = 0.5f;
const float Door::BLOCKED_SOUND_DIST = 1.f;

//=================================================================================================
void Door::Init()
{
	Register();
	CreatePhysics();
}

//=================================================================================================
void Door::CreatePhysics()
{
	phy = new btCollisionObject;
	phy->setCollisionShape(game_level->shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
	btTransform& tr = phy->getWorldTransform();
	Vec3 phyPos = pos;
	phyPos.y += HEIGHT;
	tr.setOrigin(ToVector3(phyPos));
	tr.setRotation(btQuaternion(rot, 0, 0));
	phy_world->addCollisionObject(phy, CG_DOOR);
	if(state == Opened)
	{
		btVector3& phyPos = phy->getWorldTransform().getOrigin();
		phyPos.setY(phyPos.y() - 100.f);
	}
}

//=================================================================================================
void Door::Cleanup()
{
	if(state == Closing || state == Closing2)
		state = Closed;
	else if(state == Opening || state == Opening2)
		state = Opened;
	node = nullptr;
}

//=================================================================================================
void Door::Update(float dt, LevelArea& area)
{
	MeshInstance* meshInst = node->mesh_inst;
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

				for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
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
				meshInst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_NO_BLEND | PLAY_STOP_AT_END, 0);
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
		SceneNodeHelper::Save(node, f);
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
		if(LOAD_VERSION >= V_DEV)
			node = SceneNodeHelper::Load(f);
		else
			node = SceneNodeHelper::old::Load(f, LOAD_VERSION >= V_0_13 ? 1 : 0);

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
		SceneNodeHelper::Save(node, f);
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

	if(net->mp_load)
		node = SceneNodeHelper::Load(f);

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
	node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);

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
	node->mesh_inst->SetToEnd(&node->mesh->anims[0]);
}

//=================================================================================================
void Door::Close()
{
	state = Closing;
	node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);

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
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
		}
		else if(state == Opening)
		{
			state = Closing2;
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
		}
		else if(state == Opening2)
		{
			state = Closing;
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
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
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
		}
		else if(state == Closing)
		{
			locked = LOCK_NONE;
			state = Opening2;
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
		}
		else if(state == Closing2)
		{
			locked = LOCK_NONE;
			state = Opening;
			node->mesh_inst->Play(&node->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END, 0);
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
