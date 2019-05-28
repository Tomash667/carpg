/*
 * The Final Quest - 3D Graphics Engine
 * Copyright (C) 2007  Adam Sawicki
 * http://regedit.gamedev.pl, sawickiap@poczta.onet.pl
 * License: GNU GPL
 */
#include "PCH.hpp"
#include <algorithm>
#include "MeshTask.hpp"
#include "QmshTmpLoader.h"
#include "Converter.h"
#include "QmshSaver.h"
#include "Mesh.h"
#include <cassert>
#include <conio.h>
#include <Windows.h>

void LoadQmshTmp(QMSH *Out, ConversionData& cs )
{
	// Wczytaj plik QMSH.TMP
	Info("Loading QMSH TMP file \"%s\"...", cs.input.c_str());

	QmshTmpLoader loader;
	tmp::QMSH Tmp;
	try
	{
		loader.LoadQmshTmpFile(&Tmp, cs.input);
	}
	catch(const Tokenizer::Exception& ex)
	{
		throw Format("Failed to load qmsh tmp: %s", ex.ToString());
	}
	if(Tmp.Meshes.empty())
		throw "Empty mesh!";

	// Przekonwertuj do QMSH
	Converter converter;
	converter.ConvertQmshTmpToQmsh(Out, Tmp, cs);
}

//====================================================================================================
// Konwersja qmsh.tmp -> qmsh
// Lub generowanie pliku konfiguracyjnego
//====================================================================================================
void Convert(ConversionData& cs)
{
	QMSH Qmsh;

	LoadQmshTmp(&Qmsh, cs);

	if(cs.gopt == GO_CREATE)
	{
		Info("Zapisywanie pliku konfiguracyjnego: %s.\n", cs.group_file.c_str());

		TextWriter file(cs.group_file);

		// nazwa pliku
		file.Write(Format("file: \"%s\"\n", cs.input.c_str()));

		// nazwa pliku wyjœciowego
		if(cs.force_output)
			file.Write(Format("output: \"%s\"\n", cs.output.c_str()));

		// liczba koœci, grupy
		file.Write(Format("bones: %u\ngroups: 1\ngroup: 0 {\n\tname: \"default\"\n\tparent: 0\n};\n", Qmsh.Bones.size()));

		// koœci
		for(std::vector<shared_ptr<QMSH_BONE> >::iterator it = Qmsh.Bones.begin(), end = Qmsh.Bones.end(); it != end; ++it)
			file.Write(Format("bone: \"%s\" {\n\tgroup: 0\n\tspecial: 0\n};\n", (*it)->Name.c_str()));
	}
	else
	{
		QmshSaver saver;
		saver.SaveQmsh(Qmsh, cs.output);
	}
}

void WriteMeshFlags(byte flags)
{
	if(flags & Mesh::F_TANGENTS)
		printf("F_TANGENTS ");
	if(flags & Mesh::F_ANIMATED)
		printf("F_ANIMATED ");
	if(flags & Mesh::F_STATIC)
		printf("F_STATIC ");
	if(flags & Mesh::F_PHYSICS)
		printf("F_PHYSICS ");
	if(flags & Mesh::F_SPLIT)
		printf("F_SPLIT ");
	printf("(%u)", flags);
}

void MeshInfo(const char* path, const char* options)
{
	Mesh* mesh = new Mesh;

	try
	{
		mesh->LoadSafe(path);
	}
	catch(cstring err)
	{
		Error("Failed to load '%s': %s\n", path, err);
		delete mesh;
		return;
	}

	bool points_details = false, groups_details = false;
	vector<Mesh::Point*> points;

	if(options)
	{
		Tokenizer t(Tokenizer::F_JOIN_MINUS | Tokenizer::F_UNESCAPE);
		try
		{
			t.FromString(options);
			t.Next();
			while(!t.IsEof())
			{
				const string& str = t.MustGetString();
				if(str == "groups")
					groups_details = true;
				else if(str == "points")
					points_details = true;
				else if(str == "point")
				{
					t.Next();
					const string& id = t.MustGetText();
					Mesh::Point* pt = mesh->GetPoint(id);
					if(!pt)
						Error("Error - invalid point '%s'.\n", id.c_str());
					else
						points.push_back(pt);
				}
				else
					t.Throw("Unknown option.");
				t.Next();
			}
		}
		catch(const Tokenizer::Exception& ex)
		{
			Error("Error - invalid mesh info options: %s\n", ex.ToString());
		}
	}

	auto& head = mesh->head;
	printf("File: %s\n", path);
	printf("Version: %u\n", head.version);
	printf("Flags: ");
	WriteMeshFlags(head.flags);
	printf("\nVertices: %u\n", head.n_verts);
	printf("Faces: %u\n", head.n_tris);
	printf("Submeshes: %u\n", head.n_subs);
	printf("Bones: %u\n", head.n_bones);
	printf("Animations: %u\n", head.n_anims);
	printf("Points: %u\n", head.n_points);
	if(points_details)
	{
		uint index = 0;
		for(Mesh::Point& pt : mesh->attach_points)
		{
			printf("\t[%u] %s\n", index, pt.name.c_str());
			++index;
		}
	}
	for(Mesh::Point* pt : points)
	{
		printf("\tPoint %s\n\t\tType: %u\n\t\tBone: %u\n\t\tRot: %g;%g;%g\n\t\tSize: %g;%g;%g\n", pt->name.c_str(), pt->type, pt->bone,
			pt->rot.x, pt->rot.y, pt->rot.z, pt->size.x, pt->size.y, pt->size.z);
	}
	printf("Groups: %u\n", head.n_groups);
	if(groups_details)
	{
		for(uint i = 0; i < head.n_groups; ++i)
		{
			Mesh::BoneGroup& group = mesh->groups[i];
			printf("\t[%u] %s (parent %s) - (%u) ", i, group.name.c_str(),
				group.parent == i ? "none" : mesh->groups[group.parent].name.c_str(), group.bones.size());
			for(byte b : group.bones)
				printf("%s ", mesh->bones[b - 1].name.c_str());
			printf("\n");
		}
	}

	delete mesh;
}

void Compare(const char* path1, const char* path2)
{
	Mesh* mesh1 = new Mesh,
		*mesh2 = new Mesh;

	try
	{
		mesh1->LoadSafe(path1);
	}
	catch(cstring err)
	{
		Error("Info - Failed to load '%s': %s\n", path1, err);
		delete mesh1;
		delete mesh2;
		return;
	}

	try
	{
		mesh2->LoadSafe(path2);
	}
	catch(cstring err)
	{
		Error("Info - Failed to load '%s': %s\n", path2, err);
		delete mesh1;
		delete mesh2;
		return;
	}

	Info("Info - Comparing A (%s) to B (%s)\n", path1, path2);

	bool any = false;

	if(mesh1->head.version != mesh2->head.version)
	{
		printf("Version: %u | %u\n", mesh1->head.version, mesh2->head.version);
		any = true;
	}

	if(mesh1->head.flags != mesh2->head.flags)
	{
		printf("Flags: \n\tA: ");
		WriteMeshFlags(mesh1->head.flags);
		printf("\nB: ");
		WriteMeshFlags(mesh2->head.flags);
		printf("\n");
		any = true;
	}

	// verts
	if(mesh1->head.n_verts != mesh2->head.n_verts)
	{
		printf("Verts count: %u | %u\n", mesh1->head.n_verts, mesh2->head.n_verts);
		any = true;
	}
	else if(mesh1->vdata_size != mesh2->vdata_size || memcmp(mesh1->vdata, mesh2->vdata, mesh1->vdata_size) != 0)
	{
		printf("Verts data differences\n");
		if(mesh1->vertex_size == mesh2->vertex_size && mesh1->vdata_size == mesh2->vdata_size)
		{
			uint vs = mesh1->vertex_size;
			int found = 0;
			for(uint i = 0; i < mesh1->head.n_verts; ++i)
			{
				byte* v1 = mesh1->vdata + i * vs;
				byte* v2 = mesh2->vdata + i * vs;
				if(memcmp(v1, v2, vs) != 0)
				{
					printf("Differences at vertex %u\n", i);
					VAnimated* va = (VAnimated*)v1;
					VAnimated* vb = (VAnimated*)v2;
					++found;
					if(found == 5)
						break;
				}
			}
		}

		any = true;
	}

	// tris
	if(mesh1->head.n_tris != mesh2->head.n_tris)
	{
		printf("Tris count: %u | %u\n", mesh1->head.n_tris, mesh2->head.n_tris);
		any = true;
	}
	else if(mesh1->fdata_size != mesh2->fdata_size || memcmp(mesh1->fdata, mesh2->fdata, mesh1->fdata_size) != 0)
	{
		printf("Tris data differences\n");
		any = true;
	}

	// subs
	if(mesh1->head.n_subs != mesh2->head.n_subs)
	{
		printf("Subs count: %u | %u\n", mesh1->head.n_subs, mesh2->head.n_subs);
		any = true;
	}
	bool first = true, added = false;
	for(uint i = 0; i < mesh1->head.n_subs; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh2->head.n_subs; ++j)
		{
			if(mesh1->subs[i].name == mesh2->subs[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Subs in A missing in B: ");
			}
			printf("%s ", mesh1->subs[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	first = true, added = false;
	for(uint i = 0; i < mesh2->head.n_subs; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh1->head.n_subs; ++j)
		{
			if(mesh2->subs[i].name == mesh1->subs[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Subs in B missing in A: ");
			}
			printf("%s ", mesh2->subs[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	for(uint i = 0; i < mesh1->head.n_subs; ++i)
	{
		Mesh::Submesh* found = nullptr;
		for(uint j = 0; j < mesh2->head.n_subs; ++j)
		{
			if(mesh1->subs[i].name == mesh2->subs[j].name)
			{
				found = &mesh2->subs[j];
				break;
			}
		}
		if(found && !(mesh1->subs[i] == *found))
		{
			any = true;
			printf("Sub %s have different data\n", found->name.c_str());
		}
	}

	// bones
	if(mesh1->head.n_bones != mesh2->head.n_bones)
	{
		any = true;
		printf("Bones count: %u | %u\n", mesh1->head.n_bones, mesh2->head.n_bones);
	}
	first = true, added = false;
	for(uint i = 0; i < mesh1->head.n_bones; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh2->head.n_bones; ++j)
		{
			if(mesh1->bones[i].name == mesh2->bones[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Bones in A missing in B: ");
			}
			printf("%s ", mesh1->bones[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	first = true, added = false;
	for(uint i = 0; i < mesh2->head.n_bones; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh1->head.n_bones; ++j)
		{
			if(mesh2->bones[i].name == mesh1->bones[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Bones in B missing in A: ");
			}
			printf("%s ", mesh2->bones[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	for(uint i = 0; i < mesh1->head.n_bones; ++i)
	{
		Mesh::Bone* found = nullptr;
		for(uint j = 0; j < mesh2->head.n_bones; ++j)
		{
			if(mesh1->bones[i].name == mesh2->bones[j].name)
			{
				found = &mesh2->bones[j];
				break;
			}
		}
		if(found && !(mesh1->bones[i] == *found))
		{
			any = true;
			printf("Bone %s have different data\n", found->name.c_str());
		}
	}

	// bone groups
	if(mesh1->head.n_groups != mesh2->head.n_groups)
	{
		any = true;
		printf("Bone groups count: %u | %u\n", mesh1->head.n_groups, mesh2->head.n_groups);
	}
	first = true, added = false;
	for(uint i = 0; i < mesh1->head.n_groups; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh2->head.n_groups; ++j)
		{
			if(mesh1->groups[i].name == mesh2->groups[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Bone groups in A missing in B: ");
			}
			printf("%s ", mesh1->groups[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	first = true, added = false;
	for(uint i = 0; i < mesh2->head.n_groups; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh1->head.n_groups; ++j)
		{
			if(mesh2->groups[i].name == mesh1->groups[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Bone groups in B missing in A: ");
			}
			printf("%s ", mesh2->groups[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	for(uint i = 0; i < mesh1->head.n_groups; ++i)
	{
		Mesh::BoneGroup* found = nullptr;
		for(uint j = 0; j < mesh2->head.n_groups; ++j)
		{
			if(mesh1->groups[i].name == mesh2->groups[j].name)
			{
				found = &mesh2->groups[j];
				break;
			}
		}
		if(found && !(mesh1->groups[i] == *found))
		{
			any = true;
			printf("Bone group %s have different data\n", found->name.c_str());
		}
	}

	// anims
	if(mesh1->head.n_anims != mesh2->head.n_anims)
	{
		any = true;
		printf("Anims count: %u | %u\n", mesh1->head.n_anims, mesh2->head.n_anims);
	}
	first = true, added = false;
	for(uint i = 0; i < mesh1->head.n_anims; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh2->head.n_anims; ++j)
		{
			if(mesh1->anims[i].name == mesh2->anims[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Anims in A missing in B: ");
			}
			printf("%s ", mesh1->anims[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	first = true, added = false;
	for(uint i = 0; i < mesh2->head.n_anims; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh1->head.n_anims; ++j)
		{
			if(mesh2->anims[i].name == mesh1->anims[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Anims in B missing in A: ");
			}
			printf("%s ", mesh2->anims[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	for(uint i = 0; i < mesh1->head.n_anims; ++i)
	{
		Mesh::Animation* found = nullptr;
		for(uint j = 0; j < mesh2->head.n_anims; ++j)
		{
			if(mesh1->anims[i].name == mesh2->anims[j].name)
			{
				found = &mesh2->anims[j];
				break;
			}
		}
		if(found && !(mesh1->anims[i] == *found))
		{
			any = true;
			printf("Anim %s have different data\n", found->name.c_str());
		}
	}

	// points
	if(mesh1->head.n_points != mesh2->head.n_points)
	{
		any = true;
		printf("Points count: %u | %u\n", mesh1->head.n_points, mesh2->head.n_points);
	}
	first = true, added = false;
	for(uint i = 0; i < mesh1->head.n_points; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh2->head.n_points; ++j)
		{
			if(mesh1->attach_points[i].name == mesh2->attach_points[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Points in A missing in B: ");
			}
			printf("%s ", mesh1->attach_points[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	first = true, added = false;
	for(uint i = 0; i < mesh2->head.n_points; ++i)
	{
		bool exists = false;
		for(uint j = 0; j < mesh1->head.n_points; ++j)
		{
			if(mesh2->attach_points[i].name == mesh1->attach_points[j].name)
			{
				any = true;
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			added = true;
			if(first)
			{
				first = false;
				printf("Points in B missing in A: ");
			}
			printf("%s ", mesh2->attach_points[i].name.c_str());
		}
	}
	if(added)
		printf("\n");
	for(uint i = 0; i < mesh1->head.n_points; ++i)
	{
		Mesh::Point* found = nullptr;
		for(uint j = 0; j < mesh2->head.n_points; ++j)
		{
			if(mesh1->attach_points[i].name == mesh2->attach_points[j].name)
			{
				found = &mesh2->attach_points[j];
				break;
			}
		}
		if(found && !(mesh1->attach_points[i] == *found))
		{
			any = true;
			printf("Point %s have different data\n", found->name.c_str());
		}
	}

	// misc
	if(mesh1->head.radius != mesh2->head.radius
		|| mesh1->head.bbox != mesh2->head.bbox
		|| mesh1->head.points_offset != mesh2->head.points_offset)
	{
		any = true;
		printf("Different misc data\n");
	}

	if(!any)
		printf("No differences.\n");

	delete mesh1;
	delete mesh2;
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
			Info("File '%s': version up to date\n", path);
			result = 0;
		}
		else
		{
			Info("File '%s': upgrading version %d -> %d\n", path, mesh->old_ver, mesh->head.version);
			mesh->Save(path);
			result = 1;
		}
	}
	catch(cstring err)
	{
		Error("Upgrade - Failed to load '%s': %s\n", path, err);
		result = -1;
	}

	delete mesh;
	return result;
}

bool EndsWith(cstring str, cstring end)
{
	uint str_len = strlen(str);
	uint end_len = strlen(end);
	if(end_len > str_len)
		return false;
	for(uint i = 0; i < end_len; ++i)
	{
		if(str[str_len - end_len + i] != end[i])
			return false;
	}
	return true;
}


void UpgradeDir(const char* path, bool force, bool subdir, int& upgraded, int& ok, int& failed)
{
	WIN32_FIND_DATA find;
	Info("Upgrade checking dir '%s'\n", path);
	HANDLE f = FindFirstFile(Format("%s/*", path), &find);
	if(f == INVALID_HANDLE_VALUE)
	{
		Error("FIND failed...\n");
		++failed;
		return;
	}

	do
	{
		if(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(subdir && strcmp(find.cFileName, ".") != 0 && strcmp(find.cFileName, "..") != 0)
			{
				string new_path = Format("%s/%s", path, find.cFileName);
				UpgradeDir(new_path.c_str(), force, subdir, upgraded, ok, failed);
			}
		}
		else
		{
			if(EndsWith(find.cFileName, ".qmsh") || EndsWith(find.cFileName, ".phy"))
			{
				string file_path = Format("%s/%s", path, find.cFileName);
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

	Info("UPGRADEDIR COMPLETE\nUpgraded: %d\nUp to date: %d\nFailed: %d\n", upgraded, ok, failed);
	_getch();
}
