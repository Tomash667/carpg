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
class BaseResource
{
public:
	string path;
	cstring filename;
	ResourceState state;
	ResourceType type;
	int subtype;
	int pak_index;
	uint pak_file_index;

	inline bool IsFile() const { return pak_index == INVALID_PAK; }
	inline bool IsLoaded() const { return state == ResourceState::Loaded; }
};

//-----------------------------------------------------------------------------
template<typename T, ResourceType resType>
class Resource : public BaseResource
{
public:
	static const ResourceType Type = resType;

	T data;
};

//-----------------------------------------------------------------------------
struct Animesh;
typedef Animesh Mesh;
struct VertexData;

//-----------------------------------------------------------------------------
typedef Resource<void*, ResourceType::Unknown> AnyResource;
typedef Resource<Mesh*, ResourceType::Mesh> MeshResource;
typedef Resource<SOUND, ResourceType::Sound> SoundResource;
typedef Resource<TEX, ResourceType::Texture> TextureResource;

//-----------------------------------------------------------------------------
typedef MeshResource* MeshResourcePtr;
typedef SoundResource* SoundResourcePtr;
typedef TextureResource* TextureResourcePtr;

//-----------------------------------------------------------------------------
// Texture override data
struct TexId
{
	string id;
	TextureResource* tex;

	explicit TexId(cstring _id) : tex(nullptr)
	{
		if(_id)
			id = _id;
	}
	explicit TexId(const string& id) : id(id), tex(nullptr) {}
};
