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
	inline bool operator () (const BaseResource* r1, const BaseResource* r2)
	{
		if(r1->type != r2->type)
			return r1->type > r2->type;
		else
			return _stricmp(r1->filename, r2->filename) > 0;
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
		Assign = 1<<0
	};

	TaskCallback callback;
	int flags;

	inline Task() : flags(0) {}
	inline Task(void* ptr) : TaskData(ptr), flags(Assign) {}
	inline Task(void* ptr, TaskCallback& callback) : TaskData(ptr), callback(callback), flags(0) {}
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
	enum class Mode
	{
		Instant,
		LoadScreenPrepare, // add tasks
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

	ResourceManager();
	~ResourceManager();

	inline static ResourceManager& Get()
	{
		return manager;
	}

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void AddTask(Task& task_data);
	void AddTask(VoidF& callback, cstring category, int size = 1);
	void AddTaskCategory(cstring category);
	void Cleanup();
	BufferHandle GetBuffer(BaseResource* res);
	cstring GetPath(BaseResource* res);
	StreamReader GetStream(BaseResource* res, StreamType type);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void Init(IDirect3DDevice9* device, FMOD::System* fmod_system);
	void NextTask(cstring category = nullptr);

	inline void SetLoadScreen(LoadScreen* _load_screen) { load_screen = _load_screen; }
	void PrepareLoadScreen(float cap = 1.f);
	void StartLoadScreen();
	void EndLoadScreenStage();
	inline void SetMutex(HANDLE _mutex) { mutex = _mutex; }

#define DECLARE_FUNCTIONS(TYPE, NAME, SUBTYPE, RAW_TYPE, RAW_NAME) \
	inline TYPE TryGet##NAME##(AnyString filename) { return (TYPE)TryGetResource(filename.s, SUBTYPE); } \
	inline TYPE Get##NAME##(AnyString filename) { return (TYPE)GetResource(filename.s, SUBTYPE); } \
	inline TYPE GetLoaded##NAME##(AnyString filename, PtrOrRef<Task> task = nullptr) { return (TYPE)GetLoadedResource(filename.s, SUBTYPE, task.ptr); } \
	inline void GetLoaded##NAME##(AnyString filename, RAW_TYPE& RAW_NAME) { GetLoadedResource(filename.s, SUBTYPE, &Task(&RAW_NAME)); } \
	inline void Load##NAME##(TYPE res, PtrOrRef<Task> task = nullptr) { LoadResource((AnyResource*)res, task.ptr); }

	// Mesh functions
	DECLARE_FUNCTIONS(MeshResourcePtr, Mesh, ResourceSubType::Mesh, Animesh*, mesh);
	// Mesh vertex data functions
	DECLARE_FUNCTIONS(MeshResourcePtr, MeshVertexData, ResourceSubType::MeshVertexData, VertexData*, vertex_data);
	// Music functions
	DECLARE_FUNCTIONS(SoundResourcePtr, Music, ResourceSubType::Music, SOUND, music);
	// Sound functions
	DECLARE_FUNCTIONS(SoundResourcePtr, Sound, ResourceSubType::Sound, SOUND, sound);
	// Texture functions
	DECLARE_FUNCTIONS(TextureResourcePtr, Texture, ResourceSubType::Texture, TEX, tex);

private:
	struct TaskDetail
	{
		enum Flags
		{
			Assign = 1<<0,
			VoidCallback = 1<<1
		};

		// begining should be like in TaskData
		AnyResource* res;
		void* ptr;
		// new fields
		ResourceSubType type;
		fastdelegate::DelegateMemento delegate;
		int flags;
		cstring category;
	};

	struct ResourceSubTypeInfo
	{
		ResourceSubType subtype;
		ResourceType type;
		cstring name;
	};

	BaseResource* AddResource(cstring filename, cstring path);
	void ApplyTask(Task* task);
	void RegisterExtensions();
	BaseResource* GetResource(cstring filename, ResourceType type);
	AnyResource* GetResource(cstring filename, ResourceSubType type);
	AnyResource* TryGetResource(cstring filename, ResourceSubType type);
	inline AnyResource* GetLoadedResource(cstring filename, ResourceSubType type, Task* task)
	{
		AnyResource* res = GetResource(filename, type);
		LoadResource(res, task);
		return res;
	}
	void LoadResource(AnyResource* res, Task* task);
	void LoadResource(AnyResource* res);
	void LoadMeshInternal(MeshResource* res);
	void LoadMeshVertexDataInternal(MeshResource* res);
	void LoadMusicInternal(SoundResource* res);
	void LoadSoundInternal(SoundResource* res);
	void LoadTextureInternal(TextureResource* res);
	void UpdateLoadScreen();
	void TickLoadScreen();

	Mode mode;
	IDirect3DDevice9* device;
	FMOD::System* fmod_system;
	AnyResource* last_resource;
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
