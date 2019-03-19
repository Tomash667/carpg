#include "Pch.h"
#include "GameCore.h"
#include "Door.h"
#include "Game.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Collision.h"
#include "Level.h"

int Door::netid_counter;
const float Door::SOUND_DIST = 1.f;
const float Door::UNLOCK_SOUND_DIST = 0.5f;
const float Door::BLOCKED_SOUND_DIST = 1.f;

//=================================================================================================
void Door::Save(FileWriter& f, bool local)
{
	f << pos;
	f << rot;
	f << pt;
	f << locked;
	f << state;
	f << netid;
	f << door2;

	if(local)
		mesh_inst->Save(f);
}

//=================================================================================================
void Door::Load(FileReader& f, bool local)
{
	f >> pos;
	f >> rot;
	f >> pt;
	f >> locked;
	f >> state;
	f >> netid;
	if(LOAD_VERSION >= V_0_3)
		f >> door2;
	else
		door2 = false;

	if(local)
	{
		Game& game = Game::Get();

		mesh_inst = new MeshInstance(door2 ? game.aDoor2 : game.aDoor);
		mesh_inst->Load(f);

		phy = new btCollisionObject;
		phy->setCollisionShape(L.shape_door);
		phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

		btTransform& tr = phy->getWorldTransform();
		Vec3 pos2 = pos;
		pos2.y += 1.319f;
		tr.setOrigin(ToVector3(pos2));
		tr.setRotation(btQuaternion(rot, 0, 0));
		game.phy_world->addCollisionObject(phy, CG_DOOR);

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
	f << pos;
	f << rot;
	f << pt;
	f.WriteCasted<byte>(locked);
	f.WriteCasted<byte>(state);
	f << netid;
	f << door2;
}

//=================================================================================================
bool Door::Read(BitStreamReader& f)
{
	Game& game = Game::Get();

	f >> pos;
	f >> rot;
	f >> pt;
	f.ReadCasted<byte>(locked);
	f.ReadCasted<byte>(state);
	f >> netid;
	f >> door2;
	if(!f)
		return false;

	if(state >= Door::Max)
	{
		Error("Invalid door state %d.", state);
		return false;
	}

	mesh_inst = new MeshInstance(door2 ? game.aDoor2 : game.aDoor);
	mesh_inst->groups[0].speed = 2.f;
	phy = new btCollisionObject;
	phy->setCollisionShape(L.shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

	btTransform& tr = phy->getWorldTransform();
	Vec3 pos = this->pos;
	pos.y += 1.319f;
	tr.setOrigin(ToVector3(pos));
	tr.setRotation(btQuaternion(rot, 0, 0));
	game.phy_world->addCollisionObject(phy, CG_DOOR);

	if(state == Door::Open)
	{
		btVector3& pos = phy->getWorldTransform().getOrigin();
		pos.setY(pos.y() - 100.f);
		mesh_inst->SetToEnd(mesh_inst->mesh->anims[0].name.c_str());
	}

	return true;
}
