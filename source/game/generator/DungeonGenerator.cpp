#include "Pch.h"
#include "Base.h"
#include "Engine.h"
#include "Game.h"

/*enum COLLISION_GROUP
{
	CG_WALL = 1<<8,
	CG_UNIT = 1<<9
};*/

const int COL_FLAG = 1<<10;
const int COL_DUNGEON = 1<<11;

VEC3 testHitpoint;

enum ROT_HINT
{
	BOTTOM,
	LEFT,
	TOP,
	RIGHT,
	RANDOM
};

struct RotInfo
{
	ROT_HINT dir;
	VEC2 pos;
	float rot;
	ROT_HINT other_dir;
};

RotInfo rot_info[] = {
	{ BOTTOM, VEC2(0, -1), PI/2, TOP },
	{ LEFT, VEC2(-1, 0), PI, RIGHT },
	{ TOP, VEC2(0, +1), PI*3/2, BOTTOM },
	{ RIGHT, VEC2(+1, 0), 0, LEFT }
};

namespace X
{
	enum Type
	{
		CORRIDOR,
		ROOM,
		JUNCTION
	};

	struct XRoom
	{
		vector<XRoom*> connected;
		VEC3 pos, size;
		float rot;
		Type type;
		INT2 tile_size;
		int junction_index;
		btCollisionObject* cobj;
		int holes;

		void GetHoleTiles(INT2(&tiles)[4])
		{
			if(holes & (1<<LEFT))
				tiles[LEFT] = INT2(0, tile_size.y/2);
			else
				tiles[LEFT] = INT2(-1, -1);

			if(holes & (1<<RIGHT))
				tiles[RIGHT] = INT2(tile_size.x-1, tile_size.y/2);
			else
				tiles[RIGHT] = INT2(-1, -1);

			if(holes & (1<<BOTTOM))
				tiles[BOTTOM] = INT2(tile_size.x/2, 0);
			else
				tiles[BOTTOM] = INT2(-1, -1);

			if(holes & (1<<TOP))
				tiles[TOP] = INT2(tile_size.x/2, tile_size.y-1);
			else
				tiles[TOP] = INT2(-1, -1);
		}
	};

	struct XPortal
	{
		XRoom* room;
		VEC3 pos;
		float rot;
		int index;

		XPortal(XRoom* room, const VEC3& pos, float rot, int index) : room(room), pos(pos), rot(rot), index(index) {}
	};

	struct XDoor
	{
		VEC3 pos;
		float rot;

		XDoor(const VEC3& pos, float rot) : pos(pos), rot(rot) {}
	};

	struct XDungeon
	{
		vector<XRoom*> rooms;

		~XDungeon()
		{
			DeleteElements(rooms);
		}
	};

	struct XJunction
	{
		MeshData* md;
		int floor_index, wall_index, ceil_index;
	};
	
	Animesh* portalDoor;
	vector<XJunction*> junctions;
	vector<XPortal> portals, new_portals;
	vector<XDoor> door;
	vector<btCollisionObject*> objs, invalid_objs;
	vector<btBoxShape*> shapes;
	ROT_HINT start_rot, forced_room_side;
	bool check_phy;
	int corridor_to_junction, corridor_to_room, corridor_to_corridor, room_to_corridor, room_to_junction, room_to_room, junction_to_corridor,
		junction_to_room, junction_to_junction, forced_junction_side;
	INT2 room_size_w, room_size_h, corridor_length;

	ROT_HINT ToRot(const string& s)
	{
		if(s == "random")
			return RANDOM;
		else if(s == "left")
			return LEFT;
		else if(s == "right")
			return RIGHT;
		else if(s == "top")
			return TOP;
		else if(s == "bottom")
			return BOTTOM;
		else
			return RANDOM;
	}

	void LoadConfig()
	{
		Config cfg;
		cfg.Load("generator.cfg");
		start_rot = ToRot(cfg.GetString("start_rot", "random"));
		check_phy = cfg.GetBool("check_phy", true);
		corridor_to_junction = cfg.GetInt("corridor_to_junction", 0);
		corridor_to_room = cfg.GetInt("corridor_to_room", 0);
		corridor_to_corridor = cfg.GetInt("corridor_to_corridor", 0);
		room_to_corridor = cfg.GetInt("room_to_corridor", 0);
		room_to_junction = cfg.GetInt("room_to_junction", 0);
		room_to_room = cfg.GetInt("room_to_room", 0);
		junction_to_corridor = cfg.GetInt("junction_to_corridor", 0);
		junction_to_room = cfg.GetInt("junction_to_room", 0);
		junction_to_junction = cfg.GetInt("junction_to_junction", 0);
		forced_junction_side = cfg.GetInt("forced_junction_side", 0);
		forced_room_side = ToRot(cfg.GetString("forced_room_side", "random"));
		room_size_w = cfg.GetInt2("room_size_w", INT2(3, 7));
		room_size_h = cfg.GetInt2("room_size_h", INT2(3, 7));
		corridor_length = cfg.GetInt2("corridor_length", INT2(3, 8));
	}

	struct Callback : public btCollisionWorld::ContactResultCallback
	{
		bool hit;
		const btCollisionObject* other;
		const btCollisionObject* ignore;

		Callback(btCollisionObject* ignore) : hit(false), other(nullptr), ignore(ignore) {}

		bool needsCollision(btBroadphaseProxy* proxy0) const
		{
			return proxy0->m_collisionFilterGroup == COL_FLAG && proxy0->m_collisionFilterMask == COL_FLAG;
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
		{
			if(colObj1Wrap->getCollisionObject() == ignore)
				return 1;
			other = colObj1Wrap->getCollisionObject();
			hit = true;
			return 0;
		}
	};

	bool CreatePhy(XRoom* r, btCollisionObject* other_cobj, float y = 0.f)
	{
		auto world = Game::Get().phy_world;

		btCollisionObject* obj = new btCollisionObject;
		obj->getWorldTransform().setOrigin(btVector3(r->pos.x, r->pos.y + r->size.y/2 + y, r->pos.z));
		obj->getWorldTransform().setRotation(btQuaternion(r->rot, 0, 0));
		btBoxShape* shape = new btBoxShape(btVector3(r->size.x/2, r->size.y/2, r->size.z/2));
		shape->setMargin(0.01f);
		shapes.push_back(shape);
		obj->setCollisionShape(shape);

		if(check_phy)
		{
			Callback c(other_cobj);
			world->contactTest(obj, c);
			if(c.hit)
			{
				invalid_objs.push_back(obj);
				return false;
			}
		}

		r->cobj = obj;
		world->addCollisionObject(obj, COL_FLAG, COL_FLAG);
		objs.push_back(obj);
		return true;
	}

	struct Vertex
	{
		VEC3 pos;
		VEC3 normal;
		VEC2 tex;
	};
	
	void AddJunction(cstring mesh_name)
	{
		MeshData* mesh_data = (MeshData*)ResourceManager::Get().GetLoadedMeshData(mesh_name)->data;
		XJunction* j = new XJunction;
		j->md = mesh_data;
		j->floor_index = mesh_data->GetSubmeshIndex("floor");
		j->wall_index = mesh_data->GetSubmeshIndex("wall");
		j->ceil_index = mesh_data->GetSubmeshIndex("ceil");
		if(j->floor_index == -1 || j->wall_index == -1 || j->ceil_index == -1)
			throw Format("Invalid submesh '%s' for junction, missing required submesh.", mesh_name);
		junctions.push_back(j);
	}

	void LoadMeshes()
	{
		ResourceManager& resMgr = ResourceManager::Get();		
		resMgr.GetLoadedMesh("portal_door.qmsh", portalDoor);

		AddJunction("dungeon1.qmsh");
		AddJunction("dungeon2.qmsh");
		AddJunction("dungeon3.qmsh");
	}

	XDungeon* GenerateDungeonStart()
	{
		if(!portalDoor)
			LoadMeshes();

		XDungeon* dungeon = new XDungeon;

		XRoom* room = new XRoom;
		room->pos = VEC3(0, 0, 0);
		room->tile_size = INT2(5, 5);
		room->size = VEC3(10.f, 3.f, 10.f);
		room->type = ROOM;
		room->rot = 0.f;
		room->holes = 0;
		dungeon->rooms.push_back(room);

		if(start_rot == RANDOM)
		{
			portals.push_back(XPortal(room, VEC3(room->pos.x-room->size.x/2, 0, room->pos.z), PI, LEFT));
			portals.push_back(XPortal(room, VEC3(room->pos.x+room->size.x/2, 0, room->pos.z), 0, RIGHT));
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+room->size.z/2), PI*3/2, TOP));
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-room->size.z/2), PI/2, BOTTOM));
		}
		else if(start_rot == LEFT)
			portals.push_back(XPortal(room, VEC3(room->pos.x-room->size.x/2, 0, room->pos.z), PI, LEFT));
		else if(start_rot == RIGHT)
			portals.push_back(XPortal(room, VEC3(room->pos.x+room->size.x/2, 0, room->pos.z), 0, RIGHT));
		else if(start_rot == TOP)
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+room->size.z/2), PI*3/2, TOP));
		else
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-room->size.z/2), PI/2, BOTTOM));

		CreatePhy(room, nullptr);
				
		return dungeon;
	}

	void GenerateDungeonStep(XDungeon* dungeon)
	{
		if(portals.empty())
			return;

		int index = rand2()%portals.size();
		XPortal portal = portals[index];
		RemoveElementIndex(portals, index);

		XRoom* r = new XRoom;
		Type new_type;
		int c = rand2()%100;
		switch(portal.room->type)
		{
		default:
		case CORRIDOR:
			if(c < corridor_to_corridor)
				new_type = CORRIDOR;
			else if(c < corridor_to_corridor + corridor_to_junction)
				new_type = JUNCTION;
			else
				new_type = ROOM;
			break;
		case ROOM:
			if(c < room_to_room)
				new_type = ROOM;
			else if(c < room_to_room + room_to_junction)
				new_type = JUNCTION;
			else
				new_type = CORRIDOR;
			break;
		case JUNCTION:
			if(c < junction_to_junction)
				new_type = JUNCTION;
			else if(c < junction_to_junction + junction_to_room)
				new_type = ROOM;
			else
				new_type = CORRIDOR;
			break;
		}

		switch(new_type)
		{
		case ROOM:
			{
				ROT_HINT side;
				if(forced_room_side == RANDOM)
					side = (ROT_HINT)(rand2() % 4);
				else
					side = forced_room_side;
				INT2 size = INT2(random(room_size_w), random(room_size_h));
				if(size.x % 2 == 0)
					++size.x;
				if(size.y % 2 == 0)
					++size.y;

				const float exit_rot = portal.rot;
				const float entrance_rot = rot_info[side].rot;
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = clip(shortestArc(entrance_rot, exit_normal));

				// position room
				r->tile_size = size;
				r->size = VEC3(2.f*r->tile_size.x, 3.f, 2.f*r->tile_size.y);
				r->rot = dest_rot;
				r->holes = (1 << side);
				VEC3 entrance_def_pos(rot_info[side].pos.x * r->size.x/2, 0, rot_info[side].pos.y * r->size.z/2);
				VEC3 entrance_pos;
				MATRIX m;
				D3DXMatrixRotationY(&m, r->rot);
				D3DXVec3TransformCoord(&entrance_pos, &entrance_def_pos, &m);
				r->pos = portal.pos - entrance_pos;
				if(!CreatePhy(r, portal.room->cobj))
				{
					delete r;
					return;
				}

				// set portals
				for(int s = 0; s<4; ++s)
				{
					if(s == side)
						continue;
					auto& info = rot_info[s];
					VEC3 p = VEC3(info.pos.x * r->size.x/2, 0, info.pos.y * r->size.z/2);
					D3DXVec3TransformCoord(&p, &p, &m);
					new_portals.push_back(XPortal(r, r->pos + p, clip(r->rot + info.rot), s));
				}

				// add room
				r->type = ROOM;
				r->connected.push_back(portal.room);
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);
			}
			break;
		case JUNCTION:
			{
				int junction_index = rand2() % junctions.size();
				MeshData* j = junctions[junction_index]->md;
				int sides = j->attach_points.size();
				int side;
				if(forced_junction_side == -1)
					side = rand2() % sides;
				else
					side = forced_junction_side;
				const float exit_rot = portal.rot;
				VEC3 entrance_pos = j->attach_points[side].GetPos();
				float entrance_rot = j->attach_points[side].rot.y;
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = shortestArc(entrance_rot, exit_normal);

				// position room
				r->holes = (1 << side);
				r->rot = dest_rot;
				VEC3 size = j->head.bbox.Size();
				r->size = VEC3(size.x, size.y, size.z);
				VEC3 rotated_entrance_pos;
				MATRIX m;
				D3DXMatrixRotationY(&m, r->rot);
				D3DXVec3TransformCoord(&rotated_entrance_pos, &entrance_pos, &m);
				r->pos = portal.pos - rotated_entrance_pos;
				if(!CreatePhy(r, portal.room->cobj, j->head.bbox.v1.y))
				{
					delete r;
					return;
				}

				// set portals
				for(uint i = 0; i<j->attach_points.size(); ++i)
				{
					if(i == side)
						continue;
					VEC3 portal_pos = j->attach_points[i].GetPos();
					VEC3 rotated_portal_pos;
					D3DXVec3Transform(&rotated_portal_pos, &portal_pos, &m);
					float portal_rot = j->attach_points[i].rot.y;
					new_portals.push_back(XPortal(r, r->pos + rotated_portal_pos, clip(portal_rot + dest_rot), i));
				}

				// add junction
				r->type = JUNCTION;
				r->connected.push_back(portal.room);
				r->junction_index = junction_index;
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);
			}
			break;
		case CORRIDOR:
			{
				// position room
				int len = random(corridor_length);
				r->tile_size = INT2(len, 1);
				r->size = VEC3(2.f*len, 3.f, 2.f);
				r->rot = portal.rot;
				r->holes = (1 << LEFT);
				MATRIX rr;
				D3DXMatrixRotationY(&rr, r->rot);
				VEC3 dir = VEC3(cos(r->rot)*r->size.x/2, 0, -sin(r->rot)*r->size.x/2);
				r->pos = portal.pos + dir;
				if(!CreatePhy(r, portal.room->cobj))
				{
					delete r;
					return;
				}

				// set portals
				new_portals.push_back(XPortal(r, r->pos + dir, r->rot, RIGHT));

				// add room
				r->type = CORRIDOR;
				r->connected.push_back(portal.room);
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);
			}
			break;
		}

		door.push_back(XDoor(portal.pos, portal.rot));
		// mark source room hole
		portal.room->holes |= (1 << portal.index);
	}

	void GenerateDungeon(XDungeon* dungeon, bool repeat)
	{
		if(portals.empty())
			return;

		if(!repeat)
		{
			GenerateDungeonStep(dungeon);
		}
		else
		{
			while(!portals.empty())
			{
				GenerateDungeonStep(dungeon);
			}
		}

		while(!new_portals.empty())
		{
			portals.push_back(new_portals.back());
			new_portals.pop_back();
		}
	}
};

X::XDungeon* dun;
VB vb;
int vcount;
btTriangleMesh* trimesh;
btBvhTriangleMeshShape* phy_shape;
btCollisionObject* phy_obj;

void AddTile(VEC3*& data, const VEC3& spos, const VEC3& epos, MATRIX& m)
{
	VEC3 left_bottom = epos,
		right_bottom = epos + VEC3(1, 0, 0),
		left_top = epos + VEC3(0, 0, 1),
		right_top = epos + VEC3(1, 0, 1);

	D3DXVec3TransformCoord(&left_bottom, &left_bottom, &m);
	D3DXVec3TransformCoord(&right_bottom, &right_bottom, &m);
	D3DXVec3TransformCoord(&left_top, &left_top, &m);
	D3DXVec3TransformCoord(&right_top, &right_top, &m);

	left_bottom += spos;
	right_bottom += spos;
	right_top += spos;
	left_top += spos;

	*data = left_bottom;
	++data;
	*data = left_top;
	++data;
	*data = right_top;
	++data;
	*data = right_top;
	++data;
	*data = right_bottom;
	++data;
	*data = left_bottom;
	++data;
}

void BuildVB()
{
	SafeRelease(vb);

	auto device = Engine::Get().device;

	int vertex_count = 0;
	for(X::XRoom* room : dun->rooms)
	{
		if(room->type == X::JUNCTION)
		{
			X::XJunction* j = X::junctions[room->junction_index];
			vertex_count += j->md->subs[j->floor_index].tris * 3;
		}
		else
		{
			const int bigtile_count = room->tile_size.x * room->tile_size.y;
			const int tile_count = bigtile_count * 4;
			vertex_count += tile_count * 6 + 3;
		}
	}
	
	device->CreateVertexBuffer(sizeof(VEC3)*vertex_count, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &vb, nullptr);
	VEC3* data;
	vb->Lock(0, 0, (void**)&data, 0);

	for(X::XRoom* room : dun->rooms)
	{
		MATRIX m;
		D3DXMatrixRotationY(&m, room->rot);

		if(room->type == X::JUNCTION)
		{
			X::XJunction* j = X::junctions[room->junction_index];
			VDefault* vb = (VDefault*)j->md->vb.data();
			word* ib = j->md->ib.data();
			auto& sub = j->md->subs[j->floor_index];
			VEC3 p;
			for(word idx = 0; idx < sub.tris*3; ++idx)
			{
				p = vb[ib[sub.first*3 + idx]].pos;
				D3DXVec3Transform(&p, &p, &m);
				*data = p + room->pos;
				++data;
			}
		}
		else
		{
			for(int y = 0; y<room->tile_size.y; ++y)
			{
				for(int x = 0; x<room->tile_size.x; ++x)
				{
					VEC3 tile_offset(float(-room->tile_size.x + x*2), 0, float(-room->tile_size.y + y*2));
					AddTile(data, room->pos, tile_offset + VEC3(1, 0, 1), m);
					AddTile(data, room->pos, tile_offset + VEC3(1, 0, 0), m);
					AddTile(data, room->pos, tile_offset + VEC3(0, 0, 1), m);
					AddTile(data, room->pos, tile_offset, m);
				}
			}

			// add top right sign inside room
			VEC3 tile_offset2(float(room->tile_size.x), 0, float(room->tile_size.y));
			VEC3 a = tile_offset2,
				b = tile_offset2 + VEC3(-1, 0, 0),
				c = tile_offset2 + VEC3(0, 1, 0);

			D3DXVec3TransformCoord(&a, &a, &m);
			D3DXVec3TransformCoord(&b, &b, &m);
			D3DXVec3TransformCoord(&c, &c, &m);

			a += room->pos;
			b += room->pos;
			c += room->pos;

			*data = a;
			++data;
			*data = b;
			++data;
			*data = c;
			++data;
		}
	}

	vb->Unlock();
	vcount = vertex_count;
}

void BuildPhysics()
{
	if(phy_obj)
	{
		Game::Get().phy_world->removeCollisionObject(phy_obj);
		delete phy_obj;
		phy_obj = nullptr;
	}
	delete phy_shape;
	phy_shape = nullptr;
	delete trimesh;
	trimesh = nullptr;

	trimesh = new btTriangleMesh(false, false);

	for(X::XRoom* room : dun->rooms)
	{
		MATRIX m, m2, m3, m4, m5;
		D3DXMatrixRotationY(&m, room->rot);

		if(room->type == X::JUNCTION)
		{
			X::XJunction* j = X::junctions[room->junction_index];
			VDefault* vb = (VDefault*)j->md->vb.data();
			word* ib = j->md->ib.data();
			VEC3 p;
			btVector3 v[4];
			VEC3 offset = room->pos;

			for(word tri = 0; tri<j->md->head.n_tris; ++tri)
			{
				for(int i = 0; i< 3; ++i)
				{
					p = vb[ib[tri*3+i]].pos;
					D3DXVec3Transform(&p, &p, &m);
					p += offset;
					v[i] = ToVector3(p);
				}
				trimesh->addTriangle(v[0], v[1], v[2]);
			}

			for(uint i = 0; i<j->md->attach_points.size(); ++i)
			{
				if(!IS_SET(room->holes, 1<<i))
				{
					static const VEC3 base_pts[4] = {
						VEC3(0, 0, -1),
						VEC3(0, 0, 1),
						VEC3(0, 3, -1),
						VEC3(0, 3, 1)
					};

					D3DXMatrixRotationY(&m2, j->md->attach_points[i].rot.y);
					D3DXMatrixTranslation(&m3, j->md->attach_points[i].GetPos());
					D3DXMatrixTranslation(&m4, room->pos);
					m5 = m2 * m3 * m * m4;

					for(int k = 0; k<4; ++k)
					{
						D3DXVec3Transform(&p, &base_pts[k], &m5);
						v[k] = ToVector3(p);
					}

					trimesh->addTriangle(v[0], v[1], v[2]);
					trimesh->addTriangle(v[1], v[2], v[3]);
				}
			}
		}
		else
		{
			const float height_corridor = 3.f;
			const float height_room = 4.f;
			const float height = (room->type == X::CORRIDOR ? height_corridor : height_room);

			// room floor
			VEC3 pts[4] = {
				VEC3(-room->size.x/2, 0, -room->size.z/2),
				VEC3(room->size.x/2, 0, -room->size.z/2),
				VEC3(-room->size.x/2, 0, room->size.z/2),
				VEC3(room->size.x/2, 0, room->size.z/2)
			};

			for(int i = 0; i<4; ++i)
			{
				D3DXVec3Transform(&pts[i], &pts[i], &m);
				pts[i] += room->pos;
			}

			trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
			trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));

			// room ceil
			for(int i = 0; i<4; ++i)
				pts[i].y += height;

			trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
			trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));

			// room walls
			INT2 tiles[4];
			room->GetHoleTiles(tiles);
			// bottom
			int ignore = -1;
			for(int i = 0; i<4; ++i)
			{
				if(tiles[i].y == 0)
				{
					ignore = tiles[i].x;
					break;
				}
			}
			for(int x = 0; x<room->tile_size.x; ++x)
			{
				if(x != ignore)
				{
					pts[0] = VEC3(-room->size.x/2 + x*2, 0, -room->size.z/2);
					pts[1] = VEC3(-room->size.x/2 + (x+1)*2, 0, -room->size.z/2);
					pts[2] = VEC3(-room->size.x/2 + x*2, height, -room->size.z/2);
					pts[3] = VEC3(-room->size.x/2 + (x+1)*2, height, -room->size.z/2);
				}
				else
				{
					pts[0] = VEC3(-room->size.x/2 + x*2, height_corridor, -room->size.z/2);
					pts[1] = VEC3(-room->size.x/2 + (x+1)*2, height_corridor, -room->size.z/2);
					pts[2] = VEC3(-room->size.x/2 + x*2, height_room, -room->size.z/2);
					pts[3] = VEC3(-room->size.x/2 + (x+1)*2, height_room, -room->size.z/2);
				}

				for(int i = 0; i<4; ++i)
				{
					D3DXVec3Transform(&pts[i], &pts[i], &m);
					pts[i] += room->pos;
				}

				trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
				trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));
			}
			// top
			ignore = -1;
			for(int i = 0; i<4; ++i)
			{
				if(tiles[i].y == room->tile_size.y-1)
				{
					ignore = tiles[i].x;
					break;
				}
			}
			for(int x = 0; x<room->tile_size.x; ++x)
			{
				if(x != ignore)
				{
					pts[0] = VEC3(-room->size.x/2 + x*2, 0, room->size.z/2);
					pts[1] = VEC3(-room->size.x/2 + (x+1)*2, 0, room->size.z/2);
					pts[2] = VEC3(-room->size.x/2 + x*2, height, room->size.z/2);
					pts[3] = VEC3(-room->size.x/2 + (x+1)*2, height, room->size.z/2);
				}
				else
				{
					pts[0] = VEC3(-room->size.x/2 + x*2, height_corridor, room->size.z/2);
					pts[1] = VEC3(-room->size.x/2 + (x+1)*2, height_corridor, room->size.z/2);
					pts[2] = VEC3(-room->size.x/2 + x*2, height_room, room->size.z/2);
					pts[3] = VEC3(-room->size.x/2 + (x+1)*2, height_room, room->size.z/2);
				}

				for(int i = 0; i<4; ++i)
				{
					D3DXVec3Transform(&pts[i], &pts[i], &m);
					pts[i] += room->pos;
				}

				trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
				trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));
			}
			// left
			ignore = -1;
			for(int i = 0; i<4; ++i)
			{
				if(tiles[i].x == 0)
				{
					ignore = tiles[i].y;
					break;
				}
			}
			for(int z = 0; z<room->tile_size.y; ++z)
			{
				if(z != ignore)
				{
					pts[0] = VEC3(-room->size.x/2, 0, -room->size.z/2 + z*2);
					pts[1] = VEC3(-room->size.x/2, 0, -room->size.z/2 + (z+1)*2);
					pts[2] = VEC3(-room->size.x/2, height, -room->size.z/2 + z*2);
					pts[3] = VEC3(-room->size.x/2, height, -room->size.z/2 + (z+1)*2);
				}
				else
				{
					pts[0] = VEC3(-room->size.x/2, height_corridor, -room->size.z/2 + z*2);
					pts[1] = VEC3(-room->size.x/2, height_corridor, -room->size.z/2 + (z+1)*2);
					pts[2] = VEC3(-room->size.x/2, height_room, -room->size.z/2 + z*2);
					pts[3] = VEC3(-room->size.x/2, height_room, -room->size.z/2 + (z+1)*2);
				}

				for(int i = 0; i<4; ++i)
				{
					D3DXVec3Transform(&pts[i], &pts[i], &m);
					pts[i] += room->pos;
				}

				trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
				trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));
			}
			// right
			ignore = -1;
			for(int i = 0; i<4; ++i)
			{
				if(tiles[i].x == room->tile_size.x-1)
				{
					ignore = tiles[i].y;
					break;
				}
			}
			for(int z = 0; z<room->tile_size.y; ++z)
			{
				if(z != ignore)
				{
					pts[0] = VEC3(room->size.x/2, 0, -room->size.z/2 + z*2);
					pts[1] = VEC3(room->size.x/2, 0, -room->size.z/2 + (z+1)*2);
					pts[2] = VEC3(room->size.x/2, height, -room->size.z/2 + z*2);
					pts[3] = VEC3(room->size.x/2, height, -room->size.z/2 + (z+1)*2);
				}
				else
				{
					pts[0] = VEC3(room->size.x/2, height_corridor, -room->size.z/2 + z*2);
					pts[1] = VEC3(room->size.x/2, height_corridor, -room->size.z/2 + (z+1)*2);
					pts[2] = VEC3(room->size.x/2, height_room, -room->size.z/2 + z*2);
					pts[3] = VEC3(room->size.x/2, height_room, -room->size.z/2 + (z+1)*2);
				}

				for(int i = 0; i<4; ++i)
				{
					D3DXVec3Transform(&pts[i], &pts[i], &m);
					pts[i] += room->pos;
				}

				trimesh->addTriangle(ToVector3(pts[0]), ToVector3(pts[1]), ToVector3(pts[2]));
				trimesh->addTriangle(ToVector3(pts[1]), ToVector3(pts[2]), ToVector3(pts[3]));
			}
		}
	}

	phy_shape = new btBvhTriangleMeshShape(trimesh, true);
	phy_shape->buildOptimizedBvh();

	phy_obj = new btCollisionObject;
	phy_obj->setCollisionShape(phy_shape);
	auto world = Game::Get().phy_world;
	world->addCollisionObject(phy_obj, COL_DUNGEON);
}

bool draw_dungeon, draw_room, draw_invalid_room, draw_portal, draw_door, draw_phy, draw_dungeon_wire, draw_phy_wire;

void X_Init()
{
	draw_dungeon = true;
	draw_room = true;
	draw_invalid_room = true;
	draw_portal = true;
	draw_door = true;
	draw_phy = true;
	draw_dungeon_wire = true;
	draw_phy_wire = true;
}

void X_InitDungeon()
{
	X::LoadConfig();
	dun = X::GenerateDungeonStart();
	BuildVB();
	BuildPhysics();
}

void X_Cleanup(bool ctor)
{
	delete dun;
	X::portals.clear();
	X::door.clear();
	auto world = Game::Get().phy_world;
	if(!ctor)
	{
		for(auto obj : X::objs)
		{
			world->removeCollisionObject(obj);
			delete obj;
		}
		if(phy_obj)
		{
			world->removeCollisionObject(phy_obj);
			delete phy_obj;
			phy_obj = nullptr;
		}
	}
	X::objs.clear();
	DeleteElements(X::shapes);
	DeleteElements(X::invalid_objs);
	if(ctor)
		DeleteElements(X::junctions);
	delete phy_shape;
	phy_shape = nullptr;
	delete trimesh;
	trimesh = nullptr;
}

void X_DungeonReset()
{
	X_Cleanup(false);
	X_InitDungeon();
}

void X_DungeonUpdate(float dt)
{
	if(Key.Pressed('G'))
	{
		X::GenerateDungeon(dun, Key.Down(VK_SHIFT));
		BuildVB();
		BuildPhysics();
	}
	if(Key.Pressed('R'))
	{
		if(!Key.Down(VK_SHIFT))
			X_DungeonReset();
		else
		{
			Unit& u = *Game::Get().pc->unit;
			u.pos = VEC3(0, 0, 0);
			u.visual_pos = VEC3(0, 0, 0);
			Game::Get().UpdateUnitPhysics(u, u.pos);
		}
	}
	if(Key.Pressed('3'))
	{
		if(!Key.Down(VK_SHIFT))
			draw_dungeon = !draw_dungeon;
		else
			draw_dungeon_wire = !draw_dungeon_wire;
	}
	if(Key.Pressed('4'))
	{
		if(!Key.Down(VK_SHIFT))
			draw_phy = !draw_phy;
		else
			draw_phy_wire = !draw_phy_wire;
	}
	if(Key.Pressed('5'))
		draw_room = !draw_room;
	if(Key.Pressed('6'))
		draw_invalid_room = !draw_invalid_room;
	if(Key.Pressed('7'))
		draw_portal = !draw_portal;
	if(Key.Pressed('8'))
		draw_door = !draw_door;
}

bool wireframe;

void SetWireframe(bool w)
{
	if(w != wireframe)
	{
		wireframe = w;
		Game::Get().device->SetRenderState(D3DRS_FILLMODE, w ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
	}
}


void X_DrawDungeon()
{
	Game& game = Game::Get();
	auto device = game.device;

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	V(device->SetVertexDeclaration(game.vertex_decl[VDI_POS]));
	uint passes;
	MATRIX m, m2;
	Animesh* sphere = X::portalDoor;
	Animesh* box = game.aBox;

	// rooms
	if(draw_dungeon)
	{
		SetWireframe(draw_dungeon_wire);
		V(game.eArea->SetTechnique(game.techArea));
		V(game.eArea->SetMatrix(game.hAreaCombined, &game.cam.matViewProj));
		V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 0, 0, 1)));
		VEC4 playerPos(game.pc->unit->pos, 1.f);
		playerPos.y += 0.75f;
		V(game.eArea->SetVector(game.hAreaPlayerPos, &playerPos));
		V(game.eArea->SetFloat(game.hAreaRange, 200.f));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		device->SetStreamSource(0, vb, 0, sizeof(VEC3));
		device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, vcount/3);
		V(game.eArea->EndPass());
		V(game.eArea->End());
	}

	SetWireframe(true);

	// doors
	if(draw_door)
	{
		V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 1, 0, 1)));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		V(device->SetStreamSource(0, sphere->vb, 0, sphere->vertex_size));
		V(device->SetIndices(sphere->ib));
		V(device->SetVertexDeclaration(game.vertex_decl[sphere->vertex_decl]));
		for(X::XDoor& d : X::door)
		{
			D3DXMatrixTranslation(&m, d.pos);
			D3DXMatrixRotationY(&m2, d.rot);
			V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m * game.cam.matViewProj)));
			game.eArea->CommitChanges();
			for(int i = 0; i<sphere->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sphere->subs[i].min_ind, sphere->subs[i].n_ind, sphere->subs[i].first*3, sphere->subs[i].tris));
		}
		V(game.eArea->EndPass());
		V(game.eArea->End());
	}

	// portals
	if(draw_portal)
	{
		V(game.eArea->SetVector(game.hAreaColor, &VEC4(1, 1, 0, 1)));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		V(device->SetStreamSource(0, sphere->vb, 0, sphere->vertex_size));
		V(device->SetIndices(sphere->ib));
		for(auto& portal : X::portals)
		{
			D3DXMatrixTranslation(&m, portal.pos);
			D3DXMatrixRotationY(&m2, portal.rot);
			V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m * game.cam.matViewProj)));
			game.eArea->CommitChanges();
			for(int i = 0; i<sphere->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sphere->subs[i].min_ind, sphere->subs[i].n_ind, sphere->subs[i].first*3, sphere->subs[i].tris));
		}

		
		D3DXMatrixTranslation(&m, testHitpoint);
		V(game.eArea->SetMatrix(game.hAreaCombined, &(m * game.cam.matViewProj)));
		game.eArea->CommitChanges();
		for(int i = 0; i<sphere->head.n_subs; ++i)
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sphere->subs[i].min_ind, sphere->subs[i].n_ind, sphere->subs[i].first*3, sphere->subs[i].tris));
		

		V(game.eArea->EndPass());
		V(game.eArea->End());
	}

	// room phy
	if(draw_room)
	{
		V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 1, 1, 1)));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		V(device->SetStreamSource(0, box->vb, 0, box->vertex_size));
		V(device->SetIndices(box->ib));
		for(auto& obj : X::objs)
		{
			btBoxShape* shape = (btBoxShape*)obj->getCollisionShape();
			obj->getWorldTransform().getOpenGLMatrix(&m._11);
			D3DXMatrixScaling(&m2, ToVEC3(shape->getHalfExtentsWithMargin()));
			V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m * game.cam.matViewProj)));
			game.eArea->CommitChanges();
			for(int i = 0; i<box->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, box->subs[i].min_ind, box->subs[i].n_ind, box->subs[i].first*3, box->subs[i].tris));
		}
		V(game.eArea->EndPass());
		V(game.eArea->End());
	}

	// invalid room phy
	if(draw_invalid_room)
	{
		V(game.eArea->SetVector(game.hAreaColor, &VEC4(1, 0, 0, 1)));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		V(device->SetStreamSource(0, box->vb, 0, box->vertex_size));
		V(device->SetIndices(box->ib));
		for(auto& obj : X::invalid_objs)
		{
			btBoxShape* shape = (btBoxShape*)obj->getCollisionShape();
			obj->getWorldTransform().getOpenGLMatrix(&m._11);
			D3DXMatrixScaling(&m2, ToVEC3(shape->getHalfExtentsWithMargin()));
			V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m * game.cam.matViewProj)));
			game.eArea->CommitChanges();
			for(int i = 0; i<box->head.n_subs; ++i)
				V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, box->subs[i].min_ind, box->subs[i].n_ind, box->subs[i].first*3, box->subs[i].tris));
		}
		V(game.eArea->EndPass());
		V(game.eArea->End());
	}

	// draw bullet physics
	if(draw_phy && trimesh)
	{
		SetWireframe(draw_phy_wire);

		const byte* vertexbase;
		int numverts;
		PHY_ScalarType type; // float
		int vertexstride; // 12
		const byte* indexbase;
		int indexstride; // 6
		int numfaces;
		PHY_ScalarType indicestype; // short
		trimesh->getLockedReadOnlyVertexIndexBase(&vertexbase, numverts, type, vertexstride, &indexbase, indexstride, numfaces, indicestype);

		assert(type == PHY_FLOAT && vertexstride == 12 && indexstride == 6 && indicestype == PHY_SHORT);

		V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 1, 1, 1)));
		V(game.eArea->SetMatrix(game.hAreaCombined, &game.cam.matViewProj));
		V(game.eArea->Begin(&passes, 0));
		V(game.eArea->BeginPass(0));
		V(device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, numverts, numfaces, indexbase, D3DFMT_INDEX16, vertexbase, vertexstride));
		V(game.eArea->EndPass());
		V(game.eArea->End());

		trimesh->unLockReadOnlyVertexBase(0);
	}

	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	SetWireframe(false);
}

struct TestCallback : btCollisionWorld::ContactResultCallback
{
	bool hit;

	TestCallback() : hit(false)
	{

	}

	bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		// only collide with dungeon
		return (proxy0->m_collisionFilterGroup == COL_DUNGEON);
	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		hit = true;
		return 1.f;
	}
};

// max step depends on distance? (when runing fast you on stairs you will have higher step between each step, we will see :P)
static const float MAX_STEP = 0.3f;
static const float SAFE_STEP = 0.01f;

struct ColCallback : public btCollisionWorld::ConvexResultCallback
{
	btVector3 normal;

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		// only collide with dungeon
		return (proxy0->m_collisionFilterGroup == COL_DUNGEON);
	}

	virtual	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		assert(normalInWorldSpace);
		if(m_closestHitFraction > convexResult.m_hitFraction)
		{
			m_closestHitFraction = convexResult.m_hitFraction;
			normal = convexResult.m_hitNormalLocal;
		}
		return 1.f;
	}

	void Reset()
	{
		m_closestHitFraction = 1.f;
	}
};

class XCapsule : public btCapsuleShape
{
public:
	float decrase_margin;

	void Setup(btCapsuleShape* shape, float _decrase_margin)
	{
		decrase_margin = _decrase_margin;

		m_collisionMargin = shape->getMargin();
		m_implicitShapeDimensions = shape->getImplicitShapeDimensions();
		m_localScaling = shape->getLocalScaling();
		m_shapeType = shape->getShapeType();
		m_upAxis = shape->getUpAxis();
	}

	virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
	{
		btVector3 halfExtents(getRadius() - decrase_margin, getRadius(), getRadius() - decrase_margin);
		halfExtents[m_upAxis] = getRadius() + getHalfHeight();
		halfExtents += btVector3(getMargin(), getMargin(), getMargin());
		btMatrix3x3 abs_b = t.getBasis().absolute();
		btVector3 center = t.getOrigin();
		btVector3 extent = halfExtents.dot3(abs_b[0], abs_b[1], abs_b[2]);

		aabbMin = center - extent;
		aabbMax = center + extent;
	}
};

XCapsule xcapsule;

bool X_MoveUnit(Unit& unit, const VEC3& dir, float dt)
{
	Game& game = Game::Get();
	btCollisionWorld* world = Game::Get().phy_world;
	btCapsuleShape* shape = (btCapsuleShape*)unit.cobj->getCollisionShape();

	// move along dir
	btTransform from, to;
	from = unit.cobj->getWorldTransform();
	from.setOrigin(from.getOrigin() + btVector3(0, MAX_STEP, 0));
	to = from;
	to.setOrigin(to.getOrigin() + ToVector3(dir));

	ColCallback clbk;
	world->convexSweepTest(shape, from, to, clbk);
	btVector3 target_loc;

	if(!clbk.hasHit())
	{
		// no collision, move to target location
		target_loc = to.getOrigin();
	}
	else
	{
		// place near wall
		btVector3 col_pt = from.getOrigin() + ToVector3(dir) * clbk.m_closestHitFraction;
		btVector3 slide_pt = col_pt + clbk.normal * SAFE_STEP;

		clbk.Reset();
		to.setOrigin(slide_pt);
		world->convexSweepTest(shape, from, to, clbk);
		if(!clbk.hasHit())
		{
			// no collision
			target_loc = to.getOrigin();
		}
		else
		{
			// slide hit, don't move at all
			return false;
		}
	}

	// fall down
	const float fall_dist = MAX_STEP * 20;
	to.setOrigin(from.getOrigin() + btVector3(0, -fall_dist, 0));
	clbk.Reset();
	xcapsule.Setup(shape, 0.05f);
	world->convexSweepTest(&xcapsule, from, to, clbk);

	if(!clbk.hasHit())
	{
		// no floor! falling not implemented, cancel movment
		return false;
	}

	// hit the floor
	float y = from.getOrigin().y() - clbk.m_closestHitFraction * fall_dist;
	float h2 = (unit.GetUnitHeight() + MAX_STEP) / 2;
	y -= h2;
	unit.pos.x = target_loc.x();
	unit.pos.y = y;
	unit.pos.z = target_loc.z();
	unit.visual_pos = unit.pos;
	Game::Get().UpdateUnitPhysics(unit, unit.pos);
	return true;
}

struct X_CleanupEr
{
	X_CleanupEr() { X_Init(); }
	~X_CleanupEr() { X_Cleanup(true); }
};

X_CleanupEr xce;
