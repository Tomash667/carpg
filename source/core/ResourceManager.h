#pragma once

typedef Animesh Mesh;

const short INVALID_PAK = -1;

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
};

class Pak
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
typedef ResourceMap::iterator ResourceMapI;

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	bool AddDir(cstring dir, bool subdir = true);
	bool AddPak(cstring path);
	Resource2* GetResource(cstring filename);
	Mesh* LoadMesh(cstring path);
	Mesh* LoadMesh(Resource2* res);
	bool LoadResource(Resource2* res);
	TEX LoadTexture(cstring path);
	TEX LoadTexture(Resource2* res);
	
	static Resource2::Type ExtToResourceType(cstring ext);
	static Resource2::Type FilenameToResourceType(cstring filename);

private:
	typedef vector<byte>* Buf;

	struct PakData
	{
		Buf buf;
		uint size;
	};

	bool AddNewResource(Resource2* res);
	bool GetPakData(Resource2* res, PakData& pak_data);
	cstring GetPath(Resource2* res);

	IDirect3DDevice9* device;
	ResourceMap resources;
	Resource2* last_resource;
	vector<Pak*> paks;
	ObjectPool<vector<byte>> bufs;
};
