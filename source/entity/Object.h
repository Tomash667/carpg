#pragma once

//-----------------------------------------------------------------------------
#include "BaseObject.h"
#include "Mesh.h"

//-----------------------------------------------------------------------------
// Object in game
struct Object
{
	BaseObject* base;
	Vec3 pos, rot;
	float scale;
	Mesh* mesh;
	union
	{
		MeshInstance* meshInst;
		float time;
	};
	bool requireSplit;

	static const int MIN_SIZE = 29;

	Object() : meshInst(nullptr), requireSplit(false)
	{
	}
	~Object();

	float GetRadius() const
	{
		return mesh->head.radius * scale;
	}
	bool RequireNoCulling() const
	{
		if(base)
			return IsSet(base->flags, OBJ_NO_CULLING);
		return false;
	}
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);
};
