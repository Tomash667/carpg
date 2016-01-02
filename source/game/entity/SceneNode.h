#pragma once

//-----------------------------------------------------------------------------
#define SPLIT_INDEX (1<<31)

//-----------------------------------------------------------------------------
struct SceneNode
{
	enum Flags
	{
		F_CUSTOM = 1<<0,
		F_ANIMATED = 1<<1,
		F_SPECULAR_MAP = 1<<2,
		F_BINORMALS = 1<<3,
		F_NORMAL_MAP = 1<<4,
		F_NO_ZWRITE = 1<<5,
		F_NO_CULLING = 1<<6,
		F_ALPHA_TEST = 1<<7
	};

	MATRIX mat;
	union
	{
		Animesh* mesh;
		AnimeshInstance* ani;
	};
	AnimeshInstance* parent_ani;
	int flags, lights, subs;
	const TexId* tex_override;
	VEC4 tint;
	bool billboard;

	inline const Animesh& GetMesh() const
	{
		if(!IS_SET(flags, F_ANIMATED) || parent_ani)
			return *mesh;
		else
			return *ani->ani;
	}

	inline const AnimeshInstance& GetAnimesh() const
	{
		assert(IS_SET(flags, F_ANIMATED));
		if(!parent_ani)
			return *ani;
		else
			return *parent_ani;
	}
};

//-----------------------------------------------------------------------------
struct DebugSceneNode
{
	enum Type
	{
		Box,
		Cylinder,
		Sphere,
		Capsule,
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
	MATRIX mat;
};

//-----------------------------------------------------------------------------
struct GlowNode
{
	enum Type
	{
		Item,
		Useable,
		Unit,
		Door,
		Chest
	} type;
	SceneNode* node;
	void* ptr;
};

//-----------------------------------------------------------------------------
struct Billboard
{
	TEX tex;
	VEC3 pos;
	float size;
};

//-----------------------------------------------------------------------------
struct Area
{
	VEC3 v[4];
};

//-----------------------------------------------------------------------------
struct LightData
{
	VEC3 pos;
	float range;
	VEC3 color;
	float _pad;
};

//-----------------------------------------------------------------------------
struct Lights
{
	LightData ld[3];
};

//-----------------------------------------------------------------------------
struct TexturePack
{
	TextureResourcePtr diffuse, normal, specular;

	inline int GetIndex() const
	{
		return (normal ? 2 : 0) + (specular ? 1 : 0);
	}
};

//-----------------------------------------------------------------------------
struct DungeonPart
{
	enum Flags
	{
		F_SPECULAR = 1<<1,
		F_NORMAL = 1<<2
	};
	int lights, start_index, primitive_count, matrix;
	TexturePack* tp;
};

//-----------------------------------------------------------------------------
struct NodeMatrix
{
	MATRIX matWorld;
	MATRIX matCombined;
};

//-----------------------------------------------------------------------------
struct DrawBatch
{
	vector<SceneNode*> nodes;
	vector<DebugSceneNode*> debug_nodes;
	vector<GlowNode> glow_nodes;
	vector<uint> terrain_parts;
	vector<Blood*> bloods;
	vector<Billboard> billboards;
	vector<Explo*> explos;
	vector<ParticleEmitter*> pes;
	vector<TrailParticleEmitter*>* tpes;
	vector<Electro*>* electros;
	vector<Portal*> portals;
	vector<Area> areas;
	float area_range;
	vector<Lights> lights;
	vector<DungeonPart> dungeon_parts;
	vector<NodeMatrix> matrices;

	void Clear();
};
