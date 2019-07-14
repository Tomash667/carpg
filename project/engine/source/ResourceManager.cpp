#include "EnginePch.h"
#include "EngineCore.h"
#include "ResourceManager.h"
#include "Engine.h"
#include "Mesh.h"
#include "SoundManager.h"
#include "DirectX.h"
#include "Pak.h"

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
void ResourceManager::Init(IDirect3DDevice9* device, SoundManager* sound_mgr)
{
	this->device = device;
	this->sound_mgr = sound_mgr;

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
		pak->filename_buf->Free();
		delete pak;
	}

	Buffer::Free(sound_bufs);

	task_pool.Free(tasks);
}

//=================================================================================================
bool ResourceManager::AddDir(cstring dir, bool subdir)
{
	assert(dir);

	int dirlen = strlen(dir) + 1;

	bool ok = io::FindFiles(Format("%s/*.*", dir), [=](const io::FileInfo& file_info)
	{
		if(file_info.is_dir && subdir)
		{
			LocalString path = Format("%s/%s", dir, file_info.filename);
			AddDir(path);
		}
		else
		{
			cstring path = Format("%s/%s", dir, file_info.filename);
			Resource* res = AddResource(file_info.filename, path);
			if(res)
			{
				res->pak_index = Resource::INVALID_PAK;
				res->path = path;
				res->filename = res->path.c_str() + dirlen;
			}
		}
		return true;
	});

	if(!ok)
	{
		DWORD result = GetLastError();
		Error("ResourceManager: Failed to add directory '%s' (%u).", dir, result);
		return false;
	}

	return true;
}

//=================================================================================================
bool ResourceManager::AddPak(cstring path, cstring key)
{
	assert(path);

	FileReader f(path);
	if(!f)
	{
		Error("ResourceManager: Failed to open pak '%s' (%u).", path, GetLastError());
		return false;
	}

	// read header
	Pak::Header header;
	f >> header;
	if(!f)
	{
		Error("ResourceManager: Failed to read pak '%s' header.", path);
		return false;
	}
	if(header.sign[0] != 'P' || header.sign[1] != 'A' || header.sign[2] != 'K')
	{
		Error("ResourceManager: Failed to read pak '%s', invalid signature %c%c%c.", path, header.sign[0], header.sign[1], header.sign[2]);
		return false;
	}
	if(header.version != 1)
	{
		Error("ResourceManager: Failed to read pak '%s', invalid version %d.", path, (int)header.version);
		return false;
	}

	int pak_index = paks.size();
	uint pak_size = f.GetSize();
	int total_size = pak_size - sizeof(Pak::Header);

	// read table
	if(!f.Ensure(header.file_entry_table_size) || !f.Ensure(header.files_count * sizeof(Pak::File)))
	{
		Error("ResourceManager: Failed to read pak '%s' files table (%u).", path, GetLastError());
		return false;
	}
	Buffer* buf = Buffer::Get();
	buf->Resize(header.file_entry_table_size);
	f.Read(buf->Data(), header.file_entry_table_size);
	total_size -= header.file_entry_table_size;

	// decrypt table
	if(IS_SET(header.flags, Pak::Encrypted))
	{
		if(key == nullptr)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', file is encrypted.", path);
			return false;
		}
		io::Crypt((char*)buf->Data(), buf->Size(), key, strlen(key));
	}
	if(IS_SET(header.flags, Pak::FullEncrypted) && !IS_SET(header.flags, Pak::Encrypted))
	{
		buf->Free();
		Error("ResourceManager: Failed to read pak '%s', invalid flags combination %u.", path, header.flags);
		return false;
	}

	// setup pak
	Pak* pak = new Pak;
	pak->encrypted = IS_SET(header.flags, Pak::FullEncrypted);
	if(key)
		pak->key = key;
	pak->filename_buf = buf;
	pak->files = (Pak::File*)buf->Data();
	for(uint i = 0; i < header.files_count; ++i)
	{
		Pak::File& file = pak->files[i];
		file.filename = (cstring)buf->Data() + file.filename_offset;
		total_size -= file.compressed_size;

		if(total_size < 0)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', broken file size %u at index %u.", path, file.compressed_size, i);
			delete pak;
			return false;
		}

		if(file.offset + file.compressed_size > pak_size)
		{
			buf->Free();
			Error("ResourceManager: Failed to read pak '%s', file at index %u has invalid offset %u (pak size %u).",
				path, i, file.offset, pak_size);
			delete pak;
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

	pak->file = f;
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

	pair<ResourceIterator, bool>& result = resources.insert(res);
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
		if(existing->pak_index != Resource::INVALID_PAK || existing->path != path)
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
Resource* ResourceManager::TryGetResource(Cstring filename, ResourceType type)
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
Resource* ResourceManager::GetResource(Cstring filename, ResourceType type)
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
Buffer* ResourceManager::GetBuffer(Resource* res)
{
	assert(res);

	if(res->pak_index == Resource::INVALID_PAK)
		return FileReader::ReadToBuffer(res->path);
	else
	{
		Pak& pak = *paks[res->pak_index];
		Pak::File& file = pak.files[res->pak_file_index];
		Buffer* buf = pak.file.ReadToBuffer(file.offset, file.compressed_size);
		if(pak.encrypted)
			io::Crypt((char*)buf->Data(), buf->Size(), pak.key.c_str(), pak.key.length());
		if(file.compressed_size != file.size)
			buf = buf->Decompress(file.size);
		return buf;
	}
}

//=================================================================================================
cstring ResourceManager::GetPath(Resource* res)
{
	assert(res);

	if(res->pak_index == Resource::INVALID_PAK)
		return res->path.c_str();
	else
		return Format("%s/%s", paks[res->pak_index]->path.c_str(), res->filename);
}

//=================================================================================================
void ResourceManager::AddTaskCategory(Cstring category)
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
	{
		for(TaskDetail* task : tasks)
		{
			if((task->type == TaskType::Load || task->type == TaskType::LoadAndCallback) && task->data.res->state == ResourceState::Loading)
				task->data.res->state = ResourceState::NotLoaded;
		}
		task_pool.Free(tasks);
	}
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
		progress_clbk(progress_max, "");
		engine.DoPseudotick();
		return;
	}

	// draw first frame
	if(tasks[0]->type == TaskType::Category)
		category = tasks[0]->category;
	progress_clbk(progress, category);
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
	progress_clbk(progress_max, nullptr);
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
		progress_clbk(progress, category);
		Engine::Get().DoPseudotick();
	}
}

//=================================================================================================
void ResourceManager::LoadResourceInternal(Resource* res)
{
	assert(res->state != ResourceState::Loaded);

	switch(res->type)
	{
	case ResourceType::Mesh:
		LoadMesh(static_cast<Mesh*>(res));
		break;
	case ResourceType::VertexData:
		LoadVertexData(static_cast<VertexData*>(res));
		break;
	case ResourceType::SoundOrMusic:
		LoadSoundOrMusic(static_cast<Sound*>(res));
		break;
	case ResourceType::Texture:
		LoadTexture(static_cast<Texture*>(res));
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
	try
	{
		if(mesh->IsFile())
		{
			FileReader f(mesh->path);
			mesh->Load(f, device);
		}
		else
		{
			MemoryReader f(GetBuffer(mesh));
			mesh->Load(f, device);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh '%s'. %s", GetPath(mesh), err);
	}
}

//=================================================================================================
void ResourceManager::LoadMeshMetadata(Mesh* mesh)
{
	try
	{
		if(mesh->IsFile())
		{
			FileReader f(mesh->path);
			mesh->LoadMetadata(f);
		}
		else
		{
			MemoryReader f(GetBuffer(mesh));
			mesh->LoadMetadata(f);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh metadata '%s'. %s", GetPath(mesh), err);
	}
}

//=================================================================================================
void ResourceManager::LoadVertexData(VertexData* vd)
{
	try
	{
		if(vd->IsFile())
		{
			FileReader f(vd->path);
			Mesh::LoadVertexData(vd, f);
		}
		else
		{
			MemoryReader f(GetBuffer(vd));
			Mesh::LoadVertexData(vd, f);
		}
	}
	catch(cstring err)
	{
		throw Format("ResourceManager: Failed to load mesh vertex data '%s'. %s", GetPath(vd), err);
	}
}

//=================================================================================================
void ResourceManager::LoadSoundOrMusic(Sound* sound)
{
	int result = sound_mgr->LoadSound(sound);
	if(result != 0)
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
	Texture* res = static_cast<Texture*>(data.res);
	tex = res->tex;
}

//=================================================================================================
void ResourceManager::ApplyRawSoundCallback(TaskData& data)
{
	SOUND& sound = *(SOUND*)data.ptr;
	Sound* res = static_cast<Sound*>(data.res);
	sound = res->sound;
}
