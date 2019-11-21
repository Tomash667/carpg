#include "Pch.h"
#include "GameCore.h"
#include "GameFile.h"
#include "Door.h"
#include "SaveState.h"
#include "BitStreamFunc.h"
#include "Collision.h"
#include "Level.h"
#include "GameResources.h"
#include "Location.h"
#include "SoundManager.h"
#include "Net.h"
#include <Scene.h>

EntityType<Door>::Impl EntityType<Door>::impl;
const float Door::WIDTH = 0.842f;
const float Door::THICKNESS = 0.181f;
const float Door::HEIGHT = 1.319f;
const float Door::SOUND_DIST = 1.f;
const float Door::UNLOCK_SOUND_DIST = 0.5f;
const float Door::BLOCKED_SOUND_DIST = 1.f;

//=================================================================================================
void Door::CreateNode(Scene* scene)
{
	// scene node
	node = SceneNode::Get();
	node->mat = Matrix::RotationY(rot) * Matrix::Translation(pos);
	node->mesh = door2 ? game_res->aDoor2 : game_res->aDoor;
	node->mesh_inst = new MeshInstance(node->mesh);
	node->mesh_inst->base_speed = 2.f;
	node->flags = SceneNode::F_ANIMATED;
	node->type = SceneNode::NORMAL;
	scene->Add(node);

	// physics
	phy = new btCollisionObject;
	phy->setCollisionShape(game_level->shape_door);
	phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);

	btTransform& tr = phy->getWorldTransform();
	Vec3 pos2 = pos;
	pos2.y += HEIGHT;
	tr.setOrigin(ToVector3(pos2));
	tr.setRotation(btQuaternion(rot, 0, 0));
	phy_world->addCollisionObject(phy, CG_DOOR);

	// opened animation/physics
	if(!IsBlocking())
	{
		btVector3& poss = phy->getWorldTransform().getOrigin();
		poss.setY(poss.y() - 100.f);
		node->mesh_inst->SetToEnd(&node->mesh->anims[0]);
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

	if(f.is_local)
		node->mesh_inst->Save(f);
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

	if(f.is_local)
	{
		CreateNode(f.area->scene);
		node->mesh_inst->Load(f, LOAD_VERSION >= V_DEV ? 1 : 0);
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

	CreateNode(f.area->scene);
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
