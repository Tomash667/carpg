#include "Pch.h"
#include "GameCore.h"
#include "Door.h"
#include "Game.h"
#include "SaveState.h"

int Door::netid_counter;

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
		phy->setCollisionShape(game.shape_door);
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
