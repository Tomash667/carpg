#pragma once

//-----------------------------------------------------------------------------
#include "Music.h"

//-----------------------------------------------------------------------------
struct Texture;
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
		LoadTex,
		LoadTex2,
		LoadMesh,
		LoadVertexData,
		LoadTrap,
		LoadSound,
		LoadObject,
		LoadTexResource,
		LoadItem,
		LoadMusic
	};

	Type type;
	cstring filename;
	union
	{
		ID3DXEffect** effect;
		TEX* tex;
		Texture* tex2;
		Animesh** mesh;
		VertexData** vd;
		BaseTrap* trap;
		SOUND* sound;
		Obj* obj;
		Resource** tex_res;
		Item* item;
		Music* music;
	};

	LoadTask(cstring _filename, ID3DXEffect** _effect) : type(LoadShader), filename(_filename), effect(_effect)
	{

	}
	LoadTask(Type _type) : type(_type)
	{

	}
	LoadTask(cstring _filename, TEX* _tex) : type(LoadTex), filename(_filename), tex(_tex)
	{

	}
	LoadTask(cstring _filename, Texture* _tex2) : type(LoadTex2), filename(_filename), tex2(_tex2)
	{

	}
	LoadTask(cstring _filename, Animesh** _mesh) : type(LoadMesh), filename(_filename), mesh(_mesh)
	{

	}
	LoadTask(cstring _filename, VertexData** _vd) : type(LoadVertexData), filename(_filename), vd(_vd)
	{

	}
	LoadTask(cstring _filename, BaseTrap* _trap) : type(LoadTrap), filename(_filename), trap(_trap)
	{

	}
	LoadTask(cstring _filename, SOUND* _sound) : type(LoadSound), filename(_filename), sound(_sound)
	{

	}
	LoadTask(cstring _filename, Obj* _obj) : type(LoadObject), filename(_filename), obj(_obj)
	{

	}
	LoadTask(cstring _filename, Resource** _tex_res) : type(LoadTexResource), filename(_filename), tex_res(_tex_res)
	{

	}
	LoadTask(cstring _filename, Item* _item) : type(LoadItem), filename(_filename), item(_item)
	{

	}
	LoadTask(Music* _music) : type(LoadMusic), filename(_music->file), music(_music)
	{

	}
};

//-----------------------------------------------------------------------------
typedef vector<LoadTask> LoadTasks;
