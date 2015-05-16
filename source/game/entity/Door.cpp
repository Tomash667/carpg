// drzwi
#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "SaveState.h"

//=================================================================================================
void Door::Save(HANDLE file, bool local)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &pt, sizeof(pt), &tmp, NULL);
	WriteFile(file, &locked, sizeof(locked), &tmp, NULL);
	WriteFile(file, &state, sizeof(state), &tmp, NULL);
	WriteFile(file, &netid, sizeof(netid), &tmp, NULL);
	WriteFile(file, &door2, sizeof(door2), &tmp, NULL);

	if(local)
		ani->Save(file);
}

//=================================================================================================
void Door::Load(HANDLE file, bool local)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &pt, sizeof(pt), &tmp, NULL);
	ReadFile(file, &locked, sizeof(locked), &tmp, NULL);
	ReadFile(file, &state, sizeof(state), &tmp, NULL);
	ReadFile(file, &netid, sizeof(netid), &tmp, NULL);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &door2, sizeof(door2), &tmp, NULL);

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
		ani = NULL;
}
