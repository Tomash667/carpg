#pragma once

struct QuadRect
{
	QuadRect()
	{

	}
	QuadRect(const BOX2D& box)
	{
		box.ToRectangle(x, y, w, h);
	}
	float x, y, w, h;
};

struct QuadNode
{
	QuadRect rect;
	BOX2D box;
	IBOX2D grid_box;
	QuadNode* childs[4];
	bool leaf;
};

typedef QuadNode* (*GetQuadNode)();

struct QuadTree
{
	typedef vector<QuadNode*> Nodes;

	QuadTree() : top(nullptr)
	{

	}

	void Init(QuadNode* node, const BOX2D& box, const IBOX2D& grid_box, int splits, float margin);

	void List(FrustumPlanes& frustum, Nodes& nodes);
	void ListLeafs(FrustumPlanes& frustum, Nodes& nodes);

	// move all nodes into vector, set top to nullptr
	void Clear(Nodes& nodes);

	QuadNode* GetNode(const VEC2& pos, float radius);

	QuadNode* top;
	GetQuadNode get;
	Nodes tmp_nodes;
};

struct LevelPart : QuadNode
{
	bool generated;
	//vector<Object> objects;
	vector<MATRIX> grass, grass2;
};

typedef vector<LevelPart*> LevelParts;
