#pragma once

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
struct VertexData
{
	vector<Vec3> verts;
	vector<Face> faces;
	float radius;
};
