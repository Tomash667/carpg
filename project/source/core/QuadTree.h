#pragma once

struct QuadRect
{
	QuadRect()
	{
	}
	QuadRect(const Box2d& box)
	{
		box.ToRectangle(x, y, w, h);
	}
	float x, y, w, h;
};

struct QuadObj
{
	enum Type
	{
		UNIT
	} type;
	union
	{
		void* ptr;
	};
};

struct QuadNode
{
	QuadRect rect;
	Box2d box;
	Rect grid_box;
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

	void Init(QuadNode* node, const Box2d& box, const Rect& grid_box, int splits, float margin);

	void List(FrustumPlanes& frustum, Nodes& nodes);
	void ListLeafs(FrustumPlanes& frustum, Nodes& nodes);

	// move all nodes into vector, set top to nullptr
	void Clear(Nodes& nodes);

	QuadNode* GetNode(const Vec2& pos, float radius);

	QuadNode* top;
	GetQuadNode get;
	Nodes tmp_nodes;
};

struct LevelPart : QuadNode
{
	bool generated;
	//vector<Object> objects;
	vector<Matrix> grass, grass2;
};

typedef vector<LevelPart*> LevelParts;
