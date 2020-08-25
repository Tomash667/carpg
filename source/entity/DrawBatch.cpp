#include "Pch.h"
#include "DrawBatch.h"

#include <DebugNode.h>
#include <ParticleSystem.h>

ObjectPool<Light> DrawBatch::light_pool;

//=================================================================================================
void DrawBatch::Clear()
{
	for(SceneNode* node : nodes)
	{
		if(node->tmp)
			node->Free();
	}
	nodes.clear();
	for(SceneNode* node : alpha_nodes)
	{
		if(node->tmp)
			node->Free();
	}
	alpha_nodes.clear();
	node_groups.clear();
	DebugNode::Free(debug_nodes);
	glow_nodes.clear();
	terrain_parts.clear();
	bloods.clear();
	billboards.clear();
	pes.clear();
	areas.clear();
	Area2::Free(areas2);
	dungeon_parts.clear();
	dungeon_part_groups.clear();
	light_pool.Free(tmp_lights);
}
