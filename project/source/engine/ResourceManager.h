#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"
#include "Stream.h"

//-----------------------------------------------------------------------------
class LoadScreen;

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
	bool operator () (const Resource* r1, const Resource* r2) const
	{
		if(r1->type != r2->type)
			return r1->type > r2->type;
		else
			return _stricmp(r1->filename, r2->filename) > 0;
	}
};

//-----------------------------------------------------------------------------
typedef std::set<Resource*, ResourceComparer> ResourceContainer;
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
	Resource* res;
	void* ptr;

	TaskData(void* ptr = nullptr) : ptr(ptr), res(nullptr) {}
};

//-----------------------------------------------------------------------------
typedef delegate<void(TaskData&)> TaskCallback;

//-----------------------------------------------------------------------------
// Task
struct Task : TaskData
{
	enum Flags
	{
		Assign = 1 << 0
	};

	TaskCallback callback;
	int flags;

	Task() : flags(0) {}
	Task(void* ptr) : TaskData(ptr), flags(Assign) {}
	Task(void* ptr, TaskCallback& callback) : TaskData(ptr), callback(callback), flags(0) {}
};

//-----------------------------------------------------------------------------
template<typename T>
struct PtrOrRef
{
	PtrOrRef(T* ptr) : ptr(ptr) {}
	PtrOrRef(T& ref) : ptr(&ref) {}
	T* ptr;
};

//-----------------------------------------------------------------------------
class ResourceManager
{
public:
	friend class TypeManager;

	enum class Mode
	{
		Instant,
		LoadScreenPrepare, // add tasks
		LoadScreenPrepare2, // fake loadscreen prepare
		LoadScreenNext, // add next_tasks
		LoadScreenStart, // load tasks instantly
		LoadScreenEnd // waits for prepare
	};

	enum class ResourceSubType
	{
		Unknown,
		Task,
		Callback,
		Category,
		Mesh,
		MeshVertexData,
		Music,
		Sound,
		Texture
	};

	template<typename T>
	class TypeManager
	{
		template<typename T>
		struct Internal
		{
		};

		template<>
		struct Internal<Texture>
		{
			static const ResourceType Type = ResourceType::Texture;
		};

	public:
		const ResourceType Type = Internal<T>::Type;

		TypeManager(ResourceManager& res_mgr) : res_mgr(res_mgr)
		{
		}

		// Return loaded texture, if not exists throw
		T* GetLoaded(const AnyString& path);

	private:
		ResourceManager& res_mgr;
	};

	ResourceManager();
	~ResourceManager();

	template<typename T>
	static auto Get()
	{
		return ResourceManager::Get().For<T>();
	}

	static ResourceManager& Get()
	{
		return manager;
	}

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void AddTask(Task& task_data);
	void AddTask(VoidF& callback, cstring category, int size = 1);
	void AddTaskCategory(cstring category);
	void Cleanup();
	BufferHandle GetBuffer(Resource* res);
	cstring GetPath(Resource* res);
	StreamReader GetStream(Resource* res, StreamType type);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void Init(IDirect3DDevice9* device, FMOD::System* fmod_system);
	void NextTask(cstring category = nullptr);

	void SetLoadScreen(LoadScreen* _load_screen) { load_screen = _load_screen; }
	void PrepareLoadScreen(float cap = 1.f);
	void PrepareLoadScreen2(float cap, int steps, cstring text);
	void StartLoadScreen();
	void EndLoadScreenStage();
	void SetMutex(HANDLE _mutex) { mutex = _mutex; }

	/*
#define DECLARE_FUNCTIONS(TYPE, NAME, SUBTYPE, RAW_TYPE, RAW_NAME) \
	TYPE TryGet##NAME##(const AnyString& filename) { return (TYPE)TryGetResource(filename.s, SUBTYPE); } \
	TYPE Get##NAME##(const AnyString& filename) { return (TYPE)GetResource(filename.s, SUBTYPE); } \
	TYPE GetLoaded##NAME##(const AnyString& filename, PtrOrRef<Task> task = nullptr) { return (TYPE)GetLoadedResource(filename.s, SUBTYPE, task.ptr); } \
	void GetLoaded##NAME##(const AnyString &filename, RAW_TYPE& RAW_NAME) { GetLoadedResource(filename.s, SUBTYPE, &Task(&RAW_NAME)); } \
	void Load##NAME##(TYPE res, PtrOrRef<Task> task = nullptr) { LoadResource((Resource*)res, task.ptr); }

	// Mesh functions
	DECLARE_FUNCTIONS(MeshResourcePtr, Mesh, ResourceSubType::Mesh, Mesh*, mesh);
	// Mesh vertex data functions
	DECLARE_FUNCTIONS(MeshResourcePtr, MeshVertexData, ResourceSubType::MeshVertexData, VertexData*, vertex_data);
	// Music functions
	DECLARE_FUNCTIONS(SoundResourcePtr, Music, ResourceSubType::Music, SOUND, music);
	// Sound functions
	DECLARE_FUNCTIONS(SoundResourcePtr, Sound, ResourceSubType::Sound, SOUND, sound);
	// Texture functions
	DECLARE_FUNCTIONS(TexturePtr, Texture, ResourceSubType::Texture, TEX, tex);

	*/

	Resource* ForceLoadResource(const AnyString& path, ResourceType type);

	template<typename T>
	TypeManager<T> For()
	{
		TypeManager<T> inst(*this);
		return inst;
	}

private:
	struct TaskDetail
	{
		enum Flags
		{
			Assign = 1 << 0,
			VoidCallback = 1 << 1
		};

		// begining should be like in TaskData
		Resource* res;
		void* ptr;
		// new fields
		ResourceSubType type;
		TaskCallback delegate;
		int flags;
		cstring category;
	};

	struct ResourceSubTypeInfo
	{
		ResourceSubType subtype;
		ResourceType type;
		cstring name;
	};

	Resource* AddResource(cstring filename, cstring path);
	void ApplyTask(Task* task);
	void RegisterExtensions();
	Resource* GetResource(cstring filename, ResourceType type);
	Resource* GetResource(cstring filename, ResourceSubType type);
	Resource* TryGetResource(cstring filename, ResourceSubType type);
	Resource* GetLoadedResource(cstring filename, ResourceSubType type, Task* task)
	{
		Resource* res = GetResource(filename, type);
		LoadResource(res, task);
		return res;
	}
	void LoadResource(Resource* res, Task* task);
	void LoadResource(Resource* res);
	void LoadMeshInternal(Mesh* res);
	void LoadMeshVertexDataInternal(Mesh* res);
	void LoadMusicInternal(Sound* res);
	void LoadSoundInternal(Sound* res);
	void LoadTextureInternal(Texture* res);
	void UpdateLoadScreen();
	void TickLoadScreen();

	Mode mode;
	IDirect3DDevice9* device;
	FMOD::System* fmod_system;
	Resource* last_resource;
	ResourceContainer resources;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<Buffer*> sound_bufs;
	vector<TaskDetail*> tasks, next_tasks;
	int to_load, loaded, to_load_next;
	cstring category;
	Timer timer;
	float load_cap, old_load_cap, timer_dt;
	LoadScreen* load_screen;
	HANDLE mutex;
	static ResourceManager manager;
	static ObjectPool<TaskDetail> task_pool;
	static ResourceSubTypeInfo res_info[];
};
