#pragma once

#include <QuadTree.h>

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

struct LevelQuad : QuadNode
{
	bool generated;
	vector<QuadObj> objects;
	vector<Matrix> grass, grass2;
};

typedef vector<LevelQuad*> LevelQuads;
