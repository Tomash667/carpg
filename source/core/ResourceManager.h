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
};*/

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

//-----------------------------------------------------------------------------
class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path, cstring key = nullptr);
	void Cleanup();
	BufferHandle GetBuffer(BaseResource* res);
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
	BaseResource* AddResource(cstring filename, cstring path);
	BaseResource* GetResource(cstring filename, ResourceType type);
	void RegisterExtensions();

	IDirect3DDevice9* device;
	AnyResource* last_resource;
	ResourceContainer resources;
	std::map<cstring, ResourceType, CstringComparer> exts;
	vector<Pak*> paks;
	vector<byte> buf;
	static ResourceManager manager;
};
