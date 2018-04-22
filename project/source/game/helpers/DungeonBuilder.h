#pragma once

#include "VertexDeclaration.h"

class DungeonBuilder
{
public:
	DungeonBuilder();
	~DungeonBuilder();
	void Init(CustomCollisionWorld* phy_world);
	void SpawnColliders(InsideLocation* inside);

private:
	void SpawnSimpleColliders(InsideLocationLevel& lvl);
	void SpawnStairsCollider(InsideLocationLevel& lvl);
	void SpawnCollider(const Vec3& pos);
	void SpawnFloorAndCeiling(InsideLocationLevel& lvl);
	void SpawnNewColliders(InsideLocationLevel& lvl);
	void SpawnDungeonTrimesh();
	void AddFace(const Vec3& pos, const Vec3& shift, const Vec3& shift2, int& index);

	CustomCollisionWorld* phy_world;
	InsideLocation* inside;
	btCollisionShape* shape_stairs, *shape_wall, *shape_stairs_part[2];
	SimpleMesh dungeon_mesh;
	btTriangleIndexVertexArray* dungeon_shape_data;
	btBvhTriangleMeshShape* dungeon_shape;
};
