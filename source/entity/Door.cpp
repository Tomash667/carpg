#include "Pch.h"
#include "GameCore.h"
#include "Door.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Collision.h"
#include "Level.h"
#include "GameResources.h"

EntityType<Door>::Impl EntityType<Door>::impl;
const float Door::WIDTH = 0.842f;
const float Door::THICKNESS = 0.181f;
const float Door::HEIGHT = 1.319f;
const float Door::SOUND_DIST = 1.f;
const float Door::UNLOCK_SOUND_DIST = 0.5f;
const float Door::BLOCKED_SOUND_DIST = 1.f;

//=================================================================================================
void Door::Save(FileWriter& f, bool local)
{
	f << id;
	f << pos;
	f << rot;
	f << pt;
	f << locked;
	f << state;
	f << door2;

	if(local)
		mesh_inst->Save(f);
}

//=================================================================================================
void Door::Load(FileReader& f, bool local)
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

	if(local)
	{
		mesh_inst = new MeshInstance(door2 ? game_res->aDoor2 : game_res->aDoor);
		mesh_inst->Load(f, LOAD_VERSION >= V_DEV ? 1 : 0);

		phy = new btCollisionObject;
		phy->setCollisionShape(game_level->shape_door);
		phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

		btTransform& tr = phy->getWorldTransform();
		Vec3 pos2 = pos;
		pos2.y += Door::HEIGHT;
		tr.setOrigin(ToVector3(pos2));
		tr.setRotation(btQuaternion(rot, 0, 0));
		phy_world->addCollisionObject(phy, CG_DOOR);

		if(!IsBlocking())
		{
			btVector3& poss = phy->getWorldTransform().getOrigin();
			poss.setY(poss.y() - 100.f);
			mesh_inst->SetToEnd(mesh_inst->mesh->anims[0].name.c_str());
		}
	}
	else
		mesh_inst = nullptr;
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

	if(state >= Door::Max)
	{
		Error("Invalid door state %d.", state);
		return false;
	}

	mesh_inst = new MeshInstance(door2 ? game_res->aDoor2 : game_res->aDoor);
	mesh_inst->groups[0].speed = 2.f;
	phy = new btCollisionObject;
	phy->setCollisionShape(game_level->shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

	btTransform& tr = phy->getWorldTransform();
	Vec3 pos = this->pos;
	pos.y += Door::HEIGHT;
	tr.setOrigin(ToVector3(pos));
	tr.setRotation(btQuaternion(rot, 0, 0));
	phy_world->addCollisionObject(phy, CG_DOOR);

	if(state == Door::Open)
	{
		btVector3& pos = phy->getWorldTransform().getOrigin();
		pos.setY(pos.y() - 100.f);
		mesh_inst->SetToEnd(mesh_inst->mesh->anims[0].name.c_str());
	}

	Register();
	return true;
}
