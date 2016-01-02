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

enum class ResourceState
{
	NotLoaded,
	Loaded,
	Missing
};

enum class ResourceType
{
	Unknown,
	Texture,
	Mesh,
	Sound
};

template<typename T, ResourceType resType>
class Resource3
{
public:
	static const ResourceType Type = resType;

	string path;
	cstring filename;
	ResourceState state;
	ResourceType type;
	T data;
};

struct Animesh;
typedef Animesh Mesh;

typedef Resource3<void*, ResourceType::Unknown> BaseResource;
typedef Resource3<TEX, ResourceType::Texture> TextureResource;
typedef Resource3<Mesh*, ResourceType::Mesh> MeshResource;
typedef Resource3<SOUND, ResourceType::Sound> SoundResource;

typedef TextureResource* TextureResourcePtr;

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	TextureResource* res;

	explicit TexId(cstring _id) : res(nullptr)
	{
		if(_id)
			id = _id;
	}
	explicit TexId(const string& id) : id(id), res(nullptr) {}
};
