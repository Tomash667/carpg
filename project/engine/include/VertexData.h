#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Face
{
	word idx[3];

	word operator [] (int n) const
	{
		return idx[n];
	}
};

//-----------------------------------------------------------------------------
struct VertexData : Resource
{
	vector<Vec3> verts;
	vector<Face> faces;
	float radius;

	bool RayToMesh(const Vec3& ray_pos, const Vec3& ray_dir, const Vec3& obj_pos, float obj_rot, float& out_dist) const;
};
