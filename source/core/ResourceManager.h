#pragma once

//-----------------------------------------------------------------------------
#include <set>
#include "Resource.h"
#include "Stream.h"

//-----------------------------------------------------------------------------
enum class StreamType
{
	Memory,
	FullFileOrMemory,
	File
};

//-----------------------------------------------------------------------------
struct ResourceComparer
{
	inline bool operator () (const BaseResource* r1, const BaseResource* r2)
	{
		if(r1->type != r2->type)
			return r1->type > r2->type;
		else
			return _stricmp(r1->filename, r2->filename) > 0;
	}
};

//-----------------------------------------------------------------------------
struct CstringComparer
{
	inline bool operator() (cstring s1, cstring s2)
	{
		return _stricmp(s1, s2) > 0;
	}
};

//-----------------------------------------------------------------------------
typedef std::set<AnyResource*, ResourceComparer> ResourceContainer;
typedef ResourceContainer::iterator ResourceIterator;

//-----------------------------------------------------------------------------
// Check tools/pak.pak.txt for specification
class Pak
{
public:
	struct Header
	{
		char sign[3];
		byte version;
		uint flags;
	};

	enum Flags
	{
		Encrypted = 0x01,
		FullEncrypted = 0x02
	};

	int version;
	string path;
	HANDLE file;
};

//-----------------------------------------------------------------------------
class PakV0 : public Pak
{
public:
	struct ExtraHeader
	{
		uint files_size;
		uint files;
	};

	struct File
	{
		string name;
		int size, offset;

		static const uint MIN_SIZE = 9;
	};
	
	vector<File> files;
};

//-----------------------------------------------------------------------------
class PakV1 : public Pak
{
public:
	struct ExtraHeader
	{
		uint files_count;
		uint file_entry_table_size;
	};

	struct File
	{
		union
		{
			cstring filename;
			uint filename_offset;
		};
		uint size;
		uint compressed_size;
		uint offset;
	};
	
	File* files;
	Buffer* filename_buf;
	string key;
	bool encrypted;
};

struct TaskData;

typedef fastdelegate::FastDelegate1<TaskData*> TaskCallback;

struct TaskData
{
	enum Flags
	{
		Assign = 1<<0,
		MainThreadCallback = 1<<1,
		AlwaysHandle = 1<<2
	};

	TaskCallback callback;
	AnyResource* res;
	void* ptr;
	byte flags;

	inline TaskData() {}
	inline TaskData(void* ptr, byte flags = Assign | AlwaysHandle) : ptr(ptr), flags(flags), res(nullptr) {}
};

//-----------------------------------------------------------------------------
class ResourceManager
{
public:
	enum class Mode
	{
		Instant,
		LoadScreen
	};

	enum class ResourceSubType
	{
		Mesh,
		MeshVertexData,
		Music,
		Sound,
		Texture
	};

	ResourceManager();
	~ResourceManager();

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void Cleanup();
	BufferHandle GetBuffer(BaseResource* res);
	cstring GetPath(BaseResource* res);
	StreamReader GetStream(BaseResource* res, StreamType type);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void Init(IDirect3DDevice9* device, FMOD::System* fmod_system);

	inline MeshResource* GetMesh(cstring filename, TaskData* task_data = nullptr)
	{
		return (MeshResource*)GetLoadedResource(filename, ResourceSubType::Mesh, task_data);
	}
	inline MeshResource* GetMeshVertexData(cstring filename, TaskData* task_data = nullptr)
	{
		return (MeshResource*)GetLoadedResource(filename, ResourceSubType::MeshVertexData, task_data);
	}
	inline SoundResource* GetMusic(cstring filename, TaskData* task_data = nullptr)
	{
		return (SoundResource*)GetLoadedResource(filename, ResourceSubType::Music, task_data);
	}
	inline SoundResource* GetSound(cstring filename, TaskData* task_data = nullptr)
	{
		return (SoundResource*)GetLoadedResource(filename, ResourceSubType::Sound, task_data);
	}
	inline TextureResource* GetTexture(cstring filename, TaskData* task_data = nullptr)
	{
		return (TextureResource*)GetLoadedResource(filename, ResourceSubType::Texture, task_data);
	}

	inline void GetMesh(cstring filename, Animesh*& mesh)
	{
		GetLoadedResource(filename, ResourceSubType::Mesh, &TaskData(&mesh));
	}
	inline void GetMeshVertexData(cstring filename, VertexData*& vertex_data)
	{
		GetLoadedResource(filename, ResourceSubType::MeshVertexData, &TaskData(&vertex_data));
	}
	inline void GetMusic(cstring filename, SOUND& music)
	{
		GetLoadedResource(filename, ResourceSubType::Music, &TaskData(&music));
	}
	inline void GetSound(cstring filename, SOUND& sound)
	{
		GetLoadedResource(filename, ResourceSubType::Sound, &TaskData(&sound));
	}
	inline void GetTexture(cstring filename, TEX& tex)
	{
		GetLoadedResource(filename, ResourceSubType::Texture, &TaskData(&tex));
	}

	template<typename T>
	inline T* GetResource(cstring filename)
	{
		return (T*)GetResource(filename, T::Type);
	}

	inline static ResourceManager& Get()
	{
		return manager;
	}

private:
	struct Task : TaskData
	{
		ResourceSubType type;
	};

	struct ResourceSubTypeInfo
	{
		ResourceSubType subtype;
		ResourceType type;
		cstring name;
	};

	BaseResource* AddResource(cstring filename, cstring path);
	BaseResource* GetResource(cstring filename, ResourceType type);
	BaseResource* GetLoadedResource(cstring filename, ResourceSubType sub_type, TaskData* task_data);
	void RegisterExtensions();

	void ApplyTask(TaskData* task_data, BaseResource* res);
	void AddTask(BaseResource* res, ResourceSubType type, TaskData* task_data);
	void LoadResource(BaseResource* res, ResourceSubType type);
	void LoadMesh(MeshResource* res);
	void LoadMeshVertexData(MeshResource* res);
	void LoadMusic(SoundResource* res);
	void LoadSound(SoundResource* res);
	void LoadTexture(TextureResource* res);

	Mode mode;
	IDirect3DDevice9* device;
	FMOD::System* fmod_system;
	AnyResource* last_resource;
	ResourceContainer resources;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<Buffer*> sound_bufs;
	vector<Task*> tasks, callback_tasks;
	static ResourceManager manager;
	static ObjectPool<Task> task_pool;
	static ResourceSubTypeInfo res_info[];
};
