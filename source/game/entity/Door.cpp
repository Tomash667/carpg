// drzwi
#include "Pch.h"
#include "Base.h"
#include "Door.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
void Door::Save(HANDLE file, bool local)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &pt, sizeof(pt), &tmp, nullptr);
	WriteFile(file, &locked, sizeof(locked), &tmp, nullptr);
	WriteFile(file, &state, sizeof(state), &tmp, nullptr);
	WriteFile(file, &netid, sizeof(netid), &tmp, nullptr);
	WriteFile(file, &door2, sizeof(door2), &tmp, nullptr);

	if(local)
		ani->Save(file);
}

//=================================================================================================
void Door::Load(HANDLE file, bool local)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &pt, sizeof(pt), &tmp, nullptr);
	ReadFile(file, &locked, sizeof(locked), &tmp, nullptr);
	ReadFile(file, &state, sizeof(state), &tmp, nullptr);
	ReadFile(file, &netid, sizeof(netid), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &door2, sizeof(door2), &tmp, nullptr);

	if(local)
	{
		Game& game = Game::Get();

		ani = new AnimeshInstance(door2 ? game.aDrzwi2 : game.aDrzwi);
		ani->Load(file);

		phy = new btCollisionObject;
		phy->setCollisionShape(game.shape_door);

		btTransform& tr = phy->getWorldTransform();
		VEC3 pos2 = pos;
		pos2.y += 1.319f;
		tr.setOrigin(ToVector3(pos2));
		tr.setRotation(btQuaternion(rot, 0, 0));
		game.phy_world->addCollisionObject(phy);

		if(!IsBlocking())
		{
			btVector3& poss = phy->getWorldTransform().getOrigin();
			poss.setY(poss.y() - 100.f);
			ani->SetToEnd(ani->ani->anims[0].name.c_str());
		}
	}
	else
		ani = nullptr;
}
