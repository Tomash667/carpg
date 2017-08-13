#include "PCH.hpp"
#include "MeshTask.hpp"
#include "GlobalCode.hpp"

struct Mesh
{
	enum MESH_FLAGS
	{
		F_TANGENTS = 1 << 0,
		F_ANIMATED = 1 << 1,
		F_STATIC = 1 << 2,
		F_PHYSICS = 1 << 3,
		F_SPLIT = 1 << 4
	};

	struct Header
	{
		char format[4];
		byte version, flags;
		unsigned short n_verts, n_tris, n_subs, n_bones, n_anims, n_points, n_groups;
		float radius;
		BOX bbox;
		uint points_offset;
	};
};

void Info(const char* path)
{
	FileStream f(path, FM_READ);

	Mesh::Header head;
	f.Read(&head, sizeof(head));

	if(head.format[0] != 'Q' || head.format[1] != 'M' || head.format[2] != 'S' || head.format[3] != 'H')
	{
		printf("Invalid file '%s'.\n", path);
		return;
	}

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
}
