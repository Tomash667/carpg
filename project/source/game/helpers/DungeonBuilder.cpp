#include "Pch.h"
#include "GameCore.h"
#include "DungeonBuilder.h"
#include "InsideLocation.h"
#include "BaseLocation.h"
#include "Physics.h"

DungeonBuilder::DungeonBuilder() : phy_world(nullptr), inside(nullptr), shape_stairs(nullptr), shape_wall(nullptr), shape_stairs_part(), dungeon_shape(nullptr),
dungeon_shape_data(nullptr)
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

void DungeonBuilder::SpawnColliders(InsideLocation* inside)
{
	assert(inside);
	this->inside = inside;
	InsideLocationLevel& lvl = inside->GetLevelData();
	SpawnStairsCollider(lvl);
	if((inside->type == L_DUNGEON && inside->target == LABIRYNTH) || inside->type == L_CAVE)
	{
		SpawnSimpleColliders(lvl);
		SpawnFloorAndCeiling(lvl);
	}
	else
		SpawnNewColliders(lvl);
	SpawnDungeonTrimesh();
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

void DungeonBuilder::SpawnFloorAndCeiling(InsideLocationLevel& lvl)
{
	dungeon_mesh.pos.clear();
	dungeon_mesh.index.clear();
	int index = 0,
		lw = lvl.w,
		lh = lvl.h;
	const float h = Room::HEIGHT;

	for(int x = 0; x < 16; ++x)
	{
		for(int y = 0; y < 16; ++y)
		{
			// floor
			dungeon_mesh.pos.push_back(Vec3(2.f * x * lw / 16, 0, 2.f * y * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * (x + 1) * lw / 16, 0, 2.f * y * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * x * lw / 16, 0, 2.f * (y + 1) * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * (x + 1) * lw / 16, 0, 2.f * (y + 1) * lh / 16));
			dungeon_mesh.index.push_back(index);
			dungeon_mesh.index.push_back(index + 1);
			dungeon_mesh.index.push_back(index + 2);
			dungeon_mesh.index.push_back(index + 2);
			dungeon_mesh.index.push_back(index + 1);
			dungeon_mesh.index.push_back(index + 3);
			index += 4;

			// ceil
			dungeon_mesh.pos.push_back(Vec3(2.f * x * lw / 16, h, 2.f * y * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * (x + 1) * lw / 16, h, 2.f * y * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * x * lw / 16, h, 2.f * (y + 1) * lh / 16));
			dungeon_mesh.pos.push_back(Vec3(2.f * (x + 1) * lw / 16, h, 2.f * (y + 1) * lh / 16));
			dungeon_mesh.index.push_back(index);
			dungeon_mesh.index.push_back(index + 2);
			dungeon_mesh.index.push_back(index + 1);
			dungeon_mesh.index.push_back(index + 2);
			dungeon_mesh.index.push_back(index + 3);
			dungeon_mesh.index.push_back(index + 1);
			index += 4;
		}
	}
}

void DungeonBuilder::SpawnNewColliders(InsideLocationLevel& lvl)
{
	dungeon_mesh.pos.clear();
	dungeon_mesh.index.clear();
	Pole* m = lvl.map;
	int index = 0,
		lw = lvl.w,
		lh = lvl.h;

	for(Room& room : lvl.rooms)
	{
		const float h = (room.IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);

		// floor
		AddFace(Vec3(2.f * room.pos.x, room.y, 2.f * room.pos.y), Vec3(2.f * room.size.x, 0, 0), Vec3(0, 0, 2.f * room.size.y), index);

		// ceil
		AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * room.pos.y), Vec3(2.f * room.size.x, 0, 0), Vec3(0, 0, 2.f * room.size.y), index);

		// left wall
		if(room.pos.x > 0)
		{
			int start = -1;
			for(int y = 0; y < room.size.y; ++y)
			{
				if(czy_blokuje2(m[room.pos.x - 1 + (room.pos.y + y) * lw]))
				{
					if(start == -1)
						start = y;
				}
				else if(start != -1)
				{
					int length = y - start;
					AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)), Vec3(0, 0, 2.f * length), Vec3(0, -h, 0), index);
					start = -1;
				}
			}
			if(start != -1)
			{
				int length = room.size.y - start;
				AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + start)), Vec3(0, 0, 2.f * length), Vec3(0, -h, 0), index);
			}
		}
		else
			AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * room.pos.y), Vec3(0, 0, 2.f * room.size.y), Vec3(0, -h, 0), index);
		
		// right wall
		if(room.pos.x + room.size.x < lw)
		{
			int start = -1;
			for(int y = room.size.y - 1; y >= 0; --y)
			{
				if(czy_blokuje2(m[room.pos.x + room.size.x + (room.pos.y + y) * lw]))
				{
					if(start == -1)
						start = y;
				}
				else if(start != -1)
				{
					int length = (start - y);
					AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)), Vec3(0, 0, -2.f * length), Vec3(0, -h, 0), index);
					start = -1;
				}
			}
			if(start != -1)
			{
				int length = start + 1;
				AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + start + 1)), Vec3(0, 0, -2.f * length), Vec3(0, -h, 0), index);
			}
		}
		else
			AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(0, 0, -2.f * room.size.y), Vec3(0, -h, 0), index);
		
		// front wall
		if(room.pos.y + room.size.y < lh)
		{
			int start = -1;
			for(int x = 0; x < room.size.x; ++x)
			{
				if(czy_blokuje2(m[room.pos.x + x + (room.pos.y + room.size.y) * lw]))
				{
					if(start == -1)
						start = x;
				}
				else if(start != -1)
				{
					int length = x - start;
					AddFace(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * length, 0, 0), Vec3(0, -h, 0), index);
					start = -1;
				}
			}
			if(start != -1)
			{
				int length = room.size.x - start;
				AddFace(Vec3(2.f * (room.pos.x + start), room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * length, 0, 0), Vec3(0, -h, 0), index);
			}
		}
		else
			AddFace(Vec3(2.f * room.pos.x, room.y + h, 2.f * (room.pos.y + room.size.y)), Vec3(2.f * room.size.x, 0, 0), Vec3(0, -h, 0), index);

		// back wall
		if(room.pos.y > 0)
		{
			int start = -1;
			for(int x = room.size.x - 1; x >= 0; --x)
			{
				if(czy_blokuje2(m[room.pos.x + x + (room.pos.y - 1) * lw]))
				{
					if(start == -1)
						start = x;
				}
				else if(start != -1)
				{
					int length = (start - x);
					AddFace(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y), Vec3(-2.f * length, 0, 0), Vec3(0, -h, 0), index);
					start = -1;
				}
			}
			if(start != -1)
			{
				int length = start + 1;
				AddFace(Vec3(2.f * (room.pos.x + start + 1), room.y + h, 2.f * room.pos.y), Vec3(-2.f * length, 0, 0), Vec3(0, -h, 0), index);
			}
		}
		else
			AddFace(Vec3(2.f * (room.pos.x + room.size.x), room.y + h, 2.f * room.pos.y), Vec3(-2.f * room.size.x, 0, 0), Vec3(0, -h, 0), index);
	}
}

void DungeonBuilder::SpawnDungeonTrimesh()
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

void DungeonBuilder::AddFace(const Vec3& pos, const Vec3& shift, const Vec3& shift2, int& index)
{
	dungeon_mesh.pos.push_back(pos);
	dungeon_mesh.pos.push_back(pos + shift);
	dungeon_mesh.pos.push_back(pos + shift2);
	dungeon_mesh.pos.push_back(pos + shift + shift2);
	dungeon_mesh.index.push_back(index);
	dungeon_mesh.index.push_back(index + 1);
	dungeon_mesh.index.push_back(index + 2);
	dungeon_mesh.index.push_back(index + 2);
	dungeon_mesh.index.push_back(index + 1);
	dungeon_mesh.index.push_back(index + 3);
	index += 4;
}
