#pragma once

//-----------------------------------------------------------------------------
struct Explo
{
	LevelArea* area;
	//SceneNode* node;
	Entity<Unit> owner;
	Vec3 pos;
	float size, sizemax, dmg;
	Ability* ability;
	vector<Entity<Unit>> hitted;
};
