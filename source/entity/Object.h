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
	bool require_split;

	static const int MIN_SIZE = 29;

	Object() : node(nullptr), require_split(false) {}
	float GetRadius() const { return mesh->head.radius * scale; }
	// -1 - nie, 0 - tak, 1 - tak i bez cullingu
	int RequireAlphaTest() const
	{
		if(!base)
			return -1;
		else
			return base->alpha;
	}
	bool IsBillboard() const
	{
		return base && IsSet(base->flags, OBJ_BILLBOARD);
	}
	void CreateNode(Scene* scene);
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	bool Read(BitStreamReader& f);
};
