#pragma once

#include "VertexDeclaration.h"

class DungeonBuilder
{
public:
	DungeonBuilder();
	~DungeonBuilder();
	void Init(CustomCollisionWorld* phy_world);
	void Setup(InsideLocation* inside);
	void SpawnColliders();
	void GenerateMesh();
	VB GetVertexBuffer() { return mesh_vb; }
	IB GetIndexBuffer() { return mesh_ib; }

	int primitive_count, vertex_count;

private:
	void SpawnSimpleColliders(InsideLocationLevel& lvl);
	void SpawnStairsCollider(InsideLocationLevel& lvl);
	void SpawnCollider(const Vec3& pos);
	void SpawnFloorAndCeiling(InsideLocationLevel& lvl);
	void SpawnNewColliders(InsideLocationLevel& lvl);
	void SpawnDungeonTrimesh();
	void AddFace(const Vec3& pos, const Vec3& shift, const Vec3& shift2, int& index);
	void BuildMesh(InsideLocationLevel& lvl);

	CustomCollisionWorld* phy_world;
	InsideLocation* inside;
	btCollisionShape* shape_stairs, *shape_wall, *shape_stairs_part[2];
	SimpleMesh dungeon_mesh;
	btTriangleIndexVertexArray* dungeon_shape_data;
	btBvhTriangleMeshShape* dungeon_shape;
	vector<VTangent> mesh_v;
	vector<word> mesh_i;
	VB mesh_vb;
	IB mesh_ib;
};
