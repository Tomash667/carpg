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
		LoadScreenPrepare,
		LoadScreenRuning
	};

	enum class TaskType
	{
		Unknown,
		Callback,
		Load,
		LoadAndCallback,
		Category
	};

	//-----------------------------------------------------------------------------
	// Base type manager
	template<typename T>
	class BaseTypeManager
	{
	protected:
		template<typename T>
		struct Internal
		{
		};

		template<>
		struct Internal<Mesh>
		{
			static const ResourceType Type = ResourceType::Mesh;
		};

		template<>
		struct Internal<VertexData>
		{
			static const ResourceType Type = ResourceType::VertexData;
		};

		template<>
		struct Internal<Sound>
		{
			static const ResourceType Type = ResourceType::SoundOrMusic;
		};

		template<>
		struct Internal<Texture>
		{
			static const ResourceType Type = ResourceType::Texture;
		};

	public:
		const ResourceType Type = Internal<T>::Type;

		BaseTypeManager(ResourceManager& res_mgr) : res_mgr(res_mgr)
		{
		}

		// Try to return resource, if not exists return null
		T* TryGet(const AnyString& filename)
		{
			return (T*)res_mgr.TryGetResource(filename, Type);
		}

		// Return resource, if not exists throws
		T* Get(const AnyString& filename)
		{
			return (T*)res_mgr.GetResource(filename, Type);
		}

		// Return loaded resource, if not exists or can't load throws
		T* GetLoaded(const AnyString& filename)
		{
			auto res = res_mgr.GetResource(filename, Type);
			res_mgr.LoadResource(res);
			return (T*)res;
		}

		// Load resource, if failed throws
		void Load(T* res)
		{
			res_mgr.LoadResource(res);
		}

		// Add task category
		void AddTaskCategory(const AnyString& name)
		{
			res_mgr.AddTaskCategory(name);
		}

		// Add load task, if not exists throws, if failed throws later
		T* AddLoadTask(const AnyString& filename)
		{
			auto res = res_mgr.GetResource(filename, Type);
			res_mgr.AddLoadTask(res);
			return (T*)res;
		}

		// Add load task, if failed throws later
		void AddLoadTask(T* res)
		{
			res_mgr.AddLoadTask(res);
		}

		// Add load task, if not exists throws, if failed throws later, after loading run callback
		void AddLoadTask(const AnyString& filename, void* ptr, TaskCallback callback)
		{
			auto res = res_mgr.GetResource(filename, Type);
			res_mgr.AddLoadTask(res, ptr, callback);
		}

		// Add load task, if failed throws later, after loading run callback
		void AddLoadTask(T* res, void* ptr, TaskCallback callback)
		{
			res_mgr.AddLoadTask(res, ptr, callback);
		}

	protected:
		ResourceManager& res_mgr;
	};

	//-----------------------------------------------------------------------------
	// Generic type manager
	template<typename T>
	class TypeManager : public BaseTypeManager<T>
	{
	public:
		TypeManager(ResourceManager& res_mgr) : BaseTypeManager(res_mgr)
		{
		}
	};
	
	//-----------------------------------------------------------------------------
	// Texture type manager
	template<>
	class TypeManager<Texture> : public BaseTypeManager<Texture>
	{
	public:
		TypeManager(ResourceManager& res_mgr) : BaseTypeManager(res_mgr)
		{
		}

		using BaseTypeManager::AddLoadTask;

		TEX GetLoadedRaw(const AnyString& filename)
		{
			auto tex = (Texture*)res_mgr.GetResource(filename, Type);
			res_mgr.LoadResource(tex);
			return tex->tex;
		}
		void AddLoadTask(const AnyString& filename, TEX& tex)
		{
			auto res = (Texture*)res_mgr.GetResource(filename, Type);
			res_mgr.AddLoadTask(res, &tex, TaskCallback(&res_mgr, &ResourceManager::ApplyRawTextureCallback));
		}
		void AddLoadTask(Texture* res, TEX& tex)
		{
			res_mgr.AddLoadTask(res, &tex, TaskCallback(&res_mgr, &ResourceManager::ApplyRawTextureCallback));
		}
	};

	//-----------------------------------------------------------------------------
	// Sound or music type manager
	template<>
	class TypeManager<Sound> : public BaseTypeManager<Sound>
	{
	public:
		TypeManager(ResourceManager& res_mgr) : BaseTypeManager(res_mgr)
		{
		}

		using BaseTypeManager::AddLoadTask;

		void AddLoadTask(const AnyString& filename, SOUND& sound)
		{
			Sound* res = (Sound*)res_mgr.GetResource(filename, Type);
			assert(!res->is_music);
			res_mgr.AddLoadTask(res, &sound, TaskCallback(&res_mgr, &ResourceManager::ApplyRawSoundCallback));
		}

		Sound* TryGetSound(const AnyString& filename)
		{
			return TryGetInternal(filename, false);
		}

		Sound* TryGetMusic(const AnyString& filename)
		{
			return TryGetInternal(filename, true);
		}

		Sound* GetSound(const AnyString& filename)
		{
			return GetInternal(filename, false);
		}

		Sound* GetMusic(const AnyString& filename)
		{
			return GetInternal(filename, true);
		}

		Sound* GetLoadedSound(const AnyString& filename)
		{
			return GetLoadedInternal(filename, false);
		}

		Sound* GetLoadedMusic(const AnyString& filename)
		{
			return GetLoadedInternal(filename, true);
		}

	private:
		Sound* TryGetInternal(const AnyString& filename, bool is_music)
		{
			Sound* res = (Sound*)res_mgr.TryGetResource(filename, Type);
			if(res)
			{
				assert(res->state == ResourceState::NotLoaded || res->is_music == is_music);
				res->is_music = is_music;
			}
			return res;
		}

		Sound* GetInternal(const AnyString& filename, bool is_music)
		{
			Sound* res = (Sound*)res_mgr.GetResource(filename, Type);
			assert(res->state == ResourceState::NotLoaded || res->is_music == is_music);
			res->is_music = is_music;
			return res;
		}

		Sound* GetLoadedInternal(const AnyString& filename, bool is_music)
		{
			Sound* res = GetInternal(filename, is_music);
			res_mgr.LoadResource(res);
			return res;
		}
	};

	template<typename T>
	friend class TypeManager;

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

	void Init(IDirect3DDevice9* device, FMOD::System* fmod_system);
	void Cleanup();

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	BufferHandle GetBuffer(Resource* res);
	cstring GetPath(Resource* res);
	StreamReader GetStream(Resource* res, StreamType type);
	void AddTaskCategory(const AnyString& name);
	void AddTask(void* ptr, TaskCallback callback);
	void NextTask(cstring next_category = nullptr);
	void SetLoadScreen(LoadScreen* _load_screen) { load_screen = _load_screen; }
	void PrepareLoadScreen(float progress_min = 0.f, float progress_max = 1.f);
	void StartLoadScreen(cstring category = nullptr);
	void SetMutex(HANDLE _mutex) { mutex = _mutex; }
	bool HaveTasks() const { return !tasks.empty(); }
	int GetLoadTasksCount() const { return to_load; }
	
	template<typename T>
	TypeManager<T> For()
	{
		TypeManager<T> inst(*this);
		return inst;
	}

private:
	struct TaskDetail
	{
		union
		{
			TaskData data;
			cstring category;
		};
		TaskType type;
		TaskCallback callback;

		TaskDetail()
		{
		}
	};

	void RegisterExtensions();
	void UpdateLoadScreen();
	void TickLoadScreen();
	void ReleaseMutex();
	
	Resource* AddResource(cstring filename, cstring path);
	Resource* CreateResource(ResourceType type);
	Resource* TryGetResource(const AnyString& filename, ResourceType type);
	Resource* GetResource(const AnyString& filename, ResourceType type);
	void AddLoadTask(Resource* res);
	void AddLoadTask(Resource* res, void* ptr, TaskCallback callback);
	void LoadResource(Resource* res);
	void LoadResourceInternal(Resource* res);
	void LoadMesh(Mesh* mesh);
	void LoadVertexData(VertexData* vd);
	void LoadSoundOrMusic(Sound* sound);
	void LoadTexture(Texture* tex);
	void ApplyRawTextureCallback(TaskData& data);
	void ApplyRawSoundCallback(TaskData& data);

	Mode mode;
	IDirect3DDevice9* device;
	FMOD::System* fmod_system;
	ResourceContainer resources;
	Resource res_search;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<Buffer*> sound_bufs;
	vector<TaskDetail*> tasks;
	int to_load, loaded;
	cstring category;
	Timer timer;
	float timer_dt, progress, progress_min, progress_max;
	LoadScreen* load_screen;
	HANDLE mutex;
	static ResourceManager manager;
	static ObjectPool<TaskDetail> task_pool;
};
