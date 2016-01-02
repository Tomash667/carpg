#pragma once

#include <set>
#include "Resource.h"


/*const short INVALID_PAK = -1;

class Resource2
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

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	bool AddDir(cstring dir, bool subdir = true);
	void Cleanup();
	BaseResource* CreateResource(cstring filename);
	/*bool AddPak(cstring path);
	Resource2* GetResource(cstring filename);
	Mesh* LoadMesh(cstring path);
	Mesh* LoadMesh(Resource2* res);
	bool LoadResource(Resource2* res);
	TEX LoadTexture(cstring path);
	TEX LoadTexture(Resource2* res);*/
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

	BaseResource* GetResource(cstring filename, ResourceType type);
	void RegisterExtensions();
	
	BaseResource* last_resource;
	ResourceContainer resources;
	std::map<cstring, ResourceType, CstringComparer> exts;
	static ResourceManager manager;
};
