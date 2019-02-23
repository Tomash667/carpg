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
	VertexData,
	SoundOrMusic
};

//-----------------------------------------------------------------------------
struct Resource
{
	string path;
	cstring filename;
	ResourceState state;
	ResourceType type;
	int pak_index;
	uint pak_file_index;

	static const int INVALID_PAK = -1;

	virtual ~Resource()
	{
	}
	bool IsFile() const { return pak_index == INVALID_PAK; }
	bool IsLoaded() const { return state == ResourceState::Loaded; }
};
