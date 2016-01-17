#pragma once

//-----------------------------------------------------------------------------
#include "Music.h"
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Animesh;
struct VertexData;
struct BaseTrap;
struct Obj;
struct Item;

//-----------------------------------------------------------------------------
struct LoadTask
{
	enum Type
	{
		LoadShader,
		SetupShaders,
	};

	Type type;
	cstring filename;
	union
	{
		ID3DXEffect** effect;
	};

	LoadTask(cstring filename, ID3DXEffect** effect) : type(LoadShader), filename(filename), effect(effect) {}
	LoadTask(Type type) : type(type) {}
};
