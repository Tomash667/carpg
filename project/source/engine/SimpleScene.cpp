#include "Pch.h"
#include "EngineCore.h"
#include "SimpleScene.h"
#include "Camera2.h"

void SimpleScene::ListNodes(DrawBatch2& batch)
{
	const FrustumPlanes& frustum = camera->GetFrustum();
	for(SceneNode2* node : nodes)
	{
		if(frustum.SphereToFrustum(node->GetPos(), node->GetSphereRadius()))
		{
			batch.nodes.push_back(node);
			if(node->HaveChilds())
				ListNodes(batch, frustum, node);
		}
	}
}

void SimpleScene::ListNodes(DrawBatch2& batch, const FrustumPlanes& frustum, SceneNode2* parent)
{
	for(SceneNode2* node : parent->GetChilds())
	{
		if(frustum.SphereToFrustum(node->GetPos(), node->GetSphereRadius()))
		{
			batch.nodes.push_back(node);
			if(node->HaveChilds())
				ListNodes(batch, frustum, node);
		}
	}
}

void SimpleScene::Update(float dt)
{
	for(SceneNode2* node : nodes)
	{
		if(node->GetMeshInstance())
		{
			node->GetMeshInstance()->Update(dt);
			if(node->HaveChilds())
				Update(node, dt);
		}
	}
}
