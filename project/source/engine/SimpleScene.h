#pragma once

#include "Scene2.h"

class SimpleScene final : public Scene2
{
public:
	void ListNodes(DrawBatch2& batch) override;
	void Update(float dt) override;
	void AddNode(SceneNode2* node) override
	{
		assert(node);
		nodes.push_back(node);
	}
	void RemoveNode(SceneNode2* node) override
	{
		RemoveElementTry(nodes node);
	}

private:
	void ListNodes(DrawBatch2& batch, const FrustumPlanes& frustum, SceneNode2* parent);
	void Update(SceneNode2* parent, float dt);

	vector<SceneNode2*> nodes;
};
