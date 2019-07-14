#pragma once

struct Object;

struct QuadObj
{
	enum Type
	{
		OBJECT
	} type;
	union
	{
		Object* obj;
		void* ptr;
	};

	explicit QuadObj(Object* obj) : obj(obj), type(OBJECT) {}
};

struct QuadNode
{
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

	void Init(QuadNode* node, const Box2d& box, const Rect& grid_box, int splits);

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
	vector<QuadObj> objects;
	vector<Matrix> grass, grass2;
};

typedef vector<LevelPart*> LevelParts;
