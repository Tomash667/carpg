#pragma once

#include "VertexDeclaration.h"

class DungeonBuilder
{
public:
	DungeonBuilder();
	~DungeonBuilder();
	void Init(CustomCollisionWorld* phy_world);
	void Build(InsideLocation* inside);
	VB GetVertexBuffer() { return mesh_vb; }
	IB GetIndexBuffer() { return mesh_ib; }

	int primitive_count, vertex_count;

private:
	void BuildPhysics(InsideLocationLevel& lvl);
	void SpawnSimpleColliders(InsideLocationLevel& lvl);
	void SpawnStairsCollider(InsideLocationLevel& lvl);
	void SpawnCollider(const Vec3& pos);
	void FillFloorAndCeiling(InsideLocationLevel& lvl);
	void FillDungeonCollider(InsideLocationLevel& lvl);
	void AddFace(const Vec3& pos, const Vec3& shift, const Vec3& shift2);
	void SpawnDungeonCollider();

	void BuildMesh(InsideLocationLevel& lvl);
	void FillMeshData(InsideLocationLevel& lvl);
	void CreateMesh();
	void PushIndices();

	CustomCollisionWorld* phy_world;
	InsideLocation* inside;
	btCollisionShape* shape_stairs, *shape_wall, *shape_stairs_part[2];
	SimpleMesh dungeon_mesh;
	btTriangleIndexVertexArray* dungeon_shape_data;
	btBvhTriangleMeshShape* dungeon_shape;
	vector<VTangent> mesh_v;
	vector<int> mesh_i;
	VB mesh_vb;
	IB mesh_ib;
	int index_counter;
};
