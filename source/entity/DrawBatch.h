#pragma once

//-----------------------------------------------------------------------------
#include "SceneNode.h"
#include "Light.h"

//-----------------------------------------------------------------------------
struct Area
{
	Vec3 v[4];
};

//-----------------------------------------------------------------------------
struct Area2 : public ObjectPoolProxy<Area2>
{
	vector<Vec3> points;
	vector<word> faces;
	int ok;
};

//-----------------------------------------------------------------------------
struct DungeonPart
{
	uint group, startIndex, primitiveCount;
	TexOverride* texOverride;
};

//-----------------------------------------------------------------------------
struct DungeonPartGroup
{
	Matrix matWorld;
	Matrix matCombined;
	array<Light*, 3> lights;
};

//-----------------------------------------------------------------------------
struct DrawBatch : public SceneBatch
{
	static ObjectPool<Light> lightPool;

	LocationPart* locPart;
	vector<DebugNode*> debugNodes;
	vector<GlowNode> glowNodes;
	vector<uint> terrainParts;
	vector<Blood*> bloods;
	vector<Billboard> billboards;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*>* tpes;
	vector<Area> areas;
	float areaRange;
	vector<Area2*> areas2;
	vector<DungeonPart> dungeonParts;
	vector<DungeonPartGroup> dungeonPartGroups;
	vector<Light*> tmpLights;
	SceneNode* tmpGlow;

	void Clear();
};
