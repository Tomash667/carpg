#include "Pch.h"
#include "GameCore.h"
#include "DungeonBuilder.h"
#include "InsideLocation.h"
#include "BaseLocation.h"
#include "Physics.h"
#include "Engine.h"

DungeonBuilder::DungeonBuilder() : phy_world(nullptr), inside(nullptr), shape_stairs(nullptr), shape_wall(nullptr), shape_stairs_part(), dungeon_shape(nullptr),
dungeon_shape_data(nullptr), mesh_vb(nullptr), mesh_ib(nullptr)
{
}

DungeonBuilder::~DungeonBuilder()
{
	delete shape_stairs_part[0];
	delete shape_stairs_part[1];
	delete shape_stairs;
	delete shape_wall;
	delete dungeon_shape;
	delete dungeon_shape_data;
	SafeRelease(mesh_vb);
	SafeRelease(mesh_ib);
}

void DungeonBuilder::Init(CustomCollisionWorld* phy_world)
{
	assert(phy_world);
	this->phy_world = phy_world;

	shape_wall = new btBoxShape(btVector3(1.f, 2.f, 1.f));

	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f, 2.f, 0.1f));
	shape_stairs_part[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f, 2.f, 0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f, 2.f, 1.f));
	shape_stairs_part[1] = b;
	tr.setOrigin(btVector3(-0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	shape_stairs = s;
}

void DungeonBuilder::Build(InsideLocation* inside)
{
	assert(inside);
	this->inside = inside;

	InsideLocationLevel& lvl = inside->GetLevelData();
	BuildPhysics(lvl);
	BuildMesh(lvl);
}

void DungeonBuilder::BuildPhysics(InsideLocationLevel& lvl)
{
	index_counter = 0;
	dungeon_mesh.pos.clear();
	dungeon_mesh.index.clear();

	SpawnStairsCollider(lvl);
	if((inside->type == L_DUNGEON && inside->target == LABIRYNTH) || inside->type == L_CAVE)
	{
		SpawnSimpleColliders(lvl);
		FillFloorAndCeiling(lvl);
	}
	else
		FillDungeonCollider(lvl);
	SpawnDungeonCollider();
}

void DungeonBuilder::SpawnSimpleColliders(InsideLocationLevel& lvl)
{
	Pole* m = lvl.map;
	int w = lvl.w,
		h = lvl.h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(czy_blokuje2(m[x + y*w]) && (!czy_blokuje2(m[x - 1 + (y - 1)*w]) || !czy_blokuje2(m[x + (y - 1)*w]) || !czy_blokuje2(m[x + 1 + (y - 1)*w]) ||
				!czy_blokuje2(m[x - 1 + y*w]) || !czy_blokuje2(m[x + 1 + y*w]) ||
				!czy_blokuje2(m[x - 1 + (y + 1)*w]) || !czy_blokuje2(m[x + (y + 1)*w]) || !czy_blokuje2(m[x + 1 + (y + 1)*w])))
			{
				SpawnCollider(Vec3(2.f*x + 1.f, 2.f, 2.f*y + 1.f));
			}
		}
	}

	// lewa/prawa œciana
	for(int i = 1; i < h - 1; ++i)
	{
		// lewa
		if(czy_blokuje2(m[i*w]) && !czy_blokuje2(m[1 + i*w]))
			SpawnCollider(Vec3(1.f, 2.f, 2.f*i + 1.f));

		// prawa
		if(czy_blokuje2(m[i*w + w - 1]) && !czy_blokuje2(m[w - 2 + i*w]))
			SpawnCollider(Vec3(2.f*(w - 1) + 1.f, 2.f, 2.f*i + 1.f));
	}

	// przednia/tylna œciana
	for(int i = 1; i < lvl.w - 1; ++i)
	{
		// przednia
		if(czy_blokuje2(m[i + (h - 1)*w]) && !czy_blokuje2(m[i + (h - 2)*w]))
			SpawnCollider(Vec3(2.f*i + 1.f, 2.f, 2.f*(h - 1) + 1.f));

		// tylna
		if(czy_blokuje2(m[i]) && !czy_blokuje2(m[i + w]))
			SpawnCollider(Vec3(2.f*i + 1.f, 2.f, 1.f));
	}
}

void DungeonBuilder::SpawnStairsCollider(InsideLocationLevel& lvl)
{
	if(inside->HaveUpStairs())
	{
		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_stairs);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
		cobj->getWorldTransform().setOrigin(btVector3(2.f*lvl.staircase_up.x + 1.f, 0.f, 2.f*lvl.staircase_up.y + 1.f));
		cobj->getWorldTransform().setRotation(btQuaternion(dir_to_rot(lvl.staircase_up_dir), 0, 0));
		phy_world->addCollisionObject(cobj, CG_BUILDING);
	}
}

void DungeonBuilder::SpawnCollider(const Vec3& pos)
{
	btCollisionObject* cobj = new btCollisionObject;
	cobj->setCollisionShape(shape_wall);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	cobj->getWorldTransform().setOrigin(ToVector3(pos));
	phy_world->addCollisionObject(cobj, CG_BUILDING);
}

void DungeonBuilder::FillFloorAndCeiling(InsideLocationLevel& lvl)
{
	int lw = lvl.w,
		lh = lvl.h;
	const float h = Room::HEIGHT;

	for(int x = 0; x < 16; ++x)
	{
		for(int y = 0; y < 16; ++y)
		{
			// floor
			AddFace(Vec3(2.f * x * lw / 16, 0, 2.f * (y + 1) * lh / 16), Vec3(2.f * lw / 16, 0, 0), Vec3(0, 0, -2.f * lh / 16));

			// ceil
			AddFace(Vec3(2.f * x * lw / 16, h, 2.f * y * lh / 16), Vec3(2.f * lw / 16, 0, 0), Vec3(0, 0, 2.f * lh / 16));
		}
	}
}

void DungeonBuilder::FillDungeonCollider(InsideLocationLevel& lvl)
{
	Pole* m = lvl.map;
	int lw = lvl.w,
		start;
	const float low_h_dif = Room::HEIGHT - Room::HEIGHT_LOW;

	for(Room& room : lvl.rooms)
	{
		const bool is_corridor = room.IsCorridor();
		const float h = (is_corridor ? Room::HEIGHT_LOW : Room::HEIGHT);

		// floor
		AddFace(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * room.size.x, 0, 0), Vec3(0, 0, -2.f * room.size.y));

		// ceil
		AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * room.pos.y), Vec3(2.f * room.size.x, 0, 0), Vec3(0, 0, 2.f * room.size.y));

		// left wall
		start = -1;
		for(int y = 0; y < room.size.y; ++y)
		{
			Pole& p = m[room.pos.x - 1 + (room.pos.y + y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = y;
			}
			else if(start != -1)
			{
				int length = y - start;
				AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)), Vec3(0, 0, 2.f * length), Vec3(0, -h, 0));
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + y)), Vec3(0, 0, 2.f), Vec3(0, -low_h_dif, 0));
		}
		if(start != -1)
		{
			int length = room.size.y - start;
			AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)), Vec3(0, 0, 2.f * length), Vec3(0, -h, 0));
		}
		
		// right wall
		start = -1;
		for(int y = room.size.y - 1; y >= 0; --y)
		{
			Pole& p = m[room.pos.x + room.size.x + (room.pos.y + y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = y;
			}
			else if(start != -1)
			{
				int length = (start - y);
				AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)), Vec3(0, 0, -2.f * length), Vec3(0, -h, 0));
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + y + 1)), Vec3(0, 0, -2.f), Vec3(0, -low_h_dif, 0));
		}
		if(start != -1)
		{
			int length = start + 1;
			AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)), Vec3(0, 0, -2.f * length), Vec3(0, -h, 0));
		}

		// front wall
		start = -1;
		for(int x = 0; x < room.size.x; ++x)
		{
			Pole& p = m[room.pos.x + x + (room.pos.y + room.size.y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = x;
			}
			else if(start != -1)
			{
				int length = x - start;
				AddFace(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * length, 0, 0), Vec3(0, -h, 0));
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				AddFace(Vec3(2.f * (room.pos.x + x), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f, 0, 0), Vec3(0, -low_h_dif, 0));
		}
		if(start != -1)
		{
			int length = room.size.x - start;
			AddFace(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * length, 0, 0), Vec3(0, -h, 0));
		}

		// back wall
		start = -1;
		for(int x = room.size.x - 1; x >= 0; --x)
		{
			Pole& p = m[room.pos.x + x + (room.pos.y - 1) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = x;
			}
			else if(start != -1)
			{
				int length = (start - x);
				AddFace(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y), Vec3(-2.f * length, 0, 0), Vec3(0, -h, 0));
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				AddFace(Vec3(2.f * (room.pos.x + x + 1), room.y + h, 2.f * room.pos.y), Vec3(-2.f, 0, 0), Vec3(0, -low_h_dif, 0));
		}
		if(start != -1)
		{
			int length = start + 1;
			AddFace(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y), Vec3(-2.f * length, 0, 0), Vec3(0, -h, 0));
		}
	}
}

void DungeonBuilder::AddFace(const Vec3& pos, const Vec3& shift, const Vec3& shift2)
{
	dungeon_mesh.pos.push_back(pos);
	dungeon_mesh.pos.push_back(pos + shift);
	dungeon_mesh.pos.push_back(pos + shift2);
	dungeon_mesh.pos.push_back(pos + shift + shift2);
	dungeon_mesh.index.push_back(index_counter);
	dungeon_mesh.index.push_back(index_counter + 1);
	dungeon_mesh.index.push_back(index_counter + 2);
	dungeon_mesh.index.push_back(index_counter + 2);
	dungeon_mesh.index.push_back(index_counter + 1);
	dungeon_mesh.index.push_back(index_counter + 3);
	index_counter += 4;
}

void DungeonBuilder::SpawnDungeonCollider()
{
	delete dungeon_shape;
	delete dungeon_shape_data;
	
	dungeon_shape_data = new btTriangleIndexVertexArray(dungeon_mesh.index.size() / 3, dungeon_mesh.index.data(), sizeof(int) * 3,
		dungeon_mesh.pos.size(), (btScalar*)dungeon_mesh.pos.data(), sizeof(Vec3));
	dungeon_shape = new btBvhTriangleMeshShape(dungeon_shape_data, true);

	btCollisionObject* obj_dungeon = new btCollisionObject;
	obj_dungeon->setCollisionShape(dungeon_shape);
	obj_dungeon->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	obj_dungeon->setUserPointer(&dungeon_mesh);
	phy_world->addCollisionObject(obj_dungeon, CG_BUILDING);
}

void DungeonBuilder::BuildMesh(InsideLocationLevel& lvl)
{
	mesh_v.clear();
	mesh_i.clear();
	index_counter = 0;

	FillMeshData(lvl);
	CreateMesh();
}

void DungeonBuilder::FillMeshData(InsideLocationLevel& lvl)
{
#define NTB_PX Vec3(1,0,0), Vec3(0,0,1), Vec3(0,-1,0)
#define NTB_MX Vec3(-1,0,0), Vec3(0,0,-1), Vec3(0,-1,0)
#define NTB_PY Vec3(0,1,0), Vec3(1,0,0), Vec3(0,0,-1)
#define NTB_MY Vec3(0,-1,0), Vec3(1,0,0), Vec3(0,0,1)
#define NTB_PZ Vec3(0,0,1), Vec3(-1,0,0), Vec3(0,-1,0)
#define NTB_MZ Vec3(0,0,-1), Vec3(1,0,0), Vec3(0,-1,0)

	int start, lw = lvl.w;
	const float V0 = 2.f;// (dungeon_tex_wrap ? 2.f : 1);
	const float Vlow = 1.5f;
	Pole* m = lvl.map;
	const float low_h_dif = Room::HEIGHT - Room::HEIGHT_LOW;

	for(Room& room : lvl.rooms)
	{
		const bool is_corridor = room.IsCorridor();
		const float h = (is_corridor ? Room::HEIGHT_LOW : Room::HEIGHT);

		// floor
		if(IS_SET(room.flags, Room::F_HAVE_FLOOR_HOLES))
		{
			for(int x = room.pos.x; x < room.pos.x + room.size.x; ++x)
			{
				for(int y = room.pos.y; y < room.pos.y + room.size.y; ++y)
				{
					if(!Any(m[x + y * lw].type, KRATKA, KRATKA_PODLOGA))
					{
						mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * (y + 1)),
							Vec2(0, 1), NTB_PY));
						mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * (y + 1)),
							Vec2(1, 1), NTB_PY));
						mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * y),
							Vec2(0, 0), NTB_PY));
						mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * y),
							Vec2(1, 0), NTB_PY));
						PushIndices();
					}
				}
			}
		}
		else
		{
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + room.size.y)),
				Vec2(0, (float)room.size.y), NTB_PY));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * (room.pos.y + room.size.y)),
				Vec2((float)room.size.x, (float)room.size.y), NTB_PY));
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * room.pos.y),
				Vec2(0, 0), NTB_PY));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * room.pos.y),
				Vec2((float)room.size.x, 0), NTB_PY));
			PushIndices();
		}

		// ceiling
		if(IS_SET(room.flags, Room::F_HAVE_CEIL_HOLES))
		{
			for(int x = room.pos.x; x < room.pos.x + room.size.x; ++x)
			{
				for(int y = room.pos.y; y < room.pos.y + room.size.y; ++y)
				{
					if(!Any(m[x + y * lw].type, KRATKA, KRATKA_SUFIT))
					{
						mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * y),
							Vec2(0, 0), NTB_MY));
						mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * y),
							Vec2(1, 0), NTB_MY));
						mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * (y + 1)),
							Vec2(0, 1), NTB_MY));
						mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * (y + 1)),
							Vec2(1, 1), NTB_MY));
						PushIndices();
					}
				}
			}
		}
		else
		{
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * room.pos.y),
				Vec2(0, 0), NTB_MY));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * room.pos.y),
				Vec2((float)room.size.x, 0), NTB_MY));
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + room.size.y)),
				Vec2(0, (float)room.size.y), NTB_MY));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + room.size.y)),
				Vec2((float)room.size.x, (float)room.size.y), NTB_MY));
			PushIndices();
		}

		// left wall
		start = -1;
		for(int y = 0; y < room.size.y; ++y)
		{
			Pole& p = m[room.pos.x - 1 + (room.pos.y + y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = y;
			}
			else if(start != -1)
			{
				int length = y - start;
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)),
					Vec2(0, V0), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start + length)),
					Vec2((float)length, V0), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + start)),
					Vec2(0, 0), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + start + length)),
					Vec2((float)length, 0), NTB_MX));
				PushIndices();
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + y)),
					Vec2(0, V0), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + y + 1)),
					Vec2(1.f, V0), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h - low_h_dif, 2.f * (room.pos.y + y)),
					Vec2(0, Vlow), NTB_MX));
				mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h - low_h_dif, 2.f * (room.pos.y + y + 1)),
					Vec2(1.f, Vlow), NTB_MX));
				PushIndices();
			}
		}
		if(start != -1)
		{
			int length = room.size.y - start;
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)),
				Vec2(0, V0), NTB_MX));
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start + length)),
				Vec2((float)length, V0), NTB_MX));
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + start)),
				Vec2(0, 0), NTB_MX));
			mesh_v.push_back(VTangent(Vec3(2.f * room.pos.x, room.y, 2.f * (room.pos.y + start + length)),
				Vec2((float)length, 0), NTB_MX));
			PushIndices();
		}

		// right wall
		start = -1;
		for(int y = room.size.y - 1; y >= 0; --y)
		{
			Pole& p = m[room.pos.x + room.size.x + (room.pos.y + y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = y;
			}
			else if(start != -1)
			{
				int length = (start - y);
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)),
					Vec2(0, V0), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1 - length)),
					Vec2((float)length, V0), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * (room.pos.y + start + 1)),
					Vec2(0, 0), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * (room.pos.y + start + 1 - length)),
					Vec2((float)length, 0), NTB_PX));
				PushIndices();
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + y + 1)),
					Vec2(0, V0), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + y)),
					Vec2(1, V0), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h - low_h_dif, 2.f * (room.pos.y + y + 1)),
					Vec2(0, Vlow), NTB_PX));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h - low_h_dif, 2.f * (room.pos.y + y)),
					Vec2(1, Vlow), NTB_PX));
				PushIndices();
			}
		}
		if(start != -1)
		{
			int length = start + 1;
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)),
				Vec2(0, V0), NTB_PX));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1 - length)),
				Vec2((float)length, V0), NTB_PX));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * (room.pos.y + start + 1)),
				Vec2(0, 0), NTB_PX));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + room.size.x), room.y, 2.f * (room.pos.y + start + 1 - length)),
				Vec2((float)length, 0), NTB_PX));
			PushIndices();
		}

		// front wall
		start = -1;
		for(int x = 0; x < room.size.x; ++x)
		{
			Pole& p = m[room.pos.x + x + (room.pos.y + room.size.y) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = x;
			}
			else if(start != -1)
			{
				int length = x - start;
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)),
					Vec2(0, V0), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + length), room.y + h, 2.f * (room.pos.y + room.size.y)),
					Vec2((float)length, V0), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start), room.y, 2.f * (room.pos.y + room.size.y)),
					Vec2(0, 0), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + length), room.y, 2.f * (room.pos.y + room.size.y)),
					Vec2((float)length, 0), NTB_MZ));
				PushIndices();
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x), room.y + h, 2.f * (room.pos.y + room.size.y)),
					Vec2(0, V0), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x + 1), room.y + h, 2.f * (room.pos.y + room.size.y)),
					Vec2(1, V0), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x), room.y + h - low_h_dif, 2.f * (room.pos.y + room.size.y)),
					Vec2(0, Vlow), NTB_MZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x + 1), room.y + h - low_h_dif, 2.f * (room.pos.y + room.size.y)),
					Vec2(1, Vlow), NTB_MZ));
				PushIndices();
			}
		}
		if(start != -1)
		{
			int length = room.size.x - start;
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)),
				Vec2(0, V0), NTB_MZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + length), room.y + h, 2.f * (room.pos.y + room.size.y)),
				Vec2((float)length, V0), NTB_MZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start), room.y, 2.f * (room.pos.y + room.size.y)),
				Vec2(0, 0), NTB_MZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + length), room.y, 2.f * (room.pos.y + room.size.y)),
				Vec2((float)length, 0), NTB_MZ));
			PushIndices();
		}

		// back wall
		start = -1;
		for(int x = room.size.x - 1; x >= 0; --x)
		{
			Pole& p = m[room.pos.x + x + (room.pos.y - 1) * lw];
			if(czy_blokuje2(p))
			{
				if(start == -1)
					start = x;
			}
			else if(start != -1)
			{
				int length = (start - x);
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y),
					Vec2(0, V0), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1 - length), room.y + h, 2.f * room.pos.y),
					Vec2((float)length, V0), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1), room.y, 2.f * room.pos.y),
					Vec2(0, 0), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1 - length), room.y, 2.f * room.pos.y),
					Vec2((float)length, 0), NTB_PZ));
				PushIndices();
				start = -1;
			}

			if(!is_corridor && IS_SET(p.flags, Pole::F_NISKI_SUFIT))
			{
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x + 1), room.y + h, 2.f * room.pos.y),
					Vec2(0, V0), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x), room.y + h, 2.f * room.pos.y),
					Vec2(1, V0), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x + 1), room.y + h - low_h_dif, 2.f * room.pos.y),
					Vec2(0, Vlow), NTB_PZ));
				mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + x), room.y + h - low_h_dif, 2.f * room.pos.y),
					Vec2(1, Vlow), NTB_PZ));
				PushIndices();
			}
		}
		if(start != -1)
		{
			int length = start + 1;
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y),
				Vec2(0, V0), NTB_PZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1 - length), room.y + h, 2.f * room.pos.y),
				Vec2((float)length, V0), NTB_PZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1), room.y, 2.f * room.pos.y),
				Vec2(0, 0), NTB_PZ));
			mesh_v.push_back(VTangent(Vec3(2.f * (room.pos.x + start + 1 - length), room.y, 2.f * room.pos.y),
				Vec2((float)length, 0), NTB_PZ));
			PushIndices();
		}

		// hole walls
		if(IS_SET(room.flags, Room::F_HAVE_CEIL_HOLES | Room::F_HAVE_FLOOR_HOLES))
		{
			for(int x = room.pos.x; x < room.pos.x + room.size.x; ++x)
			{
				for(int y = room.pos.y; y < room.pos.y + room.size.y; ++y)
				{
					Pole& p = m[x + y * lw];
					if(Any(p.type, KRATKA, KRATKA_PODLOGA))
					{
						// left
						if(!Any(m[x - 1 + y * lw].type, KRATKA, KRATKA_PODLOGA))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * y),
								Vec2(0, V0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * (y + 1)),
								Vec2(1, V0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y - h, 2.f * y),
								Vec2(0, 0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y - h, 2.f * (y + 1)),
								Vec2(1, 0), NTB_MX));
							PushIndices();
						}

						// right
						if(!Any(m[x + 1 + y * lw].type, KRATKA, KRATKA_PODLOGA))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * (y + 1)),
								Vec2(0, V0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * y),
								Vec2(1, V0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y - h, 2.f * (y + 1)),
								Vec2(0, 0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y - h, 2.f * y),
								Vec2(1, 0), NTB_PX));
							PushIndices();
						}

						// top
						if(!Any(m[x + (y + 1) * lw].type, KRATKA, KRATKA_PODLOGA))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * (y + 1)),
								Vec2(0, V0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * (y + 1)),
								Vec2(1, V0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y - h, 2.f * (y + 1)),
								Vec2(0, 0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y - h, 2.f * (y + 1)),
								Vec2(1, 0), NTB_MZ));
							PushIndices();
						}

						// bottom
						if(!Any(m[x + (y - 1) * lw].type, KRATKA, KRATKA_PODLOGA))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y, 2.f * y),
								Vec2(0, V0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y, 2.f * y),
								Vec2(1, V0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y - h, 2.f * y),
								Vec2(0, 0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y - h, 2.f * y),
								Vec2(1, 0), NTB_PZ));
							PushIndices();
						}
					}

					if(Any(p.type, KRATKA, KRATKA_SUFIT))
					{
						// left
						if(!Any(m[x - 1 + y * lw].type, KRATKA, KRATKA_SUFIT))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h * 2, 2.f * y),
								Vec2(0, V0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h * 2, 2.f * (y + 1)),
								Vec2(1, V0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * y),
								Vec2(0, 0), NTB_MX));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * (y + 1)),
								Vec2(1, 0), NTB_MX));
							PushIndices();
						}

						// right
						if(!Any(m[x + 1 + y * lw].type, KRATKA, KRATKA_SUFIT))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h * 2, 2.f * (y + 1)),
								Vec2(0, V0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h * 2, 2.f * y),
								Vec2(1, V0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * (y + 1)),
								Vec2(0, 0), NTB_PX));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * y),
								Vec2(1, 0), NTB_PX));
							PushIndices();
						}

						// top
						if(!Any(m[x + (y + 1) * lw].type, KRATKA, KRATKA_SUFIT))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h * 2, 2.f * (y + 1)),
								Vec2(0, V0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h * 2, 2.f * (y + 1)),
								Vec2(1, V0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * (y + 1)),
								Vec2(0, 0), NTB_MZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * (y + 1)),
								Vec2(1, 0), NTB_MZ));
							PushIndices();
						}

						// bottom
						if(!Any(m[x + (y - 1) * lw].type, KRATKA, KRATKA_SUFIT))
						{
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h * 2, 2.f * y),
								Vec2(0, V0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h * 2, 2.f * y),
								Vec2(1, V0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * (x + 1), room.y + h, 2.f * y),
								Vec2(0, 0), NTB_PZ));
							mesh_v.push_back(VTangent(Vec3(2.f * x, room.y + h, 2.f * y),
								Vec2(1, 0), NTB_PZ));
							PushIndices();
						}
					}
				}
			}
		}
	}

#undef NTB_PX
#undef NTB_MX
#undef NTB_PY
#undef NTB_MY
#undef NTB_PZ
#undef NTB_MZ
}

void DungeonBuilder::CreateMesh()
{
	SafeRelease(mesh_vb);
	SafeRelease(mesh_ib);

	IDirect3DDevice9* device = Engine::Get().device;
	void* data;

	V(device->CreateVertexBuffer(sizeof(VTangent) * mesh_v.size(), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &mesh_vb, nullptr));
	V(mesh_vb->Lock(0, 0, &data, 0));
	memcpy(data, mesh_v.data(), sizeof(VTangent) * mesh_v.size());
	V(mesh_vb->Unlock());

	V(device->CreateIndexBuffer(sizeof(int) * mesh_i.size(), D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &mesh_ib, nullptr));
	V(mesh_ib->Lock(0, 0, &data, 0));
	memcpy(data, mesh_i.data(), sizeof(int) * mesh_i.size());
	V(mesh_ib->Unlock());

	primitive_count = mesh_i.size() / 3;
	vertex_count = mesh_v.size();
}

void DungeonBuilder::PushIndices()
{
	mesh_i.push_back(index_counter);
	mesh_i.push_back(index_counter + 1);
	mesh_i.push_back(index_counter + 2);
	mesh_i.push_back(index_counter + 2);
	mesh_i.push_back(index_counter + 1);
	mesh_i.push_back(index_counter + 3);
	index_counter += 4;
}
