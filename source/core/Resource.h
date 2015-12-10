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
		return res != nullptr;
	}

	inline TEX Get() const
	{
		return res ? (TEX)res->ptr : nullptr;
	}
};

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	Resource* res;

	explicit TexId(cstring _id) : res(nullptr)
	{
		if(_id)
			id = _id;
	}
	explicit TexId(const string& id) : id(id), res(nullptr) {}
};
