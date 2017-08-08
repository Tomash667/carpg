#pragma once

//-----------------------------------------------------------------------------
enum class ResourceState
{
	NotLoaded,
	Loading,
	Loaded
};

//-----------------------------------------------------------------------------
enum class ResourceType
{
	Unknown,
	Texture,
	Mesh,
	Sound
};

//-----------------------------------------------------------------------------
const int INVALID_PAK = -1;

//-----------------------------------------------------------------------------
struct Resource
{
	string path;
	cstring filename;
	ResourceState state;
	ResourceType type;
	int subtype;
	int pak_index;
	uint pak_file_index;

	bool IsFile() const { return pak_index == INVALID_PAK; }
	bool IsLoaded() const { return state == ResourceState::Loaded; }
};

//-----------------------------------------------------------------------------
struct Texture : public Resource
{
	TEX tex;
};
typedef Texture* TexturePtr;

//-----------------------------------------------------------------------------
struct Sound : public Resource
{
	SOUND sound;
	bool is_music;
};
typedef Sound* SoundPtr;

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	Texture* tex;

	explicit TexId(cstring _id) : tex(nullptr)
	{
		if(_id)
			id = _id;
	}
	explicit TexId(const string& id) : id(id), tex(nullptr) {}
};
