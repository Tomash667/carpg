#pragma once

#include "Scene2.h"

class SimpleScene : public Scene2
{
public:
	void AddNode(SceneNode2* node)
	{
		assert(node);
		nodes.push_back(node);
	}

private:
	vector<SceneNode2*> nodes;
};
