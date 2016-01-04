#include "Pch.h"
#include "Base.h"
#include "ResourceManager.h"
#include "Animesh.h"

//-----------------------------------------------------------------------------
cstring c_resmgr = "ResourceManager";

//-----------------------------------------------------------------------------
ResourceManager ResourceManager::manager;

//=================================================================================================
ResourceManager::ResourceManager() : last_resource(nullptr)
{
}

//=================================================================================================
ResourceManager::~ResourceManager()
{
	delete last_resource;
}

//=================================================================================================
bool ResourceManager::AddDir(cstring dir, bool subdir)
{
	assert(dir);

	WIN32_FIND_DATA find_data;
	HANDLE find = FindFirstFile(Format("%s/*.*", dir), &find_data);

	if(find == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		logger->Error(c_resmgr, Format("Failed to add directory '%s' (%u).", dir, result));
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
				cstring path = Format("%s/%s", dir, find_data.cFileName);
				BaseResource* res = AddResource(find_data.cFileName, path);
				if(res)
				{
					res->pak_index = INVALID_PAK;
					res->path = path;
					res->filename = res->path.c_str() + dirlen;
				}
			}
		}
	}
	while(FindNextFile(find, &find_data) != 0);

	DWORD result = GetLastError();
	if(result != ERROR_NO_MORE_FILES)
		logger->Error(c_resmgr, Format("Failed to add other files in directory '%s' (%u).", dir, result));

	FindClose(find);

	return true;
}

//=================================================================================================
bool ResourceManager::AddPak(cstring path, cstring key)
{
	assert(path);

	StreamReader stream(path);
	if(!stream)
	{
		logger->Error(c_resmgr, Format("Failed to open pak '%s' (%u).", path, GetLastError()));
		return false;
	}
	
	// read header
	Pak::Header header;
	if(!stream.Read(header))
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s' header.", path));
		return false;
	}
	if(header.sign[0] != 'P' || header.sign[1] != 'A' || header.sign[2] != 'K')
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid signature %c%c%c.", path, header.sign[0], header.sign[1], header.sign[2]));
		return false;
	}
	if(header.sign[3] != 0)
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid version %d.", path, (int)header.sign[3]));
		return false;
	}

	// read files
	uint pak_size = stream.GetSize();
	uint total_size = pak_size - sizeof(Pak::Header);
	if(header.files_size > total_size)
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid files size %u (total size %u).", path, header.files_size, total_size));
		return false;
	}
	if(header.files * Pak::File::MIN_SIZE > header.files_size)
	{
		logger->Error(c_resmgr, Format("Failed ot read pak '%s', invalid files count %u (files size %u, required size %u).", path, header.files,
			header.files_size, header.files * Pak::File::MIN_SIZE));
		return false;
	}
	BufferHandle&& buf = stream.Read(header.files_size);
	if(!buf)
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s' files (%u).", path));
		return false;
	}
	if(IS_SET(header.flags, Pak::Encrypted))
	{
		if(key == nullptr)
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s', file is encrypted.", path));
			return false;
		}
		Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
	}
	Pak* pak = new Pak;
	pak->path = path;
	pak->files.resize(header.files);
	int pak_index = paks.size();
	StreamReader buf_stream(buf);
	for(uint i = 0; i<header.files; ++i)
	{
		Pak::File& file = pak->files[i];
		if(!buf_stream.Read(file.name)
			|| !buf_stream.Read(file.size)
			|| !buf_stream.Read(file.offset))
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s', broken file at index %u.", path, i));
			delete pak;
			return false;
		}
		else
		{
			total_size -= file.size;
			if(total_size < 0)
			{
				logger->Error(c_resmgr, Format("Failed to read pak '%s', broken file size %u at index %u.", path, file.size, i));
				delete pak;
				return false;
			}
			if(file.offset + file.size > (int)pak_size)
			{
				logger->Error(c_resmgr, Format("Failed to read pak '%s', file '%s' (%u) has invalid offset %u (pak size %u).",
					path, file.name.c_str(), i, file.offset, pak_size));
				delete pak;
				return false;
			}

			BaseResource* res = AddResource(file.name.c_str(), path);
			if(res)
			{
				res->pak_index = pak_index;
				res->pak_file_index = i;
				res->path = file.name;
				res->filename = res->path.c_str();
			}
		}
	}

	pak->file = stream.PinFile();
	paks.push_back(pak);
	return true;
}

//=================================================================================================
BaseResource* ResourceManager::AddResource(cstring filename, cstring path)
{
	assert(filename && path);

	ResourceType type = FilenameToResourceType(filename);
	if(type == ResourceType::Unknown)
		return nullptr;

	if(!last_resource)
		last_resource = new AnyResource;
	last_resource->filename = filename;
	last_resource->type = type;

	std::pair<ResourceIterator, bool>& result = resources.insert(last_resource);
	if(result.second)
	{
		// added
		AnyResource* res = last_resource;
		last_resource = nullptr;
		res->data = nullptr;
		res->state = ResourceState::NotLoaded;
		return res;
	}
	else
	{
		// already exists
		logger->Warn(c_resmgr, Format("Resource '%s' already exists (%s; %s).", filename, GetPath(*result.first), path));
		return nullptr;
	}
}

//=================================================================================================
void ResourceManager::Cleanup()
{
	for(AnyResource* res : resources)
	{
		if(res->state == ResourceState::Loaded)
		{
			switch(res->type)
			{
			case ResourceType::Texture:
				((TEX)res->data)->Release();
				break;
			case ResourceType::Mesh:
				delete (Animesh*)res->data;
				break;
			}
		}
		delete res;
	}

	for(Pak* pak : paks)
	{
		CloseHandle(pak->file);
		delete pak;
	}
}

/*
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
	short pak_id = paks.size();
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
*/

//=================================================================================================
ResourceType ResourceManager::ExtToResourceType(cstring ext)
{
	auto it = exts.find(ext);
	if(it != exts.end())
		return it->second;
	else
		return ResourceType::Unknown;
}

//=================================================================================================
ResourceType ResourceManager::FilenameToResourceType(cstring filename)
{
	cstring pos = strrchr(filename, '.');
	if(pos == nullptr || !(*pos+1))
		return ResourceType::Unknown;
	else
		return ExtToResourceType(pos + 1);
}

//=================================================================================================
BufferHandle ResourceManager::GetBuffer(BaseResource* res)
{
	assert(res);

	if(res->pak_index == INVALID_PAK)
		return BufferHandle(StreamReader::LoadToBuffer(res->path));
	else
	{
		Pak& pak = *paks[res->pak_index];
		Pak::File& file = pak.files[res->pak_file_index];
		return BufferHandle(StreamReader::LoadToBuffer(pak.file, file.offset, file.size));
	}
}

//=================================================================================================
cstring ResourceManager::GetPath(BaseResource* res)
{
	assert(res);

	if(res->pak_index == INVALID_PAK)
		return res->path.c_str();
	else
		return Format("%s/%s", paks[res->pak_index]->path.c_str(), res->filename);
}

//=================================================================================================
StreamReader ResourceManager::GetStream(BaseResource* res, StreamType type)
{
	assert(res);

	if(res->pak_index == INVALID_PAK)
	{
		if(type == StreamType::Memory)
			return StreamReader::LoadAsMemoryStream(res->path);
		else
			return StreamReader(res->path);
	}
	else
	{
		Pak& pak = *paks[res->pak_index];
		Pak::File& file = pak.files[res->pak_file_index];
		if(type == StreamType::File)
			return StreamReader(pak.file, file.offset, file.size);
		else
			return StreamReader::LoadAsMemoryStream(pak.file, file.offset, file.size);
	}
}

//=================================================================================================
TextureResource* ResourceManager::GetTexture(cstring filename)
{
	assert(filename);

	TextureResource* tex = GetResource<TextureResource>(filename);
	if(tex == nullptr)
	{
		logger->Error(c_resmgr, Format("Missing texture '%s'.", filename));
		tex = new TextureResource;
		tex->data = nullptr;
		tex->path = filename;
		tex->filename = tex->path.c_str();
		tex->state = ResourceState::Missing;
		tex->type = ResourceType::Texture;
		resources.insert((AnyResource*)tex);
		return tex;
	}

	if(tex->state == ResourceState::NotLoaded)
	{
		HRESULT hr;
		if(tex->IsFile())
			hr = D3DXCreateTextureFromFile(device, tex->path.c_str(), &tex->data);
		else
		{
			BufferHandle&& buf = GetBuffer(tex);
			hr = D3DXCreateTextureFromFileInMemory(device, buf->Data(), buf->Size(), &tex->data);
		}

		if(FAILED(hr))
		{
			logger->Error(c_resmgr, Format("Failed to load texture '%s' (%u).", GetPath((BaseResource*)tex), hr));
			tex->state = ResourceState::Missing;
		}
		else
			tex->state = ResourceState::Loaded;
	}

	return tex;
}

//=================================================================================================
BaseResource* ResourceManager::GetResource(cstring filename, ResourceType type)
{
	AnyResource res;
	res.type = type;
	res.filename = filename;

	auto it = resources.find(&res);
	if(it != resources.end())
		return *it;
	else
		return nullptr;
}

//=================================================================================================
void ResourceManager::Init(IDirect3DDevice9* _device)
{
	device = _device;

	RegisterExtensions();
}

//=================================================================================================
void ResourceManager::RegisterExtensions()
{
	exts["bmp"] = ResourceType::Texture;
	exts["dds"] = ResourceType::Texture;
	exts["dib"] = ResourceType::Texture;
	exts["hdr"] = ResourceType::Texture;
	exts["jpg"] = ResourceType::Texture;
	exts["pfm"] = ResourceType::Texture;
	exts["png"] = ResourceType::Texture;
	exts["ppm"] = ResourceType::Texture;
	exts["tga"] = ResourceType::Texture;

	exts["qmsh"] = ResourceType::Mesh;

	exts["aiff"] = ResourceType::Sound;
	exts["asf"] = ResourceType::Sound;
	exts["asx"] = ResourceType::Sound;
	exts["dls"] = ResourceType::Sound;
	exts["flac"] = ResourceType::Sound;
	exts["it"] = ResourceType::Sound;
	exts["m3u"] = ResourceType::Sound;
	exts["midi"] = ResourceType::Sound;
	exts["mod"] = ResourceType::Sound;
	exts["mp2"] = ResourceType::Sound;
	exts["mp3"] = ResourceType::Sound;
	exts["ogg"] = ResourceType::Sound;
	exts["pls"] = ResourceType::Sound;
	exts["s3m"] = ResourceType::Sound;
	exts["wav"] = ResourceType::Sound;
	exts["wax"] = ResourceType::Sound;
	exts["wma"] = ResourceType::Sound;
	exts["xm"] = ResourceType::Sound;
}
