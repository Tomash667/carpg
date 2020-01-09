#include "Pch.h"
#include "GameCore.h"
#include "SceneNode.h"
#include "Camera.h"
#include "ParticleSystem.h"
#include "ResourceManager.h"

//=================================================================================================
void DrawBatch::Clear()
{
	SceneNode::Free(nodes);
	SceneNode::Free(alpha_nodes);
	node_groups.clear();
	DebugSceneNode::Free(debug_nodes);
	glow_nodes.clear();
	terrain_parts.clear();
	bloods.clear();
	billboards.clear();
	pes.clear();
	areas.clear();
	Area2::Free(areas2);
	lights.clear();
	dungeon_parts.clear();
	matrices.clear();

	// empty lights
	Lights& l = Add1(lights);
	for(int i = 0; i < 3; ++i)
	{
		l.ld[i].range = 1.f;
		l.ld[i].pos = Vec3(0, -1000, 0);
		l.ld[i].color = Vec3(0, 0, 0);
	}
}

//=================================================================================================
void DrawBatch::Add(SceneNode* node, int sub)
{
	assert(node);

	const Mesh& mesh = node->GetMesh();
	if(mesh.state != ResourceState::Loaded)
		res_mgr->LoadInstant(const_cast<Mesh*>(&mesh));

	if(sub == -1)
	{
		assert(mesh.head.n_subs < 31);
		if(IsSet(mesh.head.flags, Mesh::F_TANGENTS))
			node->flags |= SceneNode::F_TANGENTS;
		if(use_normalmap && IsSet(mesh.head.flags, Mesh::F_NORMAL_MAP))
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(use_specularmap && IsSet(mesh.head.flags, Mesh::F_SPECULAR_MAP))
			node->flags |= SceneNode::F_SPECULAR_MAP;
		node->subs = SceneNode::SPLIT_MASK;
	}
	else
	{
		if(use_normalmap && mesh.subs[sub].tex_normal)
			node->flags |= SceneNode::F_NORMAL_MAP;
		if(use_specularmap && mesh.subs[sub].tex_specular)
			node->flags |= SceneNode::F_SPECULAR_MAP;
		node->subs = SceneNode::SPLIT_INDEX | sub;
	}

	if(IsSet(node->flags, SceneNode::F_ALPHA_BLEND))
	{
		node->dist = Vec3::DistanceSquared(node->pos, camera->from);
		alpha_nodes.push_back(node);
	}
	else
		nodes.push_back(node);
}

//=================================================================================================
void DrawBatch::Process()
{
	node_groups.clear();
	if(!nodes.empty())
	{
		// sort nodes
		std::sort(nodes.begin(), nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
		{
			if(node1->flags == node2->flags)
				return node1->mesh_inst > node2->mesh_inst;
			else
				return node1->flags < node2->flags;
		});

		// group nodes
		int prev_flags = -1, index = 0;
		for(SceneNode* node : nodes)
		{
			if(node->flags != prev_flags)
			{
				if(!node_groups.empty())
					node_groups.back().end = index - 1;
				node_groups.push_back({ node->flags, index, 0 });
				prev_flags = node->flags;
			}
			++index;
		}
		node_groups.back().end = index - 1;
	}

	// sort alpha nodes
	std::sort(alpha_nodes.begin(), alpha_nodes.end(), [](const SceneNode* node1, const SceneNode* node2)
	{
		return node1->dist > node2->dist;
	});
}
