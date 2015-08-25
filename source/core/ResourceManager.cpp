#include "Pch.h"
#include "Base.h"
#include "ResourceManager.h"

ResourceManager::ResourceManager() : last_resource(NULL)
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

				// add resource if not exists
				std::pair<ResourceMapI, bool> const& r = resources.insert(ResourceMap::value_type(last_resource->filename, last_resource));
				if(r.second)
				{
					// added
					last_resource->ptr = NULL;
					last_resource->type = Resource2::None;
					last_resource->state = Resource2::NotLoaded;
					last_resource->pak.pak_id = INVALID_PAK;
					last_resource->refs = 0;

					last_resource = NULL;
				}
				else
				{
					// already exists
					WARN(Format("ResourceManager: Resource %s already exists (%s, %s).", find_data.cFileName, r.first->second->path.c_str(), last_resource->path.c_str()));
				}
			}
		}
	} while(FindNextFile(find, &find_data) != 0);

	DWORD result = GetLastError();
	if(result != ERROR_NO_MORE_FILES)
		ERROR(Format("ResourceManager: AddDir FindNextFile failed (%u) for dir '%s'.", result, dir));

	FindClose(find);

	return true;
}

bool ResourceManager::AddPak(cstring path)
{
	assert(path);

	// file specs in tools/pak/pak.txt

	// open file
	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		DWORD result = GetLastError();
		ERROR(Format("ResourceManager: AddPak open file failed (%u) for path '%s'.", result, path));
		return false;
	}

	return false;
}

Resource2* ResourceManager::GetResource(cstring filename)
{
	assert(filename);

	ResourceMapI it = resources.find(filename);
	if(it == resources.end())
		return NULL;
	else
		return (*it).second;
}

TEX ResourceManager::LoadTexture(cstring path)
{
	assert(path);

	// find resource
	Resource2* res = GetResource(path);
	if(!res)
		throw Format("Engine: Missing texture resource '%s'!", path);

	++res->refs;
	
	// if already loaded return it
	if(res->state == Resource2::Loaded)
		return (TEX)res->ptr;

	// wczytaj
	//TEX t;
	if(res->pak.pak_id == INVALID_PAK)
	{
		//HRESULT hr = D3DXCreateTextureFromFile(device, res->path.c_str(), &t);
		//if(FAILED(hr))
		//	throw Format("Engine: Failed to load texture '%s' (%d)!", res->path.c_str(), hr);
	}
	else
	{
		//D3DXCreateTextureFromFileInMemory()
	}

	// ustaw stan
	//res->ptr = t;
	res->state = Resource2::Loaded;
	res->type = Resource2::Texture;

	return NULL; // t;
}
