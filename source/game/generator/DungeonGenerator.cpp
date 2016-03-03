#include "Pch.h"
#include "Base.h"
#include "Engine.h"
#include "Game.h"

const int COL_FLAG = 1<<10;

enum ROT_HINT
{
	BOTTOM,
	LEFT,
	TOP,
	RIGHT
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

	Animesh* portalDoor, *junction;
	vector<VEC3> junction_v;
	vector<short> junction_i;
	vector<XPortal> portals;
	vector<XDoor> door;
	vector<btCollisionObject*> objs, invalid_objs;
	vector<btBoxShape*> shapes;

	struct Callback : public btCollisionWorld::ContactResultCallback
	{
		bool hit;
		const btCollisionObject* other;

		Callback() : hit(false), other(nullptr) {}

		bool needsCollision(btBroadphaseProxy* proxy0) const
		{
			return proxy0->m_collisionFilterGroup == COL_FLAG && proxy0->m_collisionFilterMask == COL_FLAG;
		}

		btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
		{
			other = colObj1Wrap->getCollisionObject();
			hit = true;
			return 0;
		}
	};

	bool CreatePhy(XRoom* r)
	{
		auto world = Game::Get().phy_world;

		btCollisionObject* obj = new btCollisionObject;
		obj->getWorldTransform().setOrigin(btVector3(r->pos.x, r->pos.y + r->size.y/2 + 0.01f, r->pos.z));
		obj->getWorldTransform().setRotation(btQuaternion(r->rot, 0, 0));
		btBoxShape* shape = new btBoxShape(btVector3(r->size.x/2 - 0.05f, r->size.y/2, r->size.z/2 - 0.05f));
		shapes.push_back(shape);
		obj->setCollisionShape(shape);

		Callback c;
		world->contactTest(obj, c);
		if(c.hit)
		{
			invalid_objs.push_back(obj);
			return false;
		}

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

	void LoadMeshes()
	{
		ResourceManager& resMgr = ResourceManager::Get();
		
		resMgr.GetLoadedMesh("portal_door.qmsh", portalDoor);
		resMgr.GetLoadedMesh("dungeon1.qmsh", junction);

		junction_v.resize(junction->head.n_verts);
		assert(junction->vertex_decl == VDI_DEFAULT);
		Vertex* v;
		junction->vb->Lock(0, 0, (void**)&v, 0);
		for(word i = 0; i<junction->head.n_verts; ++i)
		{
			junction_v[i] = v->pos;
			++v;
		}
		junction->vb->Unlock();

		junction_i.resize(junction->head.n_tris*3);
		word* ii;
		junction->ib->Lock(0, 0, (void**)&ii, 0);
		memcpy(junction_i.data(), ii, sizeof(word)*junction->head.n_tris*3);
		junction->ib->Unlock();
	}

	XDungeon* GenerateDungeon()
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

		//portals.push_back(XPortal(room, VEC3(room->pos.x-room->size.x/2, 0, room->pos.z), PI*3/2));
		//portals.push_back(XPortal(room, VEC3(room->pos.x+room->size.x/2, 0, room->pos.z), PI/2));
		//portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-room->size.z/2), PI));
		portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+room->size.z/2), 0));

		CreatePhy(room);
				
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

		if(portal.room->type == CORRIDOR)
		{
			if(rand2() % 2 == 5 /*0 !!!!!!!!!!!!!!!!!!!!!!!!*/)
			{
				// room
				/*ROT_HINT side = (ROT_HINT)(rand2() % 4); // that will be max(portals) for room
				int size = random(3, 7);
				if(size % 2 == 0)
					++size;

				const float exit_rot = portal.rot;
				float entrance_rot;
				switch(side)
				{
				case TOP:
					entrance_rot = 0;
					break;
				case RIGHT:
					entrance_rot = PI/2;
					break;
				case BOTTOM:
					entrance_rot = PI;
					break;
				case LEFT:
					entrance_rot = PI*3/2;
					break;
				}
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = clip(shortestArc(entrance_rot, exit_normal));

				// position room
				r->size = INT2(size);
				r->rot = dest_rot;
				r->pos = portal.pos + VEC3(cos(portal.rot)*r->size.x, 0, sin(portal.rot)*r->size.y);
				if(!CreatePhy(r))
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
					p = VEC3(-1.f*r->size.x, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI*3/2)));
				}

				if(side != RIGHT)
				{
					p = VEC3(+1.f*r->size.x, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI/2)));
				}

				if(side != BOTTOM)
				{
					p = VEC3(0, 0, -1.f*r->size.x);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI)));
				}

				if(side != TOP)
				{
					p = VEC3(0, 0, +1.f*r->size.x);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot)));
				}

				// add room
				r->type = ROOM;
				r->connected.push_back(portal.room);
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);*/
			}
			else
			{
				// junction
				int sides = junction->attach_points.size();
				int side = 0;// rand2() % sides;
				const float exit_rot = portal.rot;
				VEC3 entrance_pos = junction->attach_points[side].GetPos();
				float entrance_rot = angle2d(VEC3(0, 0, 0), entrance_pos);
				const float exit_normal = clip(exit_rot + PI);
				const float dest_rot = clip(shortestArc(entrance_rot, exit_normal));

				// INVALID ROTATION!!!! phy vs mesh
				// position room
				r->rot = dest_rot;
				VEC2 size = junction->head.bbox.SizeXZ();
				r->size = VEC3(size.x, 3.f, size.y);
				r->pos = portal.pos + VEC3(cos(portal.rot)*size.x, 0, sin(portal.rot)*size.y);
				if(!CreatePhy(r))
				{
					delete r;
					return;
				}

				// set portals
				/*MATRIX m;
				D3DXMatrixRotationY(&m, r->rot);
				VEC3 p;
				for(uint i = 0; i<junction->attach_points.size(); ++i)
				{
					if(i == side)
						continue;
					VEC3 portal_pos = junction->attach_points[i].GetPos();
					float entrance_rot = angle2d(VEC3(0, 0, 0), entrance_pos);
				}
				if(side != LEFT)
				{
					p = VEC3(-1.f*r->size.x, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI*3/2)));
				}

				if(side != RIGHT)
				{
					p = VEC3(+1.f*r->size.x, 0, 0);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI/2)));
				}

				if(side != BOTTOM)
				{
					p = VEC3(0, 0, -1.f*r->size.x);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot + PI)));
				}

				if(side != TOP)
				{
					p = VEC3(0, 0, +1.f*r->size.x);
					D3DXVec3TransformCoord(&p, &p, &m);
					portals.push_back(XPortal(r, r->pos + p, clip(r->rot)));
				}*/

				// add junction
				r->type = JUNCTION;
				r->connected.push_back(portal.room);
				portal.room->connected.push_back(r);
				dungeon->rooms.push_back(r);
			}
		}
		else
		{
			// position room
			int len = random(3, 8);
			r->tile_size = INT2(1, len);
			r->size = VEC3(2.f, 3.f, 2.f*len);
			r->rot = portal.rot;
			//r->rot = portal.rot + ToRadians(15) ;//clip(portal.rot + portal.room->rot);
			MATRIX rr;
			D3DXMatrixRotationY(&rr, r->rot);
			VEC3 dir = VEC3(cos(r->rot)*r->size.z/2, 0, sin(r->rot)*r->size.z/2);
			r->pos = portal.pos + dir;
			if(!CreatePhy(r))
			{
				delete r;
				return;
			}

			// set portals
			portals.push_back(XPortal(r, r->pos + dir, r->rot));

			// add room
			r->type = CORRIDOR;
			r->connected.push_back(portal.room);
			portal.room->connected.push_back(r);
			dungeon->rooms.push_back(r);
		}

		door.push_back(XDoor(portal.pos, portal.rot));
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
			vertex_count += X::junction_i.size();
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
			VEC3 p;
			for(short idx : X::junction_i)
			{
				p = X::junction_v[idx];
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

void X_DungeonStep()
{
	X::GenerateDungeonStep(dun);
	BuildVB();
}

void X_InitDungeon()
{
	dun = X::GenerateDungeon();
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
}

void X_DungeonReset()
{
	X_Cleanup(false);
	X_InitDungeon();
}

void X_DrawDungeon()
{
	Game& game = Game::Get();
	auto device = game.device;

	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	V(device->SetVertexDeclaration(game.vertex_decl[VDI_POS]));
	uint passes;

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
	V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 1, 0, 1)));
	V(game.eArea->Begin(&passes, 0));
	V(game.eArea->BeginPass(0));
	Animesh* sphere = X::portalDoor;
	MATRIX mm, mm2;
	V(device->SetStreamSource(0, sphere->vb, 0, sphere->vertex_size));
	V(device->SetIndices(sphere->ib));
	V(device->SetVertexDeclaration(game.vertex_decl[sphere->vertex_decl]));
	for(X::XDoor& d : X::door)
	{
		D3DXMatrixTranslation(&mm, d.pos);
		D3DXMatrixRotationY(&mm2, d.rot);
		V(game.eArea->SetMatrix(game.hAreaCombined, &(mm2 * mm * game.cam.matViewProj)));
		game.eArea->CommitChanges();
		for(int i = 0; i<sphere->head.n_subs; ++i)
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sphere->subs[i].min_ind, sphere->subs[i].n_ind, sphere->subs[i].first*3, sphere->subs[i].tris));
	}
	V(game.eArea->EndPass());
	V(game.eArea->End());

	// portals
	V(game.eArea->SetVector(game.hAreaColor, &VEC4(1, 1, 0, 1)));
	V(game.eArea->Begin(&passes, 0));
	V(game.eArea->BeginPass(0));
	V(device->SetStreamSource(0, sphere->vb, 0, sphere->vertex_size));
	V(device->SetIndices(sphere->ib));
	for(auto& portal : X::portals)
	{
		D3DXMatrixTranslation(&mm, portal.pos);
		D3DXMatrixRotationY(&mm2, portal.rot);
		V(game.eArea->SetMatrix(game.hAreaCombined, &(mm2 * mm * game.cam.matViewProj)));
		game.eArea->CommitChanges();
		for(int i = 0; i<sphere->head.n_subs; ++i)
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sphere->subs[i].min_ind, sphere->subs[i].n_ind, sphere->subs[i].first*3, sphere->subs[i].tris));
	}
	V(game.eArea->EndPass());
	V(game.eArea->End());

	
	// room phy
	V(game.eArea->SetVector(game.hAreaColor, &VEC4(0, 1, 1, 1)));
	V(game.eArea->Begin(&passes, 0));
	V(game.eArea->BeginPass(0));
	auto box = game.aBox;
	V(device->SetStreamSource(0, box->vb, 0, box->vertex_size));
	V(device->SetIndices(box->ib));
	MATRIX m1, m2, m3;
	for(auto& obj : X::objs)
	{
		btBoxShape* shape = (btBoxShape*)obj->getCollisionShape();
		obj->getWorldTransform().getOpenGLMatrix(&m3._11);
		D3DXMatrixScaling(&m2, ToVEC3(shape->getHalfExtentsWithMargin()));
		V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m3 * game.cam.matViewProj)));
		game.eArea->CommitChanges();
		for(int i = 0; i<box->head.n_subs; ++i)
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, box->subs[i].min_ind, box->subs[i].n_ind, box->subs[i].first*3, box->subs[i].tris));
	}
	V(game.eArea->EndPass());
	V(game.eArea->End());

	// invalid room phy
	V(game.eArea->SetVector(game.hAreaColor, &VEC4(1, 0, 0, 1)));
	V(game.eArea->Begin(&passes, 0));
	V(game.eArea->BeginPass(0));
	V(device->SetStreamSource(0, box->vb, 0, box->vertex_size));
	V(device->SetIndices(box->ib));
	for(auto& obj : X::invalid_objs)
	{
		btBoxShape* shape = (btBoxShape*)obj->getCollisionShape();
		obj->getWorldTransform().getOpenGLMatrix(&m3._11);
		D3DXMatrixScaling(&m2, ToVEC3(shape->getHalfExtentsWithMargin()));
		V(game.eArea->SetMatrix(game.hAreaCombined, &(m2 * m3 * game.cam.matViewProj)));
		game.eArea->CommitChanges();
		for(int i = 0; i<box->head.n_subs; ++i)
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, box->subs[i].min_ind, box->subs[i].n_ind, box->subs[i].first*3, box->subs[i].tris));
	}
	V(game.eArea->EndPass());
	V(game.eArea->End());



	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

struct X_CleanupEr
{
	~X_CleanupEr() { X_Cleanup(true); }
};

X_CleanupEr xce;
