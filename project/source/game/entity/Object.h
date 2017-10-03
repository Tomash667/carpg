#pragma once

//-----------------------------------------------------------------------------
#include "BaseObject.h"
#include "Mesh.h"

//-----------------------------------------------------------------------------
// Object in game
struct Object
{
	Vec3 pos, rot;
	float scale;
	Mesh* mesh;
	BaseObject* base;
	bool require_split;

	static const int MIN_SIZE = 29;

	Object() : require_split(false)
	{
	}

	float GetRadius() const
	{
		return mesh->head.radius * scale;
	}
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
		return base && IS_SET(base->flags, OBJ_BILLBOARD);
	}
	void Save(HANDLE file);
	void Load(HANDLE file);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
};
