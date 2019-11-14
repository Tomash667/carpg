#pragma once

//-----------------------------------------------------------------------------
#include <SceneNode.h>
#include <DebugSceneNode.h>

//-----------------------------------------------------------------------------
struct GlowNode
{
	enum Type
	{
		Item,
		Usable,
		Unit,
		Door,
		Chest
	} type;
	SceneNode* node;
	void* ptr;
	bool alpha;
};

//-----------------------------------------------------------------------------
struct Billboard
{
	TEX tex;
	Vec3 pos;
	float size;
};

//-----------------------------------------------------------------------------
struct Area
{
	Vec3 v[4];
};

//-----------------------------------------------------------------------------
struct Area2
{
	vector<Vec3> points;
	vector<word> faces;
	int ok;
};

//-----------------------------------------------------------------------------
struct LightData
{
	Vec3 pos;
	float range;
	Vec3 color;
	float _pad;
};

//-----------------------------------------------------------------------------
struct Lights
{
	LightData ld[3];
};

//-----------------------------------------------------------------------------
struct DungeonPart
{
	enum Flags
	{
		F_SPECULAR = 1 << 1,
		F_NORMAL = 1 << 2
	};
	int lights, start_index, primitive_count, matrix;
	TexOverride* tex_o;
};

//-----------------------------------------------------------------------------
struct NodeMatrix
{
	Matrix matWorld;
	Matrix matCombined;
};

//-----------------------------------------------------------------------------
struct DrawBatch
{
	vector<SceneNode*> nodes, alpha_nodes;
	vector<SceneNodeGroup> groups;
	vector<DebugSceneNode*> debug_nodes;
	vector<GlowNode> glow_nodes;
	vector<uint> terrain_parts;
	vector<Blood*> bloods;
	vector<Billboard> billboards;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*>* tpes;
	vector<Area> areas;
	float area_range;
	vector<Area2*> areas2;
	vector<Lights> lights;
	vector<DungeonPart> dungeon_parts;
	vector<NodeMatrix> matrices;

	void Clear();
};
