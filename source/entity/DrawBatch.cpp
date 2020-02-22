#include "Pch.h"
#include "DrawBatch.h"
#include <ParticleSystem.h>

ObjectPool<Light> DrawBatch::light_pool;

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
	dungeon_parts.clear();
	dungeon_part_groups.clear();
	light_pool.Free(tmp_lights);
}
