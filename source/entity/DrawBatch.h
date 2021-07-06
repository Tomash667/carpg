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
	uint group, start_index, primitive_count;
	TexOverride* tex_o;
};

//-----------------------------------------------------------------------------
struct DungeonPartGroup
{
	Matrix mat_world;
	Matrix mat_combined;
	array<Light*, 3> lights;
};

//-----------------------------------------------------------------------------
struct DrawBatch : public SceneBatch
{
	static ObjectPool<Light> light_pool;

	LocationPart* locPart;
	vector<DebugNode*> debug_nodes;
	vector<GlowNode> glow_nodes;
	vector<uint> terrain_parts;
	vector<Blood*> bloods;
	vector<Billboard> billboards;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*>* tpes;
	vector<Area> areas;
	float area_range;
	vector<Area2*> areas2;
	vector<DungeonPart> dungeon_parts;
	vector<DungeonPartGroup> dungeon_part_groups;
	vector<Light*> tmp_lights;
	SceneNode* tmpGlow;

	void Clear();
};
