#include "Pch.h"
#include "Base.h"
#include "ResourceManager.h"
#include "Animesh.h"

//-----------------------------------------------------------------------------
cstring c_resmgr = "ResourceManager";

//-----------------------------------------------------------------------------
ResourceManager ResourceManager::manager;

//=================================================================================================
ResourceManager::ResourceManager() : last_resource(nullptr), mode(Mode::Instant)
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
	if(header.version > 1)
	{
		logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid version %d.", path, (int)header.sign[3]));
		return false;
	}

	Pak* pak;
	int pak_index = paks.size();
	uint pak_size = stream.GetSize();
	int total_size = pak_size - sizeof(Pak::Header);

	if(header.version == 0)
	{
		// read extra header
		PakV0::ExtraHeader header2;
		if(!stream.Read(header2))
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s' extra header (%u).", path, GetLastError()));
			return false;
		}
		total_size -= sizeof(PakV0::ExtraHeader);
		if(header2.files_size > (uint)total_size)
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid files size %u (total size %u).", path, header2.files_size, total_size));
			return false;
		}
		if(header2.files * PakV0::File::MIN_SIZE > header2.files_size)
		{
			logger->Error(c_resmgr, Format("Failed ot read pak '%s', invalid files count %u (files size %u, required size %u).", path, header2.files,
				header2.files_size, header2.files * PakV0::File::MIN_SIZE));
			return false;
		}

		// read files
		BufferHandle&& buf = stream.Read(header2.files_size);
		if(!buf)
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s' files (%u).", path));
			return false;
		}

		// decrypt files
		if(IS_SET(header.flags, Pak::Encrypted))
		{
			if(key == nullptr)
			{
				logger->Error(c_resmgr, Format("Failed to read pak '%s', file is encrypted.", path));
				return false;
			}
			Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
		}

		// setup files
		PakV0* pak0 = new PakV0;
		pak = pak0;
		pak0->files.resize(header2.files);
		StreamReader buf_stream(buf);
		for(uint i = 0; i < header2.files; ++i)
		{
			PakV0::File& file = pak0->files[i];
			if(!buf_stream.Read(file.name)
				|| !buf_stream.Read(file.size)
				|| !buf_stream.Read(file.offset))
			{
				logger->Error(c_resmgr, Format("Failed to read pak '%s', broken file at index %u.", path, i));
				delete pak0;
				return false;
			}
			else
			{
				total_size -= file.size;
				if(total_size < 0)
				{
					logger->Error(c_resmgr, Format("Failed to read pak '%s', broken file size %u at index %u.", path, file.size, i));
					delete pak0;
					return false;
				}
				if(file.offset + file.size >(int)pak_size)
				{
					logger->Error(c_resmgr, Format("Failed to read pak '%s', file '%s' (%u) has invalid offset %u (pak size %u).",
						path, file.name.c_str(), i, file.offset, pak_size));
					delete pak0;
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
	}
	else
	{
		// read extra header
		PakV1::ExtraHeader header2;
		if(!stream.Read(header2))
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s' extra header (%u).", path, GetLastError()));
			return false;
		}
		total_size -= sizeof(PakV1::ExtraHeader);

		// read table
		if(!stream.Ensure(header2.file_entry_table_size) || !stream.Ensure(header2.files_count * sizeof(PakV1::File)))
		{
			logger->Error(c_resmgr, Format("Failed to read pak '%s' files table (%u).", path, GetLastError()));
			return false;
		}
		Buffer* buf = BufferPool.Get();
		buf->Resize(header2.file_entry_table_size);
		stream.Read(buf->Data(), header2.file_entry_table_size);
		total_size -= header2.file_entry_table_size;

		// decrypt table
		if(IS_SET(header.flags, Pak::Encrypted))
		{
			if(key == nullptr)
			{
				BufferPool.Free(buf);
				logger->Error(c_resmgr, Format("Failed to read pak '%s', file is encrypted.", path));
				return false;
			}
			Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
		}
		if(IS_SET(header.flags, Pak::FullEncrypted) && !IS_SET(header.flags, Pak::Encrypted))
		{
			BufferPool.Free(buf);
			logger->Error(c_resmgr, Format("Failed to read pak '%s', invalid flags combination %u.", path, header.flags));
			return false;
		}

		// setup pak
		PakV1* pak1 = new PakV1;
		pak = pak1;
		pak1->encrypted = IS_SET(header.flags, Pak::FullEncrypted);
		if(key)
			pak1->key = key;
		pak1->filename_buf = buf;
		pak1->files = (PakV1::File*)buf->Data();
		for(uint i = 0; i < header2.files_count; ++i)
		{
			PakV1::File& file = pak1->files[i];
			file.filename = (cstring)buf->Data() + file.filename_offset;
			total_size -= file.compressed_size;

			if(total_size < 0)
			{
				BufferPool.Free(buf);
				logger->Error(c_resmgr, Format("Failed to read pak '%s', broken file size %u at index %u.", path, file.compressed_size, i));
				delete pak1;
				return false;
			}

			if(file.offset + file.compressed_size > pak_size)
			{
				BufferPool.Free(buf);
				logger->Error(c_resmgr, Format("Failed to read pak '%s', file at index %u has invalid offset %u (pak size %u).", 
					path, i, file.offset, pak_size));
				delete pak1;
				return false;
			}

			BaseResource* res = AddResource(file.filename, path);
			if(res)
			{
				res->pak_index = pak_index;
				res->pak_file_index = i;
				res->filename = file.filename;
			}
		}
	}

	pak->file = stream.PinFile();
	pak->version = header.version;
	pak->path = path;
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
				if(res->subtype == (int)ResourceSubType::Mesh)
					delete (Animesh*)res->data;
				break;
			}
		}
		delete res;
	}

	for(Pak* pak : paks)
	{
		CloseHandle(pak->file);
		if(pak->version == 0)
			delete (PakV0*)pak;
		else
		{
			PakV1* pak1 = (PakV1*)pak;
			BufferPool.Free(pak1->filename_buf);
			delete pak1;
		}
	}

	for(Buffer* buf : sound_bufs)
		BufferPool.Free(buf);
}

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
		if(pak.version == 0)
		{
			PakV0& pak0 = (PakV0&)pak;
			PakV0::File& file = pak0.files[res->pak_file_index];
			return BufferHandle(StreamReader::LoadToBuffer(pak0.file, file.offset, file.size));
		}
		else
		{
			PakV1& pak1 = (PakV1&)pak;
			PakV1::File& file = pak1.files[res->pak_file_index];
			Buffer* buf = StreamReader::LoadToBuffer(pak.file, file.offset, file.compressed_size);
			if(pak1.encrypted)
				Crypt((char*)buf->Data(), buf->Size(), pak1.key.c_str(), pak1.key.length());
			if(file.compressed_size != file.size)
				buf = buf->Decompress(file.size);
			return BufferHandle(buf);
		}
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
		uint size, compressed_size, offset;
		cstring key = nullptr;

		if(pak.version == 0)
		{
			PakV0& pak0 = (PakV0&)pak;
			PakV0::File& file = pak0.files[res->pak_file_index];
			size = file.size;
			compressed_size = file.size;
			offset = file.offset;
		}
		else
		{
			PakV1& pak1 = (PakV1&)pak;
			PakV1::File& file = pak1.files[res->pak_file_index];
			size = file.size;
			compressed_size = file.compressed_size;
			offset = file.offset;
			if(pak1.encrypted)
				key = pak1.key.c_str();
		}

		if(type == StreamType::File && size == compressed_size && !key)
			return StreamReader(pak.file, offset, size);
		else
		{
			Buffer* buf = StreamReader::LoadToBuffer(pak.file, offset, compressed_size);
			if(key)
				Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
			if(size != compressed_size)
				buf = buf->Decompress(size);
			return StreamReader(buf);
		}
	}
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
void ResourceManager::Init(IDirect3DDevice9* _device, FMOD::System* _fmod_system)
{
	device = _device;
	fmod_system = _fmod_system;

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




ResourceManager::ResourceSubTypeInfo ResourceManager::res_info[] = {
	ResourceSubType::Mesh, ResourceType::Mesh, "mesh",
	ResourceSubType::MeshVertexData, ResourceType::Mesh, "mesh vertex data",
	ResourceSubType::Music, ResourceType::Sound, "music",
	ResourceSubType::Sound, ResourceType::Sound, "sound",
	ResourceSubType::Texture, ResourceType::Texture, "texture"
};

_declspec(thread) bool is_main;
ObjectPool<ResourceManager::Task> ResourceManager::task_pool;

BaseResource* ResourceManager::GetLoadedResource(cstring filename, ResourceSubType sub_type, TaskData* task_data)
{
	assert(filename);

	ResourceSubTypeInfo& info = res_info[(int)sub_type];
	BaseResource* res = GetResource(filename, info.type);
	if(!res)
		throw Format("ResourceManager: Missing %s '%s'.", info.name, filename);

	if(res->state == ResourceState::Loaded)
	{
		if(task_data && IS_SET(task_data->flags, TaskData::AlwaysHandle))
			ApplyTask(task_data, res);
		return res;
	}

	if(res->state == ResourceState::NotLoaded)
	{
		if(mode == Mode::Instant || !is_main)
		{
			LoadResource(res, sub_type);
			if(task_data)
				ApplyTask(task_data, res);
		}
		else
			AddTask(res, sub_type, task_data);
	}

	return res;
}

void ResourceManager::ApplyTask(TaskData* task_data, BaseResource* res)
{
	AnyResource* r = (AnyResource*)res;
	if(IS_SET(task_data->flags, TaskData::Assign))
	{
		void** ptr = (void**)task_data->ptr;
		*ptr = r->data;
	}
	else if(is_main || !IS_SET(task_data->flags, TaskData::MainThreadCallback))
	{
		task_data->res = r;
		task_data->callback(task_data);
	}
	else
	{
		Task* task = task_pool.Get();
		task->callback = task_data->callback;
		task->flags = task_data->flags;
		task->ptr = task_data->ptr;
		task->res = r;
		callback_tasks.push_back(task);
	}
}

void ResourceManager::AddTask(BaseResource* res, ResourceSubType type, TaskData* task_data)
{
	Task* task = task_pool.Get();
	if(task_data)
	{
		task->callback = task_data->callback;
		task->flags = task_data->flags;
		task->ptr = task_data->ptr;
	}
	else
	{
		task->callback = nullptr;
		task->flags = 0;
	}
	task->res = (AnyResource*)res;
	task->type = type;
	tasks.push_back(task);

	res->state = ResourceState::Loading;
	res->subtype = (int)type;
}

void ResourceManager::LoadResource(BaseResource* res, ResourceSubType type)
{
	switch(type)
	{
	case ResourceSubType::Mesh:
		LoadMesh((MeshResource*)res);
		break;
	case ResourceSubType::MeshVertexData:
		LoadMeshVertexData((MeshResource*)res);
		break;
	case ResourceSubType::Music:
		LoadMusic((SoundResource*)res);
		break;
	case ResourceSubType::Sound:
		LoadSound((SoundResource*)res);
		break;
	case ResourceSubType::Texture:
		LoadTexture((TextureResource*)res);
		break;
	}

	res->subtype = (int)type;
}

void ResourceManager::LoadMesh(MeshResource* res)
{
	StreamReader&& reader = GetStream(res, StreamType::FullFileOrMemory);
	Mesh* mesh = new Mesh;

	try
	{
		mesh->Load(reader, device);
	}
	catch(cstring err)
	{
		delete mesh;
		throw Format("ResourceManager: Failed to load mesh '%s'. %s", GetPath(res), err);
	}

	mesh->res = res;
	res->data = mesh;
	res->state = ResourceState::Loaded;
}

void ResourceManager::LoadMeshVertexData(MeshResource* res)
{
	StreamReader&& reader = GetStream(res, StreamType::FullFileOrMemory);

	try
	{
		VertexData* vd = Animesh::LoadVertexData(reader);
		res->data = (Animesh*)vd;
		res->state = ResourceState::Loaded;
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh vertex data '%s'. %s", GetPath(res), err);
	}
}

void ResourceManager::LoadMusic(SoundResource* res)
{
	FMOD_RESULT result;
	if(res->IsFile())
		result = fmod_system->createStream(res->path.c_str(), FMOD_HARDWARE | FMOD_LOWMEM | FMOD_2D, nullptr, &res->data);
	else
	{
		BufferHandle&& buf = GetBuffer(res);
		FMOD_CREATESOUNDEXINFO info = { 0 };
		info.cbsize = sizeof(info);
		info.length = buf->Size();
		result = fmod_system->createStream((cstring)buf->Data(), FMOD_HARDWARE | FMOD_LOWMEM | FMOD_2D | FMOD_OPENMEMORY, &info, &res->data);
		if(result == FMOD_OK)
			sound_bufs.push_back(buf.Pin());
	}

	if(result != FMOD_OK)
		throw Format("ResourceManager: Failed to load music '%s' (%d).", res->path.c_str(), result);

	res->state = ResourceState::Loaded;
}

void ResourceManager::LoadSound(SoundResource* res)
{
	FMOD_RESULT result;
	if(res->IsFile())
		result = fmod_system->createSound(res->path.c_str(), FMOD_HARDWARE | FMOD_LOWMEM | FMOD_3D | FMOD_LOOP_OFF, nullptr, &res->data);
	else
	{
		BufferHandle&& buf = GetBuffer(res);
		FMOD_CREATESOUNDEXINFO info = { 0 };
		info.cbsize = sizeof(info);
		info.length = buf->Size();
		result = fmod_system->createSound((cstring)buf->Data(), FMOD_HARDWARE | FMOD_LOWMEM | FMOD_3D | FMOD_LOOP_OFF | FMOD_OPENMEMORY, &info, &res->data);
		if(result == FMOD_OK)
			sound_bufs.push_back(buf.Pin());
	}

	if(result != FMOD_OK)
		throw Format("ResourceManager: Failed to load sound '%s' (%d).", res->path.c_str(), result);

	res->state = ResourceState::Loaded;
}

void ResourceManager::LoadTexture(TextureResource* res)
{
	HRESULT hr;
	if(res->IsFile())
		hr = D3DXCreateTextureFromFile(device, res->path.c_str(), &res->data);
	else
	{
		BufferHandle&& buf = GetBuffer(res);
		hr = D3DXCreateTextureFromFileInMemory(device, buf->Data(), buf->Size(), &res->data);
	}

	if(FAILED(hr))
		throw Format("Failed to load texture '%s' (%u).", GetPath(res), hr);
	
	res->state = ResourceState::Loaded;
}
