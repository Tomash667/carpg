#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Texture : public Resource
{
	TEX tex;

	Texture() : tex(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	TexturePtr tex;

	explicit TexId(cstring _id) : tex(nullptr)
	{
		if(_id)
			id = _id;
	}
	explicit TexId(const string& id) : id(id), tex(nullptr) {}
};

//-----------------------------------------------------------------------------
struct TextureLock
{
	TextureLock(TEX tex);
	~TextureLock();
	uint* operator [] (uint row) { return (uint*)(data + pitch * row); }
	void GenerateMipSubLevels();

private:
	TEX tex;
	byte* data;
	int pitch;
};
