#pragma once

//-----------------------------------------------------------------------------
// Zasób
struct Resource
{
	enum State
	{
		NOT_LOADED,
		LOADING,
		LOADED,
		RELEASING
	};

	enum Type
	{
		MESH,
		TEXTURE,
		SOUND,
		MUSIC
	};

	string filename, path;
	State state;
	Type type;
	int refs, task;
	void* ptr;
};

//-----------------------------------------------------------------------------
// Tekstura
struct Texture
{
	Resource* res;

	Texture()
	{

	}

	Texture(Resource* _res) : res(_res)
	{

	}

	inline operator bool() const
	{
		return res != NULL;
	}

	inline TEX Get() const
	{
		return res ? (TEX)res->ptr : NULL;
	}
};

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	Resource* res;

	TexId(cstring _id) : res(NULL)
	{
		if(_id)
			id = _id;
	}
	TexId(const string& id) : id(id), res(NULL) {}
};
