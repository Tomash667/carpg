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
		LoadTex,
		LoadTexResource,
		LoadMesh,
		LoadVertexData,
		LoadTrap,
		LoadSound,
		LoadObject,
		LoadItem,
		LoadMusic
	};

	Type type;
	cstring filename;
	union
	{
		ID3DXEffect** effect;
		TEX* tex;
		Animesh** mesh;
		VertexData** vd;
		BaseTrap* trap;
		SOUND* sound;
		Obj* obj;
		TextureResource** tex_res;
		Item* item;
		Music* music;
	};

	LoadTask(cstring filename, ID3DXEffect** effect) : type(LoadShader), filename(filename), effect(effect) {}
	LoadTask(Type type) : type(type) {}
	LoadTask(cstring filename, TEX* tex) : type(LoadTex), filename(filename), tex(tex) {}
	LoadTask(cstring filename, TextureResource** tex_res) : type(LoadTexResource), filename(filename), tex_res(tex_res) {}
	LoadTask(cstring filename, Animesh** mesh) : type(LoadMesh), filename(filename), mesh(mesh) {}
	LoadTask(cstring filename, VertexData** vd) : type(LoadVertexData), filename(filename), vd(vd) {}
	LoadTask(cstring filename, BaseTrap* trap) : type(LoadTrap), filename(filename), trap(trap) {}
	LoadTask(cstring filename, SOUND* sound) : type(LoadSound), filename(filename), sound(sound) {}
	LoadTask(cstring filename, Obj* obj) : type(LoadObject), filename(filename), obj(obj) {}
	LoadTask(cstring filename, Item* item) : type(LoadItem), filename(filename), item(item) {}
	LoadTask(Music* music) : type(LoadMusic), filename(music->file), music(music) {}
};

//-----------------------------------------------------------------------------
typedef vector<LoadTask>& LoadTasks;
