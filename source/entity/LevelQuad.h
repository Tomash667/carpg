#pragma once

#include <QuadTree.h>

struct LevelQuad : QuadTree::Node
{
	bool generated;
	vector<Matrix> grass, grass2;
};

typedef vector<LevelQuad*> LevelQuads;
