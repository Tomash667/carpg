#include "PCH.hpp"
#include "MeshTask.hpp"
#include "GlobalCode.hpp"
#include "Mesh.h"
#include <conio.h>

void Info(const char* path)
{
	Mesh* mesh = new Mesh;

	try
	{
		mesh->LoadSafe(path);
	}
	catch(cstring err)
	{
		printf("Info - Failed to load '%s': %s\n", path, err);
		delete mesh;
		return;
	}

	auto& head = mesh->head;
	printf("File: %s\n", path);
	printf("Version: %u\n", head.version);
	printf("Flags: ");
	if(head.flags & Mesh::F_TANGENTS)
		printf("F_TANGENTS ");
	if(head.flags & Mesh::F_ANIMATED)
		printf("F_ANIMATED ");
	if(head.flags & Mesh::F_STATIC)
		printf("F_STATIC ");
	if(head.flags & Mesh::F_PHYSICS)
		printf("F_PHYSICS ");
	if(head.flags & Mesh::F_SPLIT)
		printf("F_SPLIT ");
	printf("\nVertices: %u\n", head.n_verts);
	printf("Faces: %u\n", head.n_tris);
	printf("Submeshes: %u\n", head.n_subs);
	printf("Bones: %u\n", head.n_bones);
	printf("Animations: %u\n", head.n_anims);
	printf("Points: %u\n", head.n_points);
	printf("Groups: %u\n", head.n_groups);

	delete mesh;
}

int Upgrade(const char* path, bool force)
{
	Mesh* mesh = new Mesh;
	int result;

	try
	{
		mesh->LoadSafe(path);
		if(mesh->old_ver == mesh->head.version && !force)
		{
			printf("File '%s': version up to date\n", path);
			result = 0;
		}
		else
		{
			printf("File '%s': upgrading version %d -> %d\n", path, mesh->old_ver, mesh->head.version);
			mesh->Save(path);
			result = 1;
		}
	}
	catch(cstring err)
	{
		printf("Upgrade - Failed to load '%s': %s\n", path, err);
		result = -1;
	}

	delete mesh;
	return result;
}

cstring Formats(cstring str, ...);

bool EndsWith(cstring str, cstring end)
{
	uint str_len = strlen(str);
	uint end_len = strlen(end);
	if(end_len > str_len)
		return false;
	for(uint i=0; i<end_len; ++i)
	{
		if(str[str_len - end_len + i] != end[i])
			return false;
	}
	return true;
}


void UpgradeDir(const char* path, bool force, bool subdir, int& upgraded, int& ok, int& failed)
{
	WIN32_FIND_DATA find;
	printf("Upgrade checking dir '%s'\n", path);
	HANDLE f = FindFirstFile(Formats("%s/*", path), &find);
	if(f == INVALID_HANDLE_VALUE)
	{
		printf("FIND failed...\n");
		++failed;
		return;
	}

	do
	{
		if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(subdir && strcmp(find.cFileName, ".") != 0 && strcmp(find.cFileName, "..") != 0)
			{
				string new_path = Formats("%s/%s", path, find.cFileName);
				UpgradeDir(new_path.c_str(), force, subdir, upgraded, ok, failed);
			}
		}
		else
		{
			if(EndsWith(find.cFileName, ".qmsh") || EndsWith(find.cFileName, ".phy"))
			{
				string file_path = Formats("%s/%s", path, find.cFileName);
				int result = Upgrade(file_path.c_str(), force);
				if(result == -1)
					failed++;
				else if(result == 0)
					ok++;
				else
					upgraded++;
			}
		}
	}
	while(FindNextFile(f, &find));

	FindClose(f);
}

void UpgradeDir(const char* path, bool force, bool subdir)
{
	int upgraded = 0, ok = 0, failed = 0;

	UpgradeDir(path, force, subdir, upgraded, ok, failed);

	printf("UPGRADEDIR COMPLETE\nUpgraded: %d\nUp to date: %d\nFailed: %d\n", upgraded, ok, failed);
	_getch();
}
