#include "Pch.h"
#include "Base.h"
#include "Engine.h"
#include "Game.h"

/*
human walking
*/

const int COL_FLAG = 1<<10;

enum ROT_HINT
{
	BOTTOM,
	LEFT,
	TOP,
	RIGHT,
	RANDOM
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
	};

	struct XPortal
	{
		XRoom* room;
		VEC3 pos;
		float rot;

		XPortal(XRoom* room, const VEC3& pos, float rot) : room(room), pos(pos), rot(rot) {}
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

	struct Junction
	{
		Animesh* mesh;
		vector<VEC3> vb;
		vector<short> ib;
	};

	Animesh* portalDoor;
	vector<Junction*> junctions;
	vector<XPortal> portals, new_portals;
	vector<XDoor> door;
	vector<btCollisionObject*> objs, invalid_objs;
	vector<btBoxShape*> shapes;
	ROT_HINT start_rot, forced_room_side;
	bool check_phy;
	int corridor_to_junction, corridor_to_room, corridor_to_corridor, room_to_corridor, room_to_junction, room_to_room, junction_to_corridor,
		junction_to_room, junction_to_junction, forced_junction_side;

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
		obj->getWorldTransform().setOrigin(btVector3(r->pos.x, r->pos.y + r->size.y/2 + 0.01f + y, r->pos.z));
		obj->getWorldTransform().setRotation(btQuaternion(r->rot, 0, 0));
		btBoxShape* shape = new btBoxShape(btVector3(r->size.x/2 - 0.05f, r->size.y/2, r->size.z/2 - 0.05f));
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
	
	void AddJunction(Animesh* mesh)
	{
		Junction* j = new Junction;
		j->mesh = mesh;

		j->vb.resize(mesh->head.n_verts);
		assert(mesh->vertex_decl == VDI_DEFAULT);
		Vertex* v;
		mesh->vb->Lock(0, 0, (void**)&v, 0);
		for(word i = 0; i<mesh->head.n_verts; ++i)
		{
			j->vb[i] = v->pos;
			++v;
		}
		mesh->vb->Unlock();

		j->ib.resize(mesh->head.n_tris*3);
		word* ii;
		mesh->ib->Lock(0, 0, (void**)&ii, 0);
		memcpy(j->ib.data(), ii, sizeof(word)*mesh->head.n_tris*3);
		mesh->ib->Unlock();

		junctions.push_back(j);
	}

	void LoadMeshes()
	{
		ResourceManager& resMgr = ResourceManager::Get();
		
		resMgr.GetLoadedMesh("portal_door.qmsh", portalDoor);
		AddJunction(resMgr.GetLoadedMesh("dungeon1.qmsh")->data);
		AddJunction(resMgr.GetLoadedMesh("dungeon2.qmsh")->data);
		AddJunction(resMgr.GetLoadedMesh("dungeon3.qmsh")->data);
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
		dungeon->rooms.push_back(room);

		if(start_rot == RANDOM)
		{
			portals.push_back(XPortal(room, VEC3(room->pos.x-room->size.x/2, 0, room->pos.z), PI));
			portals.push_back(XPortal(room, VEC3(room->pos.x+room->size.x/2, 0, room->pos.z), 0));
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+room->size.z/2), PI*3/2));
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-room->size.z/2), PI/2));
		}
		else if(start_rot == LEFT)
			portals.push_back(XPortal(room, VEC3(room->pos.x-room->size.x/2, 0, room->pos.z), PI));
		else if(start_rot == RIGHT)
			portals.push_back(XPortal(room, VEC3(room->pos.x+room->size.x/2, 0, room->pos.z), 0));
		else if(start_rot == TOP)
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+room->size.z/2), PI*3/2));
		else
			portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-room->size.z/2), PI/2));

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
				int size = random(3, 7);
				if(size % 2 == 0)
					++size;

				const float exit_rot = portal.rot;
				float entrance_rot;
				switch(side)
				{
				case TOP:
					entrance_rot = PI*3/2;
					break;
				case RIGHT:
					entrance_rot = 0;
					break;
				case BOTTOM:
					entrance_rot = PI/2;
					break;
				case LEFT:
					entrance_rot = PI;
					break;
				}
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = clip(shortestArc(entrance_rot, exit_normal));

				// position room
				r->tile_size = INT2(size);
				r->size = VEC3(2.f*size, 3.f, 2.f*size);
				r->rot = dest_rot;
				r->pos = portal.pos + VEC3(cos(portal.rot)*r->size.x/2, 0, -sin(portal.rot)*r->size.z/2);
				if(!CreatePhy(r, portal.room->cobj))
				{
					delete r;
					return;
				}

				// set portals
				MATRIX m;
				D3DXMatrixRotationY(&m, r->rot);
				VEC3 p;

				if(side != LEFT)
				{
					p = VEC3(-r->size.x/2, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					new_portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI)));
				}

				if(side != RIGHT)
				{
					p = VEC3(r->size.x/2, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					new_portals.push_back(XPortal(r, r->pos + p, r->rot));
				}

				if(side != BOTTOM)
				{
					p = VEC3(0, 0, -r->size.z/2);
					D3DXVec3TransformCoord(&p, &p, &m);
					new_portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI/2)));
				}

				if(side != TOP)
				{
					p = VEC3(0, 0, r->size.z/2);
					D3DXVec3TransformCoord(&p, &p, &m);
					new_portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI*3/2)));
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
				Junction* j = junctions[junction_index];
				int sides = j->mesh->attach_points.size();
				int side;
				if(forced_junction_side == -1)
					side = rand2() % sides;
				else
					side = forced_junction_side;
				const float exit_rot = portal.rot;
				VEC3 entrance_pos = j->mesh->attach_points[side].GetPos();
				float entrance_rot = j->mesh->attach_points[side].rot.y;
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = shortestArc(entrance_rot, exit_normal);

				// position room
				r->rot = dest_rot;
				VEC3 size = j->mesh->head.bbox.Size();
				r->size = VEC3(size.x, size.y+3.f, size.z);
				VEC3 rotated_entrance_pos;
				MATRIX m;
				D3DXMatrixRotationY(&m, r->rot);
				D3DXVec3TransformCoord(&rotated_entrance_pos, &entrance_pos, &m);
				r->pos = portal.pos - rotated_entrance_pos;
				if(!CreatePhy(r, portal.room->cobj, j->mesh->head.bbox.v1.y))
				{
					delete r;
					return;
				}

				// set portals
				for(uint i = 0; i<j->mesh->attach_points.size(); ++i)
				{
					if(i == side)
						continue;
					VEC3 portal_pos = j->mesh->attach_points[i].GetPos();
					VEC3 rotated_portal_pos;
					D3DXVec3Transform(&rotated_portal_pos, &portal_pos, &m);
					float portal_rot = j->mesh->attach_points[i].rot.y;
					new_portals.push_back(XPortal(r, r->pos + rotated_portal_pos, clip(portal_rot + dest_rot)));
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
				int len = random(3, 8);
				r->tile_size = INT2(len, 1);
				r->size = VEC3(2.f*len, 3.f, 2.f);
				r->rot = portal.rot;
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
				new_portals.push_back(XPortal(r, r->pos + dir, r->rot));

				// add room
				r->type = CORRIDOR;
				r->connected.push_back(portal.room);
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);
			}
			break;
		}

		door.push_back(XDoor(portal.pos, portal.rot));
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
			vertex_count += X::junctions[room->junction_index]->ib.size();
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
			X::Junction* j = X::junctions[room->junction_index];
			VEC3 p;
			for(short idx : j->ib)
			{
				p = j->vb[idx];
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

bool draw_room, draw_invalid_room, draw_portal, draw_door;

void X_Init()
{
	draw_room = true;
	draw_invalid_room = true;
	draw_portal = true;
	draw_door = true;
}

void X_InitDungeon()
{
	X::LoadConfig();
	dun = X::GenerateDungeonStart();
	BuildVB();
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
	}
	X::objs.clear();
	DeleteElements(X::shapes);
	DeleteElements(X::invalid_objs);
	if(ctor)
		DeleteElements(X::junctions);
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
	}
	if(Key.Pressed('R'))
	{
		X_DungeonReset();
	}
	if(Key.Pressed('3'))
		draw_room = !draw_room;
	if(Key.Pressed('4'))
		draw_invalid_room = !draw_invalid_room;
	if(Key.Pressed('5'))
		draw_portal = !draw_portal;
	if(Key.Pressed('6'))
		draw_door = !draw_door;
}

void X_DrawDungeon()
{
	Game& game = Game::Get();
	auto device = game.device;

	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	V(device->SetVertexDeclaration(game.vertex_decl[VDI_POS]));
	uint passes;
	MATRIX m, m2;
	Animesh* sphere = X::portalDoor;
	Animesh* box = game.aBox;

	// rooms
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

	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

struct X_CleanupEr
{
	X_CleanupEr() { X_Init(); }
	~X_CleanupEr() { X_Cleanup(true); }
};

X_CleanupEr xce;
