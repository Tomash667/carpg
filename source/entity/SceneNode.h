#pragma once

//-----------------------------------------------------------------------------
#include "MeshInstance.h"

//-----------------------------------------------------------------------------
struct SceneNode : public ObjectPoolProxy<SceneNode>
{
	static constexpr int SPLIT_INDEX = 1 << 31;
	static constexpr int SPLIT_MASK = 0x7FFFFFFF;

	enum Flags
	{
		F_CUSTOM = 1 << 0,
		F_ANIMATED = 1 << 1,
		F_SPECULAR_MAP = 1 << 2,
		F_TANGENTS = 1 << 3,
		F_NORMAL_MAP = 1 << 4,
		F_NO_ZWRITE = 1 << 5,
		F_NO_CULLING = 1 << 6,
		F_NO_LIGHTING = 1 << 7,
		F_ALPHA_TEST = 1 << 8,
		F_ALPHA_BLEND = 1 << 9
	};

	Matrix mat;
	union
	{
		Mesh* mesh;
		MeshInstance* mesh_inst;
	};
	MeshInstance* parent_mesh_inst;
	int flags, lights, subs;
	float dist;
	const TexOverride* tex_override;
	Vec4 tint;
	Vec3 pos;
	bool billboard;

	const Mesh& GetMesh() const
	{
		if(!IsSet(flags, F_ANIMATED) || parent_mesh_inst)
			return *mesh;
		else
			return *mesh_inst->mesh;
	}

	const MeshInstance& GetMeshInstance() const
	{
		assert(IsSet(flags, F_ANIMATED));
		if(!parent_mesh_inst)
			return *mesh_inst;
		else
			return *parent_mesh_inst;
	}
};

//-----------------------------------------------------------------------------
struct SceneNodeGroup
{
	int flags, start, end;
};

//-----------------------------------------------------------------------------
struct DebugSceneNode : public ObjectPoolProxy<DebugSceneNode>
{
	enum Type
	{
		Box,
		Cylinder,
		Sphere,
		Capsule,
		TriMesh,
		MaxType
	} type;
	enum Group
	{
		Hitbox,
		UnitRadius,
		ParticleRadius,
		Collider,
		Physic,
		MaxGroup
	} group;
	Matrix mat;
	void* mesh_ptr;
};

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
	vector<SceneNode*> nodes;
	vector<SceneNode*> alpha_nodes;
	vector<SceneNodeGroup> node_groups;
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
	Camera* camera;
	bool use_specularmap, use_normalmap;

	void Clear();
	void Add(SceneNode* node, int sub = -1);
	void Process();
};
