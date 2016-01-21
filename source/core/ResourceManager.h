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
// Check tools/pak/pak.txt for specification
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
// Pak 0 version
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
// Pak 1 version
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

//-----------------------------------------------------------------------------
// Task data
struct TaskData
{
	AnyResource* res;
	void* ptr;

	inline TaskData(void* ptr = nullptr) : ptr(ptr), res(nullptr) {}
};

//-----------------------------------------------------------------------------
typedef fastdelegate::FastDelegate1<TaskData&> TaskCallback;

//-----------------------------------------------------------------------------
// Task
struct Task : TaskData
{
	enum Flags
	{
		Assign = 1<<0,
		MainThreadCallback = 1<<1
	};

	TaskCallback callback;
	int flags;

	inline Task() : flags(0) {}
	inline Task(void* ptr) : TaskData(ptr), flags(Assign) {}
	inline Task(void* ptr, TaskCallback& callback, bool main_thread = false) : TaskData(ptr), callback(callback), flags(main_thread ? MainThreadCallback : 0) {}
};

//-----------------------------------------------------------------------------
class ResourceManager
{
public:
	enum class Mode
	{
		Instant,
		LoadScreenPrepare,
		LoadScreenStart,
		LoadScreenEnd
	};

	enum class ResourceSubType
	{
		Task,
		Callback,
		Category,
		Mesh,
		MeshVertexData,
		Music,
		Sound,
		Texture
	};

	ResourceManager();
	~ResourceManager();

	inline static ResourceManager& Get()
	{
		return manager;
	}

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void AddTask(Task& task_data);
	void AddTask(VoidF& callback, int category, int size = 1);
	void AddTaskCategory(int category);
	void BeginLoadScreen();
	void Cleanup();
	BufferHandle GetBuffer(BaseResource* res);
	cstring GetPath(BaseResource* res);
	StreamReader GetStream(BaseResource* res, StreamType type);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void Init(IDirect3DDevice9* device, FMOD::System* fmod_system);
	void StartLoadScreen(VoidF& callback);
	int UpdateLoadScreen(float& progress, int& category); //0-sleep, 1-don't sleep, 2-finish

	// Get mesh
	inline MeshResource* GetMesh(AnyString filename, Task* task = nullptr)
	{
		return (MeshResource*)GetLoadedResource(filename.s, ResourceSubType::Mesh, task);
	}
	inline MeshResource* GetMesh(AnyString filename, Task& task)
	{
		return (MeshResource*)GetLoadedResource(filename.s, ResourceSubType::Mesh, &task);
	}
	inline void GetMesh(AnyString filename, Animesh*& mesh)
	{
		GetLoadedResource(filename.s, ResourceSubType::Mesh, &Task(&mesh));
	}
	// Get mesh vertex data
	inline MeshResource* GetMeshVertexData(AnyString filename, Task* task = nullptr)
	{
		return (MeshResource*)GetLoadedResource(filename.s, ResourceSubType::MeshVertexData, task);
	}
	inline MeshResource* GetMeshVertexData(AnyString filename, Task& task)
	{
		return (MeshResource*)GetLoadedResource(filename.s, ResourceSubType::MeshVertexData, &task);
	}
	inline void GetMeshVertexData(AnyString filename, VertexData*& vertex_data)
	{
		GetLoadedResource(filename.s, ResourceSubType::MeshVertexData, &Task(&vertex_data));
	}
	// Get music
	inline SoundResource* GetMusic(AnyString filename, Task* task = nullptr)
	{
		return (SoundResource*)GetLoadedResource(filename.s, ResourceSubType::Music, task);
	}
	inline SoundResource* GetMusic(AnyString filename, Task& task)
	{
		return (SoundResource*)GetLoadedResource(filename.s, ResourceSubType::Music, &task);
	}
	inline void GetMusic(AnyString filename, SOUND& music)
	{
		GetLoadedResource(filename.s, ResourceSubType::Music, &Task(&music));
	}
	// Get sound
	inline SoundResource* GetSound(AnyString filename, Task* task = nullptr)
	{
		return (SoundResource*)GetLoadedResource(filename.s, ResourceSubType::Sound, task);
	}
	inline SoundResource* GetSound(AnyString filename, Task& task)
	{
		return (SoundResource*)GetLoadedResource(filename.s, ResourceSubType::Sound, &task);
	}
	inline void GetSound(AnyString filename, SOUND& sound)
	{
		GetLoadedResource(filename.s, ResourceSubType::Sound, &Task(&sound));
	}
	// Get texture
	inline TextureResource* GetTexture(AnyString filename, Task* task = nullptr)
	{
		return (TextureResource*)GetLoadedResource(filename.s, ResourceSubType::Texture, task);
	}
	inline TextureResource* GetTexture(AnyString filename, Task& task)
	{
		return (TextureResource*)GetLoadedResource(filename.s, ResourceSubType::Texture, &task);
	}
	inline void GetTexture(AnyString filename, TEX& tex)
	{
		GetLoadedResource(filename.s, ResourceSubType::Texture, &Task(&tex));
	}

private:
	friend uint __stdcall ThreadStart(void*);

	struct TaskDetail
	{
		enum Flags
		{
			Assign = 1<<0,
			MainThreadCallback = 1<<1,
			VoidCallback = 1<<2
		};

		// begining should be like in TaskData
		AnyResource* res;
		void* ptr;
		// new fields
		ResourceSubType type;
		fastdelegate::DelegateMemento delegate;
		int flags, category;
	};

	struct ResourceSubTypeInfo
	{
		ResourceSubType subtype;
		ResourceType type;
		cstring name;
	};

	BaseResource* AddResource(cstring filename, cstring path);
	void ApplyTask(Task* task);
	BaseResource* GetResource(cstring filename, ResourceType type);
	BaseResource* GetLoadedResource(cstring filename, ResourceSubType sub_type, Task* task);
	void RegisterExtensions();
	void ThreadLoop();

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
	vector<TaskDetail*> tasks, recycled_tasks;
	SafeVector<TaskDetail*> callback_tasks;
	int to_load, loaded, category;
	VoidF load_callback;
	Timer timer;
	HANDLE thread;
	string thread_error;
	static ResourceManager manager;
	static ObjectPool<TaskDetail> task_pool;
	static ResourceSubTypeInfo res_info[];
};
