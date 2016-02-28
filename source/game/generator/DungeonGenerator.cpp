#include "Pch.h"
#include "Base.h"
#include "Engine.h"
#include "Game.h"

enum ROT_HINT
{
	BOTTOM,
	LEFT,
	TOP,
	RIGHT
};

namespace X
{
	struct XRoom
	{
		vector<XRoom*> connected;
		VEC3 pos;
		INT2 size;
		float rot;
		bool corridor;
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

	Animesh* portalDoor;
	vector<XPortal> portals;
	vector<XDoor> door;

	XDungeon* GenerateDungeon()
	{
		if(!portalDoor)
			ResourceManager::Get().GetLoadedMesh("portal_door.qmsh", portalDoor);

		XDungeon* dungeon = new XDungeon;

		XRoom* room = new XRoom;
		room->pos = VEC3(0, 0, 0);
		room->size = INT2(5, 5);
		room->corridor = false;
		room->rot = 0.f;
		dungeon->rooms.push_back(room);

		//portals.push_back(XPortal(room, VEC3(room->pos.x-1.f*room->size.x, 0, room->pos.z), PI*3/2));
		//portals.push_back(XPortal(room, VEC3(room->pos.x+1.f*room->size.x, 0, room->pos.z), PI/2));
		//portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z-1.f*room->size.y), PI));
		portals.push_back(XPortal(room, VEC3(room->pos.x, 0, room->pos.z+1.f*room->size.y), 0));
				
		return dungeon;
	}

	// !!!!!!!!!!!!!!!!!!++-+++++ add sign where is north, one extra triangle at top north
	void GenerateDungeonStep(XDungeon* dungeon)
	{
		if(portals.empty())
			return;

		int index = rand2()%portals.size();
		XPortal portal = portals[index];
		RemoveElementIndex(portals, index);

		if(portal.room->corridor)
		{
			ROT_HINT side = TOP;//(ROT_HINT)(rand2() % 4); // that will be max(portals) for room
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


			XRoom* r = new XRoom;
			r->corridor = false;
			r->connected.push_back(portal.room);
			portal.room->connected.push_back(r);
			r->size = INT2(size);
			r->rot = dest_rot;
			r->pos = portal.pos + VEC3(sin(portal.rot)*r->size.x, 0, cos(portal.rot)*r->size.y);
			dungeon->rooms.push_back(r);
			if(side != LEFT)
				portals.push_back(XPortal(r, VEC3(r->pos.x-1.f*r->size.x, 0, r->pos.z), PI*3/2));
			if(side != RIGHT)
				portals.push_back(XPortal(r, VEC3(r->pos.x+1.f*r->size.x, 0, r->pos.z), PI/2));
			if(side != BOTTOM)
				portals.push_back(XPortal(r, VEC3(r->pos.x, 0, r->pos.z-1.f*r->size.y), PI));
			if(side != TOP)
				portals.push_back(XPortal(r, VEC3(r->pos.x, 0, r->pos.z+1.f*r->size.y), 0));

			// !!!! add door

			/*int size = random(3, 7);
			if(size % 2 == 0)
				++size;
			XRoom* r = new XRoom;
			r->corridor = false;
			r->connected.push_back(portal.room);
			portal.room->connected.push_back(r);
			r->size = INT2(size);
			r->rot = 0.f;
			r->pos = portal.pos + VEC3(sin(portal.rot)*r->size.x, 0, cos(portal.rot)*r->size.y);
			dungeon->rooms.push_back(r);
			if(portal.hint != RIGHT)
				portals.push_back(XPortal(r, VEC3(r->pos.x-1.f*r->size.x, 0, r->pos.z), PI*3/2, LEFT));
			if(portal.hint != LEFT)
				portals.push_back(XPortal(r, VEC3(r->pos.x+1.f*r->size.x, 0, r->pos.z), PI/2, RIGHT));
			if(portal.hint != TOP)
				portals.push_back(XPortal(r, VEC3(r->pos.x, 0, r->pos.z-1.f*r->size.y), PI, BOTTOM));
			if(portal.hint != BOTTOM)
				portals.push_back(XPortal(r, VEC3(r->pos.x, 0, r->pos.z+1.f*r->size.y), 0, TOP));*/
		}
		else
		{
			int len = random(3, 8);
			XRoom* r = new XRoom;
			r->corridor = true;
			r->connected.push_back(portal.room);
			portal.room->connected.push_back(r);
			r->size = INT2(1, len);
			r->rot = portal.rot + ToRadians(15) ;//clip(portal.rot + portal.room->rot);
			MATRIX rr;
			D3DXMatrixRotationY(&rr, r->rot);
			VEC3 dir = VEC3(sin(r->rot)*r->size.y, 0, cos(r->rot)*r->size.y);
			r->pos = portal.pos + dir;
			dungeon->rooms.push_back(r);
			portals.push_back(XPortal(r, r->pos + dir, r->rot));

			door.push_back(XDoor(portal.pos, portal.rot));
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

	int bigtile_count = 0;
	for(X::XRoom* room : dun->rooms)
		bigtile_count += room->size.x * room->size.y;
	int tile_count = bigtile_count * 4;
	int vertex_count = tile_count * 6;

	device->CreateVertexBuffer(sizeof(VEC3)*vertex_count, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &vb, nullptr);
	VEC3* data;
	vb->Lock(0, 0, (void**)&data, 0);

	for(X::XRoom* room : dun->rooms)
	{
		MATRIX m;
		D3DXMatrixRotationY(&m, room->rot);
		for(int y = 0; y<room->size.y; ++y)
		{
			for(int x = 0; x<room->size.x; ++x)
			{
				VEC3 tile_offset(float(-room->size.x + x*2), 0, float(-room->size.y + y*2));
				AddTile(data, room->pos, tile_offset + VEC3(1,0,1), m);
				AddTile(data, room->pos, tile_offset + VEC3(1,0,0), m);
				AddTile(data, room->pos, tile_offset + VEC3(0,0,1), m);
				AddTile(data, room->pos, tile_offset, m);
			}
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

void X_Cleanup()
{
	delete dun;
	X::portals.clear();
	X::door.clear();
}

void X_DungeonReset()
{
	X_Cleanup();
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
	device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, vcount);

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
	V(game.eArea->SetVector(game.hAreaColor, &VEC4(1, 0, 0, 1)));
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




	device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}

struct X_CleanupEr
{
	~X_CleanupEr() { X_Cleanup(); }
};

X_CleanupEr xce;
