#include "Pch.h"
#include "Core.h"
#include "ResourceManager.h"
#include "LoadScreen.h"
#include "Engine.h"
#include "Mesh.h"
#include "Utility.h"

//-----------------------------------------------------------------------------
ResourceManager ResourceManager::manager;
ObjectPool<ResourceManager::TaskDetail> ResourceManager::task_pool;

//=================================================================================================
ResourceManager::ResourceManager() : mode(Mode::Instant)
{
}

//=================================================================================================
ResourceManager::~ResourceManager()
{
}

//=================================================================================================
void ResourceManager::Init(IDirect3DDevice9* device, FMOD::System* fmod_system)
{
	this->device = device;
	this->fmod_system = fmod_system;

	RegisterExtensions();

	Mesh::MeshInit();
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

	exts["phy"] = ResourceType::VertexData;

	exts["aiff"] = ResourceType::SoundOrMusic;
	exts["asf"] = ResourceType::SoundOrMusic;
	exts["asx"] = ResourceType::SoundOrMusic;
	exts["dls"] = ResourceType::SoundOrMusic;
	exts["flac"] = ResourceType::SoundOrMusic;
	exts["it"] = ResourceType::SoundOrMusic;
	exts["m3u"] = ResourceType::SoundOrMusic;
	exts["midi"] = ResourceType::SoundOrMusic;
	exts["mod"] = ResourceType::SoundOrMusic;
	exts["mp2"] = ResourceType::SoundOrMusic;
	exts["mp3"] = ResourceType::SoundOrMusic;
	exts["ogg"] = ResourceType::SoundOrMusic;
	exts["pls"] = ResourceType::SoundOrMusic;
	exts["s3m"] = ResourceType::SoundOrMusic;
	exts["wav"] = ResourceType::SoundOrMusic;
	exts["wax"] = ResourceType::SoundOrMusic;
	exts["wma"] = ResourceType::SoundOrMusic;
	exts["xm"] = ResourceType::SoundOrMusic;
}

//=================================================================================================
void ResourceManager::Cleanup()
{
	for(Resource* res : resources)
		delete res;

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
		Error("ResourceManager: Failed to add directory '%s' (%u).", dir, result);
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
				Resource* res = AddResource(find_data.cFileName, path);
				if(res)
				{
					res->pak_index = INVALID_PAK;
					res->path = path;
					res->filename = res->path.c_str() + dirlen;
				}
			}
		}
	} while(FindNextFile(find, &find_data) != 0);

	DWORD result = GetLastError();
	if(result != ERROR_NO_MORE_FILES)
		Error("ResourceManager: Failed to add other files in directory '%s' (%u).", dir, result);

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
		Error("ResourceManager: Failed to open pak '%s' (%u).", path, GetLastError());
		return false;
	}

	// read header
	Pak::Header header;
	if(!stream.Read(header))
	{
		Error("ResourceManager: Failed to read pak '%s' header.", path);
		return false;
	}
	if(header.sign[0] != 'P' || header.sign[1] != 'A' || header.sign[2] != 'K')
	{
		Error("ResourceManager: Failed to read pak '%s', invalid signature %c%c%c.", path, header.sign[0], header.sign[1], header.sign[2]);
		return false;
	}
	if(header.version > 1)
	{
		Error("ResourceManager: Failed to read pak '%s', invalid version %d.", path, (int)header.version);
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
			Error("ResourceManager: Failed to read pak '%s' extra header (%u).", path, GetLastError());
			return false;
		}
		total_size -= sizeof(PakV0::ExtraHeader);
		if(header2.files_size > (uint)total_size)
		{
			Error("ResourceManager: Failed to read pak '%s', invalid files size %u (total size %u).", path, header2.files_size, total_size);
			return false;
		}
		if(header2.files * PakV0::File::MIN_SIZE > header2.files_size)
		{
			Error("ResourceManager: Failed ot read pak '%s', invalid files count %u (files size %u, required size %u).", path, header2.files,
				header2.files_size, header2.files * PakV0::File::MIN_SIZE);
			return false;
		}

		// read files
		BufferHandle&& buf = stream.ReadToBuffer(header2.files_size);
		if(!buf)
		{
			Error("ResourceManager: Failed to read pak '%s' files (%u).", path);
			return false;
		}

		// decrypt files
		if(IS_SET(header.flags, Pak::Encrypted))
		{
			if(key == nullptr)
			{
				Error("ResourceManager: Failed to read pak '%s', file is encrypted.", path);
				return false;
			}
			io::Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
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
				Error("ResourceManager: Failed to read pak '%s', broken file at index %u.", path, i);
				delete pak0;
				return false;
			}
			else
			{
				total_size -= file.size;
				if(total_size < 0)
				{
					Error("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.size, i);
					delete pak0;
					return false;
				}
				if(file.offset + file.size > (int)pak_size)
				{
					Error("ResourceManager: Failed to read pak '%s', file '%s' (%u) has invalid offset %u (pak size %u).",
						path, file.name.c_str(), i, file.offset, pak_size);
					delete pak0;
					return false;
				}

				Resource* res = AddResource(file.name.c_str(), path);
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
			Error("ResourceManager: Failed to read pak '%s' extra header (%u).", path, GetLastError());
			return false;
		}
		total_size -= sizeof(PakV1::ExtraHeader);

		// read table
		if(!stream.Ensure(header2.file_entry_table_size) || !stream.Ensure(header2.files_count * sizeof(PakV1::File)))
		{
			Error("ResourceManager: Failed to read pak '%s' files table (%u).", path, GetLastError());
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
				Error("ResourceManager: Failed to read pak '%s', file is encrypted.", path);
				return false;
			}
			io::Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
		}
		if(IS_SET(header.flags, Pak::FullEncrypted) && !IS_SET(header.flags, Pak::Encrypted))
		{
			BufferPool.Free(buf);
			Error("ResourceManager: Failed to read pak '%s', invalid flags combination %u.", path, header.flags);
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
				Error("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.compressed_size, i);
				delete pak1;
				return false;
			}

			if(file.offset + file.compressed_size > pak_size)
			{
				BufferPool.Free(buf);
				Error("ResourceManager: Failed to read pak '%s', file at index %u has invalid offset %u (pak size %u).",
					path, i, file.offset, pak_size);
				delete pak1;
				return false;
			}

			Resource* res = AddResource(file.filename, path);
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
Resource* ResourceManager::AddResource(cstring filename, cstring path)
{
	assert(filename && path);

	ResourceType type = FilenameToResourceType(filename);
	if(type == ResourceType::Unknown)
		return nullptr;

	Resource* res = CreateResource(type);
	res->filename = filename;
	res->type = type;

	std::pair<ResourceIterator, bool>& result = resources.insert(res);
	if(result.second)
	{
		// added
		res->state = ResourceState::NotLoaded;
		return res;
	}
	else
	{
		// already exists
		Resource* existing = *result.first;
		if(existing->pak_index != INVALID_PAK || existing->path != path)
			Warn("ResourceManager: Resource '%s' already exists (%s; %s).", filename, GetPath(existing), path);
		delete res;
		return nullptr;
	}
}

//=================================================================================================
Resource* ResourceManager::CreateResource(ResourceType type)
{
	switch(type)
	{
	case ResourceType::Texture:
		return new Texture;
	case ResourceType::Mesh:
		return new Mesh;
	case ResourceType::VertexData:
		return new VertexData;
	case ResourceType::SoundOrMusic:
		return new Sound;
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
Resource* ResourceManager::TryGetResource(const AnyString& filename, ResourceType type)
{
	res_search.filename = filename;
	auto it = resources.find(&res_search);
	if(it != resources.end())
	{
		assert((*it)->type == type);
		return *it;
	}
	else
		return nullptr;
}

//=================================================================================================
Resource* ResourceManager::GetResource(const AnyString& filename, ResourceType type)
{
	Resource* res = TryGetResource(filename, type);
	if(!res)
	{
		cstring type_name;
		switch(type)
		{
		case ResourceType::Mesh:
			type_name = "mesh";
			break;
		case ResourceType::Texture:
			type_name = "texture";
			break;
		case ResourceType::VertexData:
			type_name = "vertex data";
			break;
		case ResourceType::SoundOrMusic:
			type_name = "sound or music";
			break;
		default:
			assert(0);
			type_name = "unknown";
			break;
		}
		throw Format("ResourceManager: Missing %s '%s'.", type_name, filename);
	}
	return res;
}

//=================================================================================================
void ResourceManager::LoadResource(Resource* res)
{
	assert(res);
	if(res->state != ResourceState::Loaded)
		LoadResourceInternal(res);
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
	if(pos == nullptr || !(*pos + 1))
		return ResourceType::Unknown;
	else
		return ExtToResourceType(pos + 1);
}

//=================================================================================================
BufferHandle ResourceManager::GetBuffer(Resource* res)
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
				io::Crypt((char*)buf->Data(), buf->Size(), pak1.key.c_str(), pak1.key.length());
			if(file.compressed_size != file.size)
				buf = buf->Decompress(file.size);
			return BufferHandle(buf);
		}
	}
}

//=================================================================================================
cstring ResourceManager::GetPath(Resource* res)
{
	assert(res);

	if(res->pak_index == INVALID_PAK)
		return res->path.c_str();
	else
		return Format("%s/%s", paks[res->pak_index]->path.c_str(), res->filename);
}

//=================================================================================================
StreamReader ResourceManager::GetStream(Resource* res, StreamType type)
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
				io::Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
			if(size != compressed_size)
				buf = buf->Decompress(size);
			return StreamReader(buf);
		}
	}
}

//=================================================================================================
void ResourceManager::AddTaskCategory(const AnyString& category)
{
	assert(mode == Mode::LoadScreenPrepare);

	TaskDetail* td = task_pool.Get();
	td->category = category;
	td->type = TaskType::Category;
	tasks.push_back(td);
}

//=================================================================================================
void ResourceManager::AddTask(void* ptr, TaskCallback callback)
{
	assert(mode == Mode::LoadScreenPrepare);

	TaskDetail* td = task_pool.Get();
	td->callback = callback;
	td->data.ptr = ptr;
	td->type = TaskType::Callback;
	tasks.push_back(td);
	++to_load;
}

//=================================================================================================
void ResourceManager::AddLoadTask(Resource* res)
{
	assert(res && mode == Mode::LoadScreenPrepare);

	if(res->state == ResourceState::NotLoaded)
	{
		TaskDetail* td = task_pool.Get();
		td->data.res = res;
		td->type = TaskType::Load;
		tasks.push_back(td);
		++to_load;

		res->state = ResourceState::Loading;
	}
}

//=================================================================================================
void ResourceManager::AddLoadTask(Resource* res, void* ptr, TaskCallback callback, bool required)
{
	assert(res && mode == Mode::LoadScreenPrepare);

	if(required || res->state == ResourceState::NotLoaded)
	{
		TaskDetail* td = task_pool.Get();
		td->data.res = res;
		td->data.ptr = ptr;
		td->callback = callback;
		td->type = TaskType::LoadAndCallback;
		tasks.push_back(td);
		++to_load;

		if(res->state == ResourceState::NotLoaded)
			res->state = ResourceState::Loading;
	}
}

//=================================================================================================
void ResourceManager::NextTask(cstring next_category)
{
	if(next_category)
		category = next_category;
	++loaded;

	TickLoadScreen();
}

//=================================================================================================
void ResourceManager::PrepareLoadScreen(float progress_min, float progress_max)
{
	assert(mode == Mode::Instant && InRange(progress_min, 0.f, 1.f) && InRange(progress_max, 0.f, 1.f) && progress_max >= progress_min);

	this->progress_min = progress_min;
	this->progress_max = progress_max;
	progress = progress_min;
	to_load = 0;
	loaded = 0;
	mode = Mode::LoadScreenPrepare;
	category = nullptr;
}

//=================================================================================================
void ResourceManager::StartLoadScreen(cstring category)
{
	assert(mode == Mode::LoadScreenPrepare);

	mode = Mode::LoadScreenRuning;
	if(category)
		this->category = category;
	UpdateLoadScreen();
	mode = Mode::Instant;
	task_pool.Free(tasks);
}

//=================================================================================================
void ResourceManager::CancelLoadScreen(bool cleanup)
{
	assert(mode == Mode::LoadScreenPrepare);
	
	if(cleanup)
		tasks.clear();
	else
		assert(tasks.empty());

	mode = Mode::Instant;
}

//=================================================================================================
void ResourceManager::UpdateLoadScreen()
{
	Engine& engine = Engine::Get();
	if(to_load == loaded)
	{
		// no tasks to load, draw with full progress
		progress = progress_max;
		ReleaseMutex();
		load_screen->SetProgress(progress_max, "");
		engine.DoPseudotick();
		return;
	}

	// draw first frame
	if(tasks[0]->type == TaskType::Category)
		category = tasks[0]->category;
	load_screen->SetProgress(progress, category);
	engine.DoPseudotick();

	// do all tasks
	timer.Reset();
	timer_dt = 0;
	for(uint i = 0; i < tasks.size(); ++i)
	{
		TaskDetail* task = tasks[i];
		switch(task->type)
		{
		case TaskType::Category:
			category = task->category;
			break;
		case TaskType::Callback:
			task->callback(task->data);
			++loaded;
			break;
		case TaskType::Load:
			LoadResourceInternal(task->data.res);
			++loaded;
			break;
		case TaskType::LoadAndCallback:
			if(task->data.res->state != ResourceState::Loaded)
				LoadResourceInternal(task->data.res);
			task->callback(task->data);
			++loaded;
			break;
		default:
			assert(0);
			break;
		}

		TickLoadScreen();
	}

	// draw last frame
	progress = progress_max;
	ReleaseMutex();
	load_screen->SetProgress(progress_max);
	engine.DoPseudotick();
}

//=================================================================================================
void ResourceManager::TickLoadScreen()
{
	timer_dt += timer.Tick();
	if(timer_dt >= 0.05f)
	{
		timer_dt = 0.f;
		progress = float(loaded) / to_load * (progress_max - progress_min) + progress_min;
		load_screen->SetProgressOptional(progress, category);
		Engine::Get().DoPseudotick();
		ReleaseMutex();
	}
}

//=================================================================================================
void ResourceManager::ReleaseMutex()
{
	if(progress >= 0.5f)
		utility::IncrementDelayLock();
}

//=================================================================================================
void ResourceManager::LoadResourceInternal(Resource* res)
{
	assert(res->state != ResourceState::Loaded);
	
	switch(res->type)
	{
	case ResourceType::Mesh:
		LoadMesh((Mesh*)res);
		break;
	case ResourceType::VertexData:
		LoadVertexData((VertexData*)res);
		break;
	case ResourceType::SoundOrMusic:
		LoadSoundOrMusic((Sound*)res);
		break;
	case ResourceType::Texture:
		LoadTexture((Texture*)res);
		break;
	default:
		assert(0);
		break;
	}

	res->state = ResourceState::Loaded;
}

//=================================================================================================
void ResourceManager::LoadMesh(Mesh* mesh)
{
	StreamReader&& reader = GetStream(mesh, StreamType::FullFileOrMemory);

	try
	{
		mesh->Load(reader, device);
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh '%s'. %s", GetPath(mesh), err);
	}
}

//=================================================================================================
void ResourceManager::LoadMeshMetadata(Mesh* mesh)
{
	StreamReader&& reader = GetStream(mesh, StreamType::FullFileOrMemory);

	try
	{
		mesh->LoadMetadata(reader);
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh metadata '%s'. %s", GetPath(mesh), err);
	}
}

//=================================================================================================
void ResourceManager::LoadVertexData(VertexData* vd)
{
	StreamReader&& reader = GetStream(vd, StreamType::FullFileOrMemory);

	try
	{
		Mesh::LoadVertexData(vd, reader);
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh vertex data '%s'. %s", GetPath(vd), err);
	}
}

//=================================================================================================
void ResourceManager::LoadSoundOrMusic(Sound* sound)
{
	assert(fmod_system);

	int flags = FMOD_HARDWARE | FMOD_LOWMEM;
	if(sound->is_music)
		flags |= FMOD_2D;
	else
		flags |= FMOD_3D | FMOD_LOOP_OFF;

	FMOD_RESULT result;
	if(sound->IsFile())
		result = fmod_system->createStream(sound->path.c_str(), flags, nullptr, &sound->sound);
	else
	{
		BufferHandle&& buf = GetBuffer(sound);
		FMOD_CREATESOUNDEXINFO info = { 0 };
		info.cbsize = sizeof(info);
		info.length = buf->Size();
		result = fmod_system->createStream((cstring)buf->Data(), flags | FMOD_OPENMEMORY, &info, &sound->sound);
		if(result == FMOD_OK)
			sound_bufs.push_back(buf.Pin());
	}
	
	if(result != FMOD_OK)
		throw Format("ResourceManager: Failed to load %s '%s' (%d).", sound->is_music ? "music" : "sound", sound->path.c_str(), result);
}

//=================================================================================================
void ResourceManager::LoadTexture(Texture* tex)
{
	HRESULT hr;
	if(tex->IsFile())
	{
		hr = D3DXCreateTextureFromFile(device, tex->path.c_str(), &tex->tex);
		if(FAILED(hr))
		{
			Sleep(250);
			hr = D3DXCreateTextureFromFile(device, tex->path.c_str(), &tex->tex);
		}
	}
	else
	{
		BufferHandle&& buf = GetBuffer(tex);
		hr = D3DXCreateTextureFromFileInMemory(device, buf->Data(), buf->Size(), &tex->tex);
	}

	if(FAILED(hr))
		throw Format("Failed to load texture '%s' (%u).", GetPath(tex), hr);
}

//=================================================================================================
void ResourceManager::ApplyRawTextureCallback(TaskData& data)
{
	TEX& tex = *(TEX*)data.ptr;
	Texture* res = (Texture*)data.res;
	tex = res->tex;
}

//=================================================================================================
void ResourceManager::ApplyRawSoundCallback(TaskData& data)
{
	SOUND& sound = *(SOUND*)data.ptr;
	Sound* res = (Sound*)data.res;
	sound = res->sound;
}
