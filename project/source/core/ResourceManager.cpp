#include "Pch.h"
#include "Base.h"
#include "ResourceManager.h"
#include "Animesh.h"
#include "LoadScreen.h"
#include "Engine.h"

//-----------------------------------------------------------------------------
ResourceManager ResourceManager::manager;
ObjectPool<ResourceManager::TaskDetail> ResourceManager::task_pool;
ResourceManager::ResourceSubTypeInfo ResourceManager::res_info[] = {
	ResourceSubType::Unknown, ResourceType::Unknown, "unknown",
	ResourceSubType::Task, ResourceType::Unknown, "task",
	ResourceSubType::Callback, ResourceType::Unknown, "callback",
	ResourceSubType::Category, ResourceType::Unknown, "category",
	ResourceSubType::Mesh, ResourceType::Mesh, "mesh",
	ResourceSubType::MeshVertexData, ResourceType::Mesh, "mesh vertex data",
	ResourceSubType::Music, ResourceType::Sound, "music",
	ResourceSubType::Sound, ResourceType::Sound, "sound",
	ResourceSubType::Texture, ResourceType::Texture, "texture"
};

//=================================================================================================
ResourceManager::ResourceManager() : last_resource(nullptr), mode(Mode::Instant), mutex(nullptr)
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
		ERROR(Format("ResourceManager: Failed to add directory '%s' (%u).", dir, result));
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
		ERROR(Format("ResourceManager: Failed to add other files in directory '%s' (%u).", dir, result));

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
		ERROR(Format("ResourceManager: Failed to open pak '%s' (%u).", path, GetLastError()));
		return false;
	}
	
	// read header
	Pak::Header header;
	if(!stream.Read(header))
	{
		ERROR(Format("ResourceManager: Failed to read pak '%s' header.", path));
		return false;
	}
	if(header.sign[0] != 'P' || header.sign[1] != 'A' || header.sign[2] != 'K')
	{
		ERROR(Format("ResourceManager: Failed to read pak '%s', invalid signature %c%c%c.", path, header.sign[0], header.sign[1], header.sign[2]));
		return false;
	}
	if(header.version > 1)
	{
		ERROR(Format("ResourceManager: Failed to read pak '%s', invalid version %d.", path, (int)header.version));
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
			ERROR(Format("ResourceManager: Failed to read pak '%s' extra header (%u).", path, GetLastError()));
			return false;
		}
		total_size -= sizeof(PakV0::ExtraHeader);
		if(header2.files_size > (uint)total_size)
		{
			ERROR(Format("ResourceManager: Failed to read pak '%s', invalid files size %u (total size %u).", path, header2.files_size, total_size));
			return false;
		}
		if(header2.files * PakV0::File::MIN_SIZE > header2.files_size)
		{
			ERROR(Format("ResourceManager: Failed ot read pak '%s', invalid files count %u (files size %u, required size %u).", path, header2.files,
				header2.files_size, header2.files * PakV0::File::MIN_SIZE));
			return false;
		}

		// read files
		BufferHandle&& buf = stream.ReadToBuffer(header2.files_size);
		if(!buf)
		{
			ERROR(Format("ResourceManager: Failed to read pak '%s' files (%u).", path));
			return false;
		}

		// decrypt files
		if(IS_SET(header.flags, Pak::Encrypted))
		{
			if(key == nullptr)
			{
				ERROR(Format("ResourceManager: Failed to read pak '%s', file is encrypted.", path));
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
				ERROR(Format("ResourceManager: Failed to read pak '%s', broken file at index %u.", path, i));
				delete pak0;
				return false;
			}
			else
			{
				total_size -= file.size;
				if(total_size < 0)
				{
					ERROR(Format("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.size, i));
					delete pak0;
					return false;
				}
				if(file.offset + file.size >(int)pak_size)
				{
					ERROR(Format("ResourceManager: Failed to read pak '%s', file '%s' (%u) has invalid offset %u (pak size %u).",
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
			ERROR(Format("ResourceManager: Failed to read pak '%s' extra header (%u).", path, GetLastError()));
			return false;
		}
		total_size -= sizeof(PakV1::ExtraHeader);

		// read table
		if(!stream.Ensure(header2.file_entry_table_size) || !stream.Ensure(header2.files_count * sizeof(PakV1::File)))
		{
			ERROR(Format("ResourceManager: Failed to read pak '%s' files table (%u).", path, GetLastError()));
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
				ERROR(Format("ResourceManager: Failed to read pak '%s', file is encrypted.", path));
				return false;
			}
			Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
		}
		if(IS_SET(header.flags, Pak::FullEncrypted) && !IS_SET(header.flags, Pak::Encrypted))
		{
			BufferPool.Free(buf);
			ERROR(Format("ResourceManager: Failed to read pak '%s', invalid flags combination %u.", path, header.flags));
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
				ERROR(Format("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.compressed_size, i));
				delete pak1;
				return false;
			}

			if(file.offset + file.compressed_size > pak_size)
			{
				BufferPool.Free(buf);
				ERROR(Format("ResourceManager: Failed to read pak '%s', file at index %u has invalid offset %u (pak size %u).", 
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
		res->subtype = (int)ResourceSubType::Unknown;
		return res;
	}
	else
	{
		// already exists
		AnyResource* res = *result.first;
		if(res->pak_index != INVALID_PAK || res->path != path)
			WARN(Format("ResourceManager: Resource '%s' already exists (%s; %s).", filename, GetPath(res), path));
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

	task_pool.Free(tasks);
	task_pool.Free(next_tasks);
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

	Animesh::MeshInit();
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

//=================================================================================================
void ResourceManager::ApplyTask(Task* task)
{
	if(IS_SET(task->flags, Task::Assign))
	{
		void** ptr = (void**)task->ptr;
		*ptr = task->res->data;
	}
	else
		task->callback(*task);
}

//=================================================================================================
void ResourceManager::LoadResource(AnyResource* res)
{
	assert(res->state != ResourceState::Loaded);
	switch((ResourceSubType)res->subtype)
	{
	case ResourceSubType::Mesh:
		LoadMeshInternal((MeshResource*)res);
		break;
	case ResourceSubType::MeshVertexData:
		LoadMeshVertexDataInternal((MeshResource*)res);
		break;
	case ResourceSubType::Music:
		LoadMusicInternal((SoundResource*)res);
		break;
	case ResourceSubType::Sound:
		LoadSoundInternal((SoundResource*)res);
		break;
	case ResourceSubType::Texture:
		LoadTextureInternal((TextureResource*)res);
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
void ResourceManager::LoadMeshInternal(MeshResource* res)
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
	res->subtype = (int)ResourceSubType::Mesh;
}

//=================================================================================================
void ResourceManager::LoadMeshVertexDataInternal(MeshResource* res)
{
	StreamReader&& reader = GetStream(res, StreamType::FullFileOrMemory);

	try
	{
		VertexData* vd = Animesh::LoadVertexData(reader);
		res->data = (Animesh*)vd;
		res->state = ResourceState::Loaded;
		res->subtype = (int)ResourceSubType::MeshVertexData;
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh vertex data '%s'. %s", GetPath(res), err);
	}
}

//=================================================================================================
void ResourceManager::LoadMusicInternal(SoundResource* res)
{
	assert(fmod_system);

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
	res->subtype = (int)ResourceSubType::Music;
}

//=================================================================================================
void ResourceManager::LoadSoundInternal(SoundResource* res)
{
	assert(fmod_system);

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
	res->subtype = (int)ResourceSubType::Sound;
}

//=================================================================================================
void ResourceManager::LoadTextureInternal(TextureResource* res)
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
	res->subtype = (int)ResourceSubType::Texture;
}

//=================================================================================================
void ResourceManager::AddTask(Task& task)
{
	if(mode == Mode::Instant || mode == Mode::LoadScreenStart)
		ApplyTask(&task);
	else
	{
		assert(mode == Mode::LoadScreenPrepare || mode == Mode::LoadScreenNext);

		TaskDetail* td = task_pool.Get();
		td->category = nullptr;
		td->delegate = task.callback;
		td->flags = task.flags;
		td->ptr = task.ptr;
		td->res = nullptr;
		td->type = ResourceSubType::Task;

		if(mode == Mode::LoadScreenPrepare)
		{
			tasks.push_back(td);
			++to_load;
		}
		else
		{
			next_tasks.push_back(td);
			++to_load_next;
		}
	}
}

//=================================================================================================
void ResourceManager::AddTaskCategory(cstring category)
{
	assert(mode == Mode::LoadScreenPrepare || mode == Mode::LoadScreenNext);

	TaskDetail* td = task_pool.Get();
	td->category = category;
	td->delegate = nullptr;
	td->flags = 0;
	td->ptr = nullptr;
	td->res = nullptr;
	td->type = ResourceSubType::Category;

	if(mode == Mode::LoadScreenPrepare)
		tasks.push_back(td);
	else
		next_tasks.push_back(td);
}

//=================================================================================================
void ResourceManager::AddTask(VoidF& callback, cstring category, int size)
{
	assert(mode == Mode::LoadScreenPrepare || mode == Mode::LoadScreenNext);

	TaskDetail* td = task_pool.Get();
	td->category = category;
	td->delegate = callback;
	td->flags = TaskDetail::VoidCallback;
	td->ptr = nullptr;
	td->res = nullptr;
	td->type = ResourceSubType::Callback;

	if(mode == Mode::LoadScreenPrepare)
	{
		tasks.push_back(td);
		to_load += size;
	}
	else
	{
		next_tasks.push_back(td);
		to_load_next += size;
	}
}

//=================================================================================================
void ResourceManager::PrepareLoadScreen(float cap)
{
	assert((mode == Mode::Instant || mode == Mode::LoadScreenEnd) && cap >= 0.f && cap <= 1.f);

	if(mode == Mode::Instant)
	{
		to_load = 0;
		loaded = 0;
		to_load_next = 0;
		old_load_cap = 0.f;
		load_cap = cap;
	}
	else
	{
		assert(cap > load_cap);
		old_load_cap = load_cap;
		load_cap = cap;
	}
	
	mode = Mode::LoadScreenPrepare;
}

//=================================================================================================
void ResourceManager::PrepareLoadScreen2(float cap, int steps, cstring text)
{
	assert(mode == Mode::Instant && cap >= 0.f && cap <= 1.f);

	to_load = steps;
	loaded = 0;
	to_load_next = 0;
	old_load_cap = 0.f;
	load_cap = cap;
	mode = Mode::LoadScreenPrepare2;
	timer.Start();
	timer_dt = 1.f;
	category = text;
	TickLoadScreen();
}

//=================================================================================================
void ResourceManager::EndLoadScreenStage()
{
	assert((mode == Mode::LoadScreenPrepare || mode == Mode::LoadScreenPrepare2) && load_cap != 1.f);

	if(mode == Mode::LoadScreenPrepare)
		mode = Mode::LoadScreenNext;
	else
	{
		mode = Mode::LoadScreenEnd;
		task_pool.Free(tasks);
		to_load = to_load_next;
		loaded = 0;
		to_load_next = 0;
		next_tasks.swap(tasks);
	}
}

//=================================================================================================
void ResourceManager::StartLoadScreen()
{
	assert(mode == Mode::LoadScreenPrepare || mode == Mode::LoadScreenNext);
	
	if(mode == Mode::LoadScreenPrepare)
		mode = Mode::LoadScreenStart;
	UpdateLoadScreen();

	if(load_cap == 1.f)
	{
		// cleanup
		assert(next_tasks.empty());

		mode = Mode::Instant;
		task_pool.Free(tasks);
	}
	else
	{
		mode = Mode::LoadScreenEnd;
		task_pool.Free(tasks);
		to_load = to_load_next;
		loaded = 0;
		to_load_next = 0;
		next_tasks.swap(tasks);
	}
}

//=================================================================================================
void ResourceManager::UpdateLoadScreen()
{
	Engine& engine = Engine::Get();
	if(to_load == loaded)
	{
		// no tasks to load, draw with full progress
		load_screen->SetProgress(load_cap, "");
		engine.DoPseudotick();
		return;
	}

	// draw first frame
	float progress = float(loaded)/to_load * (load_cap - old_load_cap) + old_load_cap;
	category = tasks[loaded]->category;
	load_screen->SetProgressOptional(progress, category);
	engine.DoPseudotick();

	// do all tasks
	timer.Reset();
	timer_dt = 0;
	for(uint i = 0; i<tasks.size(); ++i)
	{
		TaskDetail* task = tasks[i];
		if(task->res && task->res->state != ResourceState::Loaded)
			LoadResource(task->res);

		if(task->category)
			category = task->category;

		if(task->type != ResourceSubType::Category)
			++loaded;

		if(IS_SET(task->flags, TaskDetail::Assign))
		{
			void** ptr = (void**)task->ptr;
			*ptr = task->res->data;
		}
		else if(task->delegate)
		{
			if(IS_SET(task->flags, TaskDetail::VoidCallback))
			{
				VoidF callback = (VoidF)task->delegate;
				callback();
			}
			else
				task->delegate(*(TaskData*)task);
		}

		TickLoadScreen();
	}

	// draw last frame
	load_screen->SetProgress(load_cap);
	engine.DoPseudotick();
}

//=================================================================================================
void ResourceManager::NextTask(cstring _category)
{
	if(_category)
		category = _category;
	++loaded;

	TickLoadScreen();
}

//=================================================================================================
void ResourceManager::TickLoadScreen()
{
	timer_dt += timer.Tick();
	if(timer_dt >= 0.05f)
	{
		timer_dt = 0.f;
		float progress = float(loaded)/to_load * (load_cap - old_load_cap) + old_load_cap;
		load_screen->SetProgressOptional(progress, category);
		Engine::Get().DoPseudotick();

		if(mutex && progress >= 0.5f)
		{
			ReleaseMutex(mutex);
			CloseHandle(mutex);
			mutex = nullptr;
		}
	}
}

//=================================================================================================
AnyResource* ResourceManager::GetResource(cstring filename, ResourceSubType type)
{
	assert(filename);

	auto& info = res_info[(int)type];
	BaseResource* res = GetResource(filename, info.type);
	if(!res)
		throw Format("ResourceManager: Missing %s '%s'.", info.name, filename);
	if(res->subtype != (int)type)
	{
		if(res->subtype != (int)ResourceSubType::Unknown)
			throw Format("ResourceManager: Resource type mismatch '%s' (%s, %s).", filename, res_info[res->subtype].name, info.name);
		res->subtype = (int)type;
	}

	return (AnyResource*)res;
}

//=================================================================================================
AnyResource* ResourceManager::TryGetResource(cstring filename, ResourceSubType type)
{
	assert(filename);

	auto& info = res_info[(int)type];
	BaseResource* res = GetResource(filename, info.type);
	if(!res)
		return nullptr;
	if(res->subtype != (int)type)
	{
		if(res->subtype != (int)ResourceSubType::Unknown)
			throw Format("ResourceManager: Resource type mismatch '%s' (%s, %s).", filename, res_info[res->subtype].name, info.name);
		res->subtype = (int)type;
	}

	return (AnyResource*)res;
}

//=================================================================================================
void ResourceManager::LoadResource(AnyResource* res, Task* task)
{
	assert(res);

	if(res->state == ResourceState::Loaded)
	{
		if(task)
		{
			task->res = res;
			ApplyTask(task);
		}
		return;
	}

	if(res->state == ResourceState::NotLoaded && (mode == Mode::Instant || mode == Mode::LoadScreenStart))
	{
		LoadResource(res);
		if(task)
		{
			task->res = res;
			ApplyTask(task);
		}
	}
	else
	{
		TaskDetail* td = task_pool.Get();
		if(task)
		{
			td->delegate = task->callback;
			td->flags = task->flags;
			td->ptr = task->ptr;
		}
		else
		{
			td->delegate = nullptr;
			td->flags = 0;
		}
		td->category = nullptr;
		td->res = res;
		td->type = (ResourceSubType)res->subtype;

		res->state = ResourceState::Loading;

		if(mode == Mode::LoadScreenPrepare)
		{
			tasks.push_back(td);
			++to_load;
		}
		else
		{
			assert(mode == Mode::LoadScreenNext);
			next_tasks.push_back(td);
			++to_load_next;
		}
	}
}
