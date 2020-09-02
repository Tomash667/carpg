#pragma once

//-----------------------------------------------------------------------------
#include "BaseObject.h"
#include "Mesh.h"

//-----------------------------------------------------------------------------
// Object in game
struct Object
{
	SceneNode* node;
	BaseObject* base;
	Vec3 pos, rot;
	float scale;
	Mesh* mesh;
	bool requireSplit;

	static const int MIN_SIZE = 32;

	Object() : node(nullptr), requireSplit(false) {}
	void Cleanup() { node = nullptr; }
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
