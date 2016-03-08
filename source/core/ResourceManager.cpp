#include "Pch.h"
#include "Base.h"
#include "ResourceManager.h"
#include "Animesh.h"

ResourceManager::ResourceManager() : last_resource(nullptr)
{
}

ResourceManager::~ResourceManager()
{
	delete last_resource;
}

bool ResourceManager::AddDir(cstring dir, bool subdir)
{
	assert(dir);

	WIN32_FIND_DATA find_data;
	HANDLE find = FindFirstFile(Format("%s/*.*", dir), &find_data);

	if(find == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		ERROR(Format("ResourceManager: AddDir FindFirstFile failed (%u) for dir '%s'.", result, dir));
		return false;
	}

	int dirlen = strlen(dir) + 1;

	do
	{
		if(strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
		{
			if(IS_SET(find_data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
			{
				// subdirectory
				if(subdir)
				{
					LocalString path = Format("%s/%s", dir, find_data.cFileName);
					AddDir(path);
				}
			}
			else
			{
				if(!last_resource)
					last_resource = new Resource2;
				last_resource->path = Format("%s/%s", dir, find_data.cFileName);
				last_resource->filename = last_resource->path.c_str() + dirlen;
				last_resource->pak.pak_id = INVALID_PAK;

				if(AddNewResource(last_resource))
					last_resource = nullptr;
			}
		}
	} while(FindNextFile(find, &find_data) != 0);

	DWORD result = GetLastError();
	if(result != ERROR_NO_MORE_FILES)
		ERROR(Format("ResourceManager: AddDir FindNextFile failed (%u) for dir '%s'.", result, dir));

	FindClose(find);

	return true;
}

//!!!!!!!!!!!!!!!!! decrypt
bool ResourceManager::AddPak(cstring path)
{
	assert(path);

	// file specs in tools/pak/pak.txt

	// open file
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		ERROR(Format("ResourceManager: AddPak open file failed (%u) for path '%s'.", result, path));
		return false;
	}

	// read header
	Pak::Header head;
	if(!ReadFile(file, &head, sizeof(head), &tmp, nullptr))
	{
		DWORD result = GetLastError();
		ERROR(Format("ResourceManager: Failed to read pak '%s' (%u).", path, result));
		return false;
	}
	if(head.sign[0] != 'P' || head.sign[1] != 'A' || head.sign[2] != 'K')
	{
		ERROR(Format("ResourceManager: Invalid pak '%s' signature (%c%c%c).", path, head.sign[0], head.sign[1], head.sign[2]));
		return false;
	}
	if(head.version != 1)
	{
		ERROR(Format("ResourceManager: Invalid pak '%s' version %u.", path, (uint)head.version));
		return false;
	}

	// read table
	byte* table = new byte[head.table_size];
	if(!ReadFile(file, table, head.table_size, &tmp, nullptr))
	{
		DWORD result = GetLastError();
		ERROR(Format("ResourceManager: Failed to read pak '%s' table (%u).", path, result));
		delete[] table;
		return false;
	}

	// setup pak
	short pak_id = (short)paks.size();
	Pak* pak = new Pak;
	paks.push_back(pak);
	pak->path = path;
	pak->table = table;
	pak->count = head.files;
	pak->files = (Pak::Entry*)table;
	pak->file = file;
	for(uint i = 0; i < head.files; ++i)
	{
		pak->files[i].filename = (cstring)(table + (uint)pak->files[i].filename);

		if(!last_resource)
			last_resource = new Resource2;
		last_resource->filename = pak->files[i].filename;
		last_resource->pak.pak_id = pak_id;
		last_resource->pak.entry = i;

		if(AddNewResource(last_resource))
			last_resource = nullptr;
	}

	return true;
}

Resource2* ResourceManager::GetResource(cstring filename)
{
	assert(filename);

	ResourceMapI it = resources.find(filename);
	if(it == resources.end())
		return nullptr;
	else
		return (*it).second;
}

Mesh* ResourceManager::LoadMesh(cstring path)
{
	assert(path);

	// find resource
	Resource2* res = GetResource(path);
	if(!res)
		throw Format("ResourceManager: Missing mesh resource '%s'.", path);

	return LoadMesh(res);
}

Mesh* ResourceManager::LoadMesh(Resource2* res)
{
	assert(res && res->CheckType(Resource2::Mesh));

	++res->refs;

	// if already loaded return it
	if(res->state == Resource2::Loaded)
		return (Mesh*)res->ptr;

	// load
	Mesh* m;
	if(res->pak.pak_id == INVALID_PAK)
	{
		HANDLE file = CreateFile(res->path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file == INVALID_HANDLE_VALUE)
		{
			DWORD result = GetLastError();
			throw Format("ResourceManager: Failed to load mesh '%s'. Can't open file (%u).", res->path.c_str(), result);
		}

		m = new Mesh;

		try
		{
			m->Load(file, device);
		}
		catch(cstring err)
		{
			CloseHandle(file);
			delete m;
			throw Format("ResourceManager: Failed to load mesh '%s'. %s", res->path.c_str(), err);
		}

		CloseHandle(file);
	}
	else
	{
		PakData pd;
		if(GetPakData(res, pd))
		{
			m = new Mesh;

			try
			{
				//m->LoadFromMemory(device, pd.b)
			}
			catch(cstring err)
			{
				//!~!~!~~!
				throw Format("ResourceManager: Failed to load mesh '%s'. %s", res->path.c_str(), err);
			}
			//hr = D3DXCreateTextureFromFileInMemory(device, pd.buf->data(), pd.size, &t);
			bufs.Free(pd.buf);
		}
		else
			throw Format("ResourceManager: Missing texture '%s'.", GetPath(res));
	}
	//if(FAILED(hr))
	//	throw Format("ResourceManager: Failed to load texture '%s' (%u).", GetPath(res), hr);

	// set state
	res->ptr = m;
	res->state = Resource2::Loaded;
	res->type = Resource2::Mesh;

	return m;
}

bool ResourceManager::LoadResource(Resource2* res)
{
	assert(res);

	// check type
	Resource2::Type type = res->type;
	if(res->type == Resource2::None)
	{
		type = FilenameToResourceType(res->filename);
		if(type == Resource2::None)
		{
			ERROR(Format("ResourceManager: Unknown resource type '%s'.", GetPath(res)));
			return false;
		}
	}

	// load
	switch(type)
	{
	case Resource2::Mesh:
		return LoadMesh(res) != nullptr;
	}

	return false;
}

TEX ResourceManager::LoadTexture(cstring path)
{
	assert(path);

	// find resource
	Resource2* res = GetResource(path);
	if(!res)
		throw Format("ResourceManager: Missing texture '%s'.", path);

	return LoadTexture(res);
}

TEX ResourceManager::LoadTexture(Resource2* res)
{
	assert(res && res->CheckType(Resource2::Texture));

	++res->refs;

	// if already loaded return it
	if(res->state == Resource2::Loaded)
		return (TEX)res->ptr;

	// load
	TEX t;
	HRESULT hr;
	if(res->pak.pak_id == INVALID_PAK)
		hr = D3DXCreateTextureFromFile(device, res->path.c_str(), &t);
	else
	{
		PakData pd;
		if(GetPakData(res, pd))
		{
			hr = D3DXCreateTextureFromFileInMemory(device, pd.buf->data(), pd.size, &t);
			bufs.Free(pd.buf);
		}
		else
			throw Format("ResourceManager: Missing texture '%s'.", GetPath(res));
	}
	if(FAILED(hr))
		throw Format("ResourceManager: Failed to load texture '%s' (%u).", GetPath(res), hr);

	// set state
	res->ptr = t;
	res->state = Resource2::Loaded;
	res->type = Resource2::Texture;

	return t;
}

bool ResourceManager::AddNewResource(Resource2* res)
{
	assert(res);

	// add resource if not exists
	std::pair<ResourceMapI, bool> const& r = resources.insert(ResourceMap::value_type(res->filename, res));
	if(r.second)
	{
		// added
		res->ptr = nullptr;
		res->type = Resource2::None;
		res->state = Resource2::NotLoaded;
		res->refs = 0;
		return true;
	}
	else
	{
		// already exists
		WARN(Format("ResourceManager: Resource %s already exists (%s, %s).", res->filename, GetPath(r.first->second), GetPath(res)));
		return false;
	}
}

bool ResourceManager::GetPakData(Resource2* res, PakData& pak_data)
{
	assert(res && res->pak.pak_id != INVALID_PAK);

	Pak& pak = *paks[res->pak.pak_id];
	Pak::Entry& entry = pak.files[res->pak.entry];

	// open pak if closed
	if(pak.file == INVALID_HANDLE_VALUE)
	{
		pak.file = CreateFile(pak.path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(pak.file == INVALID_HANDLE_VALUE)
		{
			DWORD result = GetLastError();
			ERROR(Format("ResourceManager: Failed to open pak '%s' for reading (%u).", pak.path.c_str(), result));
			return false;
		}
	}

	if(!entry.compressed_size)
	{
		// read data
		Buf buf = bufs.Get();
		buf->resize(entry.size);
		if(!ReadFile(pak.file, buf->data(), entry.size, &tmp, nullptr))
		{
			DWORD result = GetLastError();
			ERROR(Format("ResourceManager: Failed to read pak '%s' data (%u) for entry %d.", pak.path.c_str(), result, res->pak.entry));
			return false;
		}

		pak_data.buf = buf;
		pak_data.size = entry.size;
	}
	else
	{
		// read compressed data
		Buf cbuf = bufs.Get();
		cbuf->resize(entry.compressed_size);
		if(!ReadFile(pak.file, cbuf->data(), entry.compressed_size, &tmp, nullptr))
		{
			DWORD result = GetLastError();
			ERROR(Format("ResourceManager: Failed to read pak '%s' data (%u) for entry %d.", pak.path.c_str(), result, res->pak.entry));
			return false;
		}

		// decompress
		Buf buf = bufs.Get();
		buf->resize(entry.size);
		uLongf size = entry.size;
		int result = uncompress(buf->data(), &size, cbuf->data(), entry.compressed_size);
		assert(result == Z_OK);
		assert(size == entry.size);
		bufs.Free(cbuf);

		pak_data.buf = buf;
		pak_data.size = entry.size;
	}

	return true;
}

cstring ResourceManager::GetPath(Resource2* res)
{
	if(res->pak.pak_id == INVALID_PAK)
		return res->path.c_str();
	else
		return Format("%s/%s", paks[res->pak.pak_id]->files[res->pak.entry].filename);
}

Resource2::Type ResourceManager::ExtToResourceType(cstring ext)
{
	if(strcmp(ext, "qmsh") == 0)
		return Resource2::Mesh;
	else if(strcmp(ext, "aiff") == 0 ||
		strcmp(ext, "asf") == 0 ||
		strcmp(ext, "asx") == 0 ||
		strcmp(ext, "dls") == 0 ||
		strcmp(ext, "flac") == 0 ||
		strcmp(ext, "it") == 0 ||
		strcmp(ext, "m3u") == 0 ||
		strcmp(ext, "midi") == 0 ||
		strcmp(ext, "mod") == 0 ||
		strcmp(ext, "mp2") == 0 ||
		strcmp(ext, "mp3") == 0 ||
		strcmp(ext, "ogg") == 0 ||
		strcmp(ext, "pls") == 0 ||
		strcmp(ext, "s3m") == 0 ||
		strcmp(ext, "wav") == 0 ||
		strcmp(ext, "wax") == 0 ||
		strcmp(ext, "wma") == 0 ||
		strcmp(ext, "xm") == 0)
		return Resource2::Sound;
	else if(strcmp(ext, "dmp") == 0 ||
		strcmp(ext, "dds") == 0 ||
		strcmp(ext, "dib") == 0 ||
		strcmp(ext, "hdr") == 0 ||
		strcmp(ext, "jpg") == 0 ||
		strcmp(ext, "pfm") == 0 ||
		strcmp(ext, "png") == 0 ||
		strcmp(ext, "ppm") == 0 ||
		strcmp(ext, "tga") == 0)
		return Resource2::Texture;
	else
		return Resource2::None;
}

Resource2::Type ResourceManager::FilenameToResourceType(cstring filename)
{
	cstring pos = strrchr(filename, '.');
	if(pos == nullptr || !(*pos + 1))
		return Resource2::None;
	else
		return ExtToResourceType(pos + 1);
}
/*
struct X
{
	X()
	{
		Resource2::Type type = ResourceManager::FilenameToResourceType("czlowiek.qmsh");
		int a = 3;
	}
};

X xxx;
*/