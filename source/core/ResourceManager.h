#pragma once

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
	TEX LoadTexture(cstring path);

private:
	ResourceMap resources;
	Resource2* last_resource;
	vector<Pak*> paks;
};
