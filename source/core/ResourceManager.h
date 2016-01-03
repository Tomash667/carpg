#pragma once

#include <set>
#include "Resource.h"
#include "Stream.h"

enum class StreamType
{
	Memory,
	FullFileOrMemory,
	File
};


/*class Resource2
{
public:
	enum State
	{
		NotLoaded,
		Loading,
		Loaded,
		Releasing
	};

	enum Type
	{
		None,
		Mesh,
		Texture,
		Sound,
		Music
	};

	struct PakEntry
	{
		short pak_id;
		word entry;
	};

	inline bool CheckType(Type _type) const
	{
		return type == _type || type == None;
	}

	string path;
	cstring filename;
	State state;
	Type type;
	void* ptr;
	PakEntry pak;
	int refs;
};*/


/*class Pak
{
public:
	struct Header
	{
		char sign[3];
		byte version;
		int flags;
		uint files;
		uint table_size;
	};

	struct Entry
	{
		cstring filename;
		uint size;
		uint compressed_size;
		uint offset;
	};

	string path;
	Entry* files;
	uint count;
	byte* table;
	HANDLE file;
};

typedef std::map<cstring, Resource2*> ResourceMap;
typedef ResourceMap::iterator ResourceMapI;*/

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

struct CstringComparer
{
	inline bool operator() (cstring s1, cstring s2)
	{
		return _stricmp(s1, s2) > 0;
	}
};

typedef std::set<BaseResource*, ResourceComparer> ResourceContainer;
typedef ResourceContainer::iterator ResourceIterator;

class Pak
{
public:
	struct Header
	{
		char sign[4];
		int flags;
		uint files_size;
		uint files;
	};

	struct File
	{
		string name;
		int size, offset;

		static const uint MIN_SIZE = 9;
	};

	enum Flags
	{
		Encrypted = 0x01
	};

	string path;
	HANDLE file;
	vector<File> files;
};

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void Cleanup();
	/*bool AddPak(cstring path);
	Resource2* GetResource(cstring filename);
	Mesh* LoadMesh(cstring path);
	Mesh* LoadMesh(Resource2* res);
	bool LoadResource(Resource2* res);
	TEX LoadTexture(cstring path);
	TEX LoadTexture(Resource2* res);*/
	HANDLE GetPakFile(BaseResource* res);
	cstring GetPath(BaseResource* res);
	StreamReader GetStream(BaseResource* res, StreamType type);
	TextureResource* GetTexture(cstring filename);
	ResourceType ExtToResourceType(cstring ext);
	ResourceType FilenameToResourceType(cstring filename);
	void Init(IDirect3DDevice9* device);

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
	/*typedef vector<byte>* Buf;

	struct PakData
	{
		Buf buf;
		uint size;
	};

	bool AddNewResource(Resource2* res);
	bool GetPakData(Resource2* res, PakData& pak_data);
	cstring GetPath(Resource2* res);*/

	IDirect3DDevice9* device;
	//ResourceMap resources;
	//Resource2* last_resource;
	//vector<Pak*> paks;
	//ObjectPool<vector<byte>> bufs;

	BaseResource* AddResource(cstring filename, cstring path);
	BaseResource* GetResource(cstring filename, ResourceType type);
	void RegisterExtensions();
	
	BaseResource* last_resource;
	ResourceContainer resources;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<byte> buf;
	static ResourceManager manager;
};
