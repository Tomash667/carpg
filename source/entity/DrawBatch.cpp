#include "Pch.h"
#include "DrawBatch.h"

#include <DebugNode.h>
#include <ParticleSystem.h>

ObjectPool<Light> DrawBatch::lightPool;

//=================================================================================================
void DrawBatch::Clear()
{
	RemoveElements(nodes, [](SceneNode* node) { return node->persistent; });
	RemoveElements(alphaNodes, [](SceneNode* node) { return node->persistent; });
	SceneNode::Free(nodes);
	SceneNode::Free(alphaNodes);
	nodeGroups.clear();
	DebugNode::Free(debugNodes);
	glowNodes.clear();
	terrainParts.clear();
	bloods.clear();
	billboards.clear();
	pes.clear();
	areas.clear();
	Area2::Free(areas2);
	dungeonParts.clear();
	dungeonPartGroups.clear();
	lightPool.Free(tmpLights);
}
