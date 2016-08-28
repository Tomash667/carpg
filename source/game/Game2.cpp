#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "ItemScript.h"
#include "RoomType.h"
#include "SaveState.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "Minimap.h"
#include "Quest_Sawmill.h"
#include "Quest_Mine.h"
#include "Quest_Bandits.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Goblins.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Quest_Main.h"
#include "CityGenerator.h"
#include "Version.h"
#include "LocationHelper.h"

const int SAVE_VERSION = V_CURRENT;
int LOAD_VERSION;
const INT2 SUPPORT_LOAD_VERSION(0, V_CURRENT);

const VEC2 ALERT_RANGE(20.f,30.f);
const float PICKUP_RANGE = 2.f;
const float TRAP_ARROW_SPEED = 45.f;
const float ARROW_TIMER = 5.f;
const float MIN_H = 1.5f;
const float TRAIN_KILL_RATIO = 0.1f;
const float SS = 0.25f;//0.25f/8;
const int NN = 64;
extern const int ITEM_IMAGE_SIZE = 64;

MATRIX m1, m2, m3, m4;
UINT passes;

//=================================================================================================
// Przerywa akcjê postaci
//=================================================================================================
void Game::BreakAction(Unit& unit, bool fall, bool notify)
{
	if(notify && IsServer())
	{
		NetChange& c = Add1(net_changes);
		c.unit = &unit;
		c.type = NetChange::BREAK_ACTION;
	}

	switch(unit.action)
	{
	case A_POSITION:
		return;
	case A_SHOOT:
		if(unit.bow_instance)
		{
			bow_instances.push_back(unit.bow_instance);
			unit.bow_instance = nullptr;
		}
		unit.action = A_NONE;
		break;
	case A_DRINK:
		if(unit.animation_state == 0)
		{
			if(IsLocal())
				AddItem(unit, unit.used_item, 1, unit.used_item_is_team);
			if(!fall)
				unit.used_item = nullptr;
		}
		else
			unit.used_item = nullptr;
		unit.ani->Deactivate(1);
		unit.action = A_NONE;
		break;
	case A_EAT:
		if(unit.animation_state < 2)
		{
			if(IsLocal())
				AddItem(unit, unit.used_item, 1, unit.used_item_is_team);
			if(!fall)
				unit.used_item = nullptr;
		}
		else
			unit.used_item = nullptr;
		unit.ani->Deactivate(1);
		unit.action = A_NONE;
		break;
	case A_TAKE_WEAPON:
		if(unit.weapon_state == WS_HIDING)
		{
			if(unit.animation_state == 0)
			{
				unit.weapon_state = WS_TAKEN;
				unit.weapon_taken = unit.weapon_hiding;
				unit.weapon_hiding = W_NONE;
			}
			else
			{
				unit.weapon_state = WS_HIDDEN;
				unit.weapon_taken = unit.weapon_hiding = W_NONE;
			}
		}
		else
		{
			if(unit.animation_state == 0)
			{
				unit.weapon_state = WS_HIDDEN;
				unit.weapon_taken = W_NONE;
			}
			else
				unit.weapon_state = WS_TAKEN;
		}
		unit.action = A_NONE;
		break;
	case A_BLOCK:
		unit.ani->Deactivate(1);
		unit.action = A_NONE;
		break;
	}

	if(unit.useable)
	{
		unit.target_pos2 = unit.target_pos = unit.pos;
		const Item* prev_used_item = unit.used_item;
		Unit_StopUsingUseable(GetContext(unit), unit, !fall);
		if(prev_used_item && unit.slots[SLOT_WEAPON] == prev_used_item && !unit.HaveShield())
		{
			unit.weapon_state = WS_TAKEN;
			unit.weapon_taken = W_ONE_HANDED;
			unit.weapon_hiding = W_NONE;
		}
		else if(fall)
			unit.used_item = prev_used_item;
		unit.action = A_POSITION;
		unit.animation_state = 0;
		if(IsLocal() && unit.IsAI() && unit.ai->idle_action != AIController::Idle_None)
		{
			unit.ai->idle_action = AIController::Idle_None;
			unit.ai->timer = random(1.f, 2.f);
		}
	}
	else
		unit.action = A_NONE;

	unit.ani->frame_end_info = false;
	unit.ani->frame_end_info2 = false;
	unit.run_attack = false;

	if(unit.IsPlayer())
	{
		PlayerController& player = *unit.player;
		player.next_action = NA_NONE;
		if(&player == pc)
		{
			Inventory::lock_id = LOCK_NO;
			if(inventory_mode > I_INVENTORY)
				CloseInventory();

			if(player.action == PlayerController::Action_Talk)
			{
				if(IsLocal())
				{
					player.action_unit->busy = Unit::Busy_No;
					player.action_unit->look_target = nullptr;
					player.dialog_ctx->dialog_mode = false;
				}
				else
					dialog_context.dialog_mode = false;
				unit.look_target = nullptr;
				player.action = PlayerController::Action_None;
			}
		}
		else if(IsLocal())
		{
			if(player.action == PlayerController::Action_Talk)
			{
				player.action_unit->busy = Unit::Busy_No;
				player.action_unit->look_target = nullptr;
				player.dialog_ctx->dialog_mode = false;
				unit.look_target = nullptr;
				player.action = PlayerController::Action_None;
			}
		}
	}
	else if(IsLocal())
		unit.ai->potion = -1;
}

//=================================================================================================
// Rysowanie
//=================================================================================================
void Game::Draw()
{
	PROFILER_BLOCK("Draw");

	LevelContext& ctx = GetContext(*pc->unit);
	bool outside;
	if(ctx.type == LevelContext::Outside)
		outside = true;
	else if(ctx.type == LevelContext::Inside)
		outside = false;
	else if(city_ctx->inside_buildings[ctx.building_id]->top > 0.f)
		outside = false;
	else
		outside = true;

	ListDrawObjects(ctx, cam.frustum, outside);
	DrawScene(outside);

	// rysowanie local pathfind map
#ifdef DRAW_LOCAL_PATH
	V( eMesh->SetTechnique(techMeshSimple2) );
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	V( eMesh->SetMatrix(hMeshCombined, &cam.matViewProj) );
	V( eMesh->CommitChanges() );

	SetAlphaBlend(true);
	SetAlphaTest(false);
	SetNoZWrite(false);

	for(vector<std::pair<VEC2, int> >::iterator it = test_pf.begin(), end = test_pf.end(); it != end; ++it)
	{
		VEC3 v[4] = {
			VEC3(it->first.x, 0.1f, it->first.y+SS),
			VEC3(it->first.x+SS, 0.1f, it->first.y+SS),
			VEC3(it->first.x, 0.1f, it->first.y),
			VEC3(it->first.x+SS, 0.1f, it->first.y)
		};

		if(test_pf_outside)
		{
			float h = terrain->GetH(v[0].x, v[0].z) + 0.1f;
			for(int i=0; i<4; ++i)
				v[i].y = h;
		}

		VEC4 color;
		switch(it->second)
		{
		case 0:
			color = VEC4(0,1,0,0.5f);
			break;
		case 1:
			color = VEC4(1,0,0,0.5f);
			break;
		case 2:
			color = VEC4(0,0,0,0.5f);
			break;
		}

		V( eMesh->SetVector(hMeshTint, &color) );
		V( eMesh->CommitChanges() );

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(VEC3));
	}

	V( eMesh->EndPass()	);
	V( eMesh->End() );
#endif
}

inline TEX GetTexture(int index, const TexId* tex_override, const Animesh& mesh)
{
	if(tex_override && tex_override[index].tex)
		return tex_override[index].tex->data;
	else
		return mesh.GetTexture(index);
}

//=================================================================================================
// Generuje obrazek przedmiotu
//=================================================================================================
void Game::GenerateImage(TaskData& task_data)
{
	Item* item = (Item*)task_data.ptr;
	item->mesh = (Animesh*)task_data.res->data;

	auto it = item_texture_map.lower_bound(item->mesh);
	if(it != item_texture_map.end() && !(item_texture_map.key_comp()(item->mesh, it->first)))
	{
		item->tex = it->second;
		return;
	}

	SetAlphaBlend(false);
	SetAlphaTest(false);
	SetNoCulling(false);
	SetNoZWrite(false);

	// ustaw render target
	SURFACE surf = nullptr;
	if(sItemRegion)
		V( device->SetRenderTarget(0, sItemRegion) );
	else
	{
		V( tItemRegion->GetSurfaceLevel(0, &surf) );
		V( device->SetRenderTarget(0, surf) );
	}

	// pocz¹tek renderowania
	V( device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0) );
	V( device->BeginScene() );

	const Animesh& a = *item->mesh;

	const TexId* tex_override = nullptr;
	if(item->type == IT_ARMOR)
	{
		tex_override = item->ToArmor().GetTextureOverride();
		if(tex_override)
		{
			assert(item->ToArmor().tex_override.size() == a.head.n_subs);
		}
	}	

	MATRIX matWorld, matView, matProj;
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixLookAtLH(&matView, &a.cam_pos, &a.cam_target, &a.cam_up);
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4, 1.f, 0.1f, 25.f);

	LightData ld;
	ld.pos = a.cam_pos;
	ld.color = VEC3(1,1,1);
	ld.range = 10.f;

	V( eMesh->SetTechnique(techMesh) );
	V( eMesh->SetMatrix(hMeshCombined, &(matView*matProj)) );
	V( eMesh->SetMatrix(hMeshWorld, &matWorld) );
	V( eMesh->SetVector(hMeshFogColor, &VEC4(1,1,1,1)) );
	V( eMesh->SetVector(hMeshFogParam, &VEC4(25.f,50.f,25.f,0)) );
	V( eMesh->SetVector(hMeshAmbientColor, &VEC4(0.5f,0.5f,0.5f,1)));
	V( eMesh->SetRawValue(hMeshLights, &ld, 0, sizeof(LightData)) );
	V( eMesh->SetVector(hMeshTint, &VEC4(1,1,1,1)));

	V( device->SetVertexDeclaration(vertex_decl[a.vertex_decl]) );
	V( device->SetStreamSource(0, a.vb, 0, a.vertex_size) );
	V( device->SetIndices(a.ib) );

	UINT passes;
	V( eMesh->Begin(&passes, 0) );
	V( eMesh->BeginPass(0) );

	for(int i=0; i<a.head.n_subs; ++i)
	{
		const Animesh::Submesh& sub = a.subs[i];
		V( eMesh->SetTexture(hMeshTex, GetTexture(i, tex_override, a)) );
		V( eMesh->CommitChanges() );
		V( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first*3, sub.tris) );
	}

	V( eMesh->EndPass() );
	V( eMesh->End() );

	// koniec renderowania
	V( device->EndScene() );

	// kopiuj do tekstury
	if(sItemRegion)
	{
		V( tItemRegion->GetSurfaceLevel(0, &surf) );
		V( device->StretchRect(sItemRegion, nullptr, surf, nullptr, D3DTEXF_NONE) );
	}

	// stwórz now¹ teksturê i skopuj obrazek do niej
	TEX t;
	SURFACE out_surface;
	V( device->CreateTexture(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr) );
	V( t->GetSurfaceLevel(0, &out_surface) );
	V( D3DXLoadSurfaceFromSurface(out_surface, nullptr, nullptr, surf, nullptr, nullptr, D3DX_DEFAULT, 0) );
	surf->Release();
	out_surface->Release();

	// przywróæ stary render target
	V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( device->SetRenderTarget(0, surf) );
	surf->Release();

	item->tex = t;
	item_texture_map.insert(it, ItemTextureMap::value_type(item->mesh, t));
}

//=================================================================================================
// Zwraca jednostkê za któr¹ pod¹¿a kamera
//=================================================================================================
Unit* Game::GetFollowTarget()
{
	return pc->unit;
}

//=================================================================================================
// Ustawia kamerê
//=================================================================================================
void Game::SetupCamera(float dt)
{
	Unit* target = GetFollowTarget();
	LevelContext& ctx = GetContext(*target);

	float rotX;
	if(cam.free_rot)
		rotX = cam.real_rot.x;
	else
		rotX = target->rot;

	cam.UpdateRot(dt, VEC2(rotX, cam.real_rot.y));

	MATRIX mat, matProj, matView;
	const VEC3 cam_h(0, target->GetUnitHeight()+0.2f, 0);
	VEC3 dist(0,-cam.tmp_dist,0);
	
	D3DXMatrixRotationYawPitchRoll(&mat, cam.rot.x, cam.rot.y, 0);
	D3DXVec3TransformCoord(&dist, &dist, &mat);

	// !!! to => from !!!
	// kamera idzie od g³owy do ty³u
	VEC3 to = target->pos + cam_h;
	float tout, min_tout=2.f;

	int tx = int(target->pos.x/2),
		tz = int(target->pos.z/2);

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		// teren
		tout = terrain->Raytest(to, to+dist);
		if(tout < min_tout && tout > 0.f)
			min_tout = tout;

		// budynki
		int minx = max(0, tx-3),
			minz = max(0, tz-3),
			maxx = min(OutsideLocation::size -1, tx+3),
			maxz = min(OutsideLocation::size -1, tz+3);

		for(int z=minz; z<=maxz; ++z)
		{
			for(int x=minx; x<=maxx; ++x)
			{
				if(outside->tiles[x+z*OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
				{
					const BOX box(float(x)*2, 0, float(z)*2, float(x+1)*2, 8.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
			}
		}

		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER)
				continue;

			if(it->type == CollisionObject::SPHERE)
			{
				if(RayToCylinder(to, to+dist, VEC3(it->pt.x,0,it->pt.y), VEC3(it->pt.x,32.f,it->pt.y), it->radius, tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
			else if(it->type == CollisionObject::RECTANGLE)
			{
				BOX box(it->pt.x-it->w, 0.f, it->pt.y-it->h, it->pt.x+it->w, 32.f, it->pt.y+it->h);
				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
			else
			{
				float w, h;
				if(equal(it->rot, PI/2) || equal(it->rot, PI*3/2))
				{
					w = it->h;
					h = it->w;
				}
				else
				{
					w = it->w;
					h = it->h;
				}

				BOX box(it->pt.x-w, 0.f, it->pt.y-h, it->pt.x+w, 32.f, it->pt.y+h);
				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}
		}

		for(vector<CameraCollider>::iterator it = cam_colliders.begin(), end = cam_colliders.end(); it != end; ++it)
		{
			if(RayToBox(to, dist, it->box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx-3),
			minz = max(0, tz-3),
			maxx = min(lvl.w-1, tx+3),
			maxz = min(lvl.h-1, tz+3);

		// sufit
		const D3DXPLANE sufit(0,-1,0,4);
		if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
		{
			//tmpvar2 = 1;
			min_tout = tout;
		}

		// pod³oga
		const D3DXPLANE podloga(0,1,0,0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// podziemia
		for(int z=minz; z<=maxz; ++z)
		{
			for(int x=minx; x<=maxx; ++x)
			{
				Pole& p = lvl.map[x+z*lvl.w];
				if(czy_blokuje2(p.type))
				{
					const BOX box(float(x)*2, 0, float(z)*2, float(x+1)*2, 4.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				else if(IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				{
					const BOX box(float(x)*2, 3.f, float(z)*2, float(x+1)*2, 4.f, float(z+1)*2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				if(p.type == SCHODY_GORA)
				{
					if(RayToMesh(to, dist, pt_to_pos(lvl.staircase_up), dir_to_rot(lvl.staircase_up_dir), vdSchodyGora, tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == SCHODY_DOL)
				{
					if(!lvl.staircase_down_in_wall && RayToMesh(to, dist, pt_to_pos(lvl.staircase_down), dir_to_rot(lvl.staircase_down_dir), vdSchodyDol, tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == DRZWI || p.type == OTWOR_NA_DRZWI)
				{
					VEC3 pos(float(x*2)+1,0,float(z*2)+1);
					float rot;

					if(czy_blokuje2(lvl.map[x - 1 + z*lvl.w].type))
					{
						rot = 0;
						int mov = 0;
						if(lvl.rooms[lvl.map[x+(z-1)*lvl.w].room].IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + (z + 1)*lvl.w].room].IsCorridor())
							--mov;
						if(mov == 1)
							pos.z += 0.8229f;
						else if(mov == -1)
							pos.z -= 0.8229f;
					}
					else
					{
						rot = PI/2;
						int mov = 0;
						if(lvl.rooms[lvl.map[x - 1 + z*lvl.w].room].IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + 1 + z*lvl.w].room].IsCorridor())
							--mov;
						if(mov == 1)
							pos.x += 0.8229f;
						else if(mov == -1)
							pos.x -= 0.8229f;
					}

					if(RayToMesh(to, dist, pos, rot, vdNaDrzwi, tout) && tout < min_tout)
						min_tout = tout;

					Door* door = FindDoor(ctx, INT2(x, z));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX box(pos.x, 0.f, pos.z);
						box.v2.y = 1.319f*2;
						if(rot == 0.f)
						{
							box.v1.x -= 0.842f;
							box.v2.x += 0.842f;
							box.v1.z -= 0.181f;
							box.v2.z += 0.181f;
						}
						else
						{
							box.v1.z -= 0.842f;
							box.v2.z += 0.842f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}
						
						if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
							min_tout = tout;
					}
				}
			}
		}
	}
	else
	{
		InsideBuilding& building = *city_ctx->inside_buildings[ctx.building_id];

		// budynek

		// pod³oga
		const D3DXPLANE podloga(0,1,0,0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// sufit
		if(building.top > 0.f)
		{
			const D3DXPLANE sufit(0,-1,0,4);
			if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// xsphere
		if(building.xsphere_radius > 0.f)
		{
			VEC3 from = to + dist;
			if(RayToSphere(from, -dist, building.xsphere_pos, building.xsphere_radius, tout) && tout > 0.f)
			{
				tout = 1.f - tout;
				if(tout < min_tout)
					min_tout = tout;
			}
		}

		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX box(it->pt.x-it->w, 0.f, it->pt.y-it->h, it->pt.x+it->w, 10.f, it->pt.y+it->h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX box(door.pos);
				box.v2.y = 1.319f*2;
				if(door.rot == 0.f)
				{
					box.v1.x -= 0.842f;
					box.v2.x += 0.842f;
					box.v1.z -= 0.181f;
					box.v2.z += 0.181f;
				}
				else
				{
					box.v1.z -= 0.842f;
					box.v2.z += 0.842f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
					min_tout = tout;
			}

			if(RayToMesh(to, dist, door.pos, door.rot, vdNaDrzwi, tout) && tout < min_tout)
				min_tout = tout;
		}
	}
	
	// uwzglêdnienie znear
	if(min_tout >= 0.9f || pc->noclip)
		min_tout = 0.9f;
	else
	{
		min_tout -= 0.1f;
		if(min_tout < 0.1f)
			min_tout = 0.1f;
	}

	VEC3 from = to + dist*min_tout;

	cam.Update(dt, from, to);

	float drunk = pc->unit->alcohol/pc->unit->hpmax;
	float drunk_mod = (drunk > 0.1f ? (drunk-0.1f)/0.9f : 0.f);

	D3DXMatrixLookAtLH(&matView, &cam.from, &cam.to, &VEC3(0,1,0));
	D3DXMatrixPerspectiveFovLH(&matProj, PI/4+sin(drunk_anim)*(PI/16)*drunk_mod, float(wnd_size.x)/wnd_size.y*(1.f+sin(drunk_anim)/10*drunk_mod), 0.1f, cam.draw_range);
	cam.matViewProj = matView * matProj;
	D3DXMatrixInverse(&cam.matViewInv, nullptr, &matView);

	MATRIX matProj2;
	D3DXMatrixPerspectiveFovLH(&matProj2, PI/4+sin(drunk_anim)*(PI/16)*drunk_mod, float(wnd_size.x)/wnd_size.y*(1.f+sin(drunk_anim)/10*drunk_mod), 0.1f, grass_range);

	cam.center = cam.from;

	cam.frustum.Set(cam.matViewProj);
	cam.frustum2.Set(matView * matProj2);

	// centrum dŸwiêku 3d
	VEC3 listener_pos = target->GetHeadSoundPos();
	fmod_system->set3DListenerAttributes(0, (const FMOD_VECTOR*)&listener_pos, nullptr, (const FMOD_VECTOR*)&VEC3(sin(target->rot+PI),0,cos(target->rot+PI)), (const FMOD_VECTOR*)&VEC3(0,1,0));
}

void Game::LoadShaders()
{
	LOG("Loading shaders.");

	eMesh = CompileShader("mesh.fx");
	eParticle = CompileShader("particle.fx");
	eSkybox = CompileShader("skybox.fx");
	eTerrain = CompileShader("terrain.fx");
	eArea = CompileShader("area.fx");
	ePostFx = CompileShader("post.fx");
	eGlow = CompileShader("glow.fx");
	eGrass = CompileShader("grass.fx");

	SetupShaders();
}

//=================================================================================================
// Ustawia parametry shaderów
//=================================================================================================
void Game::SetupShaders()
{
	techMesh = eMesh->GetTechniqueByName("mesh");
	techMeshDir = eMesh->GetTechniqueByName("mesh_dir");
	techMeshSimple = eMesh->GetTechniqueByName("mesh_simple");
	techMeshSimple2 = eMesh->GetTechniqueByName("mesh_simple2");
	techMeshExplo = eMesh->GetTechniqueByName("mesh_explo");
	techParticle = eParticle->GetTechniqueByName("particle");
	techTrail = eParticle->GetTechniqueByName("trail");
	techSkybox = eSkybox->GetTechniqueByName("skybox");
	techTerrain = eTerrain->GetTechniqueByName("terrain");
	techArea = eArea->GetTechniqueByName("area");
	techGui = eGui->GetTechniqueByName("gui");
	techGlowMesh = eGlow->GetTechniqueByName("mesh");
	techGlowAni = eGlow->GetTechniqueByName("ani");
	techGrass = eGrass->GetTechniqueByName("grass");
	assert(techAnim && techHair && techAnimDir && techHairDir && techMesh && techMeshDir && techMeshSimple && techMeshSimple2 && techMeshExplo && techParticle && techTrail && techSkybox &&
		techTerrain && techArea && techGui && techGlowMesh && techGlowAni && techGrass);

	hMeshCombined = eMesh->GetParameterByName(nullptr, "matCombined");
	hMeshWorld = eMesh->GetParameterByName(nullptr, "matWorld");
	hMeshTex = eMesh->GetParameterByName(nullptr, "texDiffuse");
	hMeshFogColor = eMesh->GetParameterByName(nullptr, "fogColor");
	hMeshFogParam = eMesh->GetParameterByName(nullptr, "fogParam");
	hMeshTint = eMesh->GetParameterByName(nullptr, "tint");
	hMeshAmbientColor = eMesh->GetParameterByName(nullptr, "ambientColor");
	hMeshLightDir = eMesh->GetParameterByName(nullptr, "lightDir");
	hMeshLightColor = eMesh->GetParameterByName(nullptr, "lightColor");
	hMeshLights = eMesh->GetParameterByName(nullptr, "lights");
	assert(hMeshCombined && hMeshWorld && hMeshTex && hMeshFogColor && hMeshFogParam && hMeshTint && hMeshAmbientColor && hMeshLightDir && hMeshLightColor && hMeshLights);

	hParticleCombined = eParticle->GetParameterByName(nullptr, "matCombined");
	hParticleTex = eParticle->GetParameterByName(nullptr, "tex0");
	assert(hParticleCombined && hParticleTex);
	
	hSkyboxCombined = eSkybox->GetParameterByName(nullptr, "matCombined");
	hSkyboxTex = eSkybox->GetParameterByName(nullptr, "tex0");
	assert(hSkyboxCombined && hSkyboxTex);

	hTerrainCombined = eTerrain->GetParameterByName(nullptr, "matCombined");
	hTerrainWorld = eTerrain->GetParameterByName(nullptr, "matWorld");
	hTerrainTexBlend = eTerrain->GetParameterByName(nullptr, "texBlend");
	hTerrainTex[0] = eTerrain->GetParameterByName(nullptr, "tex0");
	hTerrainTex[1] = eTerrain->GetParameterByName(nullptr, "tex1");
	hTerrainTex[2] = eTerrain->GetParameterByName(nullptr, "tex2");
	hTerrainTex[3] = eTerrain->GetParameterByName(nullptr, "tex3");
	hTerrainTex[4] = eTerrain->GetParameterByName(nullptr, "tex4");
	hTerrainColorAmbient = eTerrain->GetParameterByName(nullptr, "colorAmbient");
	hTerrainColorDiffuse = eTerrain->GetParameterByName(nullptr, "colorDiffuse");
	hTerrainLightDir = eTerrain->GetParameterByName(nullptr, "lightDir");
	hTerrainFogColor = eTerrain->GetParameterByName(nullptr, "fogColor");
	hTerrainFogParam = eTerrain->GetParameterByName(nullptr, "fogParam");
	assert(hTerrainCombined && hTerrainWorld && hTerrainTexBlend && hTerrainTex[0] && hTerrainTex[1] && hTerrainTex[2] && hTerrainTex[3] && hTerrainTex[4] &&
		hTerrainColorAmbient && hTerrainColorDiffuse && hTerrainLightDir && hTerrainFogColor && hTerrainFogParam);

	hAreaCombined = eArea->GetParameterByName(nullptr, "matCombined");
	hAreaColor = eArea->GetParameterByName(nullptr, "color");
	hAreaPlayerPos = eArea->GetParameterByName(nullptr, "playerPos");
	hAreaRange = eArea->GetParameterByName(nullptr, "range");
	assert(hAreaCombined && hAreaColor && hAreaPlayerPos && hAreaRange);

	hPostTex = ePostFx->GetParameterByName(nullptr, "t0");
	hPostPower = ePostFx->GetParameterByName(nullptr, "power");
	hPostSkill = ePostFx->GetParameterByName(nullptr, "skill");
	assert(hPostTex && hPostPower && hPostSkill);

	hGlowCombined = eGlow->GetParameterByName(nullptr, "matCombined");
	hGlowBones = eGlow->GetParameterByName(nullptr, "matBones");
	hGlowColor = eGlow->GetParameterByName(nullptr, "color");
	assert(hGlowCombined && hGlowBones && hGlowColor);

	hGrassViewProj = eGrass->GetParameterByName(nullptr, "matViewProj");
	hGrassTex = eGrass->GetParameterByName(nullptr, "texDiffuse");
	hGrassFogColor = eGrass->GetParameterByName(nullptr, "fogColor");
	hGrassFogParams = eGrass->GetParameterByName(nullptr, "fogParam");
	hGrassAmbientColor = eGrass->GetParameterByName(nullptr, "ambientColor");
}

const INT2 g_kierunek2[4] = {
	INT2(0,-1),
	INT2(-1,0),
	INT2(0,1),
	INT2(1,0)
};

//=================================================================================================
// Aktualizuje grê
//=================================================================================================
void Game::UpdateGame(float dt)
{
	dt *= game_speed;
	if(dt == 0)
		return;

	PROFILER_BLOCK("UpdateGame");

/*#ifdef _DEBUG
	if(Key.Pressed('9'))
		VerifyObjects();
#endif*/

	// sanity checks
#ifdef _DEBUG
	if(sv_server && !sv_online)
	{
		AddGameMsg("sv_server was true!", 5.f);
		sv_server = false;
	}

	if(IsLocal())
	{
		assert(pc->is_local);
		if(IsOnline())
		{
			for(PlayerInfo& pi : game_players)
			{
				if(!pi.left)
				{
					assert(pi.pc == pi.pc->player_info->pc);
					if(pi.id != 0)
					{
						assert(!pi.pc->is_local);
					}
				}
			}
		}
	}
	else
	{
		assert(pc->is_local && pc == pc->player_info->pc);
	}
#endif

	bool getting_out = false;

	/*Object* o = nullptr;
	float dist = 999.f;
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		float d = distance(it->pos, pc->unit->pos);
		if(d < dist)
		{
			dist = d;
			o = &*it;
		}
	}
	if(o)
	{
		if(Key.Down('9'))
			o->rot.y = clip(o->rot.y+dt);
		if(Key.Down('0'))
			o->rot.y = 0;
	}*/

	minimap_opened_doors = false;

	if(in_tutorial && !IsOnline())
		UpdateTutorial();

	drunk_anim = clip(drunk_anim+dt);
	UpdatePostEffects(dt);

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	light_angle = clip(light_angle+dt/100);

	LevelContext& player_ctx = (pc->unit->in_building == -1 ? local_ctx : city_ctx->inside_buildings[pc->unit->in_building]->ctx);

	// fallback
	if(fallback_co != -1)
	{
		if(fallback_t <= 0.f)
		{
			fallback_t += dt*2;

			if(fallback_t > 0.f)
			{
				switch(fallback_co)
				{
				case FALLBACK_TRAIN:
					if(IsLocal())
					{
						if(fallback_1 == 2)
							TournamentTrain(*pc->unit);
						else
							Train(*pc->unit, fallback_1 == 1, fallback_2);
						pc->Rest(10, false);
						if(IsOnline())
							UseDays(pc, 10);
						else
							WorldProgress(10, WPM_NORMAL);
					}
					else
					{
						fallback_co = FALLBACK_CLIENT;
						fallback_t = 0.f;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRAIN;
						c.id = fallback_1;
						c.ile = fallback_2;
					}
					break;
				case FALLBACK_REST:
					if(IsLocal())
					{
						pc->Rest(fallback_1, true);
						if(IsOnline())
							UseDays(pc, fallback_1);
						else
							WorldProgress(fallback_1, WPM_NORMAL);
					}
					else
					{
						fallback_co = FALLBACK_CLIENT;
						fallback_t = 0.f;
						NetChange& c = Add1(net_changes);
						c.type = NetChange::REST;
						c.id = fallback_1;
					}
					break;
				case FALLBACK_ENTER:
					// wejœcie/wyjœcie z budynku
					{
						UnitWarpData& uwd = Add1(unit_warp_data);
						uwd.unit = pc->unit;
						uwd.where = fallback_1;
					}
					break;
				case FALLBACK_EXIT:
					ExitToMap();
					getting_out = true;
					break;
				case FALLBACK_CHANGE_LEVEL:
					ChangeLevel(fallback_1);
					getting_out = true;
					break;
				case FALLBACK_USE_PORTAL:
					{
						Portal* portal = location->GetPortal(fallback_1);
						Location* target_loc = locations[portal->target_loc];
						int at_level = 0;
						// aktualnie mo¿na siê tepn¹æ z X poziomu na 1 zawsze ale ¿eby z X na X to musi byæ odwiedzony
						// np w sekrecie z 3 na 1 i spowrotem do
						if(target_loc->portal)
							at_level = target_loc->portal->at_level;
						LeaveLocation(false, false);
						current_location = portal->target_loc;
						EnterLocation(at_level, portal->target);
					}
					return;
				case FALLBACK_NONE:
				case FALLBACK_ARENA2:
				case FALLBACK_CLIENT2:
					break;
				case FALLBACK_ARENA:
				case FALLBACK_ARENA_EXIT:
				case FALLBACK_WAIT_FOR_WARP:
				case FALLBACK_CLIENT:
					fallback_t = 0.f;
					break;
				default:
					assert(0);
					break;
				}
			}
		}
		else
		{
			fallback_t += dt*2;

			if(fallback_t >= 1.f)
			{
				if(IsLocal())
				{
					if(fallback_co != FALLBACK_ARENA2)
					{
						if(fallback_co == FALLBACK_CHANGE_LEVEL || fallback_co == FALLBACK_USE_PORTAL || fallback_co == FALLBACK_EXIT)
						{
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 0;
						}
						pc->unit->frozen = 0;
					}
				}
				else if(fallback_co == FALLBACK_CLIENT2)
					pc->unit->frozen = 0;
				fallback_co = -1;
			}
		}
	}

	if(IsLocal() && !in_tutorial)
	{
		// aktualizuj arene/wymiane sprzêtu/zawody w piciu/questy
		UpdateGame2(dt);
	}

	// info o uczoñczeniu wszystkich unikalnych questów
	if(CanShowEndScreen())
	{
		if(IsLocal())
			unique_completed_show = true;
		else
			unique_completed_show = false;

		cstring text;

		if(IsOnline())
		{
			text = txWinMp;
			if(IsServer())
			{
				PushNetChange(NetChange::GAME_STATS);
				PushNetChange(NetChange::ALL_QUESTS_COMPLETED);
			}
		}
		else
			text = txWin;

		GUI.SimpleDialog(Format(text, pc->kills, total_kills-pc->kills), nullptr);
	}

	// wyzeruj pobliskie jednostki
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
		it->valid = false;

	// licznik otrzymanych obra¿eñ
	pc->last_dmg = 0.f;
	if(!IsOnline() || !IsClient())
		pc->last_dmg_poison = 0.f;

	if(devmode && AllowKeyboard())
	{
		if(!location->outside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();

			if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && inside->HaveUpStairs())
			{
				if(!Key.Down(VK_CONTROL))
				{
					// teleportuj gracza do schodów w górê
					if(IsLocal())
					{
						INT2 tile = lvl.GetUpStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.staircase_up_dir);
						WarpUnit(*pc->unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 0;
					}
				}
				else
				{
					// poziom w górê
					if(IsLocal())
					{
						ChangeLevel(-1);
						return;
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 0;
					}
				}
			}
			if(Key.Pressed(VK_OEM_PERIOD) && Key.Down(VK_SHIFT) && inside->HaveDownStairs())
			{
				if(!Key.Down(VK_CONTROL))
				{
					// teleportuj gracza do schodów w dó³
					if(IsLocal())
					{
						INT2 tile = lvl.GetDownStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.staircase_down_dir);
						WarpUnit(*pc->unit, VEC3(2.f*tile.x+1.f, 0.f, 2.f*tile.y+1.f));
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 1;
					}
				}
				else
				{
					// poziom w dó³
					if(IsLocal())
					{
						ChangeLevel(+1);
						return;
					}
					else
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 1;
					}
				}
			}
		}
		else if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && Key.Down(VK_CONTROL))
		{
			if(IsLocal())
			{
				ExitToMap();
				return;
			}
			else
				PushNetChange(NetChange::CHEAT_GOTO_MAP);
		}
	}

	// obracanie kamery góra/dó³
	if(!IsLocal() || IsAnyoneAlive())
	{
		if(dialog_context.dialog_mode && inventory_mode <= I_INVENTORY)
		{
			cam.free_rot = false;
			if(cam.real_rot.y > 4.2875104f)
			{
				cam.real_rot.y -= dt;
				if(cam.real_rot.y < 4.2875104f)
					cam.real_rot.y = 4.2875104f;
			}
			else if(cam.real_rot.y < 4.2875104f)
			{
				cam.real_rot.y += dt;
				if(cam.real_rot.y > 4.2875104f)
					cam.real_rot.y = 4.2875104f;
			}
		}
		else
		{
			if(allow_input == ALLOW_INPUT || allow_input == ALLOW_MOUSE)
			{
				const float c_cam_angle_min = PI + 0.1f;
				const float c_cam_angle_max = PI*1.8f - 0.1f;

				int div = (pc->unit->action == A_SHOOT ? 800 : 400);
				cam.real_rot.y += -float(mouse_dif.y) * mouse_sensitivity_f / div;
				if(cam.real_rot.y > c_cam_angle_max)
					cam.real_rot.y = c_cam_angle_max;
				if(cam.real_rot.y < c_cam_angle_min)
					cam.real_rot.y = c_cam_angle_min;

				if(!cam.free_rot)
				{
					cam.free_rot_key = KeyDoReturn(GK_ROTATE_CAMERA, &KeyStates::Pressed);
					if(cam.free_rot_key != VK_NONE)
					{
						cam.real_rot.x = clip(pc->unit->rot + PI);
						cam.free_rot = true;
					}
				}
				else
				{
					if(KeyUpAllowed(cam.free_rot_key))
						cam.free_rot = false;
					else
						cam.real_rot.x = clip(cam.real_rot.x + float(mouse_dif.x) * mouse_sensitivity_f / 400);
				}
			}
			else
				cam.free_rot = false;
		}
	}
	else
	{
		cam.free_rot = false;
		if(cam.real_rot.y > PI+0.1f)
		{
			cam.real_rot.y -= dt;
			if(cam.real_rot.y < PI+0.1f)
				cam.real_rot.y = PI+0.1f;
		}
		else if(cam.real_rot.y < PI+0.1f)
		{
			cam.real_rot.y += dt;
			if(cam.real_rot.y > PI+0.1f)
				cam.real_rot.y = PI+0.1f;
		}
	}

	// przybli¿anie/oddalanie kamery
	if(AllowMouse())
	{
		if(!dialog_context.dialog_mode || !dialog_context.show_choices || !game_gui->IsMouseInsideDialog())
		{
			cam.dist -= float(mouse_wheel) / WHEEL_DELTA;
			cam.dist = clamp(cam.dist, 0.5f, 6.f);
		}

		if(Key.PressedRelease(VK_MBUTTON))
			cam.dist = 3.5f;
	}

	// umieranie
	if((IsLocal() && !IsAnyoneAlive()) || death_screen != 0)
	{
		if(death_screen == 0)
		{
			LOG("Game over: all players died.");
			SetMusic(MusicType::Death);
			CloseAllPanels(true);
			++death_screen;
			death_fade = 0;
			death_solo = (team.size() == 1u);
			if(IsOnline())
			{
				PushNetChange(NetChange::GAME_STATS);
				PushNetChange(NetChange::GAME_OVER);
			}
		}
		else if(death_screen == 1 || death_screen == 2)
		{
			death_fade += dt/2;
			if(death_fade >= 1.f)
			{
				death_fade = 0;
				++death_screen;
			}
		}
		if(death_screen >= 2 && AllowKeyboard() && Key.Pressed2Release(VK_ESCAPE, VK_RETURN))
		{
			ExitToMenu();
			return;
		}
	}

	// aktualizuj gracza
	if(pc->wasted_key != VK_NONE && Key.Up(pc->wasted_key))
		pc->wasted_key = VK_NONE;
	if(dialog_context.dialog_mode || pc->unit->look_target || inventory_mode > I_INVENTORY)
	{
		VEC3 pos;
		if(pc->unit->look_target)
		{
			pos = pc->unit->look_target->pos;
			pc->unit->animation = ANI_STAND;
		}
		else if(inventory_mode == I_LOOT_CHEST)
		{
			assert(pc->action == PlayerController::Action_LootChest);
			pos = pc->action_chest->pos;
			pc->unit->animation = ANI_KNEELS;
		}
		else if(inventory_mode == I_LOOT_BODY)
		{
			assert(pc->action == PlayerController::Action_LootUnit);
			pos = pc->action_unit->GetLootCenter();
			pc->unit->animation = ANI_KNEELS;
		}
		else if(dialog_context.dialog_mode)
		{
			pos = dialog_context.talker->pos;
			pc->unit->animation = ANI_STAND;
		}
		else
		{
			assert(pc->action == InventoryModeToActionRequired(inventory_mode));
			pos = pc->action_unit->pos;
			pc->unit->animation = ANI_STAND;
		}

		float dir = lookat_angle(pc->unit->pos, pos);
		
		if(!equal(pc->unit->rot, dir))
		{
			const float rot_speed = 3.f*dt;
			const float rot_diff = angle_dif(pc->unit->rot, dir);
			if(shortestArc(pc->unit->rot, dir) > 0.f)
				pc->unit->animation = ANI_RIGHT;
			else
				pc->unit->animation = ANI_LEFT;
			if(rot_diff < rot_speed)
				pc->unit->rot = dir;
			else
			{
				const float d = sign(shortestArc(pc->unit->rot, dir)) * rot_speed;
				pc->unit->rot = clip(pc->unit->rot + d);
			}
		}

		if(inventory_mode > I_INVENTORY)
		{
			if(inventory_mode == I_LOOT_BODY)
			{
				if(pc->action_unit->IsAlive())
				{
					// grabiony cel o¿y³
					CloseInventory();
				}
			}
			else if(inventory_mode == I_TRADE || inventory_mode == I_SHARE || inventory_mode == I_GIVE)
			{
				if(!pc->action_unit->IsStanding() || !IsUnitIdle(*pc->action_unit))
				{
					// handlarz umar³ / zaatakowany
					CloseInventory();
				}
			}
		}
		else if(dialog_context.dialog_mode && IsLocal())
		{
			if(!dialog_context.talker->IsStanding() || !IsUnitIdle(*dialog_context.talker) || dialog_context.talker->to_remove || dialog_context.talker->frozen != 0)
			{
				// rozmówca umar³ / jest usuwany / zaatakowa³ kogoœ
				EndDialog(dialog_context);
			}
		}

		UpdatePlayerView();
		before_player = BP_NONE;
		player_rot_buf = 0.f;
		autowalk = false;
	}
	else if(!IsBlocking(pc->unit->action))
		UpdatePlayer(player_ctx, dt);
	else
	{
		UpdatePlayerView();
		before_player = BP_NONE;
		player_rot_buf = 0.f;
		autowalk = false;
	}

	// aktualizuj ai
	if(!noai && IsLocal())
		UpdateAi(dt);

	// aktualizuj konteksty poziomów
	lights_dt += dt;
	UpdateContext(local_ctx, dt);
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			UpdateContext((*it)->ctx, dt);
	}
	if(lights_dt >= 1.f/60)
		lights_dt = 0.f;

	// aktualizacja minimapy
	if(minimap_opened_doors)
	{
		for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsPlayer())
				DungeonReveal(INT2(int(u.pos.x/2), int(u.pos.z/2)));
		}
	}
	UpdateDungeonMinimap(true);

	// aktualizuj pobliskie postacie
	// 0.0 -> 0.1 niewidoczne
	// 0.1 -> 0.2 alpha 0->255
	// -0.2 -> -0.1 widoczne
	// -0.1 -> 0.0 alpha 255->0
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end;)
	{
		bool removed = false;

		if(it->valid)
		{
			if(it->time >= 0.f)
				it->time += dt;
			else if(it->time < -UNIT_VIEW_A)
				it->time = UNIT_VIEW_B;
			else
				it->time = -it->time;
		}
		else
		{
			if(it->time >= 0.f)
			{
				if(it->time < UNIT_VIEW_A)
				{
					// usuñ
					if(it+1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end-1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
				else if(it->time < UNIT_VIEW_B)
					it->time = -it->time;
				else
					it->time = -UNIT_VIEW_B;
			}
			else
			{
				it->time += dt;
				if(it->time >= 0.f)
				{
					// usuñ
					if(it+1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end-1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
			}
		}

		if(!removed)
			++it;
	}

	// aktualizuj dialogi
	if(!IsOnline())
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialog(dialog_context, dt);
	}
	else if(IsServer())
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->left)
				continue;
			DialogContext& ctx = *it->u->player->dialog_ctx;
			if(ctx.dialog_mode)
			{
				if(!ctx.talker->IsStanding() || !IsUnitIdle(*ctx.talker) || ctx.talker->to_remove || ctx.talker->frozen != 0)
					EndDialog(ctx);
				else
					UpdateGameDialog(ctx, dt);
			}
		}
	}
	else
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialogClient();
	}

	UpdateAttachedSounds(dt);
	if(IsLocal())
	{
		if(IsOnline())
			UpdateWarpData(dt);
		ProcessUnitWarps();
	}

	// usuñ jednostki
	ProcessRemoveUnits();

	if(IsOnline())
	{
		UpdateGameNet(dt);
		if(!IsOnline() || game_state != GS_LEVEL)
			return;
	}

	// aktualizacja obrazka obra¿en
	pc->Update(dt);

	// aktualizuj kamerê
	SetupCamera(dt);

#ifdef _DEBUG
	if(IsLocal() && arena_free)
	{
		int err_count = 0;
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			for(vector<Unit*>::iterator it2 = team.begin(), end2 = team.end(); it2 != end2; ++it2)
			{
				if(it != it2)
				{
					if(!IsFriend(**it, **it2))
					{
						WARN(Format("%s (%d,%d) i %s (%d,%d) are not friends!", (*it)->data->id.c_str(), (*it)->in_arena, (*it)->IsTeamMember() ? 1 : 0,
							(*it2)->data->id.c_str(), (*it2)->in_arena, (*it2)->IsTeamMember() ? 1 : 0));
						++err_count;
					}
				}
			}
		}
		if(err_count)
			AddGameMsg(Format("%d arena friends errors!", err_count), 10.f);
	}
#endif
}

//=================================================================================================
// Aktualizuje gracza
//=================================================================================================
void Game::UpdatePlayer(LevelContext& ctx, float dt)
{
	Unit& u = *pc->unit;

	/* scaling unit with 9/0
	
	if(Key.Down('0'))
	{
		u.human_data->height += dt;
		if(Key.Down(VK_SHIFT) && u.human_data->height > 1.1f)
			u.human_data->height = 1.1f;
		u.human_data->ApplyScale(u.ani->ani);
		u.ani->need_update = true;
	}
	else if(Key.Down('9'))
	{
		u.human_data->height -= dt;
		if(Key.Down(VK_SHIFT) && u.human_data->height < 0.9f)
			u.human_data->height = 0.9f;
		u.human_data->ApplyScale(u.ani->ani);
		u.ani->need_update = true;
	}*/

	/* sprawdzanie które kafelki wokó³ gracza blokuj¹	
	{
		const float s = SS;
		const int n = NN;

		test_pf.clear();
		global_col.clear();
		BOX2D bo(u.pos.x-s*n/2-s/2,u.pos.z-s*n/2-s/2,u.pos.x+s*n/2+s/2,u.pos.z+s*n/2+s/2);
		IgnoreObjects ignore = {0};
		const Unit* ignore_units[2] = {&u, nullptr};
		ignore.ignored_units = ignore_units;
		GatherCollisionObjects(ctx, global_col, bo, &ignore);

		for(int y=-n/2; y<=n/2; ++y)
		{
			for(int x=-n/2; x<=n/2; ++x)
			{
				BOX2D bo2(u.pos.x-s/2+s*x,u.pos.z-s/2+s*y,u.pos.x+s/2+s*x,u.pos.z+s/2+s*y);
				if(!Collide(global_col, bo2))
					test_pf.push_back(std::pair<VEC2,int>(VEC2(u.pos.x-s/2+s*x, u.pos.z-s/2+s*y), 0));
				else
					test_pf.push_back(std::pair<VEC2,int>(VEC2(u.pos.x-s/2+s*x, u.pos.z-s/2+s*y), 1));
			}
		}
	}*/

	if(!u.IsStanding())
	{
		autowalk = false;
		player_rot_buf = 0.f;
		UnitTryStandup(u, dt);
		return;
	}

	if(u.frozen == 2)
	{
		autowalk = false;
		player_rot_buf = 0.f;
		u.animation = ANI_STAND;
		return;
	}

	if(u.useable)
	{
		if(u.action == A_ANIMATION2 && OR2_EQ(u.animation_state, AS_ANIMATION2_USING, AS_ANIMATION2_USING_SOUND))
		{
			if(KeyPressedReleaseAllowed(GK_ATTACK_USE) || KeyPressedReleaseAllowed(GK_USE))
				Unit_StopUsingUseable(ctx, u);
		}
		UpdatePlayerView();
		player_rot_buf = 0.f;
	}

	u.prev_pos = u.pos;
	u.changed = true;

	bool idle = true, this_frame_run = false;

	if(!u.useable)
	{
		if(u.weapon_taken == W_NONE)
		{
			if(u.animation != ANI_IDLE)
				u.animation = ANI_STAND;
		}
		else if(u.weapon_taken == W_ONE_HANDED)
			u.animation = ANI_BATTLE;
		else if(u.weapon_taken == W_BOW)
			u.animation = ANI_BATTLE_BOW;

		int rotate=0, move=0;
		if(KeyDownAllowed(GK_ROTATE_LEFT))
			--rotate;
		if(KeyDownAllowed(GK_ROTATE_RIGHT))
			++rotate;
		if(u.frozen == 0)
		{
			bool cancel_autowalk = (KeyPressedReleaseAllowed(GK_MOVE_FORWARD) || KeyDownAllowed(GK_MOVE_BACK));
			if(cancel_autowalk)
				autowalk = false;
			else if(KeyDownAllowed(GK_AUTOWALK))
				autowalk = true;

			if(u.run_attack)
			{
				move = 10;
				if(KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(KeyDownAllowed(GK_MOVE_LEFT))
					--move;
			}
			else
			{
				if(KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(KeyDownAllowed(GK_MOVE_LEFT))
					--move;
				if(KeyDownAllowed(GK_MOVE_FORWARD) || autowalk)
					move += 10;
				if(KeyDownAllowed(GK_MOVE_BACK))
					move -= 10;
			}
		}
		else
			autowalk = false;

		if(u.action == A_NONE && !u.talking && KeyPressedReleaseAllowed(GK_YELL))
		{
			if(IsLocal())
				PlayerYell(u);
			else
				PushNetChange(NetChange::YELL);
		}

		if((allow_input == ALLOW_INPUT || allow_input == ALLOW_MOUSE) && !cam.free_rot)
		{
			int div = (pc->unit->action == A_SHOOT ? 800 : 400);
			player_rot_buf *= (1.f-dt*2);
			player_rot_buf  += float(mouse_dif.x) * mouse_sensitivity_f / div;
			if(player_rot_buf > 0.1f)
				player_rot_buf = 0.1f;
			else if(player_rot_buf < -0.1f)
				player_rot_buf = -0.1f;
		}
		else
			player_rot_buf = 0.f;

		const bool rotating = (rotate != 0 || player_rot_buf != 0.f);

		if(rotating || move)
		{
			// rotating by mouse don't affect idle timer
			if(move || rotate)
				idle = false;

			if(rotating)
			{
				const float rot_speed_dt = u.GetRotationSpeed() * dt;
				const float mouse_rot = clamp(player_rot_buf, -rot_speed_dt, rot_speed_dt);
				const float val = rot_speed_dt*rotate + mouse_rot;

				player_rot_buf -= mouse_rot;
				u.rot = clip(u.rot + clamp(val, -rot_speed_dt, rot_speed_dt));

				if(val > 0)
					u.animation = ANI_RIGHT;
				else if(val < 0)
					u.animation = ANI_LEFT;
			}

			if(move)
			{
				// ustal k¹t i szybkoœæ ruchu
				float angle = u.rot;
				bool run = true;
				if(!u.run_attack && (KeyDownAllowed(GK_WALK) || !u.CanRun()))
					run = false;

				switch(move)
				{
				case 10: // przód
					angle += PI;
					break;
				case -10: // ty³
					run = false;
					break;
				case -1: // lewa
					angle += PI/2;
					break;
				case 1: // prawa
					angle += PI*3/2;
					break;
				case 9: // lewa góra
					angle += PI*3/4;
					break;
				case 11: // prawa góra
					angle += PI*5/4;
					break;
				case -11: // lewy ty³
					run = false;
					angle += PI/4;
					break;
				case -9: // prawy ty³
					run = false;
					angle += PI*7/4;
					break;
				}

				if(u.action == A_SHOOT)
					run = false;

				if(run)
					u.animation = ANI_RUN;
				else if(move < -9)
					u.animation = ANI_WALK_TYL;
				else if(move == -1)
					u.animation = ANI_LEFT;
				else if(move == 1)
					u.animation = ANI_RIGHT;
				else
					u.animation = ANI_WALK;

				u.speed = run ? u.GetRunSpeed() : u.GetWalkSpeed();
				u.prev_speed = (u.prev_speed + (u.speed - u.prev_speed)*dt*3);
				float speed = u.prev_speed * dt;

				u.prev_pos = u.pos;

				const VEC3 dir(sin(angle)*speed,0,cos(angle)*speed);
				INT2 prev_tile(int(u.pos.x/2), int(u.pos.z/2));
				bool moved = false;

				if(pc->noclip)
				{
					u.pos += dir;
					moved = true;
				}
				else if(CheckMove(u.pos, dir, u.GetUnitRadius(), &u))
					moved = true;

				if(moved)
				{
					MoveUnit(u);

					// train by moving
					if(IsLocal())
						u.player->TrainMove(dt, run);
					else
					{
						train_move += (run ? dt : dt/10);
						if(train_move >= 1.f)
						{
							--train_move;
							PushNetChange(NetChange::TRAIN_MOVE);
						}
					}
					
					// revealing minimap
					if(!location->outside)
					{
						INT2 new_tile(int(u.pos.x/2), int(u.pos.z/2));
						if(new_tile != prev_tile)
							DungeonReveal(new_tile);
					}
				}

				if(run && abs(u.speed - u.prev_speed) < 0.25f)
					this_frame_run = true;
			}
		}

		if(move == 0)
		{
			u.prev_speed -= dt*3;
			if(u.prev_speed < 0)
				u.prev_speed = 0;
		}
	}

	if(u.action == A_NONE || u.action == A_TAKE_WEAPON || u.CanDoWhileUsing())
	{
		if(KeyPressedReleaseAllowed(GK_TAKE_WEAPON))
		{
			idle = false;
			if(u.weapon_state == WS_TAKEN || u.weapon_state == WS_TAKING)
				u.HideWeapon();
			else
			{
				WeaponType bron = pc->ostatnia;

				// ustal któr¹ broñ wyj¹œæ
				if(bron == W_NONE)
				{
					if(u.HaveWeapon())
						bron = W_ONE_HANDED;
					else if(u.HaveBow())
						bron = W_BOW;
				}
				else if(bron == W_ONE_HANDED)
				{
					if(!u.HaveWeapon())
					{
						if(u.HaveBow())
							bron = W_BOW;
						else
							bron = W_NONE;
					}
				}
				else
				{
					if(!u.HaveBow())
					{
						if(u.HaveWeapon())
							bron = W_ONE_HANDED;
						else
							bron = W_NONE;
					}
				}

				if(bron != W_NONE)
				{
					pc->ostatnia = bron;
					pc->next_action = NA_NONE;
					Inventory::lock_id = LOCK_NO;

					switch(u.weapon_state)
					{
					case WS_HIDDEN:
						// broñ jest schowana, zacznij wyjmowaæ
						u.ani->Play(u.GetTakeWeaponAnimation(bron == W_ONE_HANDED), PLAY_ONCE|PLAY_PRIO1, 1);
						u.weapon_taken = bron;
						u.animation_state = 0;
						u.weapon_state = WS_TAKING;
						u.action = A_TAKE_WEAPON;
						break;
					case WS_HIDING:
						// broñ jest chowana, anuluj chowanie
						if(u.animation_state == 0)
						{
							// jeszcze nie schowa³ broni za pas, wy³¹cz grupê
							u.action = A_NONE;
							u.weapon_taken = u.weapon_hiding;
							u.weapon_hiding = W_NONE;
							pc->ostatnia = u.weapon_taken;
							u.weapon_state = WS_TAKEN;
							u.ani->Deactivate(1);
						}
						else
						{
							// schowa³ broñ za pas, zacznij wyci¹gaæ
							u.weapon_taken = u.weapon_hiding;
							u.weapon_hiding = W_NONE;
							pc->ostatnia = u.weapon_taken;
							u.weapon_state = WS_TAKING;
							u.animation_state = 0;
							CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
						}
						break;
					case WS_TAKING:
						// wyjmuje broñ, anuluj wyjmowanie
						if(u.animation_state == 0)
						{
							// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
							u.action = A_NONE;
							u.weapon_taken = W_NONE;
							u.weapon_state = WS_HIDDEN;
							u.ani->Deactivate(1);
						}
						else
						{
							// wyj¹³ broñ z pasa, zacznij chowaæ
							u.weapon_hiding = u.weapon_taken;
							u.weapon_taken = W_NONE;
							u.weapon_state = WS_HIDING;
							u.animation_state = 0;
							SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
						}
						break;
					case WS_TAKEN:
						// broñ jest wyjêta, zacznij chowaæ
						u.ani->Play(u.GetTakeWeaponAnimation(bron == W_ONE_HANDED), PLAY_ONCE|PLAY_BACK|PLAY_PRIO1, 1);
						u.weapon_hiding = bron;
						u.weapon_taken = W_NONE;
						u.weapon_state = WS_HIDING;
						u.action = A_TAKE_WEAPON;
						u.animation_state = 0;
						break;
					}

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = ((u.weapon_state == WS_HIDDEN || u.weapon_state == WS_HIDING) ? 1 : 0);
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else
				{
					// komunikat o braku broni
					AddGameMsg2(txINeedWeapon, 2.f, GMS_NEED_WEAPON);
				}
			}
		}
		else if(u.HaveWeapon() && KeyPressedReleaseAllowed(GK_MELEE_WEAPON))
		{
			idle = false;
			if(u.weapon_state == WS_HIDDEN)
			{
				// broñ schowana, zacznij wyjmowaæ
				u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE|PLAY_PRIO1, 1);
				u.weapon_taken = pc->ostatnia = W_ONE_HANDED;
				u.weapon_state = WS_TAKING;
				u.action = A_TAKE_WEAPON;
				u.animation_state = 0;
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.weapon_state == WS_HIDING)
			{
				// chowa broñ
				if(u.weapon_hiding == W_ONE_HANDED)
				{
					if(u.animation_state == 0)
					{
						// jeszcze nie schowa³ broni za pas, wy³¹cz grupê
						u.action = A_NONE;
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKEN;
						u.ani->Deactivate(1);
					}
					else
					{
						// schowa³ broñ za pas, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKING;
						u.animation_state = 0;
						CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 0;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else
				{
					// chowa ³uk, dodaj info ¿eby wyj¹³ broñ
					u.weapon_taken = W_ONE_HANDED;
				}
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;
			}
			else if(u.weapon_state == WS_TAKING)
			{
				// wyjmuje broñ
				if(u.weapon_taken == W_BOW)
				{
					// wyjmuje ³uk
					if(u.animation_state == 0)
					{
						// tak na prawdê to jeszcze nic nie zrobi³ wiêc mo¿na anuluowaæ
						u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE|PLAY_PRIO1, 1);
						pc->ostatnia = u.weapon_taken = W_ONE_HANDED;
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->ostatnia = u.weapon_taken = W_ONE_HANDED;
						u.weapon_hiding = W_BOW;
						u.weapon_state = WS_HIDING;
						u.animation_state = 0;
						SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = (u.weapon_state == WS_HIDING ? 1 : 0);
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else
			{
				// broñ wyjêta
				if(u.weapon_taken == W_BOW)
				{
					pc->ostatnia = u.weapon_taken = W_ONE_HANDED;
					u.weapon_hiding = W_BOW;
					u.weapon_state = WS_HIDING;
					u.animation_state = 0;
					u.action = A_TAKE_WEAPON;
					u.ani->Play(NAMES::ani_take_bow, PLAY_BACK|PLAY_ONCE|PLAY_PRIO1, 1);
					pc->next_action = NA_NONE;
					Inventory::lock_id = LOCK_NO;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}
		else if(u.HaveBow() && KeyPressedReleaseAllowed(GK_RANGED_WEAPON))
		{
			idle = false;
			if(u.weapon_state == WS_HIDDEN)
			{
				// broñ schowana, zacznij wyjmowaæ
				u.weapon_taken = pc->ostatnia = W_BOW;
				u.weapon_state = WS_TAKING;
				u.action = A_TAKE_WEAPON;
				u.animation_state = 0;
				u.ani->Play(NAMES::ani_take_bow, PLAY_ONCE|PLAY_PRIO1, 1);
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.weapon_state == WS_HIDING)
			{
				// chowa
				if(u.weapon_hiding == W_BOW)
				{
					if(u.animation_state == 0)
					{
						// jeszcze nie schowa³ ³uku, wy³¹cz grupê
						u.action = A_NONE;
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKEN;
						u.ani->Deactivate(1);
					}
					else
					{
						// schowa³ ³uk, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKING;
						u.animation_state = 0;
						CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				else
				{
					// chowa broñ, dodaj info ¿eby wyj¹³ broñ
					u.weapon_taken = W_BOW;
				}
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = 0;
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else if(u.weapon_state == WS_TAKING)
			{
				// wyjmuje broñ
				if(u.weapon_taken == W_ONE_HANDED)
				{
					// wyjmuje broñ
					if(u.animation_state == 0)
					{
						// tak na prawdê to jeszcze nic nie zrobi³ wiêc mo¿na anuluowaæ
						pc->ostatnia = u.weapon_taken = W_BOW;
						u.ani->Play(NAMES::ani_take_bow, PLAY_ONCE|PLAY_PRIO1, 1);
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->ostatnia = u.weapon_taken = W_BOW;
						u.weapon_hiding = W_ONE_HANDED;
						u.weapon_state = WS_HIDING;
						u.animation_state = 0;
						SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
					}
				}
				pc->next_action = NA_NONE;
				Inventory::lock_id = LOCK_NO;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.unit = pc->unit;
					c.id = (u.weapon_state == WS_HIDING ? 1 : 0);
					c.type = NetChange::TAKE_WEAPON;
				}
			}
			else
			{
				// broñ wyjêta
				if(u.weapon_taken == W_ONE_HANDED)
				{
					u.ani->Play(u.GetTakeWeaponAnimation(true), PLAY_BACK|PLAY_ONCE|PLAY_PRIO1, 1);
					pc->ostatnia = u.weapon_taken = W_BOW;
					u.weapon_hiding = W_ONE_HANDED;
					u.weapon_state = WS_HIDING;
					u.animation_state = 0;
					u.action = A_TAKE_WEAPON;
					pc->next_action = NA_NONE;
					Inventory::lock_id = LOCK_NO;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}

		if(KeyPressedReleaseAllowed(GK_POTION) && !equal(u.hp, u.hpmax))
		{
			idle = false;
			// wypij miksturkê lecznicz¹
			float brakuje = u.hpmax - u.hp;
			int wypij = -1, index = 0;
			float wyleczy;

			for(vector<ItemSlot>::iterator it = u.items.begin(), end = u.items.end(); it != end; ++it, ++index)
			{
				if(!it->item || it->item->type != IT_CONSUMABLE)
					continue;
				const Consumable& pot = it->item->ToConsumable();
				if(pot.effect == E_HEAL)
				{
					if(wypij == -1)
					{
						wypij = index;
						wyleczy = pot.power;
					}
					else
					{
						if(pot.power > brakuje)
						{
							if(pot.power < wyleczy)
							{
								wypij = index;
								wyleczy = pot.power;
							}
						}
						else if(pot.power > wyleczy)
						{
							wypij = index;
							wyleczy = pot.power;
						}
					}
				}
			}

			if(wypij != -1)
				u.ConsumeItem(wypij);
			else
				AddGameMsg2(txNoHpp, 2.f, GMS_NO_POTION);
		}
	} // allow_input == ALLOW_INPUT || allow_input == ALLOW_KEYBOARD

	if(u.useable)
		return;

	// sprawdŸ co jest przed graczem oraz stwórz listê pobliskich wrogów
	before_player = BP_NONE;
	float dist, best_dist = 3.0f, best_dist_target = 3.0f;
	selected_target = nullptr;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		bool mark = false;
		if(IsEnemy(u, u2))
		{
			if(u2.IsAlive())
				mark = true;
		}
		else if(IsFriend(u, u2))
			mark = true;

		dist = distance2d(u.visual_pos, u2.visual_pos);

		// wybieranie postaci
		if(u2.IsStanding())
			PlayerCheckObjectDistance(u, u2.visual_pos, &u2, best_dist, BP_UNIT);
		else if(u2.live_state == Unit::FALL || u2.live_state == Unit::DEAD)
			PlayerCheckObjectDistance(u, u2.GetLootCenter(), &u2, best_dist, BP_UNIT);

		// oznaczanie pobliskich postaci
		if(mark)
		{
			if(dist < ALERT_RANGE.x && cam.frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && CanSee(u, u2))
			{
				// dodaj do pobliskich jednostek
				bool jest = false;
				for(vector<UnitView>::iterator it2 = unit_views.begin(), end2 = unit_views.end(); it2 != end2; ++it2)
				{
					if(it2->unit == *it)
					{
						jest = true;
						it2->valid = true;
						it2->last_pos = u2.GetUnitTextPos();
						break;
					}
				}
				if(!jest)
				{
					UnitView& uv = Add1(unit_views);
					uv.valid = true;
					uv.unit = *it;
					uv.time = 0.f;
					uv.last_pos = u2.GetUnitTextPos();
				}
			}
		}
	}

	// skrzynie przed graczem
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_CHEST);
	}

	// drzwi przed graczem
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if(OR2_EQ((*it)->state, Door::Open, Door::Closed))
				PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_DOOR);
		}
	}

	// przedmioty przed graczem
	for(vector<GroundItem*>::iterator it = ctx.items->begin(), end = ctx.items->end(); it != end; ++it)
		PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_ITEM);

	// u¿ywalne przed graczem
	for(vector<Useable*>::iterator it = ctx.useables->begin(), end = ctx.useables->end(); it != end; ++it)
	{
		if(!(*it)->user)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_USEABLE);
	}

	if(best_dist < best_dist_target && before_player == BP_UNIT && before_player_ptr.unit->IsStanding())
	{
		selected_target = before_player_ptr.unit;
#ifdef DRAW_LOCAL_PATH
		if(Key.Down('K'))
			marked = selected_target;
#endif
	}

	// u¿yj czegoœ przed graczem
	if(u.frozen == 0 && before_player != BP_NONE && (KeyPressedReleaseAllowed(GK_USE) || (u.IsNotFighting() && KeyPressedReleaseAllowed(GK_ATTACK_USE))))
	{
		idle = false;
		if(before_player == BP_UNIT)
		{
			Unit* u2 = before_player_ptr.unit;

			// przed graczem jest jakaœ postaæ
			if(u2->live_state == Unit::DEAD || u2->live_state == Unit::FALL)
			{
				// grabienie zw³ok
				if(u.action != A_NONE)
				{

				}
				else if(u2->live_state == Unit::FALL)
				{
					// nie mo¿na okradaæ osoby która zaraz wstanie
					AddGameMsg2(txCantDo, 3.f, GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// nie mo¿na okradaæ sojuszników
					AddGameMsg2(txDontLootFollower, 3.f, GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->in_arena != -1)
				{
					AddGameMsg2(txDontLootArena, 3.f, GMS_DONT_LOOT_ARENA);
				}
				else if(IsLocal())
				{
					if(IsOnline() && u2->busy == Unit::Busy_Looted)
					{
						// ktoœ ju¿ ograbia zw³oki
						AddGameMsg3(GMS_IS_LOOTED);
					}
					else
					{
						// rozpoczynij wymianê przedmiotów
						pc->action = PlayerController::Action_LootUnit;
						pc->action_unit = u2;
						u2->busy = Unit::Busy_Looted;
						pc->chest_trade = &u2->items;
						StartTrade(I_LOOT_BODY, *u2);
					}
				}
				else
				{
					// wiadomoœæ o wymianie do serwera
					NetChange& c = Add1(net_changes);
					c.type = NetChange::LOOT_UNIT;
					c.id = u2->netid;
					pc->action = PlayerController::Action_LootUnit;
					pc->action_unit = u2;
					pc->chest_trade = &u2->items;
				}
			}
			else if(u2->IsAI() && IsUnitIdle(*u2) && u2->in_arena == -1 && u2->data->dialog && !IsEnemy(u, *u2))
			{
				if(IsLocal())
				{
					if(u2->busy != Unit::Busy_No || !u2->CanTalk())
					{
						// osoba jest czymœ zajêta
						AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// rozpocznij rozmowê
						StartDialog(dialog_context, u2);
						before_player = BP_NONE;
					}
				}
				else
				{
					// wiadomoœæ o rozmowie do serwera
					NetChange& c = Add1(net_changes);
					c.type = NetChange::TALK;
					c.id = u2->netid;
					pc->action = PlayerController::Action_Talk;
					pc->action_unit = u2;
					predialog.clear();
				}
			}
		}
		else if(before_player == BP_CHEST)
		{
			// pl¹drowanie skrzyni
			if(pc->unit->action != A_NONE)
			{

			}
			else if(IsLocal())
			{
				if(before_player_ptr.chest->looted)
				{
					// ktoœ ju¿ zajmuje siê t¹ skrzyni¹
					AddGameMsg3(GMS_IS_LOOTED);
				}
				else
				{
					// rozpocznij ograbianie skrzyni
					pc->action = PlayerController::Action_LootChest;
					pc->action_chest = before_player_ptr.chest;
					pc->action_chest->looted = true;
					pc->chest_trade = &pc->action_chest->items;
					CloseGamePanels();
					inventory_mode = I_LOOT_CHEST;
					BuildTmpInventory(0);
					game_gui->inv_trade_mine->mode = Inventory::LOOT_MY;
					BuildTmpInventory(1);
					game_gui->inv_trade_other->unit = nullptr;
					game_gui->inv_trade_other->items = &pc->action_chest->items;
					game_gui->inv_trade_other->slots = nullptr;
					game_gui->inv_trade_other->title = Inventory::txLootingChest;
					game_gui->inv_trade_other->mode = Inventory::LOOT_OTHER;
					game_gui->gp_trade->Show();

					// animacja / dŸwiêk
					pc->action_chest->ani->Play(&pc->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END, 0);
					if(sound_volume)
					{
						VEC3 pos = pc->action_chest->pos;
						pos.y += 0.5f;
						PlaySound3d(sChestOpen, pos, 2.f, 5.f);
					}

					// event handler
					if(before_player_ptr.chest->handler)
						before_player_ptr.chest->handler->HandleChestEvent(ChestEventHandler::Opened);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHEST_OPEN;
						c.id = pc->action_chest->netid;
					}
				}
			}
			else
			{
				// wyœlij wiadomoœæ o pl¹drowaniu skrzyni
				NetChange& c = Add1(net_changes);
				c.type = NetChange::LOOT_CHEST;
				c.id = before_player_ptr.chest->netid;
				pc->action = PlayerController::Action_LootChest;
				pc->action_chest = before_player_ptr.chest;
				pc->chest_trade = &pc->action_chest->items;
			}
		}
		else if(before_player == BP_DOOR)
		{
			// otwieranie/zamykanie drzwi
			Door* door = before_player_ptr.door;
			if(door->state == Door::Closed)
			{
				// otwieranie drzwi
				if(door->locked == LOCK_NONE)
				{
					if(!location->outside)
						minimap_opened_doors = true;
					door->state = Door::Opening;
					door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
					door->ani->frame_end_info = false;
					if(sound_volume && rand2()%2 == 0)
					{
						// skrzypienie
						VEC3 pos = door->pos;
						pos.y += 1.5f;
						PlaySound3d(sDoor[rand2()%3], pos, 2.f, 5.f);
					}
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::USE_DOOR;
						c.id = door->netid;
						c.ile = 0;
					}
				}
				else
				{
					cstring key;
					switch(door->locked)
					{
					case LOCK_MINE:
						key = "key_kopalnia";
						break;
					case LOCK_ORCS:
						key = "q_orkowie_klucz";
						break;
					case LOCK_UNLOCKABLE:
					default:
						key = nullptr;
						break;
					}

					if(key && pc->unit->HaveItem(FindItem(key)))
					{
						if(sound_volume)
						{
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sUnlock, pos, 2.f, 5.f);
						}
						AddGameMsg2(txUnlockedDoor, 3.f, GMS_UNLOCK_DOOR);
						if(!location->outside)
							minimap_opened_doors = true;
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND, 0);
						door->ani->frame_end_info = false;
						if(sound_volume && rand2()%2 == 0)
						{
							// skrzypienie
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sDoor[rand2()%3], pos, 2.f, 5.f);
						}
						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::USE_DOOR;
							c.id = door->netid;
							c.ile = 0;
						}
					}
					else
					{
						AddGameMsg2(txNeedKey, 3.f, GMS_NEED_KEY);
						if(sound_volume)
						{
							VEC3 pos = door->pos;
							pos.y += 1.5f;
							PlaySound3d(sDoorClosed[rand2()%2], pos, 2.f, 5.f);
						}
					}
				}
			}
			else
			{
				// zamykanie drzwi
				door->state = Door::Closing;
				door->ani->Play(&door->ani->ani->anims[0], PLAY_ONCE|PLAY_STOP_AT_END|PLAY_NO_BLEND|PLAY_BACK, 0);
				door->ani->frame_end_info = false;
				if(sound_volume && rand2()%2 == 0)
				{
					SOUND snd;
					if(rand2()%2 == 0)
						snd = sDoorClose;
					else
						snd = sDoor[rand2()%3];
					VEC3 pos = door->pos;
					pos.y += 1.5f;
					PlaySound3d(snd, pos, 2.f, 5.f);
				}
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::USE_DOOR;
					c.id = door->netid;
					c.ile = 1;
				}
			}
		}
		else if(before_player == BP_ITEM)
		{
			// podnieœ przedmiot
			GroundItem& item = *before_player_ptr.item;
			if(u.action == A_NONE)
			{
				bool u_gory = (item.pos.y > u.pos.y+0.5f);

				u.action = A_PICKUP;
				u.animation = ANI_PLAY;
				u.ani->Play(u_gory ? "podnosi_gora" : "podnosi", PLAY_ONCE|PLAY_PRIO2, 0);
				u.ani->frame_end_info = false;

				if(IsLocal())
				{
					AddItem(u, item);

					if(item.item->type == IT_GOLD && sound_volume)
						PlaySound2d(sCoins);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::PICKUP_ITEM;
						c.unit = pc->unit;
						c.ile = (u_gory ? 1 : 0);

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::REMOVE_ITEM;
						c2.id = before_player_ptr.item->netid;
					}

					DeleteElement(ctx.items, before_player_ptr.item);
					before_player = BP_NONE;
				}
				else
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::PICKUP_ITEM;
					c.id = item.netid;

					picking_item = &item;
					picking_item_state = 1;
				}
			}
		}
		else if(u.action == A_NONE)
			PlayerUseUseable(before_player_ptr.useable, false);
	}

	if(before_player == BP_UNIT)
		selected_unit = before_player_ptr.unit;
	else
		selected_unit = nullptr;
		
	// atak
	if(u.weapon_state == WS_TAKEN)
	{
		idle = false;
		if(u.weapon_taken == W_ONE_HANDED)
		{
			if(u.action == A_ATTACK)
			{
				if(u.animation_state == 0)
				{
					if(KeyUpAllowed(pc->action_key))
					{
						u.attack_power = u.ani->groups[1].time / u.GetAttackFrame(0);
						u.animation_state = 1;
						u.ani->groups[1].speed = u.attack_power + u.GetAttackSpeed();
						u.attack_power += 1.f;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Attack;
							c.f[1] = u.ani->groups[1].speed;
						}

						if(IsLocal())
							u.player->Train(TrainWhat::AttackStart, 0.f, 0);
					}
				}
				else if(u.animation_state == 2)
				{
					byte k = KeyDoReturn(GK_ATTACK_USE, &KeyStates::Down);
					if(k != VK_NONE)
					{
						u.action = A_ATTACK;
						u.attack_id = u.GetRandomAttack();
						u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
						u.ani->groups[1].speed = u.GetPowerAttackSpeed();
						pc->action_key = k;
						u.animation_state = 0;
						u.run_attack = false;
						u.hitted = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PowerAttack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
				}
			}
			else if(u.action == A_BLOCK)
			{
				if(KeyUpAllowed(pc->action_key))
				{
					// skoñcz blokowaæ
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StopBlock;
						c.f[1] = 1.f;
					}
				}
				else if(!u.ani->groups[1].IsBlending() && u.HaveShield())
				{
					if(KeyDownAllowed(GK_ATTACK_USE))
					{
						// uderz tarcz¹
						u.action = A_BASH;
						u.animation_state = 0;
						u.ani->Play(NAMES::ani_bash, PLAY_ONCE|PLAY_PRIO1|PLAY_RESTORE, 1);
						u.ani->groups[1].speed = 2.f;
						u.ani->frame_end_info2 = false;
						u.hitted = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Bash;
							c.f[1] = 2.f;
						}

						if(IsLocal())
							u.player->Train(TrainWhat::BashStart, 0.f, 0);
					}
				}
			}
			else if(u.action == A_NONE && u.frozen == 0)
			{
				byte k = KeyDoReturnIgnore(GK_ATTACK_USE, &KeyStates::Down, pc->wasted_key);
				if(k != VK_NONE)
				{
					u.action = A_ATTACK;
					u.attack_id = u.GetRandomAttack();
					u.ani->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
					if(this_frame_run)
					{
						// atak w biegu
						u.ani->groups[1].speed = u.GetAttackSpeed();
						u.animation_state = 1;
						u.run_attack = true;
						u.attack_power = 1.5f;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_RunningAttack;
							c.f[1] = u.ani->groups[1].speed;
						}

						if(IsLocal())
							u.player->Train(TrainWhat::AttackStart, 0.f, 0);
					}
					else
					{
						// normalny/potê¿ny atak
						u.ani->groups[1].speed = u.GetPowerAttackSpeed();
						pc->action_key = k;
						u.animation_state = 0;
						u.run_attack = false;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PowerAttack;
							c.f[1] = u.ani->groups[1].speed;
						}
					}
					u.hitted = false;
				}
			}
			if(u.frozen == 0 && u.HaveShield() && !u.run_attack && (u.action == A_NONE || u.action == A_ATTACK))
			{
				int oks = 0;
				if(u.action == A_ATTACK)
				{
					if(u.run_attack || (u.attack_power > 1.5f && u.animation_state == 1))
						oks = 1;
					else
						oks = 2;
				}

				if(oks != 1)
				{
					byte k = KeyDoReturn(GK_BLOCK, &KeyStates::Pressed);
					if(k != VK_NONE)
					{
						u.action = A_BLOCK;
						u.ani->Play(NAMES::ani_block, PLAY_PRIO1|PLAY_STOP_AT_END|PLAY_RESTORE, 1);
						u.ani->groups[1].blend_max = (oks == 2 ? 0.33f : u.GetBlockSpeed());
						pc->action_key = k;
						u.animation_state = 0;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Block;
							c.f[1] = u.ani->groups[1].blend_max;
						}
					}
				}
			}
		}
		else
		{
			// atak z ³uku
			if(u.action == A_SHOOT)
			{
				if(u.animation_state == 0 && KeyUpAllowed(pc->action_key))
				{
					u.animation_state = 1;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_Shoot;
						c.f[1] = 1.f;
					}
				}
			}
			else if(u.frozen == 0)
			{
				byte k = KeyDoReturnIgnore(GK_ATTACK_USE, &KeyStates::Down, pc->wasted_key);
				if(k != VK_NONE)
				{
					float speed = u.GetBowAttackSpeed();
					u.ani->Play(NAMES::ani_shoot, PLAY_PRIO1|PLAY_ONCE|PLAY_RESTORE, 1);
					u.ani->groups[1].speed = speed;
					u.action = A_SHOOT;
					u.animation_state = 0;
					u.hitted = false;
					pc->action_key = k;
					u.bow_instance = GetBowInstance(u.GetBow().mesh);
					u.bow_instance->Play(&u.bow_instance->ani->anims[0], PLAY_ONCE|PLAY_PRIO1|PLAY_NO_BLEND|PLAY_RESTORE, 0);
					u.bow_instance->groups[0].speed = speed;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StartShoot;
						c.f[1] = speed;
					}
				}
			}
		}
	}

	// animacja idle
	if(idle && u.action == A_NONE)
	{
		pc->idle_timer += dt;
		if(pc->idle_timer >= 4.f)
		{
			if(u.animation == ANI_LEFT || u.animation == ANI_RIGHT)
				pc->idle_timer = random(2.f,3.f);
			else
			{
				int id = rand2()%u.data->idles->size();
				pc->idle_timer = random(0.f,0.5f);
				u.ani->Play(u.data->idles->at(id).c_str(), PLAY_ONCE, 0);
				u.ani->frame_end_info = false;
				u.animation = ANI_IDLE;

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::IDLE;
					c.unit = pc->unit;
					c.id = id;
				}
			}
		}
	}
	else
		pc->idle_timer = random(0.f,0.5f);
}

void Game::PlayerCheckObjectDistance(Unit& u, const VEC3& pos, void* ptr, float& best_dist, BeforePlayer type)
{
	assert(ptr);

	if(pos.y < u.pos.y-0.5f || pos.y > u.pos.y+2.f)
		return;

	float dist = distance2d(u.pos, pos);
	if(dist < PICKUP_RANGE && dist < best_dist)
	{
		float angle = angle_dif(clip(u.rot+PI/2), clip(-angle2d(u.pos, pos)));
		assert(angle >= 0.f);
		if(angle < PI/4)
		{
			if(type == BP_CHEST)
			{
				Chest* chest = (Chest*)ptr;
				if(angle_dif(clip(chest->rot-PI/2), clip(-angle2d(u.pos, pos))) > PI/2)
					return;
			}
			dist += angle;
			if(dist < best_dist)
			{
				best_dist = dist;
				before_player_ptr.any = ptr;
				before_player = type;
			}
		}
	}
}

const float SMALL_DISTANCE = 0.001f;

struct Clbk : public btCollisionWorld::ContactResultCallback
{
	btCollisionObject* ignore;
	bool hit;

	explicit Clbk(btCollisionObject* _ignore) : ignore(_ignore), hit(false)
	{

	}

	btScalar addSingleResult(btManifoldPoint& cp,	const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
	{
		hit = true;
		return 0.f;
	}
};

int Game::CheckMove(VEC3& _pos, const VEC3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	VEC3 new_pos = _pos + _dir;
	VEC3 gather_pos = _pos + _dir/2;
	float gather_radius = D3DXVec3Length(&_dir) + _radius;
	global_col.clear();

	IgnoreObjects ignore = {0};
	Unit* ignored[] = {_me, nullptr};
	ignore.ignored_units = (const Unit**)ignored;
	GatherCollisionObjects(GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ po x
	VEC3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
}

int Game::CheckMovePhase(VEC3& _pos, const VEC3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	VEC3 new_pos = _pos + _dir;
	VEC3 gather_pos = _pos + _dir/2;
	float gather_radius = D3DXVec3Length(&_dir) + _radius;

	global_col.clear();

	IgnoreObjects ignore = {0};
	Unit* ignored[] = {_me, nullptr};
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignore_objects = true;
	GatherCollisionObjects(GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ po x
	VEC3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// jeœli zablokowa³ siê w innej jednostce to wyjdŸ z niej
	if(Collide(global_col, _pos, _radius))
	{
		if(is_small)
			*is_small = (distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 4;
	}

	// nie ma drogi
	return 0;
}

void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const VEC3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x-_radius)/2)),
		maxx = int((_pos.x+_radius)/2),
		minz = max(0, int((_pos.z-_radius)/2)),
		maxz = int((_pos.z+_radius)/2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					if(tiles[x+z*OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					POLE co = lvl.map[x + z*lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.check = &Game::CollideWithStairs;
						co.check_rect = &Game::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
					}
				}
			}
		}
	}

	// units
	float radius;
	VEC3 pos;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it || !(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do 
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
				++u;
			} while (1);

			radius = (*it)->GetUnitRadius();
			pos = (*it)->GetColliderPos();
			if(distance(pos.x, pos.z, _pos.x, _pos.z) <= radius+_radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2(pos.x, pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}

jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it || !(*it)->IsStanding())
				continue;

			radius = (*it)->GetUnitRadius();
			VEC3 pos = (*it)->GetColliderPos();
			if(distance(pos.x, pos.z, _pos.x, _pos.z) <= radius+_radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2(pos.x, pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}
		}
	}

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->type == CollisionObject::RECTANGLE)
			{
				if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
					_objects.push_back(*it);
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
						goto ignoruj;
					else if(!*objs)
						break;
					else
						++objs;
				}
				while(1);
			}

			if(it->type == CollisionObject::RECTANGLE)
			{
				if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
					_objects.push_back(*it);
			}

ignoruj:
			;
		}
	}

	// drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if((*it)->IsBlocking() && CircleToRotatedRectangle(_pos.x, _pos.z, _radius, (*it)->pos.x, (*it)->pos.z, 0.842f, 0.181f, (*it)->rot))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = 0.842f;
				co.h = 0.181f;
				co.rot = (*it)->rot;
			}
		}
	}
}

void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const BOX2D& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x/2)),
		maxx = int(_box.v2.x/2),
		minz = max(0, int(_box.v1.y/2)),
		maxz = int(_box.v2.y/2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					if(tiles[x+z*OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z=minz; z<=maxz; ++z)
			{
				for(int x=minx; x<=maxx; ++x)
				{
					POLE co = lvl.map[x + z*lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = VEC2(2.f*x+1.f, 2.f*z+1.f);
						co.check = &Game::CollideWithStairs;
						co.check_rect = &Game::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
					}
				}
			}
		}
	}

	VEC2 rectpos = _box.Midpoint(),
		 rectsize = _box.Size();

	// units
	float radius;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do 
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
				++u;
			} while (1);

			radius = (*it)->GetUnitRadius();
			if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}

jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!(*it)->IsStanding())
				continue;

			radius = (*it)->GetUnitRadius();
			if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = VEC2((*it)->pos.x, (*it)->pos.z);
				co.radius = radius;
				co.type = CollisionObject::SPHERE;
			}
		}
	}

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->type == CollisionObject::RECTANGLE)
			{
				if(RectangleToRectangle(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
					_objects.push_back(*it);
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
						goto ignoruj;
					else if(!*objs)
						break;
					else
						++objs;
				}
				while(1);
			}

			if(it->type == CollisionObject::RECTANGLE)
			{
				if(RectangleToRectangle(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
					_objects.push_back(*it);
			}
			else
			{
				if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
					_objects.push_back(*it);
			}

ignoruj:
			;
		}
	}
}

bool Game::Collide(const vector<CollisionObject>& _objects, const VEC3& _pos, float _radius)
{
	assert(_radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(distance(_pos.x, _pos.z, it->pt.x, it->pt.y) <= it->radius+_radius)
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			if(CircleToRotatedRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h, it->rot))
				return true;
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check))(*it, _pos, _radius))
				return true;
			break;
		}
	}

	return false;
}

struct AnyContactCallback : public btCollisionWorld::ContactResultCallback
{
	bool hit;

	AnyContactCallback() : hit(false)
	{

	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		hit = true;
		return 0.f;
	}
};

bool Game::Collide(const vector<CollisionObject>& _objects, const BOX2D& _box, float _margin)
{
	BOX2D box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	VEC2 rectpos = _box.Midpoint(),
		 rectsize = _box.Size()/2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pt.x, it->pt.y, it->radius+_margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w+_margin;
				r1.size.y = it->h+_margin;
				r1.rot = -it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = 0.f;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

bool Game::Collide(const vector<CollisionObject>& _objects, const BOX2D& _box, float margin, float _rot)
{
	if(!not_zero(_rot))
		return Collide(_objects, _box, margin);

	BOX2D box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	VEC2 rectpos = box.Midpoint(),
		 rectsize = box.Size()/2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRotatedRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y, _rot))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = 0.f;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

void Game::AddConsoleMsg(cstring _msg)
{
	console->AddText(_msg);
}

void Game::StartDialog(DialogContext& ctx, Unit* talker, GameDialog* dialog)
{
	assert(talker && !ctx.dialog_mode);

	// use up auto talk
	if((talker->auto_talk == AutoTalkMode::Yes || talker->auto_talk == AutoTalkMode::Wait) && talker->auto_talk_dialog == nullptr)
		talker->auto_talk = AutoTalkMode::No;

	ctx.dialog_mode = true;
	ctx.dialog_wait = -1;
	ctx.dialog_pos = 0;
	ctx.show_choices = false;
	ctx.dialog_text = nullptr;
	ctx.dialog_level = 0;
	ctx.dialog_once = true;
	ctx.dialog_quest = nullptr;
	ctx.dialog_skip = -1;
	ctx.dialog_esc = -1;
	ctx.talker = talker;
	ctx.talker->busy = Unit::Busy_Talking;
	ctx.talker->look_target = ctx.pc->unit;
	ctx.update_news = true;
	ctx.update_locations = 1;
	ctx.pc->action = PlayerController::Action_Talk;
	ctx.pc->action_unit = talker;
	ctx.not_active = false;
	ctx.choices.clear();
	ctx.can_skip = true;
	ctx.dialog = dialog ? dialog : talker->data->dialog;

	if(IsLocal())
	{
		// dŸwiêk powitania
		SOUND snd = GetTalkSound(*ctx.talker);
		if(snd)
		{
			if(sound_volume)
				PlayAttachedSound(*ctx.talker, snd, 2.f, 5.f);
			if(IsOnline() && IsServer())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::HELLO;
				c.unit = ctx.talker;
			}
		}
	}

	if(ctx.is_local)
	{
		// zamknij gui
		CloseAllPanels();
	}
}

void Game::EndDialog(DialogContext& ctx)
{
	ctx.choices.clear();
	ctx.prev.clear();
	ctx.dialog_mode = false;

	if(ctx.talker->busy == Unit::Busy_Trading)
	{
		if(!ctx.is_local)
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::END_DIALOG;
			c.pc = ctx.pc;
			GetPlayerInfo(c.pc->id).NeedUpdate();
		}

		return;
	}

	ctx.talker->busy = Unit::Busy_No;
	ctx.talker->look_target = nullptr;
	ctx.pc->action = PlayerController::Action_None;

	if(!ctx.is_local)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::END_DIALOG;
		c.pc = ctx.pc;
		GetPlayerInfo(c.pc->id).NeedUpdate();
	}
}

void Game::StartNextDialog(DialogContext& ctx, GameDialog* dialog, int& if_level, Quest* quest)
{
	assert(dialog);

	ctx.prev.push_back({ ctx.dialog, ctx.dialog_quest, ctx.dialog_pos, ctx.dialog_level });
	ctx.dialog = dialog;
	ctx.dialog_quest = quest;
	ctx.dialog_pos = -1;
	ctx.dialog_level = 0;
	if_level = 0;
}

//							WEAPON	BOW		SHIELD	ARMOR	LETTER	POTION	GOLD	OTHER
bool merchant_buy[]	  =	{	true,	true,	true,	true,	true,	true,	false,	true	};
bool blacksmith_buy[] =	{	true,	true,	true,	true,	false,	false,	false,	false	};
bool alchemist_buy[]  = {	false,	false,	false,	false,	false,	true,	false,	false	};
bool innkeeper_buy[]  = {	false,	false,	false,	false,	false,	true,	false,	false	};
bool foodseller_buy[] = {	false,	false,	false,	false,	false,	true,	false,	false	};

void Game::UpdateGameDialog(DialogContext& ctx, float dt)
{
	current_dialog = &ctx;

	// wyœwietlono opcje dialogowe, wybierz jedn¹ z nich (w mp czekaj na wybór)
	if(ctx.show_choices)
	{
		bool ok = false;
		if(!ctx.is_local)
		{
			if(ctx.choice_selected != -1)
				ok = true;
		}
		else
			ok = game_gui->UpdateChoice(ctx, ctx.choices.size());

		if(ok)
		{
			cstring msg = ctx.choices[ctx.choice_selected].msg;
			game_gui->AddSpeechBubble(ctx.pc->unit, msg);

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::TALK;
				c.unit = ctx.pc->unit;
				c.str = StringPool.Get();
				*c.str = msg;
				c.id = 0;
				c.ile = 0;
				net_talk.push_back(c.str);
			}

			ctx.show_choices = false;
			ctx.dialog_pos = ctx.choices[ctx.choice_selected].pos;
			ctx.dialog_level = ctx.choices[ctx.choice_selected].lvl;
			ctx.choices.clear();
			ctx.choice_selected = -1;
			ctx.dialog_esc = -1;
		}
		else
			return;
	}

	if(ctx.dialog_wait > 0.f)
	{
		if(ctx.is_local)
		{
			bool skip = false;
			if(ctx.can_skip)
			{
				if(KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || (AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
				skip = true;
				else
				{
					pc->wasted_key = KeyDoReturn(GK_ATTACK_USE, &KeyStates::PressedRelease);
					if(pc->wasted_key != VK_NONE)
						skip = true;
				}
			}
			
			if(skip)
				ctx.dialog_wait = -1.f;
			else
				ctx.dialog_wait -= dt;
		}
		else
		{
			if(ctx.choice_selected == 1)
			{
				ctx.dialog_wait = -1.f;
				ctx.choice_selected = -1;
			}
			else
				ctx.dialog_wait -= dt;
		}

		if(ctx.dialog_wait > 0.f)
			return;
	}

	ctx.can_skip = true;
	if(ctx.dialog_skip != -1)
	{
		ctx.dialog_pos = ctx.dialog_skip;
		ctx.dialog_skip = -1;
	}

	int if_level = ctx.dialog_level;

	while(1)
	{
		DialogEntry& de = *(ctx.dialog->code.data() + ctx.dialog_pos);

		switch(de.type)
		{
		case DT_CHOICE:
			if(if_level == ctx.dialog_level)
			{
				cstring text = ctx.GetText((int)de.msg);
				if(text[0] == '$')
				{
					if(strncmp(text, "$player", 7) == 0)
					{
						int id = int(text[7]-'1');
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, near_players_str[id].c_str(), ctx.dialog_level+1));
					}
					else
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, "!Broken choice!", ctx.dialog_level+1));
				}
				else
					ctx.choices.push_back(DialogChoice(ctx.dialog_pos+1, text, ctx.dialog_level+1));
			}
			++if_level;
			break;
		case DT_END_CHOICE:
		case DT_END_IF:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			--if_level;
			break;
		case DT_END:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.prev.empty())
				{
					EndDialog(ctx);
					return;
				}
				else
				{
					auto& prev = ctx.prev.back();
					ctx.dialog = prev.dialog;
					ctx.dialog_pos = prev.pos;
					ctx.dialog_level = prev.level;
					ctx.dialog_quest = prev.quest;
					ctx.prev.pop_back();
					if_level = ctx.dialog_level;
				}
			}
			break;
		case DT_END2:
			if(if_level == ctx.dialog_level)
			{
				EndDialog(ctx);
				return;
			}
			break;
		case DT_SHOW_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				ctx.show_choices = true;
				if(ctx.is_local)
				{
					ctx.choice_selected = 0;
					dialog_cursor_pos = INT2(-1,-1);
					game_gui->UpdateScrollbar(ctx.choices.size());
				}
				else
				{
					ctx.choice_selected = -1;
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::SHOW_DIALOG_CHOICES;
					c.pc = ctx.pc;
					GetPlayerInfo(c.pc->id).NeedUpdate();
				}
				return;
			}
			break;
		case DT_RESTART:
			if(if_level == ctx.dialog_level)
				ctx.dialog_pos = -1;
			break;
		case DT_TRADE:
			if(if_level == ctx.dialog_level)
			{
				Unit* t = ctx.talker;
				t->busy = Unit::Busy_Trading;
				EndDialog(ctx);
				ctx.pc->action = PlayerController::Action_Trade;
				ctx.pc->action_unit = t;
				bool* old_trader_buy = trader_buy;

				const string& id = t->data->id;

				if(id == "blacksmith")
				{
					ctx.pc->chest_trade = &chest_blacksmith;
					trader_buy = blacksmith_buy;
				}
				else if(id == "merchant" || id == "tut_czlowiek")
				{
					ctx.pc->chest_trade = &chest_merchant;
					trader_buy = merchant_buy;
				}
				else if(id == "alchemist")
				{
					ctx.pc->chest_trade = &chest_alchemist;
					trader_buy = alchemist_buy;
				}
				else if(id == "innkeeper")
				{
					ctx.pc->chest_trade = &chest_innkeeper;
					trader_buy = innkeeper_buy;
				}
				else if(id == "q_orkowie_kowal")
				{
					ctx.pc->chest_trade = &quest_orcs2->wares;
					trader_buy = blacksmith_buy;
				}
				else if(id == "food_seller")
				{
					ctx.pc->chest_trade = &chest_food_seller;
					trader_buy = foodseller_buy;
				}
				else
				{
					assert(0);
					return;
				}

				if(ctx.is_local)
					StartTrade(I_TRADE, *ctx.pc->chest_trade, t);
				else
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::START_TRADE;
					c.pc = ctx.pc;
					c.id = t->netid;
					GetPlayerInfo(c.pc->id).NeedUpdate();

					trader_buy = old_trader_buy;
				}

				ctx.pc->Train(TrainWhat::Trade, 0.f, 0);
				
				return;
			}
			break;
		case DT_TALK:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.GetText((int)de.msg);
				DialogTalk(ctx, msg);
				++ctx.dialog_pos;
				return;
			}
			break;
		case DT_TALK2:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.GetText((int)de.msg);

				static string str_part;
				ctx.dialog_s_text.clear();

				for(uint i=0, len = strlen(msg); i < len; ++i)
				{
					if(msg[i] == '$')
					{
						str_part.clear();
						++i;
						while(msg[i] != '$')
						{
							str_part.push_back(msg[i]);
							++i;
						}
						if(ctx.dialog_quest)
							ctx.dialog_s_text += ctx.dialog_quest->FormatString(str_part);
						else
							ctx.dialog_s_text += FormatString(ctx, str_part);
					}
					else
						ctx.dialog_s_text.push_back(msg[i]);
				}

				DialogTalk(ctx, ctx.dialog_s_text.c_str());
				++ctx.dialog_pos;
				return;
			}
			break;
		case DT_SPECIAL:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				if(strcmp(msg, "burmistrz_quest") == 0)
				{
					bool have_quest = true;
					if(city_ctx->quest_mayor == CityQuestState::Failed)
					{
						DialogTalk(ctx, random_string(txMayorQFailed));
						++ctx.dialog_pos;
						return;
					}
					else if(worldtime - city_ctx->quest_mayor_time > 30 || city_ctx->quest_mayor_time == -1)
					{
						if(city_ctx->quest_mayor == CityQuestState::InProgress)
						{
							Quest* quest = FindUnacceptedQuest(current_location, Quest::Type::Mayor);
							DeleteElement(unaccepted_quests, quest);
						}

						// jest nowe zadanie (mo¿e), czas starego min¹³
						city_ctx->quest_mayor_time = worldtime;
						city_ctx->quest_mayor = CityQuestState::InProgress;

						int force = -1;
						if(DEBUG_BOOL && Key.Focus() && Key.Down('G'))
						{
							if(Key.Down('0'))
								force = 0;
							else if(Key.Down('1'))
								force = 1;
							else if(Key.Down('2'))
								force = 2;
							else if(Key.Down('3'))
								force = 3;
							else if(Key.Down('4'))
								force = 4;
						}
						Quest* quest = quest_manager.GetMayorQuest(force);

						if(quest)
						{
							// add new quest
							quest->refid = quest_counter;
							++quest_counter;
							quest->Start();
							unaccepted_quests.push_back(quest);
							StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
						}
						else
							have_quest = false;
					}
					else if(city_ctx->quest_mayor == CityQuestState::InProgress)
					{
						// ju¿ ma przydzielone zadanie ?
						Quest* quest = FindUnacceptedQuest(current_location, Quest::Type::Mayor);
						if(quest)
						{
							// quest nie zosta³ zaakceptowany
							StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
						}
						else
						{
							quest = FindQuest(current_location, Quest::Type::Mayor);
							if(quest)
							{
								DialogTalk(ctx, random_string(txQuestAlreadyGiven));
								++ctx.dialog_pos;
								return;
							}
							else
								have_quest = false;
						}
					}
					else
						have_quest = false;
					if(!have_quest)
					{
						DialogTalk(ctx, random_string(txMayorNoQ));
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strcmp(msg, "dowodca_quest") == 0)
				{
					bool have_quest = true;
					if(city_ctx->quest_captain == CityQuestState::Failed)
					{
						DialogTalk(ctx, random_string(txCaptainQFailed));
						++ctx.dialog_pos;
						return;
					}
					else if(worldtime - city_ctx->quest_captain_time > 30 || city_ctx->quest_captain_time == -1)
					{
						if(city_ctx->quest_captain == CityQuestState::InProgress)
						{
							Quest* quest = FindUnacceptedQuest(current_location, Quest::Type::Captain);
							DeleteElement(unaccepted_quests, quest);
						}

						// jest nowe zadanie (mo¿e), czas starego min¹³
						city_ctx->quest_captain_time = worldtime;
						city_ctx->quest_captain = CityQuestState::InProgress;

						int force = -1;
						if(DEBUG_BOOL && Key.Focus() && Key.Down('G'))
						{
							if(Key.Down('0'))
								force = 0;
							else if(Key.Down('1'))
								force = 1;
							else if(Key.Down('2'))
								force = 2;
							else if(Key.Down('3'))
								force = 3;
							else if(Key.Down('4'))
								force = 4;
							else if(Key.Down('5'))
								force = 5;
						}
						Quest* quest = quest_manager.GetCaptainQuest(force);

						if(quest)
						{
							// add new quest
							quest->refid = quest_counter;
							++quest_counter;
							quest->Start();
							unaccepted_quests.push_back(quest);
							StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
						}
						else
							have_quest = false;
					}
					else if(city_ctx->quest_captain == CityQuestState::InProgress)
					{
						// ju¿ ma przydzielone zadanie
						Quest* quest = FindUnacceptedQuest(current_location, Quest::Type::Captain);
						if(quest)
						{
							// quest nie zosta³ zaakceptowany
							StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
						}
						else
						{
							quest = FindQuest(current_location, Quest::Type::Captain);
							if(quest)
							{
								DialogTalk(ctx, random_string(txQuestAlreadyGiven));
								++ctx.dialog_pos;
								return;
							}
							else
								have_quest = false;
						}
					}
					else
						have_quest = false;
					if(!have_quest)
					{
						DialogTalk(ctx, random_string(txCaptainNoQ));
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strcmp(msg, "przedmiot_quest") == 0)
				{
					if(ctx.talker->quest_refid == -1)
					{
						int force = -1;
						if(DEBUG_BOOL && Key.Focus() && Key.Down('G'))
						{
							if(Key.Down('1'))
								force = 1;
							else if(Key.Down('2'))
								force = 2;
							else if(Key.Down('3'))
								force = 3;
						}
						Quest* quest = quest_manager.GetAdventurerQuest(force);

						quest->refid = quest_counter;
						ctx.talker->quest_refid = quest_counter;
						++quest_counter;
						quest->Start();
						unaccepted_quests.push_back(quest);
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
					}
					else
					{
						Quest* quest = FindUnacceptedQuest(ctx.talker->quest_refid);
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
					}
				}
				else if(strcmp(msg, "arena_combat1") == 0 || strcmp(msg, "arena_combat2") == 0 || strcmp(msg, "arena_combat3") == 0)
					StartArenaCombat(msg[strlen("arena_combat")]-'0');
				else if(strcmp(msg, "rest1") == 0 || strcmp(msg, "rest5") == 0 || strcmp(msg, "rest10") == 0 || strcmp(msg, "rest30") == 0)
				{
					// odpoczynek w karczmie
					// ustal ile to dni i ile kosztuje
					int ile, koszt;
					if(strcmp(msg, "rest1") == 0)
					{
						ile = 1;
						koszt = 5;
					}
					else if(strcmp(msg, "rest5") == 0)
					{
						ile = 5;
						koszt = 20;
					}
					else if(strcmp(msg, "rest10") == 0)
					{
						ile = 10;
						koszt = 35;
					}
					else
					{
						ile = 30;
						koszt = 100;
					}

					// czy gracz ma z³oto?
					if(ctx.pc->unit->gold < koszt)
					{
						ctx.dialog_s_text = Format(txNeedMoreGold, koszt-ctx.pc->unit->gold);
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						// resetuj dialog
						ctx.dialog_pos = 0;
						ctx.dialog_level = 0;
						return;
					}

					// zabierz z³oto i zamróŸ
					ctx.pc->unit->gold -= koszt;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_REST;
						fallback_t = -1.f;
						fallback_1 = ile;
					}
					else
					{
						// wyœlij info o odpoczynku
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::REST;
						c.pc = ctx.pc;
						c.id = ile;
						GetPlayerInfo(ctx.pc).NeedUpdateAndGold();
					}
				}
				else if(strcmp(msg, "gossip") == 0 || strcmp(msg, "gossip_drunk") == 0)
				{
					bool pijak = (strcmp(msg, "gossip_drunk") == 0);
					if(!pijak && (rand2()%3 == 0 || (Key.Down(VK_SHIFT) && devmode)))
					{
						int co = rand2()%3;
						if(quest_rumor_counter && rand2()%2 == 0)
							co = 2;
						if(devmode)
						{
							if(Key.Down('1'))
								co = 0;
							else if(Key.Down('2'))
								co = 1;
							else if(Key.Down('3'))
								co = 2;
						}
						switch(co)
						{
						case 0:
							// info o nieznanej lokacji
							{
								int id = rand2()%locations.size();
								int id2 = id;
								bool ok = false;
								do
								{
									if(locations[id2] && locations[id2]->state == LS_UNKNOWN)
									{
										ok = true;
										break;
									}
									else
									{
										++id2;
										if(id2 == locations.size())
											id2 = 0;
									}
								}
								while(id != id2);
								if(ok)
								{
									Location& loc = *locations[id2];
									loc.state = LS_KNOWN;
									Location& cloc = *locations[current_location];
									cstring s_daleko;
									float dist = distance(loc.pos, cloc.pos);
									if(dist <= 300)
										s_daleko = txNear;
									else if(dist <= 500)
										s_daleko = "";
									else if(dist <= 700)
										s_daleko = txFar;
									else
										s_daleko = txVeryFar;

									ctx.dialog_s_text = Format(random_string(txLocationDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
									DialogTalk(ctx, ctx.dialog_s_text.c_str());
									++ctx.dialog_pos;

									if(IsOnline())
										Net_ChangeLocationState(id2, false);
									return;
								}
								else
								{
									DialogTalk(ctx, random_string(txAllDiscovered));
									++ctx.dialog_pos;
									return;
								}
							}
							break;
						case 1:
							// info o obozie
							{
								Location* new_camp = nullptr;
								static vector<Location*> camps;
								for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
								{
									if(*it && (*it)->type == L_CAMP && (*it)->state != LS_HIDDEN)
									{
										camps.push_back(*it);
										if((*it)->state == LS_UNKNOWN && !new_camp)
											new_camp = *it;
									}
								}

								if(!new_camp && !camps.empty())
									new_camp = camps[rand2()%camps.size()];

								camps.clear();

								if(new_camp)
								{
									Location& loc = *new_camp;
									Location& cloc = *locations[current_location];
									cstring s_daleko;
									float dist = distance(loc.pos, cloc.pos);
									if(dist <= 300)
										s_daleko = txNear;
									else if(dist <= 500)
										s_daleko = "";
									else if(dist <= 700)
										s_daleko = txFar;
									else
										s_daleko = txVeryFar;

									cstring co;
									switch(loc.spawn)
									{
									case SG_ORKOWIE:
										co = txSGOOrcs;
										break;
									case SG_BANDYCI:
										co = txSGOBandits;
										break;
									case SG_GOBLINY:
										co = txSGOGoblins;
										break;
									default:
										assert(0);
										co = txSGOEnemies;
										break;
									}

									if(loc.state == LS_UNKNOWN)
									{
										loc.state = LS_KNOWN;
										if(IsOnline())
											Net_ChangeLocationState(FindLocationId(new_camp), false);
									}

									ctx.dialog_s_text = Format(random_string(txCampDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), co);
									DialogTalk(ctx, ctx.dialog_s_text.c_str());
									++ctx.dialog_pos;
									return;
								}
								else
								{
									DialogTalk(ctx, random_string(txAllCampDiscovered));
									++ctx.dialog_pos;
									return;
								}
							}
							break;
						case 2:
							// plotka o quescie
							if(quest_rumor_counter == 0)
							{
								DialogTalk(ctx, random_string(txNoQRumors));
								++ctx.dialog_pos;
								return;
							}
							else
							{
								co = rand2()%P_MAX;
								while(quest_rumor[co])
									co = (co+1)%P_MAX;
								--quest_rumor_counter;
								quest_rumor[co] = true;

								switch(co)
								{
								case P_TARTAK:
									ctx.dialog_s_text = Format(txRumorQ[0],	locations[quest_sawmill->start_loc]->name.c_str());
									break;
								case P_KOPALNIA:
									ctx.dialog_s_text = Format(txRumorQ[1], locations[quest_mine->start_loc]->name.c_str());
									break;
								case P_ZAWODY_W_PICIU:
									ctx.dialog_s_text = txRumorQ[2];
									break;
								case P_BANDYCI:
									ctx.dialog_s_text = Format(txRumorQ[3], locations[quest_bandits->start_loc]->name.c_str());
									break;
								case P_MAGOWIE:
									ctx.dialog_s_text = Format(txRumorQ[4], locations[quest_mages->start_loc]->name.c_str());
									break;
								case P_MAGOWIE2:
									ctx.dialog_s_text = txRumorQ[5];
									break;
								case P_ORKOWIE:
									ctx.dialog_s_text = Format(txRumorQ[6], locations[quest_orcs->start_loc]->name.c_str());
									break;
								case P_GOBLINY:
									ctx.dialog_s_text = Format(txRumorQ[7], locations[quest_goblins->start_loc]->name.c_str());
									break;
								case P_ZLO:
									ctx.dialog_s_text = Format(txRumorQ[8], locations[quest_evil->start_loc]->name.c_str());
									break;
								default:
									assert(0);
									return;
								}

								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::ADD_RUMOR;
									c.id = rumors.size();
								}

								rumors.push_back(Format(game_gui->journal->txAddTime, day + 1, month + 1, year, ctx.dialog_s_text.c_str()));
								game_gui->journal->NeedUpdate(Journal::Rumors);
								AddGameMsg3(GMS_ADDED_RUMOR);
								DialogTalk(ctx, ctx.dialog_s_text.c_str());
								++ctx.dialog_pos;
								return;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
					else
					{
						int ile = countof(txRumor);
						if(pijak)
							ile += countof(txRumorD);
						cstring plotka;
						do
						{
							int co = rand2()%ile;
							if(co < countof(txRumor))
								plotka = txRumor[co];
							else
								plotka = txRumorD[co-countof(txRumor)];
						}
						while(ctx.ostatnia_plotka == plotka);
						ctx.ostatnia_plotka = plotka;

						static string str, str_part;
						str.clear();
						for(uint i=0, len = strlen(plotka); i < len; ++i)
						{
							if(plotka[i] == '$')
							{
								str_part.clear();
								++i;
								while(plotka[i] != '$')
								{
									str_part.push_back(plotka[i]);
									++i;
								}
								str += FormatString(ctx, str_part);
							}
							else
								str.push_back(plotka[i]);
						}

						DialogTalk(ctx, str.c_str());
						++ctx.dialog_pos;
						return;
					}
				}
				else if(strncmp(msg, "train_", 6) == 0)
				{
					bool skill;
					int co;

					if(strcmp(msg+6, "str") == 0)
					{
						skill = false;
						co = (int)Attribute::STR;
					}
					else if(strcmp(msg+6, "end") == 0)
					{
						skill = false;
						co = (int)Attribute::END;
					}
					else if(strcmp(msg+6, "dex") == 0)
					{
						skill = false;
						co = (int)Attribute::DEX;
					}
					else if(strcmp(msg+6, "wep") == 0)
					{
						skill = true;
						co = (int)Skill::ONE_HANDED_WEAPON;
					}
					else if(strcmp(msg + 6, "shb") == 0)
					{
						skill = true;
						co = (int)Skill::SHORT_BLADE;
					}
					else if(strcmp(msg + 6, "lob") == 0)
					{
						skill = true;
						co = (int)Skill::LONG_BLADE;
					}
					else if(strcmp(msg + 6, "axe") == 0)
					{
						skill = true;
						co = (int)Skill::AXE;
					}
					else if(strcmp(msg + 6, "blu") == 0)
					{
						skill = true;
						co = (int)Skill::BLUNT;
					}
					else if(strcmp(msg+6, "bow") == 0)
					{
						skill = true;
						co = (int)Skill::BOW;
					}
					else if(strcmp(msg+6, "shi") == 0)
					{
						skill = true;
						co = (int)Skill::SHIELD;
					}
					else if(strcmp(msg + 6, "lia") == 0)
					{
						skill = true;
						co = (int)Skill::LIGHT_ARMOR;
					}
					else if(strcmp(msg + 6, "mea") == 0)
					{
						skill = true;
						co = (int)Skill::MEDIUM_ARMOR;
					}
					else if(strcmp(msg+6, "hea") == 0)
					{
						skill = true;
						co = (int)Skill::HEAVY_ARMOR;
					}					
					else
					{
						assert(0);
						skill = false;
						co = (int)Attribute::STR;
					}

					// czy gracz ma z³oto?
					if(ctx.pc->unit->gold < 200)
					{
						ctx.dialog_s_text = Format(txNeedMoreGold, 200-ctx.pc->unit->gold);
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						// resetuj dialog
						ctx.dialog_pos = 0;
						ctx.dialog_level = 0;
						return;
					}

					// zabierz z³oto i zamróŸ
					ctx.pc->unit->gold -= 200;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_TRAIN;
						fallback_t = -1.f;
						fallback_1 = (skill ? 1 : 0);
						fallback_2 = co;
					}
					else
					{
						// wyœlij info o trenowaniu
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::TRAIN;
						c.pc = ctx.pc;
						c.id = (skill ? 1 : 0);
						c.ile = co;
						GetPlayerInfo(ctx.pc).NeedUpdateAndGold();
					}
				}
				else if(strcmp(msg, "near_loc") == 0)
				{
					if(ctx.update_locations == 1)
					{
						ctx.active_locations.clear();

						int index = 0;
						for(Location* loc : locations)
						{
							if(loc && loc->type != L_CITY && loc->type != L_ACADEMY && distance(loc->pos, world_pos) <= 150.f && loc->state != LS_HIDDEN)
								ctx.active_locations.push_back(std::pair<int, bool>(index, loc->state == LS_UNKNOWN));
							++index;
						}

						if(!ctx.active_locations.empty())
						{
							std::random_shuffle(ctx.active_locations.begin(), ctx.active_locations.end(), myrand);
							std::sort(ctx.active_locations.begin(), ctx.active_locations.end(),
								[](const std::pair<int, bool>& l1, const std::pair<int, bool>& l2) -> bool { return l1.second < l2.second; });
							ctx.update_locations = 0;
						}
						else
							ctx.update_locations = -1;
					}

					if(ctx.update_locations == -1)
					{
						DialogTalk(ctx, txNoNearLoc);
						++ctx.dialog_pos;
						return;
					}
					else if(ctx.active_locations.empty())
					{
						DialogTalk(ctx, txAllNearLoc);
						++ctx.dialog_pos;
						return;
					}

					int id = ctx.active_locations.back().first;
					ctx.active_locations.pop_back();					
					Location& loc = *locations[id];
					if(loc.state == LS_UNKNOWN)
					{
						loc.state = LS_KNOWN;
						if(IsOnline())
							Net_ChangeLocationState(id, false);
					}
					ctx.dialog_s_text = Format(txNearLoc, GetLocationDirName(world_pos, loc.pos), loc.name.c_str());
					if(loc.spawn == SG_BRAK)
					{
						if(loc.type != L_CAVE && loc.type != L_FOREST && loc.type != L_MOONWELL)
							ctx.dialog_s_text += random_string(txNearLocEmpty);
					}
					else if(loc.state == LS_CLEARED)
						ctx.dialog_s_text += Format(txNearLocCleared, g_spawn_groups[loc.spawn].name);
					else
					{
						SpawnGroup& sg = g_spawn_groups[loc.spawn];
						cstring jacy;
						if(loc.st < 5)
						{
							if(sg.k == K_I)
								jacy = txELvlVeryWeak[0];
							else
								jacy = txELvlVeryWeak[1];
						}
						else if(loc.st < 8)
						{
							if(sg.k == K_I)
								jacy = txELvlWeak[0];
							else
								jacy = txELvlWeak[1];
						}
						else if(loc.st < 11)
						{
							if(sg.k == K_I)
								jacy = txELvlAverage[0];
							else
								jacy = txELvlAverage[1];
						}
						else if(loc.st < 14)
						{
							if(sg.k == K_I)
								jacy = txELvlQuiteStrong[0];
							else
								jacy = txELvlQuiteStrong[1];
						}
						else
						{
							if(sg.k == K_I)
								jacy = txELvlStrong[0];
							else
								jacy = txELvlStrong[1];
						}
						ctx.dialog_s_text += Format(random_string(txNearLocEnemy), jacy, g_spawn_groups[loc.spawn].name);
					}

					DialogTalk(ctx, ctx.dialog_s_text.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(msg, "tell_name") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->know_name = true;
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TELL_NAME;
						c.unit = ctx.talker;
					}
				}
				else if(strcmp(msg, "hero_about") == 0)
				{
					assert(ctx.talker->IsHero());
					DialogTalk(ctx, g_classes[(int)ctx.talker->hero->clas].about.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(msg, "recruit") == 0)
				{
					int koszt = ctx.talker->hero->JoinCost();
					ctx.pc->unit->gold -= koszt;
					ctx.talker->gold += koszt;
					AddTeamMember(ctx.talker, false);
					ctx.talker->temporary = false;
					free_recruit = false;
					if(IS_SET(ctx.talker->data->flags2, F2_MELEE))
						ctx.talker->hero->melee = true;
					else if(IS_SET(ctx.talker->data->flags2, F2_MELEE_50) && rand2()%2 == 0)
						ctx.talker->hero->melee = true;
					if(IsOnline() && !ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(msg, "recruit_free") == 0)
				{
					AddTeamMember(ctx.talker, false);
					free_recruit = false;
					ctx.talker->temporary = false;
					if(IS_SET(ctx.talker->data->flags2, F2_MELEE))
						ctx.talker->hero->melee = true;
					else if(IS_SET(ctx.talker->data->flags2, F2_MELEE_50) && rand2()%2 == 0)
						ctx.talker->hero->melee = true;
				}
				else if(strcmp(msg, "follow_me") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Follow;
					ctx.talker->ai->city_wander = false;
				}
				else if(strcmp(msg, "go_free") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Wander;
					ctx.talker->ai->city_wander = false;
					ctx.talker->ai->loc_timer = random(5.f,10.f);
				}
				else if(strcmp(msg, "wait") == 0)
				{
					assert(ctx.talker->IsFollower());
					ctx.talker->hero->mode = HeroData::Wait;
					ctx.talker->ai->city_wander = false;
				}
				else if(strcmp(msg, "give_item") == 0)
				{
					Unit* t = ctx.talker;
					t->busy = Unit::Busy_Trading;
					EndDialog(ctx);
					ctx.pc->action = PlayerController::Action_GiveItems;
					ctx.pc->action_unit = t;
					ctx.pc->chest_trade = &t->items;
					if(ctx.is_local)
						StartTrade(I_GIVE, *t);
					else
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_GIVE;
						c.pc = ctx.pc;
						GetPlayerInfo(ctx.pc->id).NeedUpdate();
					}
					return;
				}
				else if(strcmp(msg, "share_items") == 0)
				{
					Unit* t = ctx.talker;
					t->busy = Unit::Busy_Trading;
					EndDialog(ctx);
					ctx.pc->action = PlayerController::Action_ShareItems;
					ctx.pc->action_unit = t;
					ctx.pc->chest_trade = &t->items;
					if(ctx.is_local)
						StartTrade(I_SHARE, *t);
					else
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_SHARE;
						c.pc = ctx.pc;
						GetPlayerInfo(ctx.pc->id).NeedUpdate();
					}
					return;
				}
				else if(strcmp(msg, "kick_npc") == 0)
				{
					RemoveTeamMember(ctx.talker);
					if(city_ctx)
						ctx.talker->hero->mode = HeroData::Wander;
					else
						ctx.talker->hero->mode = HeroData::Leave;
					ctx.talker->hero->credit = 0;
					ctx.talker->ai->city_wander = true;
					ctx.talker->ai->loc_timer = random(5.f, 10.f);
					CheckCredit(false);
					ctx.talker->temporary = true;
				}
				else if(strcmp(msg, "give_item_credit") == 0)
					TeamShareGiveItemCredit(ctx);
				else if(strcmp(msg, "sell_item") == 0)
					TeamShareSellItem(ctx);
				else if(strcmp(msg, "share_decline") == 0)
					TeamShareDecline(ctx);
				else if(strcmp(msg, "pvp") == 0)
				{
					// walka z towarzyszem
					StartPvp(ctx.pc, ctx.talker);
				}
				else if(strcmp(msg, "pay_100") == 0)
				{
					int ile = min(100, ctx.pc->unit->gold);
					if(ile > 0)
					{
						ctx.pc->unit->gold -= ile;
						ctx.talker->gold += ile;
						if(sound_volume)
							PlaySound2d(sCoins);
						if(!ctx.is_local)
							GetPlayerInfo(ctx.pc->id).UpdateGold();
					}
				}
				else if(strcmp(msg, "attack") == 0)
				{
					// ta komenda jest zbyt ogólna, jeœli bêdzie kilka takich grup to wystarczy ¿e jedna tego u¿yje to wszyscy zaatakuj¹, nie obs³uguje te¿ budynków
					if(ctx.talker->data->group == G_CRAZIES)
					{
						if(!atak_szalencow)
						{
							atak_szalencow = true;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::CHANGE_FLAGS;
							}
						}
					}
					else
					{
						for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
						{
							if((*it)->dont_attack && IsEnemy(**it, *leader, true))
							{
								(*it)->dont_attack = false;
								(*it)->ai->change_ai_mode = true;
							}
						}
					}
				}
				else if(strcmp(msg, "force_attack") == 0)
				{
					ctx.talker->dont_attack = false;
					ctx.talker->attack_team = true;
					ctx.talker->ai->change_ai_mode = true;
				}
				else if(strcmp(msg, "ginger_hair") == 0)
				{
					ctx.pc->unit->human_data->hair_color = g_hair_colors[8];
					if(IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HAIR_COLOR;
						c.unit = ctx.pc->unit;
					}
				}
				else if(strcmp(msg, "random_hair") == 0)
				{
					if(rand2()%2 == 0)
					{
						VEC4 kolor = ctx.pc->unit->human_data->hair_color;
						do
						{
							ctx.pc->unit->human_data->hair_color = g_hair_colors[rand2()%n_hair_colors];
						}
						while(equal(kolor, ctx.pc->unit->human_data->hair_color));
					}
					else
					{
						VEC4 kolor = ctx.pc->unit->human_data->hair_color;
						do
						{
							ctx.pc->unit->human_data->hair_color = VEC4(random(0.f,1.f), random(0.f,1.f), random(0.f,1.f), 1.f);
						}
						while(equal(kolor, ctx.pc->unit->human_data->hair_color));
					}
					if(IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HAIR_COLOR;
						c.unit = ctx.pc->unit;
					}
				}
				else if(strcmp(msg, "captive_join") == 0)
				{
					AddTeamMember(ctx.talker, true);
					ctx.talker->dont_attack = true;
				}
				else if(strcmp(msg, "captive_escape") == 0)
				{
					if(ctx.talker->hero->team_member)
						RemoveTeamMember(ctx.talker);
					ctx.talker->hero->mode = HeroData::Leave;
					ctx.talker->dont_attack = false;
				}
				else if(strcmp(msg, "use_arena") == 0)
					arena_free = false;
				else if(strcmp(msg, "chlanie_start") == 0)
				{
					contest_state = CONTEST_STARTING;
					contest_time = 0;
					contest_units.clear();
					contest_units.push_back(ctx.pc->unit);
				}
				else if(strcmp(msg, "chlanie_udzial") == 0)
					contest_units.push_back(ctx.pc->unit);
				else if(strcmp(msg, "chlanie_nagroda") == 0)
				{
					contest_winner = nullptr;
					AddItem(*ctx.pc->unit, FindItemList("contest_reward").lis->Get(), 1, false);
					if(ctx.is_local)
						AddGameMsg3(GMS_ADDED_ITEM);
					else
						Net_AddedItemMsg(ctx.pc);
				}
				else if(strcmp(msg, "news") == 0)
				{
					if(ctx.update_news)
					{
						ctx.update_news = false;
						ctx.active_news = news;
						if(ctx.active_news.empty())
						{
							DialogTalk(ctx, random_string(txNoNews));
							++ctx.dialog_pos;
							return;
						}
					}

					if(ctx.active_news.empty())
					{
						DialogTalk(ctx, random_string(txAllNews));
						++ctx.dialog_pos;
						return;
					}

					int id = rand2()%ctx.active_news.size();
					News* news = ctx.active_news[id];
					ctx.active_news.erase(ctx.active_news.begin()+id);

					DialogTalk(ctx, news->text.c_str());
					++ctx.dialog_pos;
					return;
				}
				else if(strcmp(msg, "go_melee") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->melee = true;
				}
				else if(strcmp(msg, "go_ranged") == 0)
				{
					assert(ctx.talker->IsHero());
					ctx.talker->hero->melee = false;
				}
				else if(strcmp(msg, "pvp_gather") == 0)
				{
					near_players.clear();
					for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
					{
						if((*it)->IsPlayer() && (*it)->player != ctx.pc && distance2d((*it)->pos, city_ctx->arena_pos) < 5.f)
							near_players.push_back(*it);
					}
					near_players_str.resize(near_players.size());
					for(uint i=0, size=near_players.size(); i!=size; ++i)
						near_players_str[i] = Format(txPvpWith, near_players[i]->player->name.c_str());
				}
				else if(strncmp(msg, "pvp", 3) == 0)
				{
					int id = int(msg[3]-'1');
					PlayerInfo& info = GetPlayerInfo(near_players[id]->player);
					if(distance2d(info.u->pos, city_ctx->arena_pos) > 5.f)
					{
						ctx.dialog_s_text = Format(txPvpTooFar, info.name.c_str());
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						++ctx.dialog_pos;
						return;
					}
					else
					{
						if(info.id == my_id)
						{
							DialogInfo info;
							info.event = DialogEvent(this, &Game::Event_Pvp);
							info.name = "pvp";
							info.order = ORDER_TOP;
							info.parent = nullptr;
							info.pause = false;
							info.text = Format(txPvp, ctx.pc->name.c_str());
							info.type = DIALOG_YESNO;
							dialog_pvp = GUI.ShowDialog(info);
							pvp_unit = near_players[id];
						}
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::PVP;
							c.pc = near_players[id]->player;
							c.id = ctx.pc->id;
							GetPlayerInfo(near_players[id]->player).NeedUpdate();
						}

						pvp_response.ok = true;
						pvp_response.from = ctx.pc->unit;
						pvp_response.to = info.u;
						pvp_response.timer = 0.f;
					}
				}
				else if(strcmp(msg, "ironfist_start") == 0)
				{
					StartTournament(ctx.talker);
					tournament_units.push_back(ctx.pc->unit);
					ctx.pc->unit->gold -= 100;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(msg, "ironfist_join") == 0)
				{
					tournament_units.push_back(ctx.pc->unit);
					ctx.pc->unit->gold -= 100;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(msg, "ironfist_train") == 0)
				{
					tournament_winner = nullptr;
					ctx.pc->unit->frozen = 2;
					if(ctx.is_local)
					{
						// lokalny fallback
						fallback_co = FALLBACK_TRAIN;
						fallback_t = -1.f;
						fallback_1 = 2;
						fallback_2 = 0;
					}
					else
					{
						// wyœlij info o trenowaniu
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::TRAIN;
						c.pc = ctx.pc;
						c.id = 2;
						c.ile = 0;
						GetPlayerInfo(ctx.pc).NeedUpdate();
					}
				}
				else if(strcmp(msg, "szaleni_powiedzial") == 0)
				{
					ctx.talker->ai->morale = -100.f;
					quest_crazies->crazies_state = Quest_Crazies::State::TalkedWithCrazy;
				}
				else if(strcmp(msg, "szaleni_sprzedaj") == 0)
				{
					const Item* kamien = FindItem("q_szaleni_kamien");
					RemoveItem(*ctx.pc->unit, kamien, 1);
					ctx.pc->unit->gold += 10;
					if(!ctx.is_local)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::GOLD_MSG;
						c.pc = ctx.pc;
						c.ile = 10;
						c.id = 1;
						GetPlayerInfo(ctx.pc).UpdateGold();
					}
					else
						AddGameMsg(Format(txGoldPlus, 10), 3.f);
				}
				else if(strcmp(msg, "give_1") == 0)
				{
					ctx.pc->unit->gold += 1;
					if(!ctx.is_local)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::GOLD_MSG;
						c.pc = ctx.pc;
						c.ile = 1;
						c.id = 1;
						GetPlayerInfo(ctx.pc).UpdateGold();
					}
					else
						AddGameMsg(Format(txGoldPlus, 1), 3.f);
				}
				else if(strcmp(msg, "take_1") == 0)
				{
					ctx.pc->unit->gold -= 1;
					if(!ctx.is_local)
						GetPlayerInfo(ctx.pc).UpdateGold();
				}
				else if(strcmp(msg, "crazy_give_item") == 0)
				{
					crazy_give_item = GetRandomItem(100);
					ctx.pc->unit->AddItem(crazy_give_item, 1, false);
					if(!ctx.is_local)
					{
						Net_AddItem(ctx.pc, crazy_give_item, false);
						Net_AddedItemMsg(ctx.pc);
					}
					else
						AddGameMsg3(GMS_ADDED_ITEM);	
				}
				else if(strcmp(msg, "sekret_atak") == 0)
				{
					secret_state = SECRET_FIGHT;
					at_arena.clear();

					ctx.talker->in_arena = 1;
					at_arena.push_back(ctx.talker);
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = ctx.talker;
					}

					for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
					{
						(*it)->in_arena = 0;
						at_arena.push_back(*it);
						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = *it;
						}
					}
				}
				else if(strcmp(msg, "sekret_daj") == 0)
				{
					secret_state = SECRET_REWARD;
					const Item* item = FindItem("sword_forbidden");
					ctx.pc->unit->AddItem(item, 1, true);
					if(!ctx.is_local)
					{
						Net_AddItem(ctx.pc, item, true);
						Net_AddedItemMsg(ctx.pc);
					}
					else
						AddGameMsg3(GMS_ADDED_ITEM);
				}
				else
				{
					WARN(Format("DT_SPECIAL: %s", msg));
					assert(0);
				}
			}
			break;
		case DT_SET_QUEST_PROGRESS:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				ctx.dialog_quest->SetProgress((int)de.msg);
				if(ctx.dialog_wait > 0.f)
				{
					++ctx.dialog_pos;
					return;
				}
			}
			break;
		case DT_IF_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->IsActive() && ctx.dialog_quest->IsTimedout())
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_RAND:
			if(if_level == ctx.dialog_level)
			{
				if(rand2()%int(de.msg) == 0)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_ONCE:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.dialog_once)
				{
					ctx.dialog_once = false;
					++ctx.dialog_level;
				}
			}
			++if_level;
			break;
		case DT_DO_ONCE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_once = false;
			break;
		case DT_ELSE:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			else if(if_level == ctx.dialog_level+1)
				++ctx.dialog_level;
			break;
		case DT_CHECK_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				Quest* quest = FindQuest(current_location, (Quest::Type)(int)de.msg);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					ctx.dialog_once = false;
					StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_FAIL), if_level, quest);
				}
			}
			break;
		case DT_IF_HAVE_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				if(FindQuestItem2(ctx.pc->unit, msg, nullptr, nullptr, ctx.not_active))
					++ctx.dialog_level;
				ctx.not_active = false;
			}
			++if_level;
			break;
		case DT_DO_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				Quest* quest;
				if(FindQuestItem2(ctx.pc->unit, msg, &quest, nullptr))
					StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
			}
			break;
		case DT_IF_QUEST_PROGRESS:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->prog == (int)de.msg)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_QUEST_PROGRESS_RANGE:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				int x = int(de.msg)&0xFFFF;
				int y = (int(de.msg)&0xFFFF0000)>>16;
				assert(y > x);
				if(in_range(ctx.dialog_quest->prog, x, y))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_NEED_TALK:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
				{
					if((*it)->IsActive() && (*it)->IfNeedTalk(msg))
					{
						++ctx.dialog_level;
						break;
					}
				}
			}
			++if_level;
			break;
		case DT_DO_QUEST:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				for(Quest* quest : quests)
				{
					if(quest->IsActive() && quest->IfNeedTalk(msg))
					{
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}
			}
			break;
		case DT_ESCAPE_CHOICE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_esc = (int)ctx.choices.size()-1;
			break;
		case DT_IF_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				if(strcmp(msg, "arena_combat") == 0)
				{
					int t = city_ctx->arena_time;
					if(t == -1)
						++ctx.dialog_level;
					else
					{
						int rok = t/360;
						t -= rok*360;
						rok += 100;
						int miesiac = t/30;
						t -= miesiac*30;
						int dekadzien = t/10;

						if(rok != year || miesiac != month || dekadzien != day/10)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "have_team") == 0)
				{
					if(HaveTeamMember())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_pc_team") == 0)
				{
					if(HaveTeamMemberPC())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_npc_team") == 0)
				{
					if(HaveTeamMemberNPC())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_drunk") == 0)
				{
					if(IS_SET(ctx.talker->data->flags, F_AI_DRUNKMAN) && ctx.talker->in_building != -1)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_team_member") == 0)
				{
					if(IsTeamMember(*ctx.talker))
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_not_known") == 0)
				{
					assert(ctx.talker->IsHero());
					if(!ctx.talker->hero->know_name)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_inside_dungeon") == 0)
				{
					if(local_ctx.type == LevelContext::Inside)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_team_full") == 0)
				{
					if(GetActiveTeamSize() >= MAX_TEAM_SIZE)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "can_join") == 0)
				{
					if(ctx.pc->unit->gold >= ctx.talker->hero->JoinCost())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_inside_town") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_free") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode == HeroData::Wander)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_not_free") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Wander)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_not_follow") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Follow)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_not_waiting") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->mode != HeroData::Wait)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_safe_location") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
					else if(location->outside)
					{
						if(location->state == LS_CLEARED)
							++ctx.dialog_level;
					}
					else
					{
						InsideLocation* inside = (InsideLocation*)location;
						if(inside->IsMultilevel())
						{
							MultiInsideLocation* multi = (MultiInsideLocation*)inside;
							if(multi->IsLevelClear())
							{
								if(dungeon_level == 0)
								{
									if(!multi->from_portal)
										++ctx.dialog_level;
								}
								else
									++ctx.dialog_level;
							}
						}
						else if(location->state == LS_CLEARED)
							++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "is_near_arena") == 0)
				{
					if(city_ctx && IS_SET(city_ctx->flags, City::HaveArena) && distance2d(ctx.talker->pos, city_ctx->arena_pos) < 5.f)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_lost_pvp") == 0)
				{
					assert(ctx.talker->IsHero());
					if(ctx.talker->hero->lost_pvp)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_healthy") == 0)
				{
					if(ctx.talker->GetHpp() >= 0.75f)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_arena_free") == 0)
				{
					if(arena_free)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_bandit") == 0)
				{
					if(bandyta)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_gold_100") == 0)
				{
					if(ctx.pc->unit->gold >= 100)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_ginger") == 0)
				{
					if(equal(ctx.pc->unit->human_data->hair_color, g_hair_colors[8]))
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_bald") == 0)
				{
					if(ctx.pc->unit->human_data->hair == -1)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_camp") == 0)
				{
					if(target_loc_is_camp)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "taken_guards_reward") == 0)
				{
					if(!guards_enc_reward)
					{
						AddGold(250, nullptr, true);
						guards_enc_reward = true;
						++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "dont_have_quest") == 0)
				{
					if(ctx.talker->quest_refid == -1)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_unaccepted_quest") == 0)
				{
					if(FindUnacceptedQuest(ctx.talker->quest_refid))
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_completed_quest") == 0)
				{
					Quest* q = FindQuest(ctx.talker->quest_refid, false);
					if(q && !q->IsActive())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "contest_done") == 0)
				{
					if(contest_state == CONTEST_DONE)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "contest_here") == 0)
				{
					if(contest_where == current_location)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "contest_today") == 0)
				{
					if(contest_state == CONTEST_TODAY)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "chlanie_trwa") == 0)
				{
					if(contest_state == CONTEST_IN_PROGRESS)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "contest_started") == 0)
				{
					if(contest_state == CONTEST_STARTING)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "contest_joined") == 0)
				{
					for(Unit* unit : contest_units)
					{
						if(unit == ctx.pc->unit)
						{
							++ctx.dialog_level;
							break;
						}
					}
				}
				else if(strcmp(msg, "contest_winner") == 0)
				{
					if(ctx.pc->unit == contest_winner)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_bandyci_straznikow_daj") == 0)
				{
					if(quest_bandits->prog == Quest_Bandits::Progress::NeedTalkWithCaptain && current_location == quest_bandits->start_loc)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_magowie_to_miasto") == 0)
				{
					if(quest_mages2->mages_state >= Quest_Mages2::State::TalkedWithCaptain && current_location == quest_mages2->start_loc)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_magowie_poinformuj") == 0)
				{
					if(quest_mages2->mages_state == Quest_Mages2::State::EncounteredGolem)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_magowie_kup_miksture") == 0)
				{
					if(quest_mages2->mages_state == Quest_Mages2::State::BuyPotion)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_magowie_kup") == 0)
				{
					if(ctx.pc->unit->gold >= 150)
					{
						quest_mages2->SetProgress(Quest_Mages2::Progress::BoughtPotion);
						++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "q_orkowie_to_miasto") == 0)
				{
					if(current_location == quest_orcs->start_loc)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_orkowie_zaakceptowano") == 0)
				{
					if(quest_orcs2->orcs_state >= Quest_Orcs2::State::Accepted)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_magowie_nie_ukonczono") == 0)
				{
					if(quest_mages2->mages_state != Quest_Mages2::State::Completed)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_orkowie_nie_ukonczono") == 0)
				{
					if(quest_orcs2->orcs_state < Quest_Orcs2::State::Completed)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_free_recruit") == 0)
				{
					if(ctx.talker->level < 6 && free_recruit)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_unique_quest") == 0)
				{
					if(((quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted || quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
							&& quest_orcs->start_loc == current_location)
						|| (quest_mages2->mages_state >= Quest_Mages2::State::TalkedWithCaptain
							&& quest_mages2->mages_state < Quest_Mages2::State::Completed 
							&& quest_mages2->start_loc == current_location))
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_100") == 0)
				{
					if(ctx.pc->unit->gold >= 100)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_gobliny_zapytaj") == 0)
				{
					if(quest_goblins->goblins_state >= Quest_Goblins::State::MageTalked
						&& quest_goblins->goblins_state < Quest_Goblins::State::KnownLocation
						&& current_location == quest_goblins->start_loc
						&& quest_goblins->prog != Quest_Goblins::Progress::TalkedWithInnkeeper)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_zlo_kapitan") == 0)
				{
					if(current_location == quest_evil->mage_loc
						&& quest_evil->evil_state >= Quest_Evil::State::GeneratedMage
						&& quest_evil->evil_state < Quest_Evil::State::ClosingPortals
						&& in_range((Quest_Evil::Progress)quest_evil->prog, Quest_Evil::Progress::MageToldAboutStolenBook, Quest_Evil::Progress::TalkedWithMayor))
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_zlo_burmistrz") == 0)
				{
					if(current_location == quest_evil->mage_loc
						&& quest_evil->evil_state >= Quest_Evil::State::GeneratedMage
						&& quest_evil->evil_state < Quest_Evil::State::ClosingPortals
						&& quest_evil->prog == Quest_Evil::Progress::TalkedWithCaptain)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_not_mage") == 0)
				{
					if(ctx.talker->hero->clas != Class::MAGE)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "prefer_melee") == 0)
				{
					if(ctx.talker->hero->melee)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "is_leader") == 0)
				{
					if(ctx.pc->unit == leader)
						++ctx.dialog_level;
				}
				else if(strncmp(msg, "have_player", 11) == 0)
				{
					int id = int(msg[11]-'1');
					if(id < (int)near_players.size())
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "waiting_for_pvp") == 0)
				{
					if(pvp_response.ok)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "in_city") == 0)
				{
					if(city_ctx)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "ironfist_can_start") == 0)
				{
					if(tournament_state == TOURNAMENT_NOT_DONE && tournament_city == current_location && day == 6 && month == 2 && tournament_year != year)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "ironfist_done") == 0)
				{
					if(tournament_year == year)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "ironfist_here") == 0)
				{
					if(current_location == tournament_city)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "ironfist_preparing") == 0)
				{
					if(tournament_state == TOURNAMENT_STARTING)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "ironfist_started") == 0)
				{
					if(tournament_state == TOURNAMENT_IN_PROGRESS)
					{
						// zwyciêzca mo¿e pogadaæ i przerwaæ gadanie
						if(tournament_winner == dialog_context.pc->unit && tournament_state2 == 2 && tournament_state3 == 1)
						{
							tournament_master->look_target = nullptr;
							tournament_state = TOURNAMENT_NOT_DONE;
						}
						else
							++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "ironfist_joined") == 0)
				{
					for(Unit* unit : tournament_units)
					{
						if(unit == ctx.pc->unit)
						{
							++ctx.dialog_level;
							break;
						}
					}
				}
				else if(strcmp(msg, "ironfist_winner") == 0)
				{
					if(ctx.pc->unit == tournament_winner)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "szaleni_nie_zapytano") == 0)
				{
					if(quest_crazies->crazies_state == Quest_Crazies::State::None)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_szaleni_trzeba_pogadac") == 0)
				{
					if(quest_crazies->crazies_state == Quest_Crazies::State::FirstAttack)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "have_1") == 0)
				{
					if(ctx.pc->unit->gold != 0)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "secret_first_dialog") == 0)
				{
					if(secret_state == SECRET_GENERATED2)
					{
						secret_state = SECRET_TALKED;
						++ctx.dialog_level;
					}
				}
				else if(strcmp(msg, "sekret_can_fight") == 0)
				{
					if(secret_state == SECRET_TALKED)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "sekret_win") == 0)
				{
					if(secret_state == SECRET_WIN || secret_state == SECRET_REWARD)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "sekret_can_get_reward") == 0)
				{
					if(secret_state == SECRET_WIN)
						++ctx.dialog_level;
				}
				else if(strcmp(msg, "q_main_need_talk") == 0)
				{
					Quest* q = FindQuestById(Q_MAIN);
					if(current_location == q->start_loc && q->prog == 0) // oh well :3
						++ctx.dialog_level;
				}
				else
				{
					WARN(Format("DT_SPECIAL_IF: %s", msg));
					assert(0);
				}
			}
			++if_level;
			break;
		case DT_IF_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				if(ctx.choices.size() == (int)de.msg)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_DO_QUEST2:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				for(Quest* quest : quests)
				{
					if(quest->IfNeedTalk(msg))
					{
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}

				if(ctx.dialog_pos != -1)
				{
					for(Quest* quest : unaccepted_quests)
					{
						if(quest->IfNeedTalk(msg))
						{
							StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
							break;
						}
					}
				}
			}
			break;
		case DT_IF_HAVE_ITEM:
			if(if_level == ctx.dialog_level)
			{
				const Item* item = (const Item*)de.msg;
				if(ctx.pc->unit->HaveItem(item))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_IF_QUEST_EVENT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				if(ctx.dialog_quest->IfQuestEvent())
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog '%s'.", ctx.dialog->id.c_str());
		case DT_NOT_ACTIVE:
			ctx.not_active = true;
			break;
		case DT_IF_QUEST_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				if(ctx.dialog_quest->IfSpecial(ctx, msg))
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DT_QUEST_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				ctx.dialog_quest->Special(ctx, msg);
			}
			break;
		default:
			assert(0 && "Unknown dialog type!");
			break;
		}

		++ctx.dialog_pos;
	}
}

//=============================================================================
// Ruch jednostki, ustawia pozycje Y dla terenu, opuszczanie lokacji
//=============================================================================
void Game::MoveUnit(Unit& unit, bool warped)
{
	if(location->outside)
	{
		if(unit.in_building == -1)
		{
			if(terrain->IsInside(unit.pos))
			{
				terrain->SetH(unit.pos);
				if(warped)
					return;
				if(unit.IsPlayer() && WantExitLevel() && unit.frozen == 0)
				{
					bool allow_exit = false;

					if(city_ctx && IS_SET(city_ctx->flags, City::HaveExit))
					{
						for(vector<EntryPoint>::const_iterator it = city_ctx->entry_points.begin(), end = city_ctx->entry_points.end(); it != end; ++it)
						{
							if(it->exit_area.IsInside(unit.pos))
							{
								if(!IsLeader())
									AddGameMsg3(GMS_NOT_LEADER);
								else
								{
									if(IsLocal())
									{
										int w = CanLeaveLocation(unit);
										if(w == 0)
										{
											allow_exit = true;
											SetExitWorldDir();
										}
										else
											AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										Net_LeaveLocation(WHERE_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(current_location != secret_where2 && (unit.pos.x < 33.f || unit.pos.x > 256.f-33.f || unit.pos.z < 33.f || unit.pos.z > 256.f-33.f))
					{
						if(IsLeader())
						{
							if(IsLocal())
							{
								int w = CanLeaveLocation(unit);
								if(w == 0)
								{
									allow_exit = true;
									// opuszczanie otwartego terenu (las/droga/obóz)
									if(unit.pos.x < 33.f)
										world_dir = lerp(3.f/4.f*PI, 5.f/4.f*PI, 1.f-(unit.pos.z-33.f)/(256.f-66.f));
									else if(unit.pos.x > 256.f-33.f)
									{
										if(unit.pos.z > 128.f)
											world_dir = lerp(0.f, 1.f/4*PI, (unit.pos.z-128.f)/(256.f-128.f-33.f));
										else
											world_dir = lerp(7.f/4*PI, PI*2, (unit.pos.z-33.f)/(256.f-128.f-33.f));
									}
									else if(unit.pos.z < 33.f)
										world_dir = lerp(5.f/4*PI, 7.f/4*PI, (unit.pos.x-33.f)/(256.f-66.f));
									else
										world_dir = lerp(1.f/4*PI, 3.f/4*PI, 1.f-(unit.pos.x-33.f)/(256.f-66.f));
								}
								else if(w == 1)
									AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
							}
							else
								Net_LeaveLocation(WHERE_OUTSIDE);
						}
						else
							AddGameMsg3(GMS_NOT_LEADER);
					}

					if(allow_exit)
					{
						fallback_co = FALLBACK_EXIT;
						fallback_t = -1.f;
						for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
							(*it)->frozen = 2;
						if(IsOnline())
							PushNetChange(NetChange::LEAVE_LOCATION);
					}
				}
			}
			else
				unit.pos.y = 0.f;

			if(warped)
				return;

			if(unit.IsPlayer() && location->type == L_CITY && WantExitLevel() && unit.frozen == 0)
			{
				int id = 0;
				for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it, ++id)
				{
					if((*it)->enter_area.IsInside(unit.pos))
					{
						if(IsLocal())
						{
							// wejdŸ do budynku
							fallback_co = FALLBACK_ENTER;
							fallback_t = -1.f;
							fallback_1 = id;
							unit.frozen = 2;
						}
						else
						{
							// info do serwera
							fallback_co = FALLBACK_WAIT_FOR_WARP;
							fallback_t = -1.f;
							unit.frozen = 2;
							NetChange& c = Add1(net_changes);
							c.type = NetChange::ENTER_BUILDING;
							c.id = id;
						}
						break;
					}
				}
			}
		}
		else
		{
			if(warped)
				return;

			// jest w budynku
			// sprawdŸ czy nie wszed³ na wyjœcie (tylko gracz mo¿e opuszczaæ budynek, na razie)
			City* city = (City*)location;
			InsideBuilding& building = *city->inside_buildings[unit.in_building];

			if(unit.IsPlayer() && building.exit_area.IsInside(unit.pos) && WantExitLevel() && unit.frozen == 0)
			{
				if(IsLocal())
				{
					// opuœæ budynek
					fallback_co = FALLBACK_ENTER;
					fallback_t = -1.f;
					fallback_1 = -1;
					unit.frozen = 2;
				}
				else
				{
					// info do serwera
					fallback_co = FALLBACK_WAIT_FOR_WARP;
					fallback_t = -1.f;
					unit.frozen = 2;
					PushNetChange(NetChange::EXIT_BUILDING);
				}
			}
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		INT2 pt = pos_to_pt(unit.pos);

		if(pt == lvl.staircase_up)
		{
			BOX2D box;
			switch(lvl.staircase_up_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z-2.f*lvl.staircase_up.y)/2;
				box = BOX2D(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y + 1.4f, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x - 2.f*lvl.staircase_up.x) / 2;
				box = BOX2D(2.f*lvl.staircase_up.x + 1.4f, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.staircase_up.y - unit.pos.z) / 2 + 1.f;
				box = BOX2D(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*lvl.staircase_up.y + 0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.staircase_up.x - unit.pos.x) / 2 + 1.f;
				box = BOX2D(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*lvl.staircase_up.x + 0.6f, 2.f*(lvl.staircase_up.y + 1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == 0)
			{
				if(IsLeader())
				{
					if(IsLocal())
					{
						int w = CanLeaveLocation(unit);
						if(w == 0)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = -1;
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 2;
							if(IsOnline())
								PushNetChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(WHERE_LEVEL_UP);
				}
				else
					AddGameMsg3(GMS_NOT_LEADER);
			}
		}
		else if(pt == lvl.staircase_down)
		{
			BOX2D box;
			switch(lvl.staircase_down_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z-2.f*lvl.staircase_down.y)*-1.f;
				box = BOX2D(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y+1.4f, 2.f*(lvl.staircase_down.x+1), 2.f*(lvl.staircase_down.y+1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x-2.f*lvl.staircase_down.x)*-1.f;
				box = BOX2D(2.f*lvl.staircase_down.x+1.4f, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x+1), 2.f*(lvl.staircase_down.y+1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.staircase_down.y-unit.pos.z)*-1.f-2.f;
				box = BOX2D(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x+1), 2.f*lvl.staircase_down.y+0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.staircase_down.x-unit.pos.x)*-1.f-2.f;
				box = BOX2D(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*lvl.staircase_down.x+0.6f, 2.f*(lvl.staircase_down.y+1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == 0)
			{
				if(IsLeader())
				{
					if(IsLocal())
					{
						int w = CanLeaveLocation(unit);
						if(w == 0)
						{
							fallback_co = FALLBACK_CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = +1;
							for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
								(*it)->frozen = 2;
							if(IsOnline())
								PushNetChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(WHERE_LEVEL_DOWN);
				}
				else
					AddGameMsg3(GMS_NOT_LEADER);
			}
		}
		else
			unit.pos.y = 0.f;

		if(warped)
			return;
	}

	// obs³uga portali
	if(unit.frozen == 0 && unit.IsPlayer() && WantExitLevel())
	{
		Portal* portal = location->portal;
		int index = 0;
		while(portal)
		{
			if(portal->target_loc != -1 && distance2d(unit.pos, portal->pos) < 2.f)
			{
				if(CircleToRotatedRectangle(unit.pos.x, unit.pos.z, unit.GetUnitRadius(), portal->pos.x, portal->pos.z, 0.67f, 0.1f, portal->rot))
				{
					if(IsLeader())
					{
						if(IsLocal())
						{
							int w = CanLeaveLocation(unit);
							if(w == 0)
							{
								fallback_co = FALLBACK_USE_PORTAL;
								fallback_t = -1.f;
								fallback_1 = index;
								for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
									(*it)->frozen = 2;
								if(IsOnline())
									PushNetChange(NetChange::LEAVE_LOCATION);
							}
							else
								AddGameMsg3(w == 1 ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							Net_LeaveLocation(WHERE_PORTAL+index);
					}
					else
						AddGameMsg3(GMS_NOT_LEADER);

					break;
				}
			}
			portal = portal->next_portal;
			++index;
		}
	}

	if(!warped)
	{
		if(IsLocal() || &unit != pc->unit || interpolate_timer <= 0.f)
			unit.visual_pos = unit.pos;
		UpdateUnitPhysics(unit, unit.pos);
	}
}

bool Game::RayToMesh(const VEC3& _ray_pos, const VEC3& _ray_dir, const VEC3& _obj_pos, float _obj_rot, VertexData* _vd, float& _dist)
{
	assert(_vd);

	// najpierw sprawdŸ kolizje promienia ze sfer¹ otaczaj¹c¹ model
	if(!RayToSphere(_ray_pos, _ray_dir, _obj_pos, _vd->radius, _dist))
		return false;

	// przekszta³æ promieñ o pozycjê i obrót modelu
	D3DXMatrixTranslation(&m1, _obj_pos);
	D3DXMatrixRotationY(&m2, _obj_rot);
	D3DXMatrixMultiply(&m3, &m2, &m1);
	D3DXMatrixInverse(&m1, nullptr, &m3);

	VEC3 ray_pos, ray_dir;
	D3DXVec3TransformCoord(&ray_pos, &_ray_pos, &m1);
	D3DXVec3TransformNormal(&ray_dir, &_ray_dir, &m1);

	// szukaj kolizji
	_dist = 1.01f;
	float dist;
	bool hit = false;

	for(vector<Face>::iterator it = _vd->faces.begin(), end = _vd->faces.end(); it != end; ++it)
	{
		if(RayToTriangle(ray_pos, ray_dir, _vd->verts[it->idx[0]], _vd->verts[it->idx[1]], _vd->verts[it->idx[2]], dist) && dist < _dist && dist >= 0.f)
		{
			hit = true;
			_dist = dist;
		}
	}

	return hit;
}

bool Game::CollideWithStairs(const CollisionObject& _co, const VEC3& _pos, float _radius) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check == &Game::CollideWithStairs && !location->outside && _radius > 0.f);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	int dir;

	if(_co.extra == 0)
	{
		assert(!lvl.staircase_down_in_wall);
		dir = lvl.staircase_down_dir;
	}
	else
		dir = lvl.staircase_up_dir;

	if(dir != 0)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y-0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 1)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x-0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	if(dir != 2)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y+0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 3)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x+0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	return false;
}

bool Game::CollideWithStairsRect(const CollisionObject& _co, const BOX2D& _box) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check_rect == &Game::CollideWithStairsRect && !location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	int dir;

	if(_co.extra == 0)
	{
		assert(!lvl.staircase_down_in_wall);
		dir = lvl.staircase_down_dir;
	}
	else
		dir = lvl.staircase_up_dir;

	if(dir != 0)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y-1.f, _co.pt.x+1.f, _co.pt.y-0.9f))
			return true;
	}

	if(dir != 1)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y-1.f, _co.pt.x-0.9f, _co.pt.y+1.f))
			return true;
	}

	if(dir != 2)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x-1.f, _co.pt.y+0.9f, _co.pt.x+1.f, _co.pt.y+1.f))
			return true;
	}

	if(dir != 3)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x+0.9f, _co.pt.y-1.f, _co.pt.x+1.f, _co.pt.y+1.f))
			return true;
	}

	return false;
}

inline bool IsNotNegative(const VEC3& v)
{
	return v.x >= 0.f && v.y >= 0.f && v.z >= 0.f;
}

uint Game::TestGameData(bool major)
{
	string str;
	uint errors = 0;

	LOG("Test: Checking items...");

	// bronie
	for(Weapon* weapon : g_weapons)
	{
		const Weapon& w = *weapon;
		if(!w.mesh)
		{
			ERROR(Format("Test: Weapon %s: missing mesh %s.", w.id.c_str(), w.mesh_id.c_str()));
			++errors;
		}
		else
		{
			Animesh::Point* pt = w.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				ERROR(Format("Test: Weapon %s: no hitbox in mesh %s.", w.id.c_str(), w.mesh_id.c_str()));
				++errors;
			}
			else if(!IsNotNegative(pt->size))
			{
				ERROR(Format("Test: Weapon %s: invalid hitbox %g, %g, %g in mesh %s.", w.id.c_str(), pt->size.x, pt->size.y, pt->size.z, w.mesh_id.c_str()));
				++errors;
			}
		}
	}

	// tarcze
	for(Shield* shield : g_shields)
	{
		const Shield& s = *shield;
		if(!s.mesh)
		{
			ERROR(Format("Test: Shield %s: missing mesh %s.", s.id.c_str(), s.mesh_id.c_str()));
			++errors;
		}
		else
		{
			Animesh::Point* pt = s.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				ERROR(Format("Test: Shield %s: no hitbox in mesh %s.", s.id.c_str(), s.mesh_id.c_str()));
				++errors;
			}
			else if(!IsNotNegative(pt->size))
			{
				ERROR(Format("Test: Shield %s: invalid hitbox %g, %g, %g in mesh %s.", s.id.c_str(), pt->size.x, pt->size.y, pt->size.z, s.mesh_id.c_str()));
				++errors;
			}
		}
	}

	// postacie
	LOG("Test: Checking units...");
	for(UnitData* ud_ptr : unit_datas)
	{
		UnitData& ud = *ud_ptr;
		str.clear();

		// przedmioty postaci
		if(ud.items)
		{
			uint crc;
			TestItemScript(ud.items, str, errors, crc);
		}

		// czary postaci
		if(ud.spells)
			TestUnitSpells(*ud.spells, str, errors);

		// ataki
		if(ud.frames)
		{
			if(ud.frames->extra)
			{
				if(in_range(ud.frames->attacks, 1, NAMES::max_attacks))
				{
					for(int i=0; i<ud.frames->attacks; ++i)
					{
						if(!in_range2(0.f, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end, 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i+1, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			else
			{
				if(in_range(ud.frames->attacks, 1, 3))
				{
					for(int i=0; i<ud.frames->attacks; ++i)
					{
						if(!in_range2(0.f, ud.frames->t[F_ATTACK1_START+i*2], ud.frames->t[F_ATTACK1_END+i*2], 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i+1, ud.frames->t[F_ATTACK1_START+i*2],
								ud.frames->t[F_ATTACK1_END+i*2]);
							++errors;
						}
					}
				}
				else
				{
					str += Format("\tInvalid attacks count (%d).\n", ud.frames->attacks);
					++errors;
				}
			}
			if(!in_range(ud.frames->t[F_BASH], 0.f, 1.f))
			{
				str += Format("\tInvalid attack point '%g' bash.\n", ud.frames->t[F_BASH]);
				++errors;
			}
		}
		else
		{
			str += "\tMissing attacks information.\n";
			++errors;
		}

		// model postaci
		if(ud.mesh_id.empty())
		{
			if(!IS_SET(ud.flags, F_HUMAN))
			{
				str += "\tNo mesh!\n";
				++errors;
			}
		}
		else if(major)
		{
			if(!ud.mesh)
			{
				str += Format("\tMissing mesh %s.\n", ud.mesh_id.c_str());
				++errors;
			}
			else
			{
				Animesh& a = *ud.mesh;

				for(uint i=0; i<NAMES::n_ani_base; ++i)
				{
					if(!a.GetAnimation(NAMES::ani_base[i]))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_base[i]);
						++errors;
					}
				}

				if(!IS_SET(ud.flags, F_SLOW))
				{
					if(!a.GetAnimation(NAMES::ani_run))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_run);
						++errors;
					}
				}

				if(!IS_SET(ud.flags, F_DONT_SUFFER))
				{
					if(!a.GetAnimation(NAMES::ani_hurt))
					{
						str += Format("\tMissing animation '%s'.\n", NAMES::ani_hurt);
						++errors;
					}
				}

				if(IS_SET(ud.flags, F_HUMAN) || IS_SET(ud.flags, F_HUMANOID))
				{
					for(uint i=0; i<NAMES::n_points; ++i)
					{
						if(!a.GetPoint(NAMES::points[i]))
						{
							str += Format("\tMissing attachment point '%s'.\n", NAMES::points[i]);
							++errors;
						}
					}

					for(uint i=0; i<NAMES::n_ani_humanoid; ++i)
					{
						if(!a.GetAnimation(NAMES::ani_humanoid[i]))
						{
							str += Format("\tMissing animation '%s'.\n", NAMES::ani_humanoid[i]);
							++errors;
						}
					}
				}

				// animacje ataków
				if(ud.frames)
				{
					if(ud.frames->attacks > NAMES::max_attacks)
					{
						str += Format("\tToo many attacks (%d)!\n", ud.frames->attacks);
						++errors;
					}
					for(int i=0; i<min(ud.frames->attacks, NAMES::max_attacks); ++i)
					{
						if(!a.GetAnimation(NAMES::ani_attacks[i]))
						{
							str += Format("\tMissing animation '%s'.\n", NAMES::ani_attacks[i]);
							++errors;
						}
					}
				}

				// animacje idle
				if(ud.idles)
				{
					for(const string& s : *ud.idles)
					{
						if(!a.GetAnimation(s.c_str()))
						{
							str += Format("\tMissing animation '%s'.\n", s.c_str());
							++errors;
						}
					}
				}

				// punkt czaru
				if(ud.spells && !a.GetPoint(NAMES::point_cast))
				{
					str += Format("\tMissing attachment point '%s'.\n", NAMES::point_cast);
					++errors;
				}
			}
		}

		if(!str.empty())
			ERROR(Format("Test: Unit %s:\n%s", ud.id.c_str(), str.c_str()));
	}

	return errors;
}

void Game::TestUnitSpells(const SpellList& _spells, string& _errors, uint& _count)
{
	for(int i=0; i<3; ++i)
	{
		if(_spells.name[i])
		{
			Spell* spell = FindSpell(_spells.name[i]);
			if(!spell)
			{
				_errors += Format("\tMissing spell '%s'.\n", _spells.name[i]);
				++_count;
			}
		}
	}
}

Unit* Game::CreateUnit(UnitData& base, int level, Human* human_data, Unit* test_unit, bool create_physics, bool custom)
{
	Unit* u;
	if(test_unit)
		u = test_unit;
	else
		u = new Unit;

	u->human_data = nullptr;

	// typ
	if(!test_unit)
	{
		if(IS_SET(base.flags, F_HUMAN))
		{
			u->type = Unit::HUMAN;
			if(human_data)
				u->human_data = human_data;
			else
			{
#define HEX(h) VEC4(1.f/256*(((h)&0xFF0000)>>16), 1.f/256*(((h)&0xFF00)>>8), 1.f/256*((h)&0xFF), 1.f)
				u->human_data = new Human;
				u->human_data->beard = rand2() % MAX_BEARD - 1;
				u->human_data->hair = rand2() % MAX_HAIR - 1;
				u->human_data->mustache = rand2() % MAX_MUSTACHE - 1;
				u->human_data->height = random(0.9f, 1.1f);
				if(IS_SET(base.flags2, F2_OLD))
					u->human_data->hair_color = HEX(0xDED5D0);
				else if(IS_SET(base.flags, F_CRAZY))
					u->human_data->hair_color = VEC4(random_part(8), random_part(8), random_part(8), 1.f);
				else if(IS_SET(base.flags, F_GRAY_HAIR))
					u->human_data->hair_color = g_hair_colors[rand2()%4];
				else if(IS_SET(base.flags, F_TOMASHU))
				{
					u->human_data->beard = 4;
					u->human_data->mustache = -1;
					u->human_data->hair = 0;
					u->human_data->hair_color = g_hair_colors[0];
					u->human_data->height = 1.1f;
				}
				else
					u->human_data->hair_color = g_hair_colors[rand2()%n_hair_colors];
				u->human_data->ApplyScale(aHumanBase);
#undef HEX
			}

			u->ani = new AnimeshInstance(aHumanBase);
		}
		else
		{
			u->ani = new AnimeshInstance(base.mesh);
			
			if(IS_SET(base.flags, F_HUMANOID))
				u->type = Unit::HUMANOID;
			else
				u->type = Unit::ANIMAL;
		}

		u->animation = u->current_animation = ANI_STAND;
		u->ani->Play("stoi", PLAY_PRIO1|PLAY_NO_BLEND, 0);

		if(u->ani->ani->head.n_groups > 1)
			u->ani->groups[1].state = 0;

		u->ani->ptr = u;
	}

	u->pos = VEC3(0,0,0);
	u->rot = 0.f;
	u->used_item = nullptr;
	u->live_state = Unit::ALIVE;
	for(int i=0; i<SLOT_MAX; ++i)
		u->slots[i] = nullptr;
	u->action = A_NONE;
	u->weapon_taken = W_NONE;
	u->weapon_hiding = W_NONE;
	u->weapon_state = WS_HIDDEN;
	u->data = &base;
	if(level == -2)
		u->level = random2(base.level);
	else if(level == -3)
		u->level = clamp(dungeon_level, base.level);
	else
		u->level = clamp(level, base.level);
	u->player = nullptr;
	u->ai = nullptr;
	u->speed = u->prev_speed = 0.f;
	u->invisible = false;
	u->hurt_timer = 0.f;
	u->talking = false;
	u->useable = nullptr;
	u->in_building = -1;
	u->frozen = 0;
	u->in_arena = -1;
	u->run_attack = false;
	u->event_handler = nullptr;
	u->to_remove = false;
	u->temporary = false;
	u->quest_refid = -1;
	u->bubble = nullptr;
	u->busy = Unit::Busy_No;
	u->look_target = nullptr;
	u->interp = nullptr;
	u->dont_attack = false;
	u->assist = false;
	u->auto_talk = AutoTalkMode::No;
	u->attack_team = false;
	u->last_bash = 0.f;
	u->guard_target = nullptr;
	u->alcohol = 0.f;

	float t;
	if(base.level.x == base.level.y)
		t = 1.f;
	else
		t = float(u->level-base.level.x)/(base.level.y-base.level.x);

	if(!custom)
	{
		// to prevent sending hp changed message set temporary as fake unit
		u->fake_unit = true;

		// attributes & skills
		u->data->GetStatProfile().Set(u->level, u->unmod_stats.attrib, u->unmod_stats.skill);
		u->CalculateStats();

		// hp
		u->hp = u->hpmax = u->CalculateMaxHp();

		u->fake_unit = false;
	}

	// items
	u->weight = 0;
	u->CalculateLoad();
	if(!custom && base.items)
	{
		ParseItemScript(*u, base.items);
		SortItems(u->items);
		u->RecalculateWeight();
	}

	// gold
	u->gold = random2(lerp(base.gold, base.gold2, t));

	if(!test_unit)
	{
		if(IS_SET(base.flags, F_HERO))
		{
			u->hero = new HeroData;
			u->hero->Init(*u);
		}
		else
			u->hero = nullptr;

		if(IS_SET(u->data->flags2, F2_BOSS))
			boss_levels.push_back(INT2(current_location, dungeon_level));

		// kolizje
		if(create_physics)
		{
			btCapsuleShape* caps = new btCapsuleShape(u->GetUnitRadius(), max(MIN_H, u->GetUnitHeight()));
			u->cobj = new btCollisionObject;
			u->cobj->setCollisionShape(caps);
			u->cobj->setUserPointer(u);
			u->cobj->setCollisionFlags(CG_UNIT);
			phy_world->addCollisionObject(u->cobj);
		}
		else
			u->cobj = nullptr;
	}

	if(IsOnline() && IsServer())
		u->netid = netid_counter++;

	return u;
}

void GiveItem(Unit& unit, const int* ps, int count)
{
	int type = *ps;
	++ps;
	if(type == PS_ITEM)
		unit.AddItemAndEquipIfNone((const Item*)(*ps), count);
	else if(type == PS_LIST)
	{
		const ItemList& lis = *(const ItemList*)(*ps);
		for(int i = 0; i < count; ++i)
			unit.AddItemAndEquipIfNone(lis.Get());
	}
	else if(type == PS_LEVELED_LIST)
	{
		const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
		int level = max(0, unit.level - 4);
		for(int i = 0; i < count; ++i)
			unit.AddItemAndEquipIfNone(lis.Get(random(random(level, unit.level))));
	}
	else if(type == PS_LEVELED_LIST_MOD)
	{
		int mod = *ps;
		++ps;
		const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
		int level = unit.level + mod;
		if(level >= 0)
		{
			int l = max(0, level - 4);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(random(l, level)));
		}
	}

	++ps;
}

void SkipItem(const int*& ps, int count)
{
	for(int i = 0; i < count; ++i)
	{
		int type = *ps;
		++ps;
		if(type == PS_LEVELED_LIST_MOD)
			++ps;
		++ps;
	}
}

void Game::ParseItemScript(Unit& unit, const int* script)
{
	assert(script);

	const int* ps = script;
	int a, b, depth=0, depth_if=0;

	while(*ps != PS_END)
	{
		ParseScript type = (ParseScript)*ps;
		++ps;

		switch(type)
		{
		case PS_ONE:
			if(depth == depth_if)
				GiveItem(unit, ps, 1);
			else
				SkipItem(ps, 1);
			break;
		case PS_ONE_OF_MANY:
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				b = rand2() % a;
				for(int i = 0; i < a; ++i)
				{
					if(i == b)
						GiveItem(unit, ps, 1);
					else
						SkipItem(ps, 1);
				}
			}
			else
				SkipItem(ps, a);
			break;
		case PS_CHANCE:
			a = *ps;
			++ps;
			if(depth == depth_if && rand2() % 100 < a)
				GiveItem(unit, ps, 1);
			else
				SkipItem(ps, 1);
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				if(rand2()%100 < a)
				{
					GiveItem(unit, ps, 1);
					SkipItem(ps, 1);
				}
				else
				{
					SkipItem(ps, 1);
					GiveItem(unit, ps, 1);
				}
			}
			else
				SkipItem(ps, 2);
			break;
		case PS_IF_CHANCE:
			a = *ps;
			if(depth == depth_if && rand2()%100 < a)
				++depth_if;
			++depth;
			++ps;
			break;
		case PS_IF_LEVEL:
			if(depth == depth_if && unit.level >= *ps)
				++depth_if;
			++depth;
			++ps;
			break;
		case PS_ELSE:
			if(depth == depth_if)
				--depth_if;
			else if(depth == depth_if+1)
				++depth_if;
			break;
		case PS_END_IF:
			if(depth == depth_if)
				--depth_if;
			--depth;
			break;
		case PS_MANY:
			a = *ps;
			++ps;
			if(depth == depth_if)
				GiveItem(unit, ps, a);
			else
				SkipItem(ps, 1);
			break;
		case PS_RANDOM:
			a = *ps;
			++ps;
			b = *ps;
			++ps;
			a = random(a,b);
			if(depth == depth_if && a > 0)
				GiveItem(unit, ps, a);
			else
				SkipItem(ps, 1);
			break;
		}
	}
}

bool Game::IsEnemy(Unit &u1, Unit &u2, bool ignore_dont_attack)
{
	if(u1.in_arena == -1 && u2.in_arena == -1)
	{
		if(!ignore_dont_attack)
		{
			if(u1.IsAI() && IsUnitDontAttack(u1))
				return false;
			if(u2.IsAI() && IsUnitDontAttack(u2))
				return false;
		}

		UNIT_GROUP g1 = u1.data->group,
			g2 = u2.data->group;

		if(u1.IsTeamMember())
			g1 = G_TEAM;
		if(u2.IsTeamMember())
			g2 = G_TEAM;

		if(g1 == g2)
			return false;
		else if(g1 == G_CITIZENS)
		{
			if(g2 == G_CRAZIES)
				return true;
			else if(g2 == G_TEAM)
				return bandyta || WantAttackTeam(u1);
			else
				return true;
		}
		else if(g1 == G_CRAZIES)
		{
			if(g2 == G_CITIZENS)
				return true;
			else if(g2 == G_TEAM)
				return atak_szalencow || WantAttackTeam(u1);
			else
				return true;
		}
		else if(g1 == G_TEAM)
		{
			if(WantAttackTeam(u2))
				return true;
			else if(g2 == G_CITIZENS)
				return bandyta;
			else if(g2 == G_CRAZIES)
				return atak_szalencow;
			else
				return true;
		}
		else
			return true;
	}
	else
	{
		if(u1.in_arena == -1 || u2.in_arena == -1)
			return false;
		else if(u1.in_arena == u2.in_arena)
			return false;
		else
			return true;
	}
}

bool Game::IsFriend(Unit& u1, Unit& u2)
{
	if(u1.in_arena == -1 && u2.in_arena == -1)
	{
		if(u1.IsTeamMember())
		{
			if(u2.IsTeamMember())
				return true;
			else if(u2.IsAI() && !bandyta && IsUnitAssist(u2))
				return true;
			else
				return false;
		}
		else if(u2.IsTeamMember())
		{
			if(u1.IsAI() && !bandyta && IsUnitAssist(u1))
				return true;
			else
				return false;
		}
		else
			return (u1.data->group == u2.data->group);
	}
	else
		return u1.in_arena == u2.in_arena;
}

bool Game::CanSee(Unit& u1, Unit& u2)
{
	if(u1.in_building != u2.in_building)
		return false;

	INT2 tile1(int(u1.pos.x/2),int(u1.pos.z/2)),
		 tile2(int(u2.pos.x/2),int(u2.pos.z/2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = GetContext(u1);

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(outside->tiles[x+y*OutsideLocation::size].mode >= TM_BUILDING_BLOCK
					&& LineToRectangle(u1.pos, u2.pos, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(czy_blokuje2(lvl.map[x + y*lvl.w].type) && LineToRectangle(u1.pos, u2.pos, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y*lvl.w].type == DRZWI)
				{
					Door* door = FindDoor(ctx, INT2(x,y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX2D box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= 0.181f;
							box.v2.y += 0.181f;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}

						if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		// kolizje z obiektami
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != CAM_COLLIDER || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX2D box(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX2D box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= 0.181f;
					box.v2.y += 0.181f;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

bool Game::CanSee(const VEC3& v1, const VEC3& v2)
{
	INT2 tile1(int(v1.x/2),int(v1.z/2)),
		tile2(int(v2.x/2),int(v2.z/2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = local_ctx;

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(outside->tiles[x+y*OutsideLocation::size].mode >= TM_BUILDING_BLOCK && LineToRectangle(v1, v2, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x+1),2.f*(y+1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y=ymin; y<=ymax; ++y)
		{
			for(int x=xmin; x<=xmax; ++x)
			{
				if(czy_blokuje2(lvl.map[x + y*lvl.w].type) && LineToRectangle(v1, v2, VEC2(2.f*x, 2.f*y), VEC2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y*lvl.w].type == DRZWI)
				{
					Door* door = FindDoor(ctx, INT2(x,y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						BOX2D box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= 0.181f;
							box.v2.y += 0.181f;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= 0.181f;
							box.v2.x += 0.181f;
						}

						if(LineToRectangle(v1, v2, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		// kolizje z obiektami
		/*for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr != (void*)1 || it->type != CollisionObject::RECTANGLE)
				continue;

			BOX2D box(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				BOX2D box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= 0.181f;
					box.v2.y += 0.181f;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= 0.181f;
					box.v2.x += 0.181f;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}*/
	}

	return true;
}

bool Game::CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, VEC3& hitpoint)
{	
	// atak broni¹ lub naturalny

	Animesh::Point* hitbox, *point;

	if(unit.ani->ani->head.n_groups > 1)
	{
		Animesh* mesh = unit.GetWeapon().mesh;
		if(!mesh)
			return false;
		hitbox = mesh->FindPoint("hit");
		point = unit.ani->ani->GetPoint(NAMES::point_weapon);
		assert(point);
	}
	else
	{
		point = nullptr;
		hitbox = unit.ani->ani->GetPoint(Format("hitbox%d", unit.attack_id+1));
		if(!hitbox)
			hitbox = unit.ani->ani->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(ctx, unit, hitted, *hitbox, point, hitpoint);
}

// Sprawdza czy jest kolizja hitboxa z jak¹œ postaci¹
// S¹ dwie opcje:
//  - _bone to punkt "bron", _hitbox to hitbox z bronii
//  - _bone jest nullptr, _hitbox jest na modelu postaci
// Na drodze testów ustali³em ¿e najlepiej dzia³a gdy stosuje siê sam¹ funkcjê OrientedBoxToOrientedBox
bool Game::CheckForHit(LevelContext& ctx, Unit& _unit, Unit*& _hitted, Animesh::Point& _hitbox, Animesh::Point* _bone, VEC3& _hitpoint)
{
	assert(_hitted && _hitbox.IsBox());

	//BOX box1, box2;

	// ustaw koœci
	if(_unit.human_data)
		_unit.ani->SetupBones(&_unit.human_data->mat_scale[0]);
	else
		_unit.ani->SetupBones();

	// oblicz macierz hitbox

	// transformacja postaci
	D3DXMatrixTranslation(&m1, _unit.pos);
	D3DXMatrixRotationY(&m2, _unit.rot);
	D3DXMatrixMultiply(&m1, &m2, &m1); // m1 (World) = Rot * Pos

	if(_bone)
	{
		// transformacja punktu broni
		D3DXMatrixMultiply(&m2, &_bone->mat, &_unit.ani->mat_bones[_bone->bone]); // m2 = PointMatrix * BoneMatrix
		D3DXMatrixMultiply(&m3, &m2, &m1); // m3 = PointMatrix * BoneMatrix * UnitRot * UnitPos

		// transformacja hitboxa broni
		D3DXMatrixMultiply(&m1, &_hitbox.mat, &m3); // m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitRot * UnitPos
	}
	else
	{
		D3DXMatrixMultiply(&m2, &_hitbox.mat, &_unit.ani->mat_bones[_hitbox.bone]);
		D3DXMatrixMultiply(&m3, &m2, &m1);
		m1 = m3;
	}

	// m1 to macierz hitboxa
	// teraz mo¿emy stworzyæ AABBOX wokó³ broni
	//CreateAABBOX(box1, m1);

	// przy okazji stwórz obrócony BOX
	/*OOBBOX obox1, obox2;
	D3DXMatrixIdentity(&obox2.rot);
	D3DXVec3TransformCoord(&obox1.pos, &VEC3(0,0,0), &m1);
	obox1.size = _hitbox.size*2;
	obox1.rot = m1;
	obox1.rot._11 = obox2.rot._22 = obox2.rot._33 = 1.f;

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		BOX box2;
		(*it)->GetBox(box2);

		//bool jest1 = BoxToBox(box1, box2);

		// wstêpnie mo¿e byæ tu jakaœ kolizja, trzeba sprawdziæ dok³adniejszymi metodami
		// stwórz obrócony box
		obox2.pos = box2.Midpoint();
		obox2.size = box2.Size()*2;

		// sprawdŸ czy jest kolizja
		//bool jest2 = OrientedBoxToOrientedBox(obox1, obox2, &_hitpoint);


		//if(jest1 && jest2)
		if(OrientedBoxToOrientedBox(obox1, obox2, &_hitpoint))
		{
			// teraz z kolei hitpoint jest za daleko >:(
			float dist = distance2d((*it)->pos, _hitpoint);
			float r = (*it)->GetUnitRadius()*3/2;
			if(dist > r)
			{
				VEC3 dir = _hitpoint - (*it)->pos;
				D3DXVec3Length(&dir);
				dir *= r;
				dir.y = _hitpoint.y;
				_hitpoint = dir;
				_hitpoint.x += (*it)->pos.x;
				_hitpoint.z += (*it)->pos.z;
			}
			if(_hitpoint.y > (*it)->pos.y + (*it)->GetUnitHeight())
				_hitpoint.y = (*it)->pos.y + (*it)->GetUnitHeight();
			// jest kolizja
			_hitted = *it;
			return true;
		}
	}*/

	// a - hitbox broni, b - hitbox postaci
	OOB a, b;
	m1._11 = m1._22 = m1._33 = 1.f;
	D3DXVec3TransformCoord(&a.c, &VEC3(0,0,0), &m1);
	a.e = _hitbox.size;
	a.u[0] = VEC3(m1._11, m1._12, m1._13);
	a.u[1] = VEC3(m1._21, m1._22, m1._23);
	a.u[2] = VEC3(m1._31, m1._32, m1._33);
	b.u[0] = VEC3(1,0,0);
	b.u[1] = VEC3(0,1,0);
	b.u[2] = VEC3(0,0,1);

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		BOX box2;
		(*it)->GetBox(box2);
		b.c = box2.Midpoint();
		b.e = box2.Size()/2;

		if(OOBToOOB(b, a))
		{
			_hitpoint = a.c;
			_hitted = *it;
			return true;
		}
	}

	return false;
}

void Game::UpdateParticles(LevelContext& ctx, float dt)
{
	// aktualizuj cz¹steczki
	bool deletions = false;
	for(vector<ParticleEmitter*>::iterator it = ctx.pes->begin(), end = ctx.pes->end(); it != end; ++it)
	{
		ParticleEmitter& pe = **it;

		if(pe.emisions == 0 || (pe.life > 0 && (pe.life -= dt) <= 0.f))
			pe.destroy = true;

		if(pe.destroy && pe.alive == 0)
		{
			deletions = true;
			if(pe.manual_delete == 0)
				delete *it;
			else
				pe.manual_delete = 2;
			*it = nullptr;
			continue;
		}

		// aktualizuj cz¹steczki
		for(vector<Particle>::iterator it2 = pe.particles.begin(), end2 = pe.particles.end(); it2 != end2; ++it2)
		{
			Particle& p = *it2;
			if(!p.exists)
				continue;

			if((p.life -= dt) <= 0.f)
			{
				p.exists = false;
				--pe.alive;
			}
			else
			{
				p.pos += p.speed * dt;
				p.speed.y -= p.gravity * dt;
			}
		}

		// emisja
		if(!pe.destroy && (pe.emisions == -1 || pe.emisions > 0) && ((pe.time += dt) >= pe.emision_interval))
		{
			if(pe.emisions > 0)
				--pe.emisions;
			pe.time -= pe.emision_interval;

			int ile = min(random(pe.spawn_min, pe.spawn_max), pe.max_particles-pe.alive);
			vector<Particle>::iterator it2 = pe.particles.begin();

			for(int i=0; i<ile; ++i)
			{
				while(it2->exists)
					++it2;

				Particle& p = *it2;
				p.exists = true;
				p.gravity = 9.81f;
				p.life = pe.particle_life;
				p.pos = pe.pos + random(pe.pos_min, pe.pos_max);
				p.speed = random(pe.speed_min, pe.speed_max);
			}

			pe.alive += ile;
		}
	}

	if(deletions)
		RemoveNullElements(ctx.pes);

	// aktualizuj cz¹steczki szlakowe
	deletions = false;

	for(vector<TrailParticleEmitter*>::iterator it = ctx.tpes->begin(), end = ctx.tpes->end(); it != end; ++it)
	{
		if((*it)->Update(dt, nullptr, nullptr))
		{
			delete *it;
			*it = nullptr;
			deletions = true;
		}
	}

	if(deletions)
		RemoveNullElements(ctx.tpes);
}

int ObliczModyfikator(int type, int flags)
{
	// -2 invalid (weapon don't have any dmg type)
	// -1 susceptibility
	// 0 normal
	// 1 resistance

	int mod = -2;

	if(IS_SET(type, DMG_SLASH))
	{
		if(IS_SET(flags, F_SLASH_RES25))
			mod = 1;
		else if(IS_SET(flags, F_SLASH_WEAK25))
			mod = -1;
		else
			mod = 0;
	}

	if(IS_SET(type, DMG_PIERCE))
	{
		if(IS_SET(flags, F_PIERCE_RES25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IS_SET(flags, F_PIERCE_WEAK25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}
	
	if(IS_SET(type, DMG_BLUNT))
	{
		if(IS_SET(flags, F_BLUNT_RES25))
		{
			if(mod == -2)
				mod = 1;
		}
		else if(IS_SET(flags, F_BLUNT_WEAK25))
			mod = -1;
		else if(mod != -1)
			mod = 0;
	}

	assert(mod != -2);
	return mod;
}

Game::ATTACK_RESULT Game::DoAttack(LevelContext& ctx, Unit& unit)
{
	VEC3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(ctx, unit, hitted, hitpoint))
		return ATTACK_NOT_HIT;

	return DoGenericAttack(ctx, unit, *hitted, hitpoint, unit.CalculateAttack()*unit.attack_power, unit.GetDmgType(), false);
}

void Game::GiveDmg(LevelContext& ctx, Unit* giver, float dmg, Unit& taker, const VEC3* hitpoint, int dmg_flags)
{
	// blood particles
	if(!IS_SET(dmg_flags, DMG_NO_BLOOD))
	{
		// krew
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = tKrew[taker.data->blood];
		pe->emision_interval = 0.01f;
		pe->life = 5.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 10;
		pe->spawn_max = 15;
		pe->max_particles = 15;
		if(hitpoint)
			pe->pos = *hitpoint;
		else
		{
			pe->pos = taker.pos;
			pe->pos.y += taker.GetUnitHeight() * 2.f/3;
		}
		pe->speed_min = VEC3(-1,0,-1);
		pe->speed_max = VEC3(1,1,1);
		pe->pos_min = VEC3(-0.1f,-0.1f,-0.1f);
		pe->pos_max = VEC3(0.1f,0.1f,0.1f);
		pe->size = 0.3f;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 0;
		pe->Init();
		ctx.pes->push_back(pe);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SPAWN_BLOOD;
			c.id = taker.data->blood;
			c.pos = pe->pos;
		}
	}

	// apply magic resistance
	if(IS_SET(dmg_flags, DMG_MAGICAL))
		dmg *= taker.CalculateMagicResistance();

	if(giver && giver->IsPlayer())
	{
		// update player damage done
		giver->player->dmg_done += (int)dmg;
		giver->player->stat_flags |= STAT_DMG_DONE;
	}

	if(taker.IsPlayer())
	{
		// update player damage taken
		taker.player->dmg_taken += (int)dmg;
		taker.player->stat_flags |= STAT_DMG_TAKEN;

		// train endurance
		taker.player->Train(TrainWhat::TakeDamage, min(dmg, taker.hp)/taker.hpmax, (giver ? giver->level : -1));

		// red screen
		taker.player->last_dmg += dmg;
	}

	// aggregate units
	if(giver && giver->IsPlayer())
		AttackReaction(taker, *giver);
	
	if((taker.hp -= dmg) <= 0.f && !taker.IsImmortal())
	{
		// unit killed		
		UnitDie(taker, &ctx, giver);
	}
	else
	{
		// unit hurt sound
		if(taker.hurt_timer <= 0.f && taker.data->sounds->sound[SOUND_PAIN])
		{
			if(sound_volume)
				PlayAttachedSound(taker, taker.data->sounds->sound[SOUND_PAIN], 2.f, 15.f);
			taker.hurt_timer = random(1.f, 1.5f);
			if(IS_SET(dmg_flags, DMG_NO_BLOOD))
				taker.hurt_timer += 1.f;
			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::HURT_SOUND;
				c.unit = &taker;
			}
		}
		
		// immortality
		if(taker.hp < 1.f)
			taker.hp = 1.f;

		// send update hp
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::UPDATE_HP;
			c.unit = &taker;
		}
	}
}

//=================================================================================================
// Aktualizuj jednostki na danym poziomie (pomieszczeniu)
//=================================================================================================
void Game::UpdateUnits(LevelContext& ctx, float dt)
{
	PROFILER_BLOCK("UpdateUnits");
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		//assert(u.rot >= 0.f && u.rot < PI*2);

// 		u.block_power += dt/10;
// 		if(u.block_power > 1.f)
// 			u.block_power = 1.f;

		// licznik okrzyku od ostatniego trafienia
		u.hurt_timer -= dt;
		u.last_bash -= dt;

		// aktualizuj efekty i machanie ustami
		if(u.IsAlive())
		{
			if(IsLocal())
				u.UpdateEffects(dt);
			if(u.IsStanding() && u.talking)
			{
				u.talk_timer += dt;
				u.ani->need_update = true;
			}
		}

		// zmieñ podstawow¹ animacjê
		if(u.animation != u.current_animation)
		{
			u.changed = true;
			switch(u.animation)
			{
			case ANI_WALK:
				u.ani->Play(NAMES::ani_move, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_WALK_TYL:
				u.ani->Play(NAMES::ani_move, PLAY_BACK|PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_RUN:
				u.ani->Play(NAMES::ani_run, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRunSpeed() / u.data->run_speed;
				break;
			case ANI_LEFT:
				u.ani->Play(NAMES::ani_left, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_RIGHT:
				u.ani->Play(NAMES::ani_right, PLAY_PRIO1|PLAY_RESTORE, 0);
				if(!IsClient2())
					u.ani->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_STAND:
				u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
				break;
			case ANI_BATTLE:
				u.ani->Play(NAMES::ani_battle, PLAY_PRIO1, 0);
				break;
			case ANI_BATTLE_BOW:
				u.ani->Play(NAMES::ani_battle_bow, PLAY_PRIO1, 0);
				break;
			case ANI_DIE:
				u.ani->Play(NAMES::ani_die, PLAY_STOP_AT_END|PLAY_ONCE|PLAY_PRIO3, 0);
				break;
			case ANI_PLAY:
				break;
			case ANI_IDLE:
				break;
			case ANI_KNEELS:
				u.ani->Play("kleka", PLAY_STOP_AT_END|PLAY_ONCE|PLAY_PRIO3, 0);
				break;
			default:
				assert(0);
				break;
			}
			u.current_animation = u.animation;
		}

		// koniec animacji idle
		if(u.animation == ANI_IDLE && u.ani->frame_end_info)
		{
			u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
			u.animation = ANI_STAND;
		}

		// aktualizuj animacjê
		u.ani->Update(dt);

		// zmieñ stan z umiera na umar³ i stwórz krew (chyba ¿e tylko upad³)
		if(!u.IsStanding())
		{
			if(u.ani->frame_end_info)
			{
				if(u.live_state == Unit::DYING)
				{
					u.live_state = Unit::DEAD;
					CreateBlood(ctx, u);
				}
				else if(u.live_state == Unit::FALLING)
					u.live_state = Unit::FALL;
				u.ani->frame_end_info = false;
			}
			if(u.action != A_POSITION)
				continue;
		}

		// aktualizuj akcjê
		switch(u.action)
		{
		case A_NONE:
			break;
		case A_TAKE_WEAPON:
			if(u.weapon_state == WS_TAKING)
			{
				if(u.animation_state == 0 && (u.ani->GetProgress2() >= u.data->frames->t[F_TAKE_WEAPON] || u.ani->frame_end_info2))
					u.animation_state = 1;
				if(u.ani->frame_end_info2)
				{
					u.weapon_state = WS_TAKEN;
					if(u.useable)
					{
						u.action = A_ANIMATION2;
						u.animation_state = AS_ANIMATION2_USING;
					}
					else
						u.action = A_NONE;
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;
				}
			}
			else
			{
				// chowanie broni
				if(u.animation_state == 0 && (u.ani->GetProgress2() <= u.data->frames->t[F_TAKE_WEAPON] || u.ani->frame_end_info2))
					u.animation_state = 1;
				if(u.weapon_taken != W_NONE && (u.animation_state == 1 || u.ani->frame_end_info2))
				{
					u.ani->Play(u.GetTakeWeaponAnimation(u.weapon_taken == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					u.weapon_state = WS_TAKING;
					u.weapon_hiding = W_NONE;
					u.animation_state = 1;
					u.ani->frame_end_info2 = false;
					u.animation_state = 0;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.unit = &u;
						c.id = 0;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else if(u.ani->frame_end_info2)
				{
					u.weapon_state = WS_HIDDEN;
					u.weapon_hiding = W_NONE;
					u.action = A_NONE;
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;

					if(&u == pc->unit)
					{
						switch(pc->next_action)
						{
						// zdejmowanie za³o¿onego przedmiotu
						case NA_REMOVE:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inventory->RemoveSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// zak³adanie przedmiotu po zdjêciu innego
						case NA_EQUIP:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_id != LOCK_REMOVED)
								game_gui->inventory->EquipSlotItem(Inventory::lock_index);
							break;
						// wyrzucanie za³o¿onego przedmiotu
						case NA_DROP:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inventory->DropSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// wypijanie miksturki
						case NA_CONSUME:
							assert(Inventory::lock_id == LOCK_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inventory->ConsumeItem(Inventory::lock_index);
							break;
						// u¿yj obiekt
						case NA_USE:
							if(before_player == BP_USEABLE && before_player_ptr.useable == pc->next_action_useable)
								PlayerUseUseable(pc->next_action_useable, true);
							break;
						// sprzedawanie za³o¿onego przedmiotu
						case NA_SELL:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inv_trade_mine->SellSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// chowanie za³o¿onego przedmiotu do kontenera
						case NA_PUT:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inv_trade_mine->PutSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						// daj przedmiot po schowaniu
						case NA_GIVE:
							assert(Inventory::lock_id == LOCK_TRADE_MY);
							Inventory::lock_id = LOCK_NO;
							if(Inventory::lock_index != LOCK_REMOVED)
								game_gui->inv_trade_mine->GiveSlotItem(IIndexToSlot(Inventory::lock_index));
							break;
						}
						pc->next_action = NA_NONE;
						assert(Inventory::lock_id == LOCK_NO);

						if(u.action == A_NONE && u.useable)
						{
							u.action = A_ANIMATION2;
							u.animation_state = AS_ANIMATION2_USING;
						}
					}
					else if(IsLocal() && u.IsAI() && u.ai->potion != -1)
					{
						u.ConsumeItem(u.ai->potion);
						u.ai->potion = -1;
					}
				}
			}
			break;
		case A_SHOOT:
			if(u.animation_state == 0)
			{
				if(u.ani->GetProgress2() > 20.f/40)
					u.ani->groups[1].time = 20.f/40*u.ani->groups[1].anim->length;
			}
			else if(u.animation_state == 1)
			{
				if(IsLocal() && !u.hitted && u.ani->GetProgress2() > 20.f / 40)
				{
					u.hitted = true;
					Bullet& b = Add1(ctx.bullets);
					b.level = u.level;
					b.backstab = 0;
					if(IS_SET(u.data->flags, F2_BACKSTAB))
						++b.backstab;
					if(IS_SET(u.GetBow().flags, ITEM_BACKSTAB))
						++b.backstab;

					if(u.human_data)
						u.ani->SetupBones(&u.human_data->mat_scale[0]);
					else
						u.ani->SetupBones();
					
					Animesh::Point* point = u.ani->ani->GetPoint(NAMES::point_weapon);
					assert(point);

					D3DXMatrixTranslation(&m1, u.pos);
					D3DXMatrixRotationY(&m2, u.rot);
					D3DXMatrixMultiply(&m1, &m2, &m1);
					m2 = point->mat * u.ani->mat_bones[point->bone] * m1;

					VEC3 coord;
					D3DXVec3TransformCoord(&b.pos, &VEC3(0,0,0), &m2);

					b.attack = u.CalculateAttack(&u.GetBow());
					b.rot = VEC3(PI/2, u.rot+PI, 0);
					b.mesh = aArrow;
					b.speed = u.GetArrowSpeed();
					b.timer = ARROW_TIMER;
					b.owner = &u;
					b.remove = false;
					b.pe = nullptr;
					b.spell = nullptr;
					b.tex = nullptr;
					b.poison_attack = 0.f;
					b.start_pos = b.pos;

					if(u.IsPlayer())
					{
						if(&u == pc->unit)
							b.yspeed = PlayerAngleY()*36;
						else
							b.yspeed = GetPlayerInfo(u.player->id).yspeed;
						u.player->Train(TrainWhat::BowStart, 0.f, 0);
					}
					else
					{
						b.yspeed = u.ai->shoot_yspeed;
						if(u.ai->state == AIController::Idle && u.ai->idle_action == AIController::Idle_TrainBow)
							b.attack = -100.f;
					}

					// losowe odchylenie
					int sk = u.Get(Skill::BOW);
					if(u.IsPlayer())
						sk += 10;
					else
						sk -= 10;
					if(sk < 50)
					{
						int szansa;
						float odchylenie_x, odchylenie_y;
						if(sk < 10)
						{
							szansa = 100;
							odchylenie_x = PI/16;
							odchylenie_y = 5.f;
						}
						else if(sk < 20)
						{
							szansa = 80;
							odchylenie_x = PI/20;
							odchylenie_y = 4.f;
						}
						else if(sk < 30)
						{
							szansa = 60;
							odchylenie_x = PI/26;
							odchylenie_y = 3.f;
						}
						else if(sk < 40)
						{
							szansa = 40;
							odchylenie_x = PI/34;
							odchylenie_y = 2.f;
						}
						else
						{
							szansa = 20;
							odchylenie_x = PI/48;
							odchylenie_y = 1.f;
						}

						if(rand2()%100 < szansa)
							b.rot.y += random_normalized(odchylenie_x);
						if(rand2()%100 < szansa)
							b.yspeed += random_normalized(odchylenie_y);
					}

					b.rot.y = clip(b.rot.y);

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = VEC4(1,1,1,0.5f);
					tpe->color2 = VEC4(1,1,1,0);
					tpe->Init(50);
					ctx.tpes->push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = VEC4(1,1,1,0.5f);
					tpe2->color2 = VEC4(1,1,1,0);
					tpe2->Init(50);
					ctx.tpes->push_back(tpe2);
					b.trail2 = tpe2;

					if(sound_volume)
						PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::SHOOT_ARROW;
						c.unit = &u;
						c.pos = b.start_pos;
						c.f[0] = b.rot.y;
						c.f[1] = b.yspeed;
						c.f[2] = b.rot.x;
						c.extra_f = b.speed;
					}
				}
				if(u.ani->GetProgress2() > 20.f/40)
					u.animation_state = 2;
			}
			else if(u.ani->GetProgress2() > 35.f/40)
			{
				u.animation_state = 3;
				if(u.ani->frame_end_info2)
				{
koniec_strzelania:
					u.ani->Deactivate(1);
					u.ani->frame_end_info2 = false;
					u.action = A_NONE;
					assert(u.bow_instance);
					bow_instances.push_back(u.bow_instance);
					u.bow_instance = nullptr;
					if(IsLocal() && u.IsAI())
					{
						float v = 1.f - float(u.Get(Skill::BOW)) / 100;
						u.ai->next_attack = random(v/2, v);
					}
					break;
				}
			}
			if(!u.ani)
			{
				// fix na skutek, nie na przyczynê ;(
#ifdef _DEBUG
				WARN(Format("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", u.GetName(), u.live_state, u.action, u.animation, u.current_animation, u.animation_state));
				AddGameMsg("Unit don't have shooting animation!", 5.f);
#endif
				goto koniec_strzelania;
			}
			u.bow_instance->groups[0].time = min(u.ani->groups[1].time, u.bow_instance->groups[0].anim->length);
			u.bow_instance->need_update = true;
			break;
		case A_ATTACK:
			if(u.animation_state == 0)
			{
				float t = u.GetAttackFrame(0);
				if(u.ani->ani->head.n_groups == 1)
				{
					if(u.ani->GetProgress() > t)
					{
						if(IsLocal() && u.IsAI())
						{
							u.ani->groups[0].speed = 1.f + u.GetAttackSpeed();
							u.attack_power = 2.f;
							++u.animation_state;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::ATTACK;
								c.unit = &u;
								c.id = AID_Attack;
								c.f[1] = u.ani->groups[0].speed;
							}
						}
						else
							u.ani->groups[0].time = t*u.ani->groups[0].anim->length;
					}
				}
				else
				{
					if(u.ani->GetProgress2() > t)
					{
						if(IsLocal() && u.IsAI())
						{
							u.ani->groups[1].speed = 1.f + u.GetAttackSpeed();
							u.attack_power = 2.f;
							++u.animation_state;
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::ATTACK;
								c.unit = &u;
								c.id = AID_Attack;
								c.f[1] = u.ani->groups[1].speed;
							}
						}
						else
							u.ani->groups[1].time = t*u.ani->groups[1].anim->length;
					}
				}
			}
			else
			{
				if(u.ani->ani->head.n_groups > 1)
				{
					if(u.animation_state == 1 && u.ani->GetProgress2() > u.GetAttackFrame(0))
					{
						if(IsLocal() && !u.hitted && u.ani->GetProgress2() >= u.GetAttackFrame(1))
						{
							ATTACK_RESULT result = DoAttack(ctx, u);
							if(result != ATTACK_NOT_HIT)
							{
								/*if(result == ATTACK_PARRIED)
								{
									u.ani->frame_end_info2 = false;
									u.ani->Play(rand2()%2 == 0 ? "atak1_p" : "atak2_p", PLAY_PRIO1|PLAY_ONCE, 1);
									u.action = A_PAIN;
									if(IsLocal() && u.IsAI())
									{
										float v = 1.f - float(u.skill[S_WEAPON])/100;
										u.ai->next_attack = 1.f+random(v/2, v);
									}
								}
								else*/
								u.hitted = true;
							}
						}
						if(u.ani->GetProgress2() >= u.GetAttackFrame(2) || u.ani->frame_end_info2)
						{
							// koniec mo¿liwego ataku
							u.animation_state = 2;
							u.ani->groups[1].speed = 1.f;
							u.run_attack = false;
						}
					}
					if(u.animation_state == 2 && u.ani->frame_end_info2)
					{
						u.run_attack = false;
						u.ani->Deactivate(1);
						u.ani->frame_end_info2 = false;
						u.action = A_NONE;
						if(IsLocal() && u.IsAI())
						{
							float v = 1.f - float(u.Get(Skill::ONE_HANDED_WEAPON)) / 100;
							u.ai->next_attack = random(v/2, v);
						}
					}
				}
				else
				{
					if(u.animation_state == 1 && u.ani->GetProgress() > u.GetAttackFrame(0))
					{
						if(IsLocal() && !u.hitted && u.ani->GetProgress() >= u.GetAttackFrame(1))
						{
							ATTACK_RESULT result = DoAttack(ctx, u);
							if(result != ATTACK_NOT_HIT)
							{
								/*u.ani->Deactivate(0);
								u.atak_w_biegu = false;
								u.action = A_NONE;
								if(IsLocal() && u.IsAI())
								{
									float v = 1.f - float(u.skill[S_WEAPON])/100;
									u.ai->next_attack = random(v/2, v);
								}*/
								u.hitted = true;
							}
						}
						if(u.ani->GetProgress() >= u.GetAttackFrame(2) || u.ani->frame_end_info)
						{
							// koniec mo¿liwego ataku
							u.animation_state = 2;
							u.ani->groups[0].speed = 1.f;
							u.run_attack = false;
						}
					}
					if(u.animation_state == 2 && u.ani->frame_end_info)
					{
						u.run_attack = false;
						u.animation = ANI_BATTLE;
						u.current_animation = ANI_STAND;
						u.action = A_NONE;
						if(IsLocal() && u.IsAI())
						{
							float v = 1.f - float(u.Get(Skill::ONE_HANDED_WEAPON)) / 100;
							u.ai->next_attack = random(v/2, v);
						}
					}
				}
			}
			break;
		case A_BLOCK:
			break;
		case A_BASH:
			if(u.animation_state == 0)
			{
				if(u.ani->GetProgress2() >= u.data->frames->t[F_BASH])
					u.animation_state = 1;
			}
			if(IsLocal() && u.animation_state == 1 && !u.hitted)
			{
				if(DoShieldSmash(ctx, u))
					u.hitted = true;
			}
			if(u.ani->frame_end_info2)
			{
				u.action = A_NONE;
				u.ani->frame_end_info2 = false;
				u.ani->Deactivate(1);
			}
			break;
		/*case A_PAROWANIE:
			if(u.ani->frame_end_info2)
			{
				u.action = A_NONE;
				u.ani->frame_end_info2 = false;
				u.ani->Deactivate(1);
			}
			break;*/
		case A_DRINK:
			{
				float p = u.ani->GetProgress2();
				if(p >= 28.f/52.f && u.animation_state == 0)
				{
					if(sound_volume)
						PlayUnitSound(u, sGulp);
					u.animation_state = 1;
					if(IsLocal())
						u.ApplyConsumableEffect(u.used_item->ToConsumable());
				}
				if(p >= 49.f/52.f && u.animation_state == 1)
				{
					u.animation_state = 2;
					u.used_item = nullptr;
				}
				if(u.ani->frame_end_info2)
				{
					if(u.useable)
					{
						u.animation_state = AS_ANIMATION2_USING;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			break;
		case A_EAT:
			{
				float p = u.ani->GetProgress2();
				if(p >= 32.f/70 && u.animation_state == 0)
				{
					u.animation_state = 1;
					if(sound_volume)
						PlayUnitSound(u, sEat);
				}
				if(p >= 48.f/70 && u.animation_state == 1)
				{
					u.animation_state = 2;
					if(IsLocal())
						u.ApplyConsumableEffect(u.used_item->ToConsumable());
				}
				if(p >= 60.f/70 && u.animation_state == 2)
				{
					u.animation_state = 3;
					u.used_item = nullptr;
				}
				if(u.ani->frame_end_info2)
				{
					if(u.useable)
					{
						u.animation_state = AS_ANIMATION2_USING;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			break;
		case A_PAIN:
			if(u.ani->ani->head.n_groups == 2)
			{
				if(u.ani->frame_end_info2)
				{
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			else if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_BATTLE;
				u.ani->frame_end_info = false;
			}
			break;
		case A_CAST:
			if(u.ani->ani->head.n_groups == 2)
			{
				if(IsLocal() && u.animation_state == 0 && u.ani->GetProgress2() >= u.data->frames->t[F_CAST])
				{
					u.animation_state = 1;
					CastSpell(ctx, u);
				}
				if(u.ani->frame_end_info2)
				{
					u.action = A_NONE;
					u.ani->frame_end_info2 = false;
					u.ani->Deactivate(1);
				}
			}
			else
			{
				if(IsLocal() && u.animation_state == 0 && u.ani->GetProgress() >= u.data->frames->t[F_CAST])
				{
					u.animation_state = 1;
					CastSpell(ctx, u);
				}
				if(u.ani->frame_end_info)
				{
					u.action = A_NONE;
					u.animation = ANI_BATTLE;
					u.ani->frame_end_info = false;
				}
			}
			break;
		case A_ANIMATION:
			if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_STAND;
				u.current_animation = (Animation)-1;
			}
			break;
		case A_ANIMATION2:
			{
				bool allow_move = true;
				if(IsOnline())
				{
					if(IsServer())
					{
						if(u.IsPlayer() && &u != pc->unit)
							allow_move = false;
					}
					else
					{
						if(!u.IsPlayer() || &u != pc->unit)
							allow_move = false;
					}
				}
				if(u.animation_state == AS_ANIMATION2_MOVE_TO_ENDPOINT)
				{
					u.timer += dt;
					if(allow_move && u.timer >= 0.5f)
					{
						u.visual_pos = u.pos = u.target_pos;
						u.useable->user = nullptr;
						if(IsOnline() && IsServer())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::USE_USEABLE;
							c.unit = &u;
							c.id = u.useable->netid;
							c.ile = 0;
						}
						u.useable = nullptr;
						u.action = A_NONE;
						break;
					}

					if(allow_move)
					{
						// przesuñ postaæ
						u.visual_pos = u.pos = lerp(u.target_pos2, u.target_pos, u.timer*2);

						// obrót
						float target_rot = lookat_angle(u.target_pos, u.useable->pos);
						float dif = angle_dif(u.rot, target_rot);
						if(not_zero(dif))
						{
							const float rot_speed = u.GetRotationSpeed() * 2 * dt;
							const float arc = shortestArc(u.rot, target_rot);

							if(dif <= rot_speed)
								u.rot = target_rot;
							else
								u.rot = clip(u.rot + sign(arc) * rot_speed);
						}
					}
				}
				else
				{
					BaseUsable& bu = g_base_usables[u.useable->type];

					if(u.animation_state > AS_ANIMATION2_MOVE_TO_OBJECT)
					{
						// odtwarzanie dŸwiêku
						if(bu.sound)
						{
							if(u.ani->GetProgress() >= bu.sound_timer)
							{
								if(u.animation_state == AS_ANIMATION2_USING)
								{
									u.animation_state = AS_ANIMATION2_USING_SOUND;
									if(sound_volume)
										PlaySound3d(bu.sound, u.GetCenter(), 2.f, 5.f);
									if(IsOnline() && IsServer())
									{
										NetChange& c = Add1(net_changes);
										c.type = NetChange::USEABLE_SOUND;
										c.unit = &u;
									}
								}
							}
							else if(u.animation_state == AS_ANIMATION2_USING_SOUND)
								u.animation_state = AS_ANIMATION2_USING;
						}
					}
					else if(IsLocal() || &u == pc->unit)
					{
						// ustal docelowy obrót postaci
						float target_rot;
						if(bu.limit_rot == 0)
							target_rot = u.rot;
						else if(bu.limit_rot == 1)
						{
							float rot1 = clip(u.use_rot + PI/2),
								dif1 = angle_dif(rot1, u.useable->rot),
								rot2 = clip(u.useable->rot+PI),
								dif2 = angle_dif(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.useable->rot;
							else
								target_rot = rot2;
						}
						else if(bu.limit_rot == 2)
							target_rot = u.useable->rot;
						else if(bu.limit_rot == 3)
						{
							float rot1 = clip(u.use_rot + PI),
								dif1 = angle_dif(rot1, u.useable->rot),
								rot2 = clip(u.useable->rot+PI),
								dif2 = angle_dif(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.useable->rot;
							else
								target_rot = rot2;
						}
						else
							target_rot = u.useable->rot+PI;
						target_rot = clip(target_rot);

						// obrót w strone obiektu
						const float dif = angle_dif(u.rot, target_rot);
						const float rot_speed = u.GetRotationSpeed()*2;
						if(allow_move && not_zero(dif))
						{
							const float rot_speed_dt = rot_speed * dt;
							if(dif <= rot_speed_dt)
								u.rot = target_rot;
							else
							{
								const float arc = shortestArc(u.rot, target_rot);
								u.rot = clip(u.rot + sign(arc) * rot_speed_dt);
							}
						}

						// czy musi siê obracaæ zanim zacznie siê przesuwaæ?
						if(dif < rot_speed*0.5f)
						{
							u.timer += dt;
							if(u.timer >= 0.5f)
							{
								u.timer = 0.5f;
								++u.animation_state;
							}

							// przesuñ postaæ i fizykê
							if(allow_move)
							{
								u.visual_pos = u.pos = lerp(u.target_pos, u.target_pos2, u.timer*2);
								global_col.clear();
								float my_radius = u.GetUnitRadius();
								bool ok = true;
								for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
								{
									if(&u == *it2 || !(*it2)->IsStanding())
										continue;

									float radius = (*it2)->GetUnitRadius();
									if(distance((*it2)->pos.x, (*it2)->pos.z, u.pos.x, u.pos.z) <= radius+my_radius)
									{
										ok = false;
										break;
									}
								}
								if(ok)
									UpdateUnitPhysics(u, u.pos);
							}
						}
					}
				}
			}
			break;
		case A_POSITION:
			u.timer += dt;
			if(u.animation_state == 1)
			{
				// obs³uga animacji cierpienia
				if(u.ani->ani->head.n_groups == 2)
				{
					if(u.ani->frame_end_info2 || u.timer >= 0.5f)
					{
						u.ani->frame_end_info2 = false;
						u.ani->Deactivate(1);
						u.animation_state = 2;
					}
				}
				else if(u.ani->frame_end_info || u.timer >= 0.5f)
				{
					u.animation = ANI_BATTLE;
					u.ani->frame_end_info = false;
					u.animation_state = 2;
				}
			}
			if(u.timer >= 0.5f)
			{
				u.visual_pos = u.pos = u.target_pos;
				u.useable->user = nullptr;
				if(IsOnline() && IsServer())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::USE_USEABLE;
					c.unit = &u;
					c.id = u.useable->netid;
					c.ile = 0;
				}
				u.useable = nullptr;
				u.action = A_NONE;
			}
			else
				u.visual_pos = u.pos = lerp(u.target_pos2, u.target_pos, u.timer*2);
			break;
		case A_PICKUP:
			if(u.ani->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_STAND;
				u.current_animation = (Animation)-1;
			}
			break;
		default:
			assert(0);
			break;
		}
	}
}

// dzia³a tylko dla cz³onków dru¿yny!
void Game::UpdateUnitInventory(Unit& u)
{
	bool changes = false;
	int index = 0;
	const Item* prev_slots[SLOT_MAX];
	for(int i=0; i<SLOT_MAX; ++i)
		prev_slots[i] = u.slots[i];

	for(vector<ItemSlot>::iterator it = u.items.begin(), end = u.items.end(); it != end; ++it, ++index)
	{
		if(!it->item || it->team_count != 0)
			continue;

		switch(it->item->type)
		{
		case IT_WEAPON:
			if(!u.HaveWeapon())
			{
				u.slots[SLOT_WEAPON] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(IS_SET(u.data->flags, F_MAGE))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(u.GetWeapon().flags, ITEM_MAGE))
					{
						if(u.GetWeapon().value < it->item->value)
						{
							std::swap(u.slots[SLOT_WEAPON], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(u.slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(u.GetWeapon().flags, ITEM_MAGE) && u.IsBetterWeapon(it->item->ToWeapon()))
					{
						std::swap(u.slots[SLOT_WEAPON], it->item);
						changes = true;
					}
				}
			}
			else if(u.IsBetterWeapon(it->item->ToWeapon()))
			{
				std::swap(u.slots[SLOT_WEAPON], it->item);
				changes = true;
			}
			break;
		case IT_BOW:
			if(!u.HaveBow())
			{
				u.slots[SLOT_BOW] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(u.GetBow().value < it->item->value)
			{
				std::swap(u.slots[SLOT_BOW], it->item);
				changes = true;
			}
			break;
		case IT_ARMOR:
			if(!u.HaveArmor())
			{
				u.slots[SLOT_ARMOR] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(IS_SET(u.data->flags, F_MAGE))
			{
				if(IS_SET(it->item->flags, ITEM_MAGE))
				{
					if(IS_SET(u.GetArmor().flags, ITEM_MAGE))
					{
						if(it->item->value > u.GetArmor().value)
						{
							std::swap(u.slots[SLOT_ARMOR], it->item);
							changes = true;
						}
					}
					else
					{
						std::swap(u.slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
				else
				{
					if(!IS_SET(u.GetArmor().flags, ITEM_MAGE) && u.IsBetterArmor(it->item->ToArmor()))
					{
						std::swap(u.slots[SLOT_ARMOR], it->item);
						changes = true;
					}
				}
			}
			else if(u.IsBetterArmor(it->item->ToArmor()))
			{
				std::swap(u.slots[SLOT_ARMOR], it->item);
				changes = true;
			}
			break;
		case IT_SHIELD:
			if(!u.HaveShield())
			{
				u.slots[SLOT_SHIELD] = it->item;
				it->item = nullptr;
				changes = true;
			}
			else if(u.GetShield().value < it->item->value)
			{
				std::swap(u.slots[SLOT_SHIELD], it->item);
				changes = true;
			}
			break;
		default:
			break;
		}
	}

	if(changes)
	{
		RemoveNullItems(u.items);
		SortItems(u.items);

		if(IsOnline() && players > 1)
		{
			for(int i=0; i<SLOT_MAX; ++i)
			{
				if(u.slots[i] != prev_slots[i])
				{
					NetChange& c = Add1(net_changes);
					c.unit = &u;
					c.type = NetChange::CHANGE_EQUIPMENT;
					c.id = i;
				}
			}
		}
	}
}

// sprawdza czy ktoœ z dru¿yny jeszcze ¿yje
bool Game::IsAnyoneAlive() const
{
	for(vector<Unit*>::const_iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsAlive() || (*it)->in_arena != -1)
			return true;
	}
	return false;
}

bool Game::DoShieldSmash(LevelContext& ctx, Unit& attacker)
{
	assert(attacker.HaveShield());

	VEC3 hitpoint;
	Unit* hitted;
	Animesh* mesh = attacker.GetShield().mesh;

	if(!mesh)
		return false;

	if(!CheckForHit(ctx, attacker, hitted, *mesh->FindPoint("hit"), attacker.ani->ani->GetPoint(NAMES::point_shield), hitpoint))
		return false;

	if(!IS_SET(hitted->data->flags, F_DONT_SUFFER) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->Get(Attribute::END)) / 50.f;

		BreakAction(*hitted);

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animation_state = 1;

		if(hitted->ani->ani->head.n_groups == 2)
		{
			hitted->ani->frame_end_info2 = false;
			hitted->ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
			hitted->ani->groups[1].speed = 1.f;
		}
		else
		{
			hitted->ani->frame_end_info = false;
			hitted->ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
			hitted->ani->groups[0].speed = 1.f;
			hitted->animation = ANI_PLAY;
		}

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.unit = hitted;
			c.type = NetChange::STUNNED;
		}
	}

	DoGenericAttack(ctx, attacker, *hitted, hitpoint, attacker.CalculateShieldAttack(), DMG_BLUNT, true);

	return true;
}

VEC4 Game::GetFogColor()
{
	return fog_color;
}

VEC4 Game::GetFogParams()
{
	if(cl_fog)
		return fog_params;
	else
		return VEC4(cam.draw_range, cam.draw_range+1, 1, 0);
}

VEC4 Game::GetAmbientColor()
{
	if(!cl_lighting)
		return VEC4(1, 1, 1, 1);
	//return VEC4(0.1f,0.1f,0.1f,1);
	return ambient_color;
}

VEC4 Game::GetLightDir()
{
// 	VEC3 light_dir(1, 1, 1);
// 	D3DXVec3Normalize(&light_dir, &light_dir);
// 	return VEC4(light_dir, 1);

	VEC3 light_dir(sin(light_angle), 2.f, cos(light_angle));
	D3DXVec3Normalize(&light_dir, &light_dir);
	return VEC4(light_dir, 1);
}

VEC4 Game::GetLightColor()
{
	return VEC4(1,1,1,1);
	//return VEC4(0.5f,0.5f,0.5f,1);
}

inline bool CanRemoveBullet(const Bullet& b)
{
	return b.remove;
}

struct BulletCallback : public btCollisionWorld::ContactResultCallback
{
	BulletCallback(btCollisionObject* _bullet, btCollisionObject* _ignore) : target(nullptr), hit(false), bullet(_bullet), ignore(_ignore), hit_unit(false)
	{
		assert(_bullet);
	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		if(colObj0Wrap->getCollisionObject() == bullet)
		{
			if(colObj1Wrap->getCollisionObject() == ignore)
				return 1.f;
			hitpoint = ToVEC3(cp.getPositionWorldOnA());
			if(!target)
			{
				if(IS_SET(colObj1Wrap->getCollisionObject()->getCollisionFlags(), CG_UNIT))
					hit_unit = true;
				// to nie musi byæ jednostka :/
				target = (Unit*)colObj1Wrap->getCollisionObject()->getUserPointer();
			}
		}
		else
		{
			if(colObj0Wrap->getCollisionObject() == ignore)
				return 1.f;
			hitpoint = ToVEC3(cp.getPositionWorldOnB());
			if(!target)
			{
				if(IS_SET(colObj0Wrap->getCollisionObject()->getCollisionFlags(), CG_UNIT))
					hit_unit = true;
				// to nie musi byæ jednostka :/
				target = (Unit*)colObj0Wrap->getCollisionObject()->getUserPointer();
			}
		}

		hit = true;
		return 0.f;
	}

	btCollisionObject* bullet, *ignore;
	Unit* target;
	VEC3 hitpoint;
	bool hit, hit_unit;
};

void Game::UpdateBullets(LevelContext& ctx, float dt)
{
	bool deletions = false;

	for(vector<Bullet>::iterator it = ctx.bullets->begin(), end = ctx.bullets->end(); it != end; ++it)
	{
		// update position
		it->pos += VEC3(sin(it->rot.y)*it->speed, it->yspeed, cos(it->rot.y)*it->speed) * dt;
		if(it->spell && it->spell->type == Spell::Ball)
			it->yspeed -= 10.f*dt;

		// update particles
		if(it->pe)
			it->pe->pos = it->pos;
		if(it->trail)
		{
			VEC3 pt1 = it->pos;
			pt1.y += 0.05f;
			VEC3 pt2 = it->pos;
			pt2.y -= 0.05f;
			it->trail->Update(0, &pt1, &pt2);

			pt1 = it->pos;
			pt1.x += sin(it->rot.y+PI/2)*0.05f;
			pt1.z += cos(it->rot.y+PI/2)*0.05f;
			pt2 = it->pos;
			pt2.x -= sin(it->rot.y+PI/2)*0.05f;
			pt2.z -= cos(it->rot.y+PI/2)*0.05f;
			it->trail2->Update(0, &pt1, &pt2);
		}
		
		if((it->timer -= dt) <= 0.f)
		{
			// timeout, delete bullet
			it->remove = true;
			deletions = true;
			if(it->trail)
			{
				it->trail->destroy = true;
				it->trail2->destroy = true;
			}
			if(it->pe)
				it->pe->destroy = true;
		}
		else
		{
			btCollisionObject* cobj;

			if(!it->spell)
				cobj = obj_arrow;
			else
			{
				cobj = obj_spell;
				cobj->setCollisionShape(it->spell->shape);
			}
				
			btTransform& tr = cobj->getWorldTransform();
			tr.setOrigin(ToVector3(it->pos));
			tr.setRotation(btQuaternion(it->rot.y, it->rot.x, it->rot.z));

			BulletCallback callback(cobj, it->owner ? it->owner->cobj : nullptr);
			phy_world->contactTest(cobj, callback);

			if(callback.hit)
			{
				Unit* hitted = callback.target;

				// something was hit, remove bullet
				it->remove = true;
				deletions = true;
				if(it->trail)
				{
					it->trail->destroy = true;
					it->trail2->destroy = true;
				}
				if(it->pe)
					it->pe->destroy = true;

				if(callback.hit_unit && hitted)
				{
					if(!IsLocal())
						continue;

					if(!it->spell)
					{
						if(it->owner && IsFriend(*it->owner, *hitted) || it->attack < -50.f)
						{
							// frendly fire
							if(hitted->action == A_BLOCK && angle_dif(clip(it->rot.y+PI), hitted->rot) < PI*2/5)
							{
								MATERIAL_TYPE mat = hitted->GetShield().material;
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::HIT_SOUND;
									c.id = MAT_IRON;
									c.ile = mat;
									c.pos = callback.hitpoint;
								}
							}
							else
								PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, false);
							continue;
						}

						if(it->owner && hitted->IsAI())
							AI_HitReaction(*hitted, it->start_pos);

						// trafienie w postaæ
						float dmg = it->attack,
							  def = hitted->CalculateDefense();

						int mod = ObliczModyfikator(DMG_PIERCE, hitted->data->flags);
						float m = 1.f;
						if(mod == -1)
							m += 0.25f;
						else if(mod == 1)
							m -= 0.25f;
						if(hitted->IsNotFighting())
							m += 0.1f; // 10% do dmg jeœli ma schowan¹ broñ

						// backstab bonus damage
						float kat = angle_dif(it->rot.y, hitted->rot);
						float backstab_mod;
						if(it->backstab == 0)
							backstab_mod = 0.25f;
						if(it->backstab == 1)
							backstab_mod = 0.5f;
						else
							backstab_mod = 0.75f;
						if(IS_SET(hitted->data->flags2, F2_BACKSTAB_RES))
							backstab_mod /= 2;
						m += kat/PI*backstab_mod;

						// modyfikator obra¿eñ
						dmg *= m;
						float base_dmg = dmg;

						if(hitted->action == A_BLOCK && kat < PI*2/5)
						{
							float blocked = hitted->CalculateBlock(&hitted->GetShield()) * hitted->ani->groups[1].GetBlendT() * (1.f - kat/(PI*2/5));
							dmg -= blocked;

							MATERIAL_TYPE mat = hitted->GetShield().material;
							if(sound_volume)
								PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::HIT_SOUND;
								c.id = MAT_IRON;
								c.ile = mat;
								c.pos = callback.hitpoint;
							}

							if(hitted->IsPlayer())
							{
								// player blocked bullet, train shield
								hitted->player->Train(TrainWhat::BlockBullet, base_dmg/hitted->hpmax, it->level);
							}

							if(dmg < 0)
							{
								// shot blocked by shield
								if(it->owner && it->owner->IsPlayer())
								{
									// train player in bow
									it->owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
									// aggregate
									AttackReaction(*hitted, *it->owner);
								}
								continue;
							}
						}

						// odpornoœæ/pancerz
						dmg -= def;

						// szkol gracza w pancerzu/hp
						if(hitted->IsPlayer())
							hitted->player->Train(TrainWhat::TakeDamageArmor, base_dmg/hitted->hpmax, it->level);

						// hit sound
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, dmg>0.f);

						if(dmg < 0)
						{
							if(it->owner && it->owner->IsPlayer())
							{
								// train player in bow
								it->owner->player->Train(TrainWhat::BowNoDamage, 0.f, hitted->level);
								// aggregate
								AttackReaction(*hitted, *it->owner);
							}
							continue;
						}

						// szkol gracza w ³uku
						if(it->owner && it->owner->IsPlayer())
						{
							float v = dmg/hitted->hpmax;
							if(hitted->hp - dmg < 0.f && !hitted->IsImmortal())
								v = max(TRAIN_KILL_RATIO, v);
							if(v > 1.f)
								v = 1.f;
							it->owner->player->Train(TrainWhat::BowAttack, v, hitted->level);
						}

						GiveDmg(ctx, it->owner, dmg, *hitted, &callback.hitpoint, 0);

						// apply poison
						if(it->poison_attack > 0.f && !IS_SET(hitted->data->flags, F_POISON_RES))
						{
							Effect& e = Add1(hitted->effects);
							e.power = it->poison_attack/5;
							e.time = 5.f;
							e.effect = E_POISON;
						}
					}
					else
					{
						// trafienie w postaæ z czara
						if(it->owner && IsFriend(*it->owner, *hitted))
						{
							// frendly fire
							SpellHitEffect(ctx, *it, callback.hitpoint, hitted);

							// dŸwiêk trafienia w postaæ
							if(hitted->action == A_BLOCK && angle_dif(clip(it->rot.y+PI), hitted->rot) < PI*2/5)
							{
								MATERIAL_TYPE mat = hitted->GetShield().material;
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, mat), callback.hitpoint, 2.f, 10.f);
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::HIT_SOUND;
									c.id = MAT_IRON;
									c.ile = mat;
									c.pos = callback.hitpoint;
								}
							}
							else
								PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), callback.hitpoint, 2.f, false);
							continue;
						}

						if(hitted->IsAI())
							AI_HitReaction(*hitted, it->start_pos);

						float dmg = it->attack;
						if(it->owner)
							dmg += it->owner->level * it->spell->dmg_bonus;
						float kat = angle_dif(clip(it->rot.y+PI), hitted->rot);
						float base_dmg = dmg;

						if(hitted->action == A_BLOCK && kat < PI*2/5)
						{
							float blocked = hitted->CalculateBlock(&hitted->GetShield()) * hitted->ani->groups[1].GetBlendT() * (1.f - kat/(PI*2/5));
							dmg -= blocked/2;

							if(hitted->IsPlayer())
							{
								// player blocked spell, train him
								hitted->player->Train(TrainWhat::BlockBullet, base_dmg/hitted->hpmax, it->level);
							}

							if(dmg < 0)
							{
								// blocked by shield
								SpellHitEffect(ctx, *it, callback.hitpoint, hitted);
								continue;
							}
						}

						GiveDmg(ctx, it->owner, dmg, *hitted, &callback.hitpoint, !IS_SET(it->spell->flags, Spell::Poison) ? DMG_MAGICAL : 0);

						// apply poison
						if(IS_SET(it->spell->flags, Spell::Poison) && !IS_SET(hitted->data->flags, F_POISON_RES))
						{
							Effect& e = Add1(hitted->effects);
							e.power = dmg/5;
							e.time = 5.f;
							e.effect = E_POISON;
						}

						// apply spell effect
						SpellHitEffect(ctx, *it, callback.hitpoint, hitted);
					}
				}
				else
				{
					// trafiono w obiekt
					if(!it->spell)
					{
						if(sound_volume)
							PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), callback.hitpoint, 2.f, 10.f);

						ParticleEmitter* pe = new ParticleEmitter;
						pe->tex = tIskra;
						pe->emision_interval = 0.01f;
						pe->life = 5.f;
						pe->particle_life = 0.5f;
						pe->emisions = 1;
						pe->spawn_min = 10;
						pe->spawn_max = 15;
						pe->max_particles = 15;
						pe->pos = callback.hitpoint;
						pe->speed_min = VEC3(-1,0,-1);
						pe->speed_max = VEC3(1,1,1);
						pe->pos_min = VEC3(-0.1f,-0.1f,-0.1f);
						pe->pos_max = VEC3(0.1f,0.1f,0.1f);
						pe->size = 0.3f;
						pe->op_size = POP_LINEAR_SHRINK;
						pe->alpha = 0.9f;
						pe->op_alpha = POP_LINEAR_SHRINK;
						pe->mode = 0;
						pe->Init();
						ctx.pes->push_back(pe);

						if(IsLocal() && in_tutorial && callback.target)
						{
							void* ptr = (void*)callback.target;
							if((ptr == tut_shield || ptr == tut_shield2) && tut_state == 12)
							{
								Train(*pc->unit, true, (int)Skill::BOW, 1);
								tut_state = 13;
								int unlock = 6;
								int activate = 8;
								for(vector<TutorialText>::iterator it = ttexts.begin(), end = ttexts.end(); it != end; ++it)
								{
									if(it->id == activate)
									{
										it->state = 1;
										break;
									}
								}
								for(vector<Door*>::iterator it = local_ctx.doors->begin(), end = local_ctx.doors->end(); it != end; ++it)
								{
									if((*it)->locked == LOCK_TUTORIAL+unlock)
									{
										(*it)->locked = LOCK_NONE;
										break;
									}
								}
							}
						}
					}
					else
					{
						// trafienie czarem w obiekt
						SpellHitEffect(ctx, *it, callback.hitpoint, nullptr);
					}
				}
			}
		}
	}

	if(deletions)
		RemoveElements(ctx.bullets, CanRemoveBullet);
}

void Game::SpawnDungeonColliders()
{
	assert(!location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Pole* m = lvl.map;
	int w = lvl.w,
		h = lvl.h;

	btCollisionObject* cobj;

	for(int y=1; y<h-1; ++y)
	{
		for(int x=1; x<w-1; ++x)
		{
			if(czy_blokuje2(m[x+y*w]) && (!czy_blokuje2(m[x-1+(y-1)*w]) || !czy_blokuje2(m[x+(y-1)*w]) || !czy_blokuje2(m[x+1+(y-1)*w]) ||
				!czy_blokuje2(m[x-1+y*w]) || !czy_blokuje2(m[x+1+y*w]) ||
				!czy_blokuje2(m[x-1+(y+1)*w]) || !czy_blokuje2(m[x+(y+1)*w]) || !czy_blokuje2(m[x+1+(y+1)*w])))
			{
				cobj = new btCollisionObject;
				cobj->setCollisionShape(shape_wall);
				cobj->getWorldTransform().getOrigin().setValue(2.f*x+1.f, 2.f, 2.f*y+1.f);
				cobj->setCollisionFlags(CG_WALL);
				phy_world->addCollisionObject(cobj);
			}
		}
	}

	// lewa/prawa œciana
	for(int i=1; i<h-1; ++i)
	{
		// lewa
		if(czy_blokuje2(m[i*w]) && !czy_blokuje2(m[1+i*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(1.f, 2.f, 2.f*i+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}

		// prawa
		if(czy_blokuje2(m[i*w+w-1]) && !czy_blokuje2(m[w-2+i*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*(w-1)+1.f, 2.f, 2.f*i+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}
	}

	// przednia/tylna œciana
	for(int i=1; i<lvl.w-1; ++i)
	{
		// przednia
		if(czy_blokuje2(m[i+(h-1)*w]) && !czy_blokuje2(m[i+(h-2)*w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*i+1.f, 2.f, 2.f*(h-1)+1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}

		// tylna
		if(czy_blokuje2(m[i]) && !czy_blokuje2(m[i+w]))
		{
			cobj = new btCollisionObject;
			cobj->setCollisionShape(shape_wall);
			cobj->getWorldTransform().getOrigin().setValue(2.f*i+1.f, 2.f, 1.f);
			cobj->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(cobj);
		}
	}

	// schody w górê
	if(inside->HaveUpStairs())
	{
		cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_schody);
		cobj->getWorldTransform().getOrigin().setValue(2.f*lvl.staircase_up.x + 1.f, 0.f, 2.f*lvl.staircase_up.y + 1.f);
		cobj->getWorldTransform().setRotation(btQuaternion(dir_to_rot(lvl.staircase_up_dir), 0, 0));
		cobj->setCollisionFlags(CG_WALL);
		phy_world->addCollisionObject(cobj);
	}
}

void Game::RemoveColliders()
{
	if(phy_world)
		phy_world->Reset();

	DeleteElements(shapes);
}

void Game::CreateCollisionShapes()
{
	shape_wall = new btBoxShape(btVector3(1.f, 2.f, 1.f));
	shape_low_ceiling = new btBoxShape(btVector3(1.f, 0.5f, 1.f));
	shape_ceiling = new btStaticPlaneShape(btVector3(0.f, -1.f, 0.f), 4.f);
	shape_floor = new btStaticPlaneShape(btVector3(0.f, 1.f, 0.f), 0.f);
	shape_door = new btBoxShape(btVector3(0.842f, 1.319f, 0.181f));
	shape_block = new btBoxShape(btVector3(1.f,4.f,1.f));
	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f,2.f,0.1f));
	shape_schody_c[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f,2.f,0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f,2.f,1.f));
	shape_schody_c[1] = b;
	tr.setOrigin(btVector3(-0.95f,2.f,0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f,2.f,0.f));
	s->addChildShape(tr, b);
	shape_schody = s;

	Animesh::Point* point = aArrow->FindPoint("Empty");
	assert(point && point->IsBox());

	btBoxShape* box = new btBoxShape(ToVector3(point->size));

	/*btCompoundShape* comp = new btCompoundShape;
	btTransform tr(btQuaternion(), btVector3(0.f,point->size.y,0.f));
	comp->addChildShape(tr, box);*/

	obj_arrow = new btCollisionObject;
	obj_arrow->setCollisionShape(box);

	obj_spell = new btCollisionObject;
}

inline float dot2d(const VEC3& v1, const VEC3& v2)
{
	return (v1.x*v2.x + v1.z*v2.z);
}

inline float dot2d(const VEC3& v1)
{
	return (v1.x*v1.x + v1.z*v1.z);
}

VEC3 Game::PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const
{
	if(bullet_speed == 0.f)
		return target.GetCenter();
	else
	{
		VEC3 vel = target.pos - target.prev_pos;
		vel *= 60;

		float a = bullet_speed*bullet_speed - dot2d(vel);
		float b = -2*dot2d(vel, target.pos-me.pos);
		float c = -dot2d(target.pos-me.pos);

		float delta = b*b-4*a*c;
		// brak rozwi¹zania, nie mo¿e trafiæ wiêc strzel w aktualn¹ pozycjê
		if(delta < 0)
			return target.GetCenter();

		VEC3 pos = target.pos + ((b+std::sqrt(delta))/(2*a)) * VEC3(vel.x,0,vel.z);
		pos.y += target.GetUnitHeight()/2;
		return pos;
	}
}

struct BulletRaytestCallback : public btCollisionWorld::RayResultCallback
{
	BulletRaytestCallback(const void* _ignore1, const void* _ignore2) : ignore1(_ignore1), ignore2(_ignore2), clear(true)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(OR2_EQ(rayResult.m_collisionObject->getUserPointer(), ignore1, ignore2))
			return 1.f;
		clear = false;
		return 0.f;
	}

	bool clear;
	const void* ignore1, *ignore2;
};

struct BulletRaytestCallback2 : public btCollisionWorld::RayResultCallback
{
	BulletRaytestCallback2() : clear(true)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		if(IS_SET(rayResult.m_collisionObject->getCollisionFlags(), CG_UNIT))
			return 1.f;
		clear = false;
		return 0.f;
	}

	bool clear;
};

bool Game::CanShootAtLocation(const VEC3& from, const VEC3& to) const
{
	BulletRaytestCallback2 callback;
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);
	return callback.clear;
}

bool Game::CanShootAtLocation2(const Unit& me, const void* ptr, const VEC3& to) const
{
	BulletRaytestCallback callback(&me, ptr);
	phy_world->rayTest(btVector3(me.pos.x, me.pos.y+1.f, me.pos.z), btVector3(to.x, to.y+1.f, to.z), callback);
	return callback.clear;
}

void Game::LoadItemsData()
{
	for(auto it : g_items)
	{
		Item& item = *it.second;

		if(IS_SET(item.flags, ITEM_TEX_ONLY))
		{
			item.mesh = nullptr;
			auto tex = resMgr.TryGetTexture(item.mesh_id.c_str());
			if(tex)
				resMgr.GetLoadedTexture(item.mesh_id.c_str(), item.tex);
			else
			{
				item.tex = missing_texture;
				WARN(Format("Missing item texture '%s'.", item.mesh_id.c_str()));
				++load_errors;
			}
		}
		else
		{
			auto mesh = resMgr.TryGetMesh(item.mesh_id.c_str());
			if(mesh)
				resMgr.GetLoadedMesh(item.mesh_id, Task(&item, TaskCallback(this, &Game::GenerateImage)));
			else
			{
				item.mesh = nullptr;
				item.tex = missing_texture;
				item.flags &= ~ITEM_GROUND_MESH;
				WARN(Format("Missing item mesh '%s'.", item.mesh_id.c_str()));
				++load_errors;
			}
		}
	}
}

void Game::SpawnTerrainCollider()
{
	if(terrain_shape)
		delete terrain_shape;

	terrain_shape = new btHeightfieldTerrainShape(OutsideLocation::size+1, OutsideLocation::size+1, terrain->GetHeightMap(), 1.f, 0.f, 10.f, 1, PHY_FLOAT, false);
	terrain_shape->setLocalScaling(btVector3(2.f, 1.f, 2.f));

	obj_terrain = new btCollisionObject;
	obj_terrain->setCollisionShape(terrain_shape);
	obj_terrain->getWorldTransform().setOrigin(btVector3(float(OutsideLocation::size), 5.f, float(OutsideLocation::size)));
	obj_terrain->setCollisionFlags(CG_WALL);
	phy_world->addCollisionObject(obj_terrain);
}

void Game::GenerateDungeonObjects()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	static vector<Chest*> room_chests;
	static vector<VEC3> on_wall;
	static vector<INT2> blocks;
	IgnoreObjects ignore = {0};
	ignore.ignore_blocks = true;
	int chest_lvl = GetDungeonLevelChest();
	
	// dotyczy tylko pochodni
	int flags = 0;
	if(IS_SET(base.options, BLO_MAGIC_LIGHT))
		flags = SOE_MAGIC_LIGHT;

	bool wymagany = false;
	if(base.wymagany.room)
		wymagany = true;

	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
	{
		if(it->IsCorridor())
			continue;

		RoomType* rt;

		// dodaj blokady
		for(int x=0; x<it->size.x; ++x)
		{
			// górna krawêdŸ
			POLE co = lvl.map[it->pos.x + x + (it->pos.y + it->size.y - 1)*lvl.w].type;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-1));
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-2));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+it->size.y-1));

			// dolna krawêdŸ
			co = lvl.map[it->pos.x + x + it->pos.y*lvl.w].type;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+x, it->pos.y));
				blocks.push_back(INT2(it->pos.x+x, it->pos.y+1));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+x, it->pos.y));
		}
		for(int y=0; y<it->size.y; ++y)
		{
			// lewa krawêdŸ
			POLE co = lvl.map[it->pos.x + (it->pos.y + y)*lvl.w].type;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x, it->pos.y+y));
				blocks.push_back(INT2(it->pos.x+1, it->pos.y+y));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x, it->pos.y+y));

			// prawa krawêdŸ
			co = lvl.map[it->pos.x + it->size.x - 1 + (it->pos.y + y)*lvl.w].type;
			if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
			{
				blocks.push_back(INT2(it->pos.x+it->size.x-1, it->pos.y+y));
				blocks.push_back(INT2(it->pos.x+it->size.x-2, it->pos.y+y));
			}
			else if(co == SCIANA || co == BLOKADA_SCIANA)
				blocks.push_back(INT2(it->pos.x+it->size.x-1, it->pos.y+y));
		}
		if(it->target != RoomTarget::None)
		{
			if(it->target == RoomTarget::Treasury)
				rt = FindRoomType("krypta_skarb");
			else if(it->target == RoomTarget::Throne)
				rt = FindRoomType("tron");
			else if(it->target == RoomTarget::PortalCreate)
				rt = FindRoomType("portal");
			else
			{
				INT2 pt;
				if(it->target == RoomTarget::StairsDown)
					pt = lvl.staircase_down;
				else if(it->target == RoomTarget::StairsUp)
					pt = lvl.staircase_up;
				else if(it->target == RoomTarget::Portal)
				{
					if(inside->portal)
						pt = pos_to_pt(inside->portal->pos);
					else
					{
						Object* o = local_ctx.FindObject(FindObject("portal"));
						if(o)
							pt = pos_to_pt(o->pos);
						else
							pt = it->CenterTile();
					}
				}

				for(int y=-1; y<=1; ++y)
				{
					for(int x=-1; x<=1; ++x)
						blocks.push_back(INT2(pt.x+x, pt.y+y));
				}

				if(base.schody.room)
					rt = base.schody.room;
				else
					rt = base.GetRandomRoomType();
			}
		}
		else
		{
			if(wymagany)
				rt = base.wymagany.room;
			else
				rt = base.GetRandomRoomType();
		}

		int fail = 10;
		bool wymagany_obiekt = false;

		for(int i=0; i<rt->count && fail > 0; ++i)
		{
			bool is_variant = false;
			Obj* obj = FindObject(rt->objs[i].id, &is_variant);
			if(!obj)
				continue;

			int count = random(rt->objs[i].count);

			for(int j=0; j<count && fail > 0; ++j)
			{
				VEC3 pos;
				float rot;
				VEC2 shift;

				if(obj->type == OBJ_CYLINDER)
				{
					shift.x = obj->r + obj->extra_dist;
					shift.y = obj->r + obj->extra_dist;
				}
				else
					shift = obj->size + VEC2(obj->extra_dist, obj->extra_dist);

				if(IS_SET(obj->flags, OBJ_NEAR_WALL))
				{
					INT2 tile;
					int dir;
					if(!lvl.GetRandomNearWallTile(*it, tile, dir, IS_SET(obj->flags, OBJ_ON_WALL)))
					{
						if(IS_SET(obj->flags, OBJ_IMPORTANT))
							--j;
						--fail;
						continue;
					}
					
					rot = dir_to_rot(dir);

					if(dir == 2 || dir == 3)
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f)+2, 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f)+2);
					else
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f), 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f));

					if(IS_SET(obj->flags, OBJ_ON_WALL))
					{
						switch(dir)
						{
						case 0:
							pos.x += 1.f;
							break;
						case 1:
							pos.z += 1.f;
							break;
						case 2:
							pos.x -= 1.f;
							break;
						case 3:
							pos.z -= 1.f;
							break;
						}

						bool ok = true;

						for(vector<VEC3>::iterator it2 = on_wall.begin(), end2 = on_wall.end(); it2 != end2; ++it2)
						{
							float dist = distance2d(*it2, pos);
							if(dist < 2.f)
							{
								ok = false;
								continue;
							}
						}

						if(!ok)
						{
							if(IS_SET(obj->flags, OBJ_IMPORTANT))
								--j;
							fail = true;
							continue;
						}
					}
					else
					{
						switch(dir)
						{
						case 0:
							pos.x += random(0.2f,1.8f);
							break;
						case 1:
							pos.z += random(0.2f,1.8f);
							break;
						case 2:
							pos.x -= random(0.2f,1.8f);
							break;
						case 3:
							pos.z -= random(0.2f,1.8f);
							break;
						}
					}
				}
				else if(IS_SET(obj->flags, OBJ_IN_MIDDLE))
				{
					rot = PI/2*(rand2()%4);
					pos = it->Center();
					switch(rand2()%4)
					{
					case 0:
						pos.x += 2;
						break;
					case 1:
						pos.x -= 2;
						break;
					case 2:
						pos.z += 2;
						break;
					case 3:
						pos.z -= 2;
						break;
					}
				}
				else
				{
					rot = random(MAX_ANGLE);

					if(obj->type == OBJ_CYLINDER)
						pos = it->GetRandomPos(max(obj->size.x, obj->size.y));
					else
						pos = it->GetRandomPos(obj->r);
				}

				if(IS_SET(obj->flags, OBJ_HIGH))
					pos.y += 1.5f;

				if(obj->type == OBJ_HITBOX)
				{
					// sprawdŸ kolizje z blokami
					if(!IS_SET(obj->flags, OBJ_NO_PHYSICS))
					{
						bool ok = true;
						if(not_zero(rot))
						{
							RotRect r1, r2;
							r1.center.x = pos.x;
							r1.center.y = pos.z;
							r1.rot = rot;
							r1.size = shift;
							r2.rot = 0;
							r2.size = VEC2(1,1);
							for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
							{
								r2.center.x = 2.f*b_it->x+1.f;
								r2.center.y = 2.f*b_it->y+1.f;
								if(RotatedRectanglesCollision(r1, r2))
								{
									ok = false;
									break;
								}
							}
						}
						else
						{
							for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
							{
								if(RectangleToRectangle(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y, 2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x+1), 2.f*(b_it->y+1)))
								{
									ok = false;
									break;
								}
							}
						}
						if(!ok)
						{
							if(IS_SET(obj->flags, OBJ_IMPORTANT))
								--j;
							--fail;
							continue;
						}
					}

					// sprawdŸ kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
					if(!global_col.empty() && Collide(global_col, BOX2D(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y), 0.8f, rot))
					{
						if(IS_SET(obj->flags, OBJ_IMPORTANT))
							--j;
						--fail;
						continue;
					}
				}
				else
				{
					// sprawdŸ kolizje z blokami
					if(!IS_SET(obj->flags, OBJ_NO_PHYSICS))
					{
						bool ok = true;
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x+1.f, 2.f*b_it->y+1.f, 1.f, 1.f))
							{
								ok = false;
								break;
							}
						}
						if(!ok)
						{
							if(IS_SET(obj->flags, OBJ_IMPORTANT))
								--j;
							--fail;
							continue;
						}
					}

					// sprawdŸ kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, obj->r, &ignore);
					if(!global_col.empty() && Collide(global_col, pos, obj->r+0.8f))
					{
						if(IS_SET(obj->flags, OBJ_IMPORTANT))
							--j;
						--fail;
						continue;
					}
				}

				if(IS_SET(obj->flags, OBJ_TABLE))
				{
					Obj* stol = FindObject(rand2()%2 == 0 ? "table" : "table2");

					// stó³
					{
						Object& o = Add1(local_ctx.objects);
						o.mesh = stol->mesh;
						o.rot = VEC3(0,rot,0);
						o.pos = pos;
						o.scale = 1;
						o.base = stol;

						SpawnObjectExtras(local_ctx, stol, pos, rot, nullptr, nullptr);
					}

					// sto³ki
					Obj* stolek = FindObject("stool");
					int ile = random(2, 4);
					int d[4] = {0,1,2,3};
					for(int i=0; i<4; ++i)
						std::swap(d[rand2()%4], d[rand2()%4]);

					for(int i=0; i<ile; ++i)
					{
						float sdir, slen;
						switch(d[i])
						{
						case 0:
							sdir = 0.f;
							slen = stol->size.y+0.3f;
							break;
						case 1:
							sdir = PI/2;
							slen = stol->size.x+0.3f;
							break;
						case 2:
							sdir = PI;
							slen = stol->size.y+0.3f;
							break;
						case 3:
							sdir = PI*3/2;
							slen = stol->size.x+0.3f;
							break;
						default:
							assert(0);
							break;
						}

						sdir += rot;
						
						Useable* u = new Useable;
						local_ctx.useables->push_back(u);
						u->type = U_STOOL;
						u->pos = pos + VEC3(sin(sdir)*slen, 0, cos(sdir)*slen);
						u->rot = sdir;
						u->user = nullptr;

						if(IsOnline())
							u->netid = useable_netid_counter++;

						SpawnObjectExtras(local_ctx, stolek, u->pos, u->rot, u, nullptr);
					}
				}
				else
				{
					void* obj_ptr = nullptr;
					void** result_ptr = nullptr;

					if(IS_SET(obj->flags, OBJ_CHEST))
					{
						Chest* chest = new Chest;
						chest->ani = new AnimeshInstance(aSkrzynia);
						chest->pos = pos;
						chest->rot = rot;
						chest->handler = nullptr;
						chest->looted = false;
						room_chests.push_back(chest);
						local_ctx.chests->push_back(chest);
						if(IsOnline())
							chest->netid = chest_netid_counter++;
					}
					else if(IS_SET(obj->flags, OBJ_USEABLE))
					{
						int typ;
						if(IS_SET(obj->flags, OBJ_BENCH))
							typ = U_BENCH;
						else if(IS_SET(obj->flags, OBJ_ANVIL))
							typ = U_ANVIL;
						else if(IS_SET(obj->flags, OBJ_CHAIR))
							typ = U_CHAIR;
						else if(IS_SET(obj->flags, OBJ_CAULDRON))
							typ = U_CAULDRON;
						else if(IS_SET(obj->flags, OBJ_IRON_VAIN))
							typ = U_IRON_VAIN;
						else if(IS_SET(obj->flags, OBJ_GOLD_VAIN))
							typ = U_GOLD_VAIN;
						else if(IS_SET(obj->flags, OBJ_THRONE))
							typ = U_THRONE;
						else if(IS_SET(obj->flags, OBJ_STOOL))
							typ = U_STOOL;
						else if(IS_SET(obj->flags2, OBJ2_BENCH_ROT))
							typ = U_BENCH_ROT;
						else
						{
							assert(0);
							typ = U_CHAIR;
						}

						Useable* u = new Useable;
						u->type = typ;
						u->pos = pos;
						u->rot = rot;
						u->user = nullptr;
						Obj* base_obj = u->GetBase()->obj;
						if(IS_SET(base_obj->flags2, OBJ2_VARIANT))
						{
							// extra code for bench
							if(typ == U_BENCH || typ == U_BENCH_ROT)
							{
								switch(location->type)
								{
								case L_CITY:
									u->variant = 0;
									break;
								case L_DUNGEON:
								case L_CRYPT:
									u->variant = rand2()%2;
									break;
								default:
									u->variant = rand2()%2+2;
									break;
								}
							}
							else
								u->variant = random<int>(base_obj->variant->count-1);
						}
						else
							u->variant = -1;
						local_ctx.useables->push_back(u);
						obj_ptr = u;

						if(IsOnline())
							u->netid = useable_netid_counter++;
					}
					else
					{
						Object& o = Add1(local_ctx.objects);
						o.mesh = obj->mesh;
						o.rot = VEC3(0,rot,0);
						o.pos = pos;
						o.scale = 1;
						o.base = obj;
						result_ptr = &o.ptr;
						obj_ptr = &o;
					}

					SpawnObjectExtras(local_ctx, obj, pos, rot, obj_ptr, (btCollisionObject**)result_ptr, 1.f, flags);

					if(IS_SET(obj->flags, OBJ_REQUIRED))
						wymagany_obiekt = true;

					if(IS_SET(obj->flags, OBJ_ON_WALL))
						on_wall.push_back(pos);
				}

				if(is_variant)
					obj = FindObject(rt->objs[i].id);
			}
		}

		if(wymagany && wymagany_obiekt && it->target == RoomTarget::None)
			wymagany = false;

		if(!room_chests.empty())
		{
			int gold;
			if(room_chests.size() == 1)
			{
				vector<ItemSlot>& items = room_chests.front()->items;
				GenerateTreasure(chest_lvl, 10, items, gold);
				InsertItemBare(items, gold_item_ptr, (uint)gold, (uint)gold);
				SortItems(items);
			}
			else
			{
				static vector<ItemSlot> items;
				GenerateTreasure(chest_lvl, 9+2*room_chests.size(), items, gold);
				SplitTreasure(items, gold, &room_chests[0], room_chests.size());
			}
			
			room_chests.clear();
		}

		on_wall.clear();
		blocks.clear();
	}

	if(wymagany)
		throw "Failed to generate required object!";

	if(local_ctx.chests->empty())
	{
		// niech w podziemiach bêdzie minimum 1 skrzynia
		Obj* obj = FindObject("chest");
		for(int i=0; i<100; ++i)
		{
			on_wall.clear();
			blocks.clear();
			Room& r = lvl.rooms[rand2() % lvl.rooms.size()];
			if(r.target == RoomTarget::None)
			{
				// dodaj blokady
				for(int x=0; x<r.size.x; ++x)
				{
					// górna krawêdŸ
					POLE co = lvl.map[r.pos.x + x + (r.pos.y + r.size.y - 1)*lvl.w].type;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(r.pos.x + x, r.pos.y + r.size.y - 1));
						blocks.push_back(INT2(r.pos.x + x, r.pos.y + r.size.y - 2));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(r.pos.x + x, r.pos.y + r.size.y - 1));

					// dolna krawêdŸ
					co = lvl.map[r.pos.x + x + r.pos.y*lvl.w].type;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(r.pos.x + x, r.pos.y));
						blocks.push_back(INT2(r.pos.x + x, r.pos.y + 1));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(r.pos.x + x, r.pos.y));
				}
				for(int y=0; y<r.size.y; ++y)
				{
					// lewa krawêdŸ
					POLE co = lvl.map[r.pos.x + (r.pos.y + y)*lvl.w].type;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(r.pos.x, r.pos.y + y));
						blocks.push_back(INT2(r.pos.x + 1, r.pos.y + y));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(r.pos.x, r.pos.y + y));

					// prawa krawêdŸ
					co = lvl.map[r.pos.x + r.size.x - 1 + (r.pos.y + y)*lvl.w].type;
					if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
					{
						blocks.push_back(INT2(r.pos.x + r.size.x - 1, r.pos.y + y));
						blocks.push_back(INT2(r.pos.x + r.size.x - 2, r.pos.y + y));
					}
					else if(co == SCIANA || co == BLOKADA_SCIANA)
						blocks.push_back(INT2(r.pos.x + r.size.x - 1, r.pos.y + y));
				}

				VEC3 pos;
				float rot;
				VEC2 shift;

				if(obj->type == OBJ_CYLINDER)
				{
					shift.x = obj->r;
					shift.y = obj->r;
				}
				else
					shift = obj->size;

				if(IS_SET(obj->flags, OBJ_NEAR_WALL))
				{
					INT2 tile;
					int dir;
					if(!lvl.GetRandomNearWallTile(r, tile, dir, IS_SET(obj->flags, OBJ_ON_WALL)))
						continue;

					rot = dir_to_rot(dir);
					if(dir == 2 || dir == 3)
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f)+2, 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f)+2);
					else
						pos = VEC3(2.f*tile.x+sin(rot)*(2.f-shift.y-0.01f), 0.f, 2.f*tile.y+cos(rot)*(2.f-shift.y-0.01f));

					switch(dir)
					{
					case 0:
						pos.x += random(0.2f,1.8f);
						break;
					case 1:
						pos.z += random(0.2f,1.8f);
						break;
					case 2:
						pos.x -= random(0.2f,1.8f);
						break;
					case 3:
						pos.z -= random(0.2f,1.8f);
						break;
					}
				}
				else if(IS_SET(obj->flags, OBJ_IN_MIDDLE))
				{
					rot = PI/2*(rand2()%4);
					pos = r.Center();
					switch(rand2()%4)
					{
					case 0:
						pos.x += 2;
						break;
					case 1:
						pos.x -= 2;
						break;
					case 2:
						pos.z += 2;
						break;
					case 3:
						pos.z -= 2;
						break;
					}
				}
				else
				{
					rot = random(MAX_ANGLE);

					if(obj->type == OBJ_CYLINDER)
						pos = r.GetRandomPos(max(obj->size.x, obj->size.y));
					else
						pos = r.GetRandomPos(obj->r);
				}

				if(IS_SET(obj->flags, OBJ_HIGH))
					pos.y += 1.5f;

				if(obj->type == OBJ_HITBOX)
				{
					// sprawdŸ kolizje z blokami
					bool ok = true;
					if(not_zero(rot))
					{
						RotRect r1, r2;
						r1.center.x = pos.x;
						r1.center.y = pos.z;
						r1.rot = rot;
						r1.size = shift;
						r2.rot = 0;
						r2.size = VEC2(1,1);
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							r2.center.x = 2.f*b_it->x+1.f;
							r2.center.y = 2.f*b_it->y+1.f;
							if(RotatedRectanglesCollision(r1, r2))
							{
								ok = false;
								break;
							}
						}
					}
					else
					{
						for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
						{
							if(RectangleToRectangle(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y, 2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x+1), 2.f*(b_it->y+1)))
							{
								ok = false;
								break;
							}
						}
					}
					if(!ok)
						continue;

					// sprawdŸ kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
					if(!global_col.empty() && Collide(global_col, BOX2D(pos.x-shift.x, pos.z-shift.y, pos.x+shift.x, pos.z+shift.y), 0.4f, rot))
						continue;
				}
				else
				{
					// sprawdŸ kolizje z blokami
					bool ok = true;
					for(vector<INT2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
					{
						if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x+1.f, 2.f*b_it->y+1.f, 1.f, 1.f))
						{
							ok = false;
							break;
						}
					}
					if(!ok)
						continue;

					// sprawdŸ kolizje z innymi obiektami
					global_col.clear();
					GatherCollisionObjects(local_ctx, global_col, pos, obj->r, &ignore);
					if(!global_col.empty() && Collide(global_col, pos, obj->r+0.4f))
						continue;
				}

				Chest* chest = new Chest;
				chest->ani = new AnimeshInstance(aSkrzynia);
				chest->pos = pos;
				chest->rot = rot;
				chest->handler = nullptr;
				chest->looted = false;
				local_ctx.chests->push_back(chest);
				if(IsOnline())
					chest->netid = chest_netid_counter++;

				SpawnObjectExtras(local_ctx, obj, pos, rot, nullptr, nullptr, 1.f, flags);

				vector<ItemSlot>& items = chest->items;
				int gold;
				GenerateTreasure(chest_lvl, 10, items, gold);
				InsertItemBare(items, gold_item_ptr, (uint)gold, (uint)gold);
				SortItems(items);

				break;
			}
		}
	}
}

int wartosc_skarbu[] = {
	10, //0
	50, //1
	100, //2
	200, //3
	350, //4
	500, //5
	700, //6
	900, //7
	1200, //8
	1500, //9
	1900, //10
	2400, //11
	2900, //12
	3400, //13
	4000, //14
	4600, //15
	5300, //16
	6000, //17
	6800, //18
	7600, //19
	8500 //20
};

void Game::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold)
{
	assert(in_range(level, 1, 20));

	int value = random(wartosc_skarbu[level-1], wartosc_skarbu[level]);

	items.clear();

	const Item* item;
	uint count;

	for(int tries = 0; tries < _count; ++tries)
	{
		switch(rand2()%IT_MAX_GEN)
		{
		case IT_WEAPON:
			item = g_weapons[rand2() % g_weapons.size()];
			count = 1;
			break;
		case IT_ARMOR:
			item = g_armors[rand2() % g_armors.size()];
			count = 1;
			break;
		case IT_BOW:
			item = g_bows[rand2() % g_bows.size()];
			count = 1;
			break;
		case IT_SHIELD:
			item = g_shields[rand2() % g_shields.size()];
			count = 1;
			break;
		case IT_CONSUMABLE:
			item = g_consumables[rand2() % g_consumables.size()];
			count = random(2,5);
			break;
		case IT_OTHER:
			item = g_others[rand2() % g_others.size()];
			count = random(1,4);
			break;
		default:
			assert(0);
			item = nullptr;
			break;
		}

		if(!item->CanBeGenerated())
			continue;

		int cost = item->value * count;
		if(cost > value)
			continue;

		value -= cost;

		InsertItemBare(items, item, count, count);
	}

	gold = value+level*5;
}

Item* gold_item_ptr;

void Game::SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count)
{
	assert(gold >= 0 && count > 0 && chests);

	while(!items.empty())
	{
		for(int i=0; i<count && !items.empty(); ++i)
		{
			chests[i]->items.push_back(items.back());
			items.pop_back();
		}
	}

	int ile = gold/count-1;
	if(ile < 0)
		ile = 0;
	gold -= ile * count;

	for(int i=0; i<count; ++i)
	{
		if(i == count-1)
			ile += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(gold_item_ptr, ile, ile);
		SortItems(chests[i]->items);
	}
}

void Game::GenerateDungeonUnits()
{
	SPAWN_GROUP spawn_group;
	int base_level;

	if(location->spawn != SG_WYZWANIE)
	{
		spawn_group = location->spawn;
		base_level = GetDungeonLevel();
	}
	else
	{
		base_level = location->st;
		if(dungeon_level == 0)
			spawn_group = SG_ORKOWIE;
		else if(dungeon_level == 1)
			spawn_group = SG_MAGOWIE_I_GOLEMY;
		else
			spawn_group = SG_ZLO;
	}
	
	SpawnGroup& spawn = g_spawn_groups[spawn_group];
	if(!spawn.unit_group)
		return;
	UnitGroup& group = *spawn.unit_group;
	static vector<TmpUnitGroup> groups;

	int level = base_level;

	// poziomy 1..3
	for(int i=0; i<3; ++i)
	{
		TmpUnitGroup& part = Add1(groups);
		part.total = 0;
		part.max_level = level;

		for(auto& entry : group.entries)
		{
			if(entry.ud->level.x <= level)
			{
				part.entries.push_back(entry);
				part.total += entry.count;
			}
		}

		level = min(base_level-i-1, base_level*4/(i+5));
	}

	// opcje wejœciowe (póki co tu)
	// musi byæ w sumie 100%
	int szansa_na_brak = 10,
		szansa_na_1 = 20,
		szansa_na_2 = 30,
		szansa_na_3 = 40,
		szansa_na_wrog_w_korytarz = 25;

	assert(in_range(szansa_na_brak, 0, 100) && in_range(szansa_na_1, 0, 100) && in_range(szansa_na_2, 0, 100) && in_range(szansa_na_3, 0, 100) && in_range(szansa_na_wrog_w_korytarz, 0, 100) &&
		szansa_na_brak + szansa_na_1 + szansa_na_2 + szansa_na_3 == 100);

	int szansa[3] = {szansa_na_brak, szansa_na_brak+szansa_na_1, szansa_na_brak+szansa_na_1+szansa_na_2};

	// dodaj jednostki
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	INT2 pt = lvl.staircase_up, pt2 = lvl.staircase_down;
	if(!inside->HaveDownStairs())
		pt2 = INT2(-1000,-1000);
	if(inside->from_portal)
		pt = pos_to_pt(inside->portal->pos);

	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
	{
		int ile;

		if(it->target == RoomTarget::Treasury || it->target == RoomTarget::Prison)
			continue;

		if(it->IsCorridor())
		{
			if(rand2()%100 < szansa_na_wrog_w_korytarz)
				ile = 1;
			else
				continue;
		}
		else
		{
			int x = rand2()%100;
			if(x < szansa[0])
				continue;
			else if(x < szansa[1])
				ile = 1;
			else if(x < szansa[2])
				ile = 2;
			else
				ile = 3;
		}

		TmpUnitGroup& part = groups[ile-1];
		if(part.total == 0)
			continue;

		for(int i=0; i<ile; ++i)
		{
			int x = rand2()%part.total,
				y = 0;

			for(auto& entry : part.entries)
			{
				y += entry.count;

				if(x < y)
				{
					// dodaj
					SpawnUnitInsideRoom(*it, *entry.ud, random(part.max_level/2, part.max_level), pt, pt2);
					break;
				}
			}
		}
	}

	// posprz¹taj
	groups.clear();
}

Unit* Game::SpawnUnitInsideRoom(Room &p, UnitData &unit, int level, const INT2& stairs_pt, const INT2& stairs_down_pt)
{
	const float radius = unit.GetRadius();
	VEC3 stairs_pos(2.f*stairs_pt.x+1.f, 0.f, 2.f*stairs_pt.y+1.f);

	for(int i=0; i<10; ++i)
	{
		VEC3 pt = p.GetRandomPos(radius);

		if(distance(stairs_pos, pt) < 10.f)
			continue;

		INT2 my_pt = INT2(int(pt.x/2),int(pt.y/2));
		if(my_pt == stairs_down_pt)
			continue;

		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, radius, nullptr);

		if(!Collide(global_col, pt, radius))
		{
			float rot = random(MAX_ANGLE);
			return CreateUnitWithAI(local_ctx, unit, level, nullptr, &pt, &rot);
		}
	}

	return nullptr;
}

Unit* Game::SpawnUnitNearLocation(LevelContext& ctx, const VEC3 &pos, UnitData &unit, const VEC3* look_at, int level, float extra_radius)
{
	const float radius = unit.GetRadius();

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, pos, extra_radius+radius, nullptr);

	VEC3 tmp_pos = pos;

	for(int i=0; i<10; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
		{
			float rot;
			if(look_at)
				rot = lookat_angle(tmp_pos, *look_at);
			else
				rot = random(MAX_ANGLE);
			return CreateUnitWithAI(ctx, unit, level, nullptr, &tmp_pos, &rot);
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}
	
	return nullptr;
}

Unit* Game::CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level, Human* human_data, const VEC3* pos, const float* rot, AIController** ai)
{
	Unit* u = CreateUnit(unit, level, human_data);
	u->in_building = ctx.building_id;

	if(pos)
	{
		if(ctx.type == LevelContext::Outside)
		{
			VEC3 pt = *pos;
			terrain->SetH(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		UpdateUnitPhysics(*u, u->pos);
		u->visual_pos = u->pos;
	}
	if(rot)
		u->rot = *rot;

	ctx.units->push_back(u);

	AIController* a = new AIController;
	a->Init(u);

	ais.push_back(a);

	if(ai)
		*ai = a;

	return u;
}

const INT2 g_kierunekk[4] = {
	INT2(0,-1),
	INT2(-1,0),
	INT2(0,1),
	INT2(1,0)
};

void Game::ChangeLevel(int gdzie)
{
	assert(gdzie == 1 || gdzie == -1);

	LOG(gdzie == 1 ? "Changing level to lower." : "Changing level to upper.");

	location_event_handler = nullptr;
	UpdateDungeonMinimap(false);

	if(!in_tutorial && quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && quest_crazies->crazies_state < Quest_Crazies::State::End)
		CheckCraziesStone();

	if(IsOnline() && players > 1)
	{
		int poziom = dungeon_level;
		if(gdzie == -1)
			--poziom;
		else
			++poziom;
		if(poziom >= 0)
		{
			packet_data.resize(3);
			packet_data[0] = ID_CHANGE_LEVEL;
			packet_data[1] = (byte)current_location;
			packet_data[2] = (byte)poziom;
			int ack = peer->Send((cstring)&packet_data[0], 3, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
			{
				if(it->id == my_id)
					it->state = PlayerInfo::IN_GAME;
				else
				{
					it->state = PlayerInfo::WAITING_FOR_RESPONSE;
					it->ack = ack;
					it->timer = 5.f;
				}
			}
		}

		Net_FilterServerChanges();
	}

	if(gdzie == -1)
	{
		// poziom w górê
		if(dungeon_level == 0)
		{
			if(in_tutorial)
			{
				TutEvent(3);
				fallback_co = FALLBACK_CLIENT;
				fallback_t = 0.f;
				return;
			}

			// wyjœcie na powierzchniê
			ExitToMap();
			return;
		}
		else
		{
			LoadingStart(4);
			LoadingStep(txLevelUp);

			MultiInsideLocation* inside = (MultiInsideLocation*)location;
			LeaveLevel();
			--dungeon_level;
			inside->SetActiveLevel(dungeon_level);
			EnterLevel(false, false, true, -1, false);
		}
	}
	else
	{
		MultiInsideLocation* inside = (MultiInsideLocation*)location;
		
		int steps = 3;
		if(dungeon_level+1 >= inside->generated)
			steps += 2;
		else
			++steps;

		LoadingStart(steps);
		LoadingStep(txLevelDown);

		// poziom w dó³
		LeaveLevel();
		++dungeon_level;

		inside->SetActiveLevel(dungeon_level);

		bool first = false;

		// czy to pierwsza wizyta?
		if(dungeon_level >= inside->generated)
		{
			if(next_seed != 0)
			{
				srand2(next_seed);
				next_seed = 0;
			}
			else if(force_seed != 0 && force_seed_all)
				srand2(force_seed);

			inside->generated = dungeon_level+1;
			inside->infos[dungeon_level].seed = rand_r2();

			LOG(Format("Generating location '%s', seed %u.", location->name.c_str(), rand_r2()));
			LOG(Format("Generating dungeon, level %d, target %d.", dungeon_level+1, inside->target));

			LoadingStep(txGeneratingMap);
			GenerateDungeon(*location);
			first = true;
		}

		EnterLevel(first, false, false, -1, false);
	}

	local_ctx_valid = true;
	location->last_visit = worldtime;
	CheckIfLocationCleared();
	LoadingStep(txLoadingComplete);

	if(IsOnline() && players > 1)
	{
		net_mode = NM_SERVER_SEND;
		net_state = 0;
		net_stream.Reset();
		PrepareLevelData(net_stream);
		LOG(Format("Generated location packet: %d.", net_stream.GetNumberOfBytesUsed()));
		info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui->visible = true;
	}

	LOG(Format("Randomness integrity: %d", rand2_tmp()));
}

void Game::AddPlayerTeam(const VEC3& pos, float rot, bool reenter, bool hide_weapon)
{
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		Unit& u = **it;

		if(!reenter)
		{
			local_ctx.units->push_back(&u);

			btCapsuleShape* caps = new btCapsuleShape(u.GetUnitRadius(), max(MIN_H, u.GetUnitHeight()));
			u.cobj = new btCollisionObject;
			u.cobj->setCollisionShape(caps);
			u.cobj->setUserPointer(&u);
			u.cobj->setCollisionFlags(CG_UNIT);
			phy_world->addCollisionObject(u.cobj);

			if(u.IsHero())
				ais.push_back(u.ai);
		}

		if(hide_weapon || u.weapon_state == WS_HIDING)
		{
			u.weapon_state = WS_HIDDEN;
			u.weapon_taken = W_NONE;
			u.weapon_hiding = W_NONE;
			if(u.action == A_TAKE_WEAPON)
				u.action = A_NONE;
		}
		else if(u.weapon_state == WS_TAKING)
			u.weapon_state = WS_TAKEN;

		u.rot = rot;
		u.animation = u.current_animation = ANI_STAND;
		u.ani->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		BreakAction(u);
		u.SetAnimationAtEnd();
		if(u.in_building != -1)
		{
			if(reenter)
			{
				RemoveElement(GetContext(u).units, &u);
				local_ctx.units->push_back(&u);
			}
			u.in_building = -1;
		}

		if(u.IsAI())
		{
			u.ai->state = AIController::Idle;
			u.ai->idle_action = AIController::Idle_None;
			u.ai->target = nullptr;
			u.ai->alert_target = nullptr;
			u.ai->timer = random(2.f,5.f);
		}

		WarpNearLocation(local_ctx, u, pos, city_ctx ? 4.f : 2.f, true, 20);
		u.visual_pos = u.pos;

		if(!location->outside)
			DungeonReveal(INT2(int(u.pos.x/2), int(u.pos.z/2)));

		if(u.interp)
			u.interp->Reset(u.pos, u.rot);
	}
}

void Game::OpenDoorsByTeam(const INT2& pt)
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		INT2 unit_pt = pos_to_pt((*it)->pos);
		if(FindPath(local_ctx, unit_pt, pt, tmp_path))
		{
			for(vector<INT2>::iterator it2 = tmp_path.begin(), end2 = tmp_path.end(); it2 != end2; ++it2)
			{
				Pole& p = lvl.map[(*it2)(lvl.w)];
				if(p.type == DRZWI)
				{
					Door* door = lvl.FindDoor(*it2);					
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->ani->SetToEnd(&door->ani->ani->anims[0]);
					}
				}
			}
		}
		else
			WARN(Format("OpenDoorsByTeam: Can't find path from unit %s (%d,%d) to spawn point (%d,%d).", (*it)->data->id.c_str(), unit_pt.x, unit_pt.y, pt.x, pt.y));
	}
}

void Game::ExitToMap()
{
	// zamknij gui layer
	CloseAllPanels();

	clear_color = BLACK;
	game_state = GS_WORLDMAP;
	if(open_location != -1 && location->type == L_ENCOUNTER)
		LeaveLocation();
	
	if(world_state == WS_ENCOUNTER)
		world_state = WS_TRAVEL;
	else if(world_state != WS_TRAVEL)
		world_state = WS_MAIN;

	SetMusic(MusicType::Travel);

	if(IsOnline() && IsServer())
		PushNetChange(NetChange::EXIT_TO_MAP);

	show_mp_panel = true;
	world_map->visible = true;
	game_gui->visible = false;
}

void Game::RespawnObjectColliders(bool spawn_pes)
{
	RespawnObjectColliders(local_ctx, spawn_pes);

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			RespawnObjectColliders((*it)->ctx, spawn_pes);
	}
}

// wbrew nazwie tworzy te¿ ogieñ :3
void Game::RespawnObjectColliders(LevelContext& ctx, bool spawn_pes)
{
	int flags = SOE_DONT_CREATE_LIGHT;
	if(!spawn_pes)
		flags |= SOE_DONT_SPAWN_PARTICLES;

	// dotyczy tylko pochodni
	if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		BaseLocation& base = g_base_locations[inside->target];
		if(IS_SET(base.options, BLO_MAGIC_LIGHT))
			flags |= SOE_MAGIC_LIGHT;
	}

	for(vector<Object>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
	{
		if(!it->base)
			continue;

		Obj* obj = it->base;

		if(IS_SET(obj->flags, OBJ_BUILDING))
		{
			float rot = it->rot.y;
			int roti;
			if(equal(rot, 0))
				roti = 0;
			else if(equal(rot, PI/2))
				roti = 1;
			else if(equal(rot, PI))
				roti = 2;
			else if(equal(rot, PI*3/2))
				roti = 3;
			else
			{
				assert(0);
				roti = 0;
				rot = 0.f;
			}

			ProcessBuildingObjects(ctx, nullptr, nullptr, obj->mesh, nullptr, rot, roti, it->pos, nullptr, nullptr, true);
		}
		else
			SpawnObjectExtras(ctx, obj, it->pos, it->rot.y, &*it, (btCollisionObject**)&it->ptr, it->scale, flags);
	}

	if(ctx.chests)
	{
		Obj* chest = FindObject("chest");
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			SpawnObjectExtras(ctx, chest, (*it)->pos, (*it)->rot, nullptr, nullptr, 1.f, flags);
	}

	for(vector<Useable*>::iterator it = ctx.useables->begin(), end = ctx.useables->end(); it != end; ++it)
		SpawnObjectExtras(ctx, g_base_usables[(*it)->type].obj, (*it)->pos, (*it)->rot, *it, nullptr, 1.f, flags);
}

void Game::SetRoomPointers()
{
	for(uint i=0; i<n_base_locations; ++i)
	{
		BaseLocation& base = g_base_locations[i];

		if(base.schody.id)
			base.schody.room = FindRoomType(base.schody.id);

		if(base.wymagany.id)
			base.wymagany.room = FindRoomType(base.wymagany.id);

		if(base.rooms)
		{
			base.room_total = 0;
			for(int j=0; j<base.room_count; ++j)
			{
				base.rooms[j].room = FindRoomType(base.rooms[j].id);
				base.room_total += base.rooms[j].chance;
			}
		}
	}
}

SOUND Game::GetMaterialSound(MATERIAL_TYPE atakuje, MATERIAL_TYPE trafiony)
{
	switch(trafiony)
	{
	case MAT_BODY:
		return sBody[rand2()%5];
	case MAT_BONE:
		return sBone;
	case MAT_CLOTH:
	case MAT_SKIN:
		return sSkin;
	case MAT_IRON:
		return sMetal;
	case MAT_WOOD:
		return sWood;
	case MAT_ROCK:
		return sRock;
	case MAT_CRYSTAL:
		return sCrystal;
	default:
		assert(0);
		return nullptr;
	}
}

void Game::PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax)
{
	assert(sound && sound_volume > 0);

	VEC3 pos = unit.GetHeadSoundPos();

	if(&unit == pc->unit)
		PlaySound2d(sound);
	else
	{
		FMOD::Channel* channel;
		fmod_system->playSound(FMOD_CHANNEL_FREE, sound, true, &channel);
		channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
		channel->set3DMinMaxDistance(smin, 10000.f/*smax*/);
		channel->setPaused(false);

		AttachedSound& s = Add1(attached_sounds);
		s.channel = channel;
		s.unit = &unit;
	}
}

Game::ATTACK_RESULT Game::DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const VEC3& hitpoint, float start_dmg, int dmg_type, bool bash)
{
	int mod = ObliczModyfikator(dmg_type, hitted.data->flags);
	float m = 1.f;
	if(mod == -1)
		m += 0.25f;
	else if(mod == 1)
		m -= 0.25f;
	if(hitted.IsNotFighting())
		m += 0.1f;
	else if(hitted.IsHoldingBow())
		m += 0.05f;
	if(hitted.action == A_PAIN)
		m += 0.1f;

	// backstab bonus
	float kat = angle_dif(clip(attacker.rot+PI), hitted.rot);
	float backstab_mod = 0.25f;
	if(IS_SET(attacker.data->flags, F2_BACKSTAB))
		backstab_mod += 0.25f;
	if(attacker.HaveWeapon() && IS_SET(attacker.GetWeapon().flags, ITEM_BACKSTAB))
		backstab_mod += 0.25f;
	if(IS_SET(hitted.data->flags2, F2_BACKSTAB_RES))
		backstab_mod /= 2;
	m += kat/PI*backstab_mod;

	// apply modifiers
	float dmg = start_dmg * m;
	float base_dmg = dmg;

	// calculate defense
	float armor_def = hitted.CalculateArmorDefense(),
		  dex_def = hitted.CalculateDexterityDefense(),
		  base_def = hitted.CalculateBaseDefense();

	// blocking
	if(hitted.action == A_BLOCK && kat < PI/2)
	{
		int block_value = 3;
		MATERIAL_TYPE hitted_mat;
		if(IS_SET(attacker.data->flags2, F2_IGNORE_BLOCK))
			--block_value;
		if(attacker.attack_power >= 1.9f)
			--block_value;

		// reduce damage
		float blocked = hitted.CalculateBlock(&hitted.GetShield());
		hitted_mat = hitted.GetShield().material;
		blocked *= block_value;
		dmg -= blocked;

		// play sound
		MATERIAL_TYPE weapon_mat = (!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material);
		if(IsServer())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.ile = hitted_mat;
		}
		if(sound_volume)
			PlaySound3d(GetMaterialSound(weapon_mat, hitted_mat), hitpoint, 2.f, 10.f);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, base_dmg/hitted.hpmax, attacker.level);

		// pain animation & break blocking
		if(attacker.attack_power >= 1.9f && !IS_SET(hitted.data->flags, F_DONT_SUFFER))
		{
			BreakAction(hitted);

			if(hitted.action != A_POSITION)
				hitted.action = A_PAIN;
			else
				hitted.animation_state = 1;

			if(hitted.ani->ani->head.n_groups == 2)
			{
				hitted.ani->frame_end_info2 = false;
				hitted.ani->Play(NAMES::ani_hurt, PLAY_PRIO1|PLAY_ONCE, 1);
			}
			else
			{
				hitted.ani->frame_end_info = false;
				hitted.ani->Play(NAMES::ani_hurt, PLAY_PRIO3|PLAY_ONCE, 0);
				hitted.animation = ANI_PLAY;
			}
		}

		// attack fully blocked
		if(dmg < 0)
		{
			if(attacker.IsPlayer())
			{
				// player attack blocked
				attacker.player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
				// aggregate
				AttackReaction(hitted, attacker);
			}
			return ATTACK_BLOCKED;
		}
	}
	else if(hitted.HaveShield() && hitted.action != A_PAIN)
	{
		// defense bonus from shield
		base_def += hitted.CalculateBlock(&hitted.GetShield()) / 5;
	}

	// decrease defense when stunned
	bool clean_hit = false;
	if(hitted.action == A_PAIN)
	{
		dex_def = 0.f;
		if(hitted.HaveArmor())
		{
			const Armor& a = hitted.GetArmor();
			switch(a.skill)
			{
			case Skill::LIGHT_ARMOR:
				armor_def *= 0.25f;
				break;
			case Skill::MEDIUM_ARMOR:
				armor_def *= 0.5f;
				break;
			case Skill::HEAVY_ARMOR:
				armor_def *= 0.75f;
				break;
			}
		}
		clean_hit = true;
	}

	// armor defense
	dmg -= (armor_def + dex_def + base_def);

	// hit sound
	PlayHitSound(!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material, hitted.GetBodyMaterial(), hitpoint, 2.f, dmg>0.f);

	// train player armor skill
	if(hitted.IsPlayer())
		hitted.player->Train(TrainWhat::TakeDamageArmor, base_dmg/hitted.hpmax, attacker.level);

	// fully blocked by armor
	if(dmg < 0)
	{
		if(attacker.IsPlayer())
		{
			// player attack blocked
			attacker.player->Train(bash ? TrainWhat::BashNoDamage : TrainWhat::AttackNoDamage, 0.f, hitted.level);
			// aggregate
			AttackReaction(hitted, attacker);
		}
		return ATTACK_NO_DAMAGE;
	}

	if(attacker.IsPlayer())
	{
		// player hurt someone - train
		float dmgf = (float)dmg;
		float ratio;
		if(hitted.hp - dmgf <= 0.f && !hitted.IsImmortal())
			ratio = max(TRAIN_KILL_RATIO, dmgf/hitted.hpmax);
		else
			ratio = dmgf/hitted.hpmax;
		attacker.player->Train(bash ? TrainWhat::BashHit : TrainWhat::AttackHit, ratio, hitted.level);
	}

	GiveDmg(ctx, &attacker, dmg, hitted, &hitpoint);

	// apply poison
	if(IS_SET(attacker.data->flags, F_POISON_ATTACK) && !IS_SET(hitted.data->flags, F_POISON_RES))
	{
		Effect& e = Add1(hitted.effects);
		e.power = dmg/10;
		e.time = 5.f;
		e.effect = E_POISON;
	}

	return clean_hit ? ATTACK_CLEAN_HIT : ATTACK_HIT;
}

void Game::GenerateLabirynthUnits()
{
	// ustal jakie jednostki mo¿na tu wygenerowaæ
	cstring group_id;
	int count, tries;
	if(location->spawn == SG_UNK)
	{
		group_id = "unk";
		count = 30;
		tries = 150;
	}
	else
	{
		group_id = "labirynth";
		count = 20;
		tries = 100;
	}
	UnitGroup* group = FindUnitGroup(group_id);
	static TmpUnitGroup t;
	t.group = FindUnitGroup(group_id);
	t.total = 0;
	t.entries.clear();
	int level = GetDungeonLevel();

	for(auto& entry : group->entries)
	{
		if(entry.ud->level.x <= level)
		{
			t.entries.push_back(entry);
			t.total += entry.count;
		}
	}

	// generuj jednostki
	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();
	for(int added=0; added<count && tries; --tries)
	{
		INT2 pt(random(1,lvl.w-2), random(1,lvl.h-2));
		if(czy_blokuje21(lvl.map[pt(lvl.w)]))
			continue;
		if(distance(pt, lvl.staircase_up) < 5)
			continue;

		// co wygenerowaæ
		int x = rand2() % t.total,
			y = 0;

		for(int i=0; i<int(t.entries.size()); ++i)
		{
			y += t.entries[i].count;

			if(x < y)
			{
				// dodaj
				if(SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x+1.f, 0, 2.f*pt.y+1.f), *t.entries[i].ud, nullptr, random(level/2, level)))
					++added;
				break;
			}
		}
	}

	// wrogowie w skarbcu
	if(location->spawn == SG_UNK)
	{
		for(int i=0; i<3; ++i)
			SpawnUnitInsideRoom(lvl.rooms[0], *t.entries[0].ud, random(level/2, level));
	}
}

void Game::GenerateCave(Location& l)
{
	CaveLocation* cave = (CaveLocation*)&l;
	InsideLocationLevel& lvl = cave->GetLevelData();

	generate_cave(lvl.map, 52, lvl.staircase_up, lvl.staircase_up_dir, cave->holes, &cave->ext, devmode);
	
	lvl.w = lvl.h = 52;
}

void Game::GenerateCaveObjects()
{
	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();

	// œwiat³a
	for(vector<INT2>::iterator it = cave->holes.begin(), end = cave->holes.end(); it != end; ++it)
	{
		Light& s = Add1(lvl.lights);
		s.pos = VEC3(2.f*it->x+1.f, 3.f, 2.f*it->y+1.f);
		s.range = 5;
		s.color = VEC3(1.f,1.0f,1.0f);
	}

	// stalaktyty
	Obj* obj = FindObject("stalactite");
	static vector<INT2> sta;
	for(int count=0, tries=200; count<50 && tries>0; --tries)
	{
		INT2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y*lvl.w].type != PUSTE)
			continue;

		bool ok = true;
		for(vector<INT2>::iterator it = sta.begin(), end = sta.end(); it != end; ++it)
		{
			if(pt == *it)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			++count;

			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->mesh;
			o.scale = random(1.f,2.f);
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+1.f, 4.f, 2.f*pt.y+1.f);

			sta.push_back(pt);
		}
	}

	// krzaki
	obj = FindObject("plant2");
	for(int i=0; i<150; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.map[pt.x + pt.y*lvl.w].type == PUSTE)
		{
			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->mesh;
			o.scale = 1.f;
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));
		}
	}

	// grzyby
	obj = FindObject("mushrooms");
	for(int i=0; i<100; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.map[pt.x + pt.y*lvl.w].type == PUSTE)
		{
			Object& o = Add1(local_ctx.objects);
			o.base = obj;
			o.mesh = obj->mesh;
			o.scale = 1.f;
			o.rot = VEC3(0,random(MAX_ANGLE),0);
			o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));
		}
	}

	// kamienie
	obj = FindObject("rock");
	sta.clear();
	for(int i=0; i<80; ++i)
	{
		INT2 pt = cave->GetRandomTile();

		if(lvl.map[pt.x + pt.y*lvl.w].type == PUSTE)
		{
			bool ok = true;

			for(vector<INT2>::iterator it = sta.begin(), end = sta.end(); it != end; ++it)
			{
				if(*it == pt)
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				Object& o = Add1(local_ctx.objects);
				o.base = obj;
				o.mesh = obj->mesh;
				o.scale = 1.f;
				o.rot = VEC3(0,random(MAX_ANGLE),0);
				o.pos = VEC3(2.f*pt.x+random(0.1f,1.9f), 0.f, 2.f*pt.y+random(0.1f,1.9f));

				if(obj->shape)
				{
					CollisionObject& c = Add1(local_ctx.colliders);

					btCollisionObject* cobj = new btCollisionObject;
					cobj->setCollisionShape(obj->shape);

					if(obj->type == OBJ_CYLINDER)
					{
						cobj->getWorldTransform().setOrigin(btVector3(o.pos.x, o.pos.y+obj->h/2, o.pos.z));
						c.type = CollisionObject::SPHERE;
						c.pt = VEC2(o.pos.x, o.pos.z);
						c.radius = obj->r;
					}
					else
					{
						btTransform& tr = cobj->getWorldTransform();
						VEC3 zero(0,0,0), pos2;
						D3DXVec3TransformCoord(&pos2, &zero, obj->matrix);
						pos2 += o.pos;
						//VEC3 pos2 = o.pos;
						tr.setOrigin(ToVector3(pos2));
						tr.setRotation(btQuaternion(o.rot.y, 0, 0));
						//tr.setBasis(btMatrix3x3(obj->matrix->_11, obj->matrix->_12, obj->matrix->_13, obj->matrix->_21, obj->matrix->_22, obj->matrix->_23, obj->matrix->_31, obj->matrix->_32, obj->matrix->_33));

						c.pt = VEC2(pos2.x, pos2.z);
						c.w = obj->size.x;
						c.h = obj->size.y;
						if(not_zero(o.rot.y))
						{
							c.type = CollisionObject::RECTANGLE_ROT;
							c.rot = o.rot.y;
							c.radius = max(c.w, c.h) * SQRT_2;
						}
						else
							c.type = CollisionObject::RECTANGLE;
					}
					
					cobj->setCollisionFlags(CG_WALL);
					phy_world->addCollisionObject(cobj);
				}

				sta.push_back(pt);
			}
		}
	}
	sta.clear();
}

void Game::GenerateCaveUnits()
{
	// zbierz grupy
	static TmpUnitGroup e[3] = {
		{ FindUnitGroup("wolfs") },
		{ FindUnitGroup("spiders") },
		{ FindUnitGroup("rats") }
	};

	static vector<INT2> tiles;
	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();
	int level = GetDungeonLevel();
	tiles.clear();
	tiles.push_back(lvl.staircase_up);

	// ustal wrogów
	for(int i=0; i<3; ++i)
	{
		e[i].entries.clear();
		e[i].total = 0;
		for(auto& entry : e[i].group->entries)
		{
			if(entry.ud->level.x <= level)
			{
				e[i].total += entry.count;
				e[i].entries.push_back(entry);
			}
		}
	}

	for(int added=0, tries=50; added<8 && tries>0; --tries)
	{
		INT2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y*lvl.w].type != PUSTE)
			continue;

		bool ok = true;
		for(vector<INT2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			if(distance(pt, *it) < 10)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// losuj grupe
			TmpUnitGroup& group = e[rand2()%3];
			if(group.total == 0)
				continue;

			tiles.push_back(pt);
			++added;

			// postaw jednostki
			int levels = level * 2;
			while(levels > 0)
			{
				int k = rand2()%group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = random2(ud->level.x, min3(ud->level.y, levels, level));
				if(!SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x+1.f, 0, 2.f*pt.y+1.f), *ud, nullptr, enemy_level, 3.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

void Game::CastSpell(LevelContext& ctx, Unit& u)
{
	Spell& spell = *u.data->spells->spell[u.attack_id];

	Animesh::Point* point = u.ani->ani->GetPoint(NAMES::point_cast);
	assert(point);

	if(u.human_data)
		u.ani->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.ani->SetupBones();

	D3DXMatrixTranslation(&m1, u.pos);
	D3DXMatrixRotationY(&m2, u.rot);
	D3DXMatrixMultiply(&m1, &m2, &m1);
	m2 = point->mat * u.ani->mat_bones[point->bone] * m1;

	VEC3 coord;
	D3DXVec3TransformCoord(&coord, &VEC3(0,0,0), &m2);

	if(spell.type == Spell::Ball || spell.type == Spell::Point)
	{
		int ile = 1;
		if(IS_SET(spell.flags, Spell::Triple))
			ile = 3;

		for(int i=0; i<ile; ++i)
		{
			Bullet& b = Add1(ctx.bullets);

			b.level = u.level+u.CalculateMagicPower();
			b.backstab = 0;
			b.pos = coord;
			b.attack = float(spell.dmg);
			b.rot = VEC3(0, clip(u.rot+PI+random(-0.05f,0.05f)), 0);
			b.mesh = spell.mesh;
			b.tex = spell.tex;
			b.tex_size = spell.size;
			b.speed = spell.speed;
			b.timer = spell.range/(spell.speed-1);
			b.owner = &u;
			b.remove = false;
			b.trail = nullptr;
			b.trail2 = nullptr;
			b.pe = nullptr;
			b.spell = &spell;
			b.start_pos = b.pos;

			// ustal z jak¹ si³¹ rzuciæ kul¹
			if(spell.type == Spell::Ball)
			{
				float dist = distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h/t + (10.f*t)/2;
			}
			else if(spell.type == Spell::Point)
			{
				float dist = distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h/t;
			}

			if(spell.tex_particle)
			{
				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = spell.tex_particle;
				pe->emision_interval = 0.1f;
				pe->life = -1;
				pe->particle_life = 0.5f;
				pe->emisions = -1;
				pe->spawn_min = 3;
				pe->spawn_max = 4;
				pe->max_particles = 50;
				pe->pos = b.pos;
				pe->speed_min = VEC3(-1,-1,-1);
				pe->speed_max = VEC3(1,1,1);
				pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
				pe->pos_max = VEC3(spell.size, spell.size, spell.size);
				pe->size = spell.size_particle;
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				ctx.pes->push_back(pe);
				b.pe = pe;
			}

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CREATE_SPELL_BALL;
				c.spell = &spell;
				c.pos = b.start_pos;
				c.f[0] = b.rot.y;
				c.f[1] = b.yspeed;
				c.extra_netid = u.netid;
			}
		}
	}
	else
	{
		if(IS_SET(spell.flags, Spell::Jump))
		{
			Electro* e = new Electro;
			e->hitted.push_back(&u);
			e->dmg = float(spell.dmg + spell.dmg_bonus * (u.CalculateMagicPower() + u.level));
			e->owner = &u;
			e->spell = &spell;
			e->start_pos = u.pos;

			VEC3 hitpoint;
			Unit* hitted;

			u.target_pos.y += random(-0.5f,0.5f);
			VEC3 dir = u.target_pos - coord;
			D3DXVec3Normalize(&dir, &dir);
			VEC3 target = coord+dir*spell.range;

			if(RayTest(coord, target, &u, hitpoint, hitted))
			{
				if(hitted)
				{
					// trafiono w cel
					e->valid = true;
					e->hitsome = true;
					e->hitted.push_back(hitted);
					e->AddLine(coord, hitpoint);
				}
				else
				{
					// trafienie w obiekt
					e->valid = false;
					e->hitsome = true;
					e->AddLine(coord, hitpoint);
				}
			}
			else
			{
				// w nic nie trafiono
				e->valid = false;
				e->hitsome = false;
				e->AddLine(coord, target);
			}

			ctx.electros->push_back(e);

			if(IsOnline())
			{
				e->netid = electro_netid_counter++;

				NetChange& c = Add1(net_changes);
				c.type = NetChange::CREATE_ELECTRO;
				c.e_id = e->netid;
				c.pos = e->lines[0].pts.front();
				memcpy(c.f, &e->lines[0].pts.back(), sizeof(VEC3));
			}
		}
		else if(IS_SET(spell.flags, Spell::Drain))
		{
			VEC3 hitpoint;
			Unit* hitted;

			u.target_pos.y += random(-0.5f,0.5f);

			if(RayTest(coord, u.target_pos, &u, hitpoint, hitted) && hitted)
			{
				// trafiono w cel
				if(!IS_SET(hitted->data->flags2, F2_BLOODLESS) && !IsFriend(u, *hitted))
				{
					Drain& drain = Add1(ctx.drains);
					drain.from = hitted;
					drain.to = &u;

					GiveDmg(ctx, &u, float(spell.dmg + (u.CalculateMagicPower() + u.level)*spell.dmg_bonus), *hitted, nullptr, DMG_MAGICAL);

					drain.pe = ctx.pes->back();
					drain.t = 0.f;
					drain.pe->manual_delete = 1;
					drain.pe->speed_min = VEC3(-3,0,-3);
					drain.pe->speed_max = VEC3(3,3,3);

					u.hp += float(spell.dmg);
					if(u.hp > u.hpmax)
						u.hp = u.hpmax;

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::UPDATE_HP;
						c.unit = drain.to;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::CREATE_DRAIN;
						c2.unit = drain.to;
					}
				}
			}
		}
		else if(IS_SET(spell.flags, Spell::Raise))
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if((*it)->live_state == Unit::DEAD && !IsEnemy(u, **it) && IS_SET((*it)->data->flags, F_UNDEAD) && distance(u.target_pos, (*it)->pos) < 0.5f)
				{
					Unit& u2 = **it;
					u2.hp = u2.hpmax;
					u2.live_state = Unit::ALIVE;
					u2.weapon_state = WS_HIDDEN;
					u2.animation = ANI_STAND;
					u2.animation_state = 0;
					u2.weapon_taken = W_NONE;
					u2.weapon_hiding = W_NONE;

					// za³ó¿ przedmioty / dodaj z³oto
					ReequipItemsMP(u2);

					// przenieœ fizyke
					WarpUnit(u2, u2.pos);

					// resetuj ai
					u2.ai->Reset();

					// efekt cz¹steczkowy
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = u2.pos;
					pe->pos.y += u2.GetUnitHeight()/2;
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::RAISE_EFFECT;
						c.pos = pe->pos;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::STAND_UP;
						c2.unit = &u2;

						NetChange& c3 = Add1(net_changes);
						c3.type = NetChange::UPDATE_HP;
						c3.unit = &u2;
					}

					break;
				}
			}
			u.ai->state = AIController::Idle;
		}
		else if(IS_SET(spell.flags, Spell::Heal))
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if(!IsEnemy(u, **it) && !IS_SET((*it)->data->flags, F_UNDEAD) && distance(u.target_pos, (*it)->pos) < 0.5f)
				{
					Unit& u2 = **it;
					u2.hp += float(spell.dmg + spell.dmg_bonus*(u.level + u.CalculateMagicPower()));
					if(u2.hp > u2.hpmax)
						u2.hp = u2.hpmax;
					
					// efekt cz¹steczkowy
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 16;
					pe->spawn_max = 25;
					pe->max_particles = 25;
					pe->pos = u2.pos;
					pe->pos.y += u2.GetUnitHeight()/2;
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = VEC3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::HEAL_EFFECT;
						c.pos = pe->pos;

						NetChange& c3 = Add1(net_changes);
						c3.type = NetChange::UPDATE_HP;
						c3.unit = &u2;
					}

					break;
				}
			}
			u.ai->state = AIController::Idle;
		}
	}

	// dŸwiêk
	if(spell.sound_cast)
	{
		if(sound_volume)
			PlaySound3d(spell.sound_cast, coord, spell.sound_cast_dist.x, spell.sound_cast_dist.y);
		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SPELL_SOUND;
			c.spell = &spell;
			c.pos = coord;
		}
	}
}

void Game::SpellHitEffect(LevelContext& ctx, Bullet& bullet, const VEC3& pos, Unit* hitted)
{
	Spell& spell = *bullet.spell;

	// dŸwiêk
	if(spell.sound_hit && sound_volume)
		PlaySound3d(spell.sound_hit, pos, spell.sound_hit_dist.x, spell.sound_hit_dist.y);

	// cz¹steczki
	if(spell.tex_particle && spell.type == Spell::Ball)
	{
		ParticleEmitter* pe = new ParticleEmitter;
		pe->tex = spell.tex_particle;
		pe->emision_interval = 0.01f;
		pe->life = 0.f;
		pe->particle_life = 0.5f;
		pe->emisions = 1;
		pe->spawn_min = 8;
		pe->spawn_max = 12;
		pe->max_particles = 12;
		pe->pos =pos;
		pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
		pe->speed_max = VEC3(1.5f,1.5f,1.5f);
		pe->pos_min = VEC3(-spell.size, -spell.size, -spell.size);
		pe->pos_max = VEC3(spell.size, spell.size, spell.size);
		pe->size = spell.size/2;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 1.f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 1;
		pe->Init();
		//pe->gravity = true;
		ctx.pes->push_back(pe);
	}

	// wybuch
	if(IsLocal() && spell.tex_explode && IS_SET(spell.flags, Spell::Explode))
	{
		Explo* explo = new Explo;
		explo->dmg = (float)spell.dmg;
		if(bullet.owner)
			explo->dmg += float((bullet.owner->level + bullet.owner->CalculateMagicPower()) * spell.dmg_bonus);
		explo->size = 0.f;
		explo->sizemax = spell.explode_range;
		explo->pos = pos;
		explo->tex = spell.tex_explode;
		explo->owner = bullet.owner;
		if(hitted)
			explo->hitted.push_back(hitted);
		ctx.explos->push_back(explo);

		if(IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CREATE_EXPLOSION;
			c.spell = &spell;
			c.pos = explo->pos;
		}
	}
}

extern vector<uint> _to_remove;

void Game::UpdateExplosions(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Explo*>::iterator it = ctx.explos->begin(), end = ctx.explos->end(); it != end; ++it, ++index)
	{
		Explo& e = **it;
		bool delete_me = false;

		// powiêksz
		e.size += e.sizemax*dt;
		if(e.size >= e.sizemax)
		{
			delete_me = true;
			e.size = e.sizemax;
			_to_remove.push_back(index);
		}

		float dmg = e.dmg * lerp(1.f, 0.1f, e.size/e.sizemax);

		if(IsLocal())
		{
			// zadaj obra¿enia
			for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
			{
				if(!(*it2)->IsAlive() || ((*it)->owner && IsFriend(*(*it)->owner, **it2)))
					continue;

				// sprawdŸ czy ju¿ nie zosta³ trafiony
				bool jest = false;
				for(vector<Unit*>::iterator it3 = e.hitted.begin(), end3 = e.hitted.end(); it3 != end3; ++it3)
				{
					if(*it2 == *it3)
					{
						jest = true;
						break;
					}
				}

				// nie zosta³ trafiony
				if(!jest)
				{
					BOX box;
					(*it2)->GetBox(box);

					if(SphereToBox(e.pos, e.size, box))
					{
						// zadaj obra¿enia
						GiveDmg(ctx, e.owner, dmg, **it2, nullptr, DMG_NO_BLOOD|DMG_MAGICAL);
						e.hitted.push_back(*it2);
					}
				}
			}
		}

		if(delete_me)
			delete *it;
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.explos->size()-1)
			ctx.explos->pop_back();
		else
		{
			std::iter_swap(ctx.explos->begin()+index, ctx.explos->end()-1);
			ctx.explos->pop_back();
		}
	}
}

void Game::UpdateTraps(LevelContext& ctx, float dt)
{
	const bool is_local = IsLocal();
	uint index = 0;
	for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end; ++it, ++index)
	{
		Trap& trap = **it;

		if(trap.state == -1)
		{
			trap.time -= dt;
			if(trap.time <= 0.f)
				trap.state = 0;
			continue;
		}

		switch(trap.base->type)
		{
		case TRAP_SPEAR:
			if(trap.state == 0)
			{
				// sprawdŸ czy ktoœ nadepn¹³
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if((*it2)->IsStanding() && !IS_SET((*it2)->data->flags, F_SLIGHT) && CircleToCircle(trap.pos.x, trap.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(jest)
				{
					// ktoœ nadepn¹³
					if(sound_volume)
						PlaySound3d(trap.base->sound, trap.pos, 1.f, 4.f);
					trap.state = 1;
					trap.time = random(0.5f,0.75f);

					if(IsOnline() && IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			else if(trap.state == 1)
			{
				// odliczaj czas do wyjœcia w³óczni
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.state = 2;
					trap.time = 0.f;

					// dŸwiêk wychodzenia
					if(sound_volume)
						PlaySound3d(trap.base->sound2, trap.pos, 2.f, 8.f);
				}
			}
			else if(trap.state == 2)
			{
				// wychodzenie w³óczni
				bool koniec = false;
				trap.time += dt;
				if(trap.time >= 0.27f)
				{
					trap.time = 0.27f;
					koniec = true;
				}

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + 2.f*(trap.time/0.27f);

				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->IsAlive())
							continue;
						if(CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
						{
							bool jest = false;
							for(vector<Unit*>::iterator it3 = trap.hitted->begin(), end3 = trap.hitted->end(); it3 != end3; ++it3)
							{
								if(*it2 == *it3)
								{
									jest = true;
									break;
								}
							}

							if(!jest)
							{
								Unit* hitted = *it2;

								// uderzenie w³óczniami
								float dmg = float(trap.base->dmg),
									def = hitted->CalculateDefense();

								int mod = ObliczModyfikator(DMG_PIERCE, hitted->data->flags);
								float m = 1.f;
								if(mod == -1)
									m += 0.25f;
								else if(mod == 1)
									m -= 0.25f;

								// modyfikator obra¿eñ
								dmg *= m;
								float base_dmg = dmg;
								dmg -= def;

								// dŸwiêk trafienia
								if(sound_volume)
									PlaySound3d(GetMaterialSound(MAT_IRON, hitted->GetBodyMaterial()), hitted->pos + VEC3(0,1.f,0), 2.f, 8.f);

								// train player armor skill
								if(hitted->IsPlayer())
									hitted->player->Train(TrainWhat::TakeDamageArmor, base_dmg/hitted->hpmax, 4);

								// obra¿enia
								if(dmg > 0)
									GiveDmg(ctx, nullptr, dmg, **it2);

								trap.hitted->push_back(hitted);
							}
						}
					}
				}

				if(koniec)
				{
					trap.state = 3;
					if(is_local)
						trap.hitted->clear();
					trap.time = 1.f;
				}
			}
			else if(trap.state == 3)
			{
				// w³ócznie wysz³y, odliczaj czas do chowania
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.state = 4;
					trap.time = 1.5f;
					if(sound_volume)
						PlaySound3d(trap.base->sound3, trap.pos, 1.f, 4.f);
				}
			}
			else if(trap.state == 4)
			{
				// chowanie w³óczni
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.time = 0.f;
					trap.state = 5;
				}

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + trap.time / 1.5f * 2.f;
			}
			else if(trap.state == 5)
			{
				// w³ócznie schowane, odliczaj czas do ponownej aktywacji
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					if(is_local)
					{
						// jeœli ktoœ stoi to nie aktywuj
						bool ok = true;
						for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
						{
							if(!IS_SET((*it2)->data->flags, F_SLIGHT) && CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, (*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius()))
							{
								ok = false;
								break;
							}
						}
						if(ok)
							trap.state = 0;
					}
					else
						trap.state = 0;
				}
			}
			break;
		case TRAP_ARROW:
		case TRAP_POISON:
			if(trap.state == 0)
			{
				// sprawdŸ czy ktoœ nadepn¹³
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if((*it2)->IsStanding() && !IS_SET((*it2)->data->flags, F_SLIGHT) &&
							CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(jest)
				{
					// ktoœ nadepn¹³, wystrzel strza³ê
					trap.state = 1;
					trap.time = random(5.f, 7.5f);

					if(is_local)
					{
						Bullet& b = Add1(ctx.bullets);
						b.level = 4;
						b.backstab = 0;
						b.attack = float(trap.base->dmg);
						b.mesh = aArrow;
						b.pos = VEC3(2.f*trap.tile.x+trap.pos.x-float(int(trap.pos.x/2)*2)+random(-trap.base->rw, trap.base->rw)-1.2f*g_kierunek2[trap.dir].x,
							random(0.5f,1.5f),
							2.f*trap.tile.y+trap.pos.z-float(int(trap.pos.z/2)*2)+random(-trap.base->h, trap.base->h)-1.2f*g_kierunek2[trap.dir].y);
						b.rot = VEC3(0,dir_to_rot(trap.dir),0);
						b.owner = nullptr;
						b.pe = nullptr;
						b.remove = false;
						b.speed = TRAP_ARROW_SPEED;
						b.spell = nullptr;
						b.tex = nullptr;
						b.tex_size = 0.f;
						b.timer = ARROW_TIMER;
						b.yspeed = 0.f;
						b.poison_attack = (trap.base->type == TRAP_POISON ? float(trap.base->dmg) : 0.f);

						TrailParticleEmitter* tpe = new TrailParticleEmitter;
						tpe->fade = 0.3f;
						tpe->color1 = VEC4(1,1,1,0.5f);
						tpe->color2 = VEC4(1,1,1,0);
						tpe->Init(50);
						ctx.tpes->push_back(tpe);
						b.trail = tpe;

						TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
						tpe2->fade = 0.3f;
						tpe2->color1 = VEC4(1,1,1,0.5f);
						tpe2->color2 = VEC4(1,1,1,0);
						tpe2->Init(50);
						ctx.tpes->push_back(tpe2);
						b.trail2 = tpe2;

						if(sound_volume)
						{
							// dŸwiêk nadepniêcia
							PlaySound3d(trap.base->sound, trap.pos, 1.f, 4.f);
							// dŸwiêk strza³u
							PlaySound3d(sBow[rand2()%2], b.pos, 2.f, 8.f);
						}

						if(IsOnline() && IsServer())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::SHOOT_ARROW;
							c.unit = nullptr;
							c.pos = b.start_pos;
							c.f[0] = b.rot.y;
							c.f[1] = 0.f;
							c.f[2] = 0.f;
							c.extra_f = b.speed;

							NetChange& c2 = Add1(net_changes);
							c2.type = NetChange::TRIGGER_TRAP;
							c2.id = trap.netid;
						}
					}
				}
			}
			else if(trap.state == 1)
			{
				trap.time -= dt;
				if(trap.time <= 0.f)
					trap.state = 2;
			}
			else
			{
				// sprawdŸ czy zeszli
				bool jest = false;
				if(is_local)
				{
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!IS_SET((*it2)->data->flags, F_SLIGHT) && 
							CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							jest = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					jest = true;
					trap.trigger = false;
				}

				if(!jest)
				{
					trap.state = 0;
					if(IsOnline() && IsServer())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			break;
		case TRAP_FIREBALL:
			{
				if(!is_local)
					break;

				bool jest = false;
				for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
				{
					if((*it2)->IsStanding() && CircleToRectangle((*it2)->pos.x, (*it2)->pos.z, (*it2)->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
					{
						jest = true;
						break;
					}
				}

				if(jest)
				{
					_to_remove.push_back(index);

					Spell* fireball = FindSpell("fireball");

					Explo* explo = new Explo;
					explo->pos = trap.pos;
					explo->pos.y += 0.2f;
					explo->size = 0.f;
					explo->sizemax = 2.f;
					explo->dmg = float(trap.base->dmg);
					explo->tex = fireball->tex_explode;
					explo->owner = nullptr;

					if(sound_volume)
						PlaySound3d(fireball->sound_hit, explo->pos, fireball->sound_hit_dist.x, fireball->sound_hit_dist.y);

					ctx.explos->push_back(explo);

					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::CREATE_EXPLOSION;
						c.spell = fireball;
						c.pos = explo->pos;

						NetChange& c2 = Add1(net_changes);
						c2.type = NetChange::REMOVE_TRAP;
						c2.id = trap.netid;
					}

					delete *it;
				}
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.traps->size()-1)
			ctx.traps->pop_back();
		else
		{
			std::iter_swap(ctx.traps->begin()+index, ctx.traps->end()-1);
			ctx.traps->pop_back();
		}
	}
}

struct TrapLocation
{
	INT2 pt;
	int dist, dir;
};

inline bool SortTrapLocations(TrapLocation& pt1, TrapLocation& pt2)
{
	return abs(pt1.dist-5) < abs(pt2.dist-5);
}

Trap* Game::CreateTrap(INT2 pt, TRAP_TYPE type, bool timed)
{
	Trap* t = new Trap;
	Trap& trap = *t;
	local_ctx.traps->push_back(t);

	trap.base = &g_traps[type];
	trap.hitted = nullptr;
	trap.state = 0;
	trap.pos = VEC3(2.f*pt.x+random(trap.base->rw, 2.f-trap.base->rw), 0.f, 2.f*pt.y+random(trap.base->h, 2.f-trap.base->h));
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.netid = trap_netid_counter++;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		trap.obj.rot = VEC3(0,0,0);

		static vector<TrapLocation> possible;

		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

		// ustal tile i dir
		for(int i=0; i<4; ++i)
		{
			bool ok = false;
			int j;

			for(j=1; j<=10; ++j)
			{
				if(czy_blokuje2(lvl.map[pt.x+g_kierunek2[i].x*j+(pt.y+g_kierunek2[i].y*j)*lvl.w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + g_kierunek2[i] * j;

				if(CanShootAtLocation(VEC3(trap.pos.x+(2.f*j-1.2f)*g_kierunek2[i].x, 1.f, trap.pos.z+(2.f*j-1.2f)*g_kierunek2[i].y), VEC3(trap.pos.x, 1.f, trap.pos.z)))
				{
					TrapLocation& tr = Add1(possible);
					tr.pt = trap.tile;
					tr.dist = j;
					tr.dir = i;
				}
			}
		}

		if(!possible.empty())
		{
			if(possible.size() > 1)
				std::sort(possible.begin(), possible.end(), SortTrapLocations);

			trap.tile = possible[0].pt;
			trap.dir = possible[0].dir;

			possible.clear();
		}
		else
		{
			local_ctx.traps->pop_back();
			delete t;
			return nullptr;
		}
	}
	else if(type == TRAP_SPEAR)
	{
		trap.obj.rot = VEC3(0,random(MAX_ANGLE),0);
		trap.obj2.base = nullptr;
		trap.obj2.mesh = trap.base->mesh2;
		trap.obj2.pos = trap.obj.pos;
		trap.obj2.rot = trap.obj.rot;
		trap.obj2.scale = 1.f;
		trap.obj2.pos.y -= 2.f;
		trap.hitted = new vector<Unit*>;
	}
	else
	{
		trap.obj.rot = VEC3(0,PI/2*(rand2()%4),0);
		trap.obj.base = &obj_alpha;
	}

	if(timed)
	{
		trap.state = -1;
		trap.time = 2.f;
	}

	return &trap;
}

struct BulletRaytestCallback3 : public btCollisionWorld::RayResultCallback
{
	explicit BulletRaytestCallback3(Unit* ignore) : hit(false), ignore(ignore), hitted(nullptr), fraction(1.01f)
	{
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
	{
		Unit* u = (Unit*)rayResult.m_collisionObject->getUserPointer();
		if(ignore && u == ignore)
			return 1.f;

		if(rayResult.m_hitFraction < fraction)
		{
			hit = true;
			hitted = u;
			fraction = rayResult.m_hitFraction;
		}

		return 0.f;
	}

	bool hit;
	Unit* ignore, *hitted;
	float fraction;
};

bool Game::RayTest(const VEC3& from, const VEC3& to, Unit* ignore, VEC3& hitpoint, Unit*& hitted)
{
	BulletRaytestCallback3 callback(ignore);
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hit)
	{
		hitpoint = from + (to-from) * callback.fraction;
		hitted = callback.hitted;
		return true;
	}
	else
		return false;
}

inline bool SortElectroTargets(const std::pair<Unit*, float>& target1, const std::pair<Unit*, float>& target2)
{
	return target1.second < target2.second;
}

void Game::UpdateElectros(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it, ++index)
	{
		Electro& e = **it;

		for(vector<ElectroLine>::iterator it2 = e.lines.begin(), end2 = e.lines.end(); it2 != end2; ++it2)
			it2->t += dt;

		if(!IsLocal())
		{
			if(e.lines.back().t >= 0.5f)
			{
				_to_remove.push_back(index);
				delete *it;
			}
		}
		else if(e.valid)
		{
			if(e.lines.back().t >= 0.25f)
			{
				// zadaj obra¿enia
				if(!IsFriend(*e.owner, *e.hitted.back()))
				{
					if(e.hitted.back()->IsAI())
						AI_HitReaction(*e.hitted.back(), e.start_pos);
					GiveDmg(ctx, e.owner, e.dmg, *e.hitted.back(), nullptr, DMG_NO_BLOOD|DMG_MAGICAL);
				}

				if(sound_volume && e.spell->sound_hit)
					PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist.x, e.spell->sound_hit_dist.y);

				// cz¹steczki
				if(e.spell->tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = e.spell->tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = e.lines.back().pts.back();
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = VEC3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = e.lines.back().pts.back();
				}

				if(e.dmg >= 10.f)
				{
					static vector<std::pair<Unit*, float> > targets;

					// traf w kolejny cel
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->IsAlive() || IsInside(e.hitted, *it2))
							continue;

						float dist = distance((*it2)->pos, e.hitted.back()->pos);
						if(dist <= 5.f)
							targets.push_back(std::pair<Unit*, float>(*it2, dist));
					}

					if(!targets.empty())
					{
						if(targets.size() > 1)
							std::sort(targets.begin(), targets.end(), SortElectroTargets);

						Unit* target = nullptr;
						float dist;

						for(vector<std::pair<Unit*, float> >::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
						{
							VEC3 hitpoint;
							Unit* hitted;

							if(RayTest(e.lines.back().pts.back(), it2->first->GetCenter(), e.hitted.back(), hitpoint, hitted))
							{
								if(hitted == it2->first)
								{
									target = it2->first;
									dist = it2->second;
									break;
								}
							}
						}

						if(target)
						{
							// kolejny cel
							e.dmg = min(e.dmg/2, lerp(e.dmg, e.dmg/5, dist/5));
							e.valid = true;
							e.hitted.push_back(target);
							VEC3 from = e.lines.back().pts.back();
							VEC3 to = target->GetCenter();
							e.AddLine(from, to);

							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::UPDATE_ELECTRO;
								c.e_id = e.netid;
								c.pos = to;
							}
						}
						else
						{
							// brak kolejnego celu
							e.valid = false;
						}

						targets.clear();
					}
					else
						e.valid = false;
				}
				else
				{
					// trafi³ ju¿ wystarczaj¹co du¿o postaci
					e.valid = false;
				}
			}
		}
		else
		{
			if(e.hitsome && e.lines.back().t >= 0.25f)
			{
				e.hitsome = false;

				if(sound_volume && e.spell->sound_hit)
					PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist.x, e.spell->sound_hit_dist.y);

				// cz¹steczki
				if(e.spell->tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = e.spell->tex_particle;
					pe->emision_interval = 0.01f;
					pe->life = 0.f;
					pe->particle_life = 0.5f;
					pe->emisions = 1;
					pe->spawn_min = 8;
					pe->spawn_max = 12;
					pe->max_particles = 12;
					pe->pos = e.lines.back().pts.back();
					pe->speed_min = VEC3(-1.5f,-1.5f,-1.5f);
					pe->speed_max = VEC3(1.5f,1.5f,1.5f);
					pe->pos_min = VEC3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = VEC3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ELECTRO_HIT;
					c.pos = e.lines.back().pts.back();
				}
			}
			if(e.lines.back().t >= 0.5f)
			{
				_to_remove.push_back(index);
				delete *it;
			}
		}
	}

	// usuñ
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.electros->size()-1)
			ctx.electros->pop_back();
		else
		{
			std::iter_swap(ctx.electros->begin()+index, ctx.electros->end()-1);
			ctx.electros->pop_back();
		}
	}
}

void Game::UpdateDrains(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Drain>::iterator it = ctx.drains->begin(), end = ctx.drains->end(); it != end; ++it, ++index)
	{
		Drain& d = *it;
		d.t += dt;
		VEC3 center = d.to->GetCenter();

		if(d.pe->manual_delete == 2)
		{
			delete d.pe;
			_to_remove.push_back(index);
		}
		else
		{
			for(vector<Particle>::iterator it2 = d.pe->particles.begin(), end2 = d.pe->particles.end(); it2 != end2; ++it2)
				it2->pos = lerp(it2->pos, center, d.t/1.5f);
		}
	}

	// usuñ
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.drains->size()-1)
			ctx.drains->pop_back();
		else
		{
			std::iter_swap(ctx.drains->begin()+index, ctx.drains->end()-1);
			ctx.drains->pop_back();
		}
	}
}

void Game::UpdateAttachedSounds(float dt)
{
	uint index = 0;
	for(vector<AttachedSound>::iterator it = attached_sounds.begin(), end = attached_sounds.end(); it != end; ++it, ++index)
	{
		bool is_playing;
		if(it->channel->isPlaying(&is_playing) == FMOD_OK && is_playing)
		{
			VEC3 pos = it->unit->GetHeadSoundPos();
			it->channel->set3DAttributes((const FMOD_VECTOR*)&pos, nullptr);
		}
		else
			_to_remove.push_back(index);
	}

	// usuñ
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == attached_sounds.size()-1)
			attached_sounds.pop_back();
		else
		{
			std::iter_swap(attached_sounds.begin()+index, attached_sounds.end()-1);
			attached_sounds.pop_back();
		}
	}
}



vector<Unit*> Unit::refid_table;
vector<std::pair<Unit**, int> > Unit::refid_request;
vector<ParticleEmitter*> ParticleEmitter::refid_table;
vector<TrailParticleEmitter*> TrailParticleEmitter::refid_table;
vector<Useable*> Useable::refid_table;
vector<UseableRequest> Useable::refid_request;

void Game::BuildRefidTables()
{
	// jednostki i u¿ywalne
	Unit::refid_table.clear();
	Useable::refid_table.clear();
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		if(*it)
			(*it)->BuildRefidTable();
	}

	// cz¹steczki
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();
	if(open_location != -1)
	{
		for(vector<ParticleEmitter*>::iterator it2 = local_ctx.pes->begin(), end2 = local_ctx.pes->end(); it2 != end2; ++it2)
		{
			(*it2)->refid = (int)ParticleEmitter::refid_table.size();
			ParticleEmitter::refid_table.push_back(*it2);
		}
		for(vector<TrailParticleEmitter*>::iterator it2 = local_ctx.tpes->begin(), end2 = local_ctx.tpes->end(); it2 != end2; ++it2)
		{
			(*it2)->refid = (int)TrailParticleEmitter::refid_table.size();
			TrailParticleEmitter::refid_table.push_back(*it2);
		}
		if(city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			{
				for(vector<ParticleEmitter*>::iterator it2 = (*it)->ctx.pes->begin(), end2 = (*it)->ctx.pes->end(); it2 != end2; ++it2)
				{
					(*it2)->refid = (int)ParticleEmitter::refid_table.size();
					ParticleEmitter::refid_table.push_back(*it2);
				}
				for(vector<TrailParticleEmitter*>::iterator it2 = (*it)->ctx.tpes->begin(), end2 = (*it)->ctx.tpes->end(); it2 != end2; ++it2)
				{
					(*it2)->refid = (int)TrailParticleEmitter::refid_table.size();
					TrailParticleEmitter::refid_table.push_back(*it2);
				}
			}
		}
	}
}

void Game::ClearGameVarsOnNewGameOrLoad()
{
	inventory_mode = I_NONE;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	death_screen = 0;
	koniec_gry = false;
	selected_unit = nullptr;
	selected_target = nullptr;
	before_player = BP_NONE;
	minimap_reveal.clear();
	game_gui->minimap->city = nullptr;
	team_shares.clear();
	team_share_id = -1;
	draw_flags = 0xFFFFFFFF;
	unit_views.clear();
	to_remove.clear();
	game_gui->journal->Reset();
	arena_viewers.clear();
	debug_info = false;
	debug_info2 = false;
	dialog_enc = nullptr;
	dialog_pvp = nullptr;
	game_gui->visible = false;
	Inventory::lock_id = LOCK_NO;
	Inventory::lock_give = false;
	picked_location = -1;
	post_effects.clear();
	grayout = 0.f;
	cam.Reset();
	player_rot_buf = 0.f;
	lights_dt = 1.f;

#ifdef DRAW_LOCAL_PATH
	marked = nullptr;
#endif

	// odciemnianie na pocz¹tku
	fallback_co = FALLBACK_NONE;
	fallback_t = 0.f;
}

void Game::ClearGameVarsOnNewGame()
{
	ClearGameVarsOnNewGameOrLoad();

	devmode = default_devmode;
	cl_fog = true;
	cl_lighting = true;
	draw_particle_sphere = false;
	draw_unit_radius = false;
	draw_hitbox = false;
	noai = false;
	draw_phy = false;
	draw_col = false;
	cam.real_rot = VEC2(0, 4.2875104f);
	cam.dist = 3.5f;
	game_speed = 1.f;
	dungeon_level = 0;
	quest_counter = 0;
	notes.clear();
	year = 100;
	day = 0;
	month = 0;
	worldtime = 0;
	gt_hour = 0;
	gt_minute = 0;
	gt_second = 0;
	gt_tick = 0.f;
	team_gold = 0;
	dont_wander = false;
	szansa_na_spotkanie = 0.f;
	atak_szalencow = false;
	bandyta = false;
	create_camp = 0;
	arena_fighter = nullptr;
	quest_rumor_counter = P_MAX;
	for(int i=0; i<P_MAX; ++i)
		quest_rumor[i] = false;
	rumors.clear();
	free_recruit = true;
	first_city = true;
	unique_quests_completed = 0;
	unique_completed_show = false;
	news.clear();
	picking_item_state = 0;
	arena_tryb = Arena_Brak;
	world_state = WS_MAIN;
	open_location = -1;
	picked_location = -1;
	arena_free = true;
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	quests_timeout.clear();
	quests_timeout2.clear();
	total_kills = 0;
	world_dir = random(MAX_ANGLE);
	game_gui->PositionPanels();
	ClearGui(true);
	game_gui->mp_box->visible = sv_online;
	drunk_anim = 0.f;
	light_angle = random(PI*2);
	cam.Reset();
	player_rot_buf = 0.f;
	start_version = VERSION;

#ifdef _DEBUG
	noai = true;
	dont_wander = true;
#endif
}

void Game::ClearGameVarsOnLoad()
{
	ClearGameVarsOnNewGameOrLoad();
}

Quest* Game::FindQuest(int refid, bool active)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((!active || (*it)->IsActive()) && (*it)->refid == refid)
			return *it;
	}

	return nullptr;
}

Quest* Game::FindQuest(int loc, Quest::Type type)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if((*it)->start_loc == loc && (*it)->type == type)
			return *it;
	}

	return nullptr;
}

Quest* Game::FindQuestById(QUEST quest_id)
{
	for(Quest* q : quests)
	{
		if(q->quest_id == quest_id)
			return q;
	}

	for(Quest* q : unaccepted_quests)
	{
		if(q->quest_id == quest_id)
			return q;
	}

	return nullptr;
}

Quest* Game::FindUnacceptedQuest(int loc, Quest::Type type)
{
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
	{
		if((*it)->start_loc == loc && (*it)->type == type)
			return *it;
	}

	return nullptr;
}

Quest* Game::FindUnacceptedQuest(int refid)
{
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
	{
		if((*it)->refid == refid)
			return *it;
	}

	return nullptr;
}

int Game::GetRandomSettlement(int this_city)
{
	int id = rand2() % settlements;
	if(id == this_city)
		id = (id + 1) % settlements;
	return id;
}

int Game::GetRandomFreeSettlement(int this_city)
{
	int id = rand2() % settlements, tries = (int)settlements;
	while((id == this_city || locations[id]->active_quest) && tries>0)
	{
		id = (id + 1) % settlements;
		--tries;
	}
	return (tries == 0 ? -1 : id);
}

int Game::GetRandomCity(int this_city)
{
	// policz ile jest miast
	int ile = 0;
	while(LocationHelper::IsCity(locations[ile]))
		++ile;

	int id = rand2()%ile;
	if(id == this_city)
	{
		++id;
		if(id == ile)
			id = 0;
	}

	return id;
}

void Game::LoadQuests(vector<Quest*>& v_quests, HANDLE file)
{
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	v_quests.resize(count);
	for(uint i=0; i<count; ++i)
	{
		QUEST q;
		ReadFile(file, &q, sizeof(q), &tmp, nullptr);

		if(LOAD_VERSION == V_0_2)
		{
			if(q > Q_EVIL)
				q = (QUEST)(q-1);
		}

		Quest* quest = quest_manager.CreateQuest(q);

		quest->quest_id = q;
		quest->quest_index = i;
		quest->Load(file);
		v_quests[i] = quest;
	}
}

// czyszczenie gry
void Game::ClearGame()
{
	LOG("Clearing game.");

	draw_batch.Clear();

	LeaveLocation(true);

	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP) && open_location == -1 && IsLocal() && !was_client)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit* unit = *it;

			if(unit->bow_instance)
				bow_instances.push_back(unit->bow_instance);
			if(unit->interp)
				interpolators.Free(unit->interp);

			delete unit->ai;
			delete unit;
		}

		prev_game_state = GS_LOAD;
	}

	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// usuñ lokalizacje
	DeleteElements(locations);
	open_location = -1;
	local_ctx_valid = false;
	city_ctx = nullptr;

	// usuñ quest
	DeleteElements(quests);
	DeleteElements(unaccepted_quests);
	DeleteElements(quest_items);

	DeleteElements(news);
	DeleteElements(encs);

	ClearGui(true);
}

cstring Game::FormatString(DialogContext& ctx, const string& str_part)
{
	if(str_part == "burmistrzem")
		return LocationHelper::IsCity(location) ? "burmistrzem" : "so³tysem";
	else if(str_part == "mayor")
		return LocationHelper::IsCity(location) ? "mayor" : "soltys";
	else if(str_part == "rcitynhere")
		return locations[GetRandomSettlement(current_location)]->name.c_str();
	else if(str_part == "name")
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->name.c_str();
	}
	else if(str_part == "join_cost")
	{
		assert(ctx.talker->IsHero());
		return Format("%d", ctx.talker->hero->JoinCost());
	}
	else if(str_part == "item")
	{
		assert(ctx.team_share_id != -1);
		return ctx.team_share_item->name.c_str();
	}
	else if(str_part == "item_value")
	{
		assert(ctx.team_share_id != -1);
		return Format("%d", ctx.team_share_item->value/2);
	}
	else if(str_part == "chlanie_loc")
		return locations[contest_where]->name.c_str();
	else if(str_part == "player_name")
		return current_dialog->pc->name.c_str();
	else if(str_part == "ironfist_city")
		return locations[tournament_city]->name.c_str();
	else if(str_part == "rhero")
	{
		static string str;
		GenerateHeroName(ClassInfo::GetRandom(), rand2()%4==0, str);
		return str.c_str();
	}
	else if(str_part == "ritem")
		return crazy_give_item->name.c_str();
	else
	{
		assert(0);
		return "";
	}
}

int Game::GetNearestLocation(const VEC2& pos, bool not_quest, bool not_city)
{
	float best_dist = 999.f;
	int best_loc = -1;

	int index = 0;
	for(vector<Location*>::iterator it = locations.begin()+(not_city ? settlements : 0), end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;
		float dist = distance((*it)->pos, pos);
		if(dist < best_dist)
		{
			best_loc = index;
			best_dist = dist;
		}
	}

	return best_loc;
}

void Game::AddGameMsg(cstring msg, float time)
{
	game_gui->game_messages->AddMessage(msg, time, 0);
}

void Game::AddGameMsg2(cstring msg, float time, int id)
{
	assert(id > 0);
	game_gui->game_messages->AddMessageIfNotExists(msg, time, id);
}

void Game::CreateCityMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, nullptr, 0);

	OutsideLocation* loc = (OutsideLocation*)location;

	for(int y=0; y<OutsideLocation::size; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x=0; x<OutsideLocation::size; ++x)
		{
			const TerrainTile& t = loc->tiles[x+(OutsideLocation::size-1-y)*OutsideLocation::size];
			DWORD col;
			if(t.mode >= TM_BUILDING)
				col = COLOR_RGB(128,64,0);
			else if(t.alpha == 0)
			{
				if(t.t == TT_GRASS)
					col = COLOR_RGB(0,128,0);
				else if(t.t == TT_ROAD)
					col = COLOR_RGB(128,128,128);
				else if(t.t == TT_FIELD)
					col = COLOR_RGB(200,200,100);
				else
					col = COLOR_RGB(128,128,64);
			}
			else
			{
				int r, g, b, r2, g2, b2;
				switch(t.t)
				{
				case TT_GRASS:
					r = 0;
					g = 128;
					b = 0;
					break;
				case TT_ROAD:
					r = 128;
					g = 128;
					b = 128;
					break;
				case TT_FIELD:
					r = 200;
					g = 200;
					b = 0;
					break;
				case TT_SAND:
				default:
					r = 128;
					g = 128;
					b = 64;
					break;
				}
				switch(t.t2)
				{
				case TT_GRASS:
					r2 = 0;
					g2 = 128;
					b2 = 0;
					break;
				case TT_ROAD:
					r2 = 128;
					g2 = 128;
					b2 = 128;
					break;
				case TT_FIELD:
					r2 = 200;
					g2 = 200;
					b2 = 0;
					break;
				case TT_SAND:
				default:
					r2 = 128;
					g2 = 128;
					b2 = 64;
					break;
				}
				const float T = float(t.alpha)/255;
				col = COLOR_RGB(lerp(r,r2,T), lerp(g,g2,T), lerp(b,b2,T));
			}
			if(x < 16 || x > 128-16 || y < 16 || y > 128-16)
			{
				col = ((col & 0xFF) / 2) |
					((((col & 0xFF00) >> 8) / 2) << 8) |
					((((col & 0xFF0000) >> 16) / 2) << 16) |
					0xFF000000;
			}
			*pix = col;
			++pix;
		}
	}

	tMinimap->UnlockRect(0);

	game_gui->minimap->minimap_size = OutsideLocation::size;
}

void Game::CreateDungeonMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, nullptr, 0);

	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

	for(int y = 0; y<lvl.h; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x = 0; x<lvl.w; ++x)
		{
			Pole& p = lvl.map[x+(lvl.w-1-y)*lvl.w];
			if(IS_SET(p.flags, Pole::F_ODKRYTE))
			{
				if(OR2_EQ(p.type, SCIANA, BLOKADA_SCIANA))
					*pix = COLOR_RGB(100, 100, 100);
				else if(p.type == DRZWI)
					*pix = COLOR_RGB(127, 51, 0);
				else
					*pix = COLOR_RGB(220, 220, 240);
			}
			else
				*pix = 0;
			++pix;
		}
	}

	// extra borders
	DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*lvl.h);
	for(int x = 0; x<lvl.w+1; ++x)
	{
		*pix = COLOR_RGB(100, 100, 100);
		++pix;
	}
	for(int y = 0; y<lvl.h+1; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		pix += lvl.w;
		*pix = COLOR_RGB(100, 100, 100);
	}

	tMinimap->UnlockRect(0);

	game_gui->minimap->minimap_size = lvl.w;
}

void Game::RebuildMinimap()
{
	if(game_state == GS_LEVEL)
	{
		switch(location->type)
		{
		case L_CITY:
			CreateCityMinimap();
			break;
		case L_DUNGEON:
		case L_CRYPT:
		case L_CAVE:
			CreateDungeonMinimap();
			break;
		case L_FOREST:
		case L_CAMP:
		case L_ENCOUNTER:
		case L_MOONWELL:
			CreateForestMinimap();
			break;
		default:
			assert(0);
			WARN(Format("RebuildMinimap: unknown location %d.", location->type));
			break;
		}
	}
}

void Game::UpdateDungeonMinimap(bool send)
{
	if(minimap_reveal.empty())
		return;

	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, nullptr, 0);

	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

	for(vector<INT2>::iterator it = minimap_reveal.begin(), end = minimap_reveal.end(); it != end; ++it)
	{
		Pole& p = lvl.map[it->x+(lvl.w-it->y-1)*lvl.w];
		SET_BIT(p.flags, Pole::F_ODKRYTE);
		DWORD* pix = ((DWORD*)(((byte*)lock.pBits)+lock.Pitch*it->y))+it->x;
		if(OR2_EQ(p.type, SCIANA, BLOKADA_SCIANA))
			*pix = COLOR_RGB(100,100,100);
		else if(p.type == DRZWI)
			*pix = COLOR_RGB(127,51,0);
		else
			*pix = COLOR_RGB(220,220,240);
	}
	
	if(IsLocal())
	{
		if(send)
		{
			if(IsOnline())
				minimap_reveal_mp.insert(minimap_reveal_mp.end(), minimap_reveal.begin(), minimap_reveal.end());
		}
		else
			minimap_reveal_mp.clear();
	}

	minimap_reveal.clear();

	tMinimap->UnlockRect(0);
}

Door* Game::FindDoor(LevelContext& ctx, const INT2& pt)
{
	for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
	{
		if((*it)->pt == pt)
			return *it;
	}

	return nullptr;
}

const Item* Game::FindQuestItem(cstring name, int refid)
{
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
	{
		if(refid == (*it)->refid)
			return (*it)->GetQuestItem();
	}
	assert(0);
	return nullptr;
}

void Game::AddGroundItem(LevelContext& ctx, GroundItem* item)
{
	assert(item);

	if(ctx.type == LevelContext::Outside)
		terrain->SetH(item->pos);
	ctx.items->push_back(item);

	if(IsOnline())
	{
		item->netid = item_netid_counter++;
		NetChange& c = Add1(net_changes);
		c.type = NetChange::SPAWN_ITEM;
		c.item = item;
	}
}

SOUND Game::GetItemSound(const Item* item)
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return sItem[6];
	case IT_ARMOR:
		if(item->ToArmor().skill != Skill::LIGHT_ARMOR)
			return sItem[2];
		else
			return sItem[1];
	case IT_BOW:
		return sItem[4];
	case IT_SHIELD:
		return sItem[5];
	case IT_CONSUMABLE:
		if(item->ToConsumable().cons_type != Food)
			return sItem[0];
		else
			return sItem[7];
	case IT_OTHER:
		if(IS_SET(item->flags, ITEM_CRYSTAL_SOUND))
			return sItem[3];
		else
			return sItem[7];
	case IT_GOLD:
		return sCoins;
	default:
		return sItem[7];
	}
}

cstring Game::GetCurrentLocationText()
{
	if(game_state == GS_LEVEL)
	{
		if(location->outside)
			return location->name.c_str();
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
				return Format(txLocationText, location->name.c_str(), dungeon_level+1);
			else
				return location->name.c_str();
		}
	}
	else
		return Format(txLocationTextMap, locations[current_location]->name.c_str());
}

void Game::Unit_StopUsingUseable(LevelContext& ctx, Unit& u, bool send)
{
	u.animation = ANI_STAND;
	u.animation_state = AS_ANIMATION2_MOVE_TO_ENDPOINT;
	u.timer = 0.f;
	u.used_item = nullptr;

	const float unit_radius = u.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&u, nullptr};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, u.pos, 2.f+unit_radius, &ignore);

	VEC3 tmp_pos = u.target_pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i=0; i<20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			if(i != 0 && ctx.have_terrain)
				tmp_pos.y = terrain->GetH(tmp_pos);
			u.target_pos = tmp_pos;
			ok = true;
			break;
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = u.target_pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*radius;

		if(i < 10)
			radius += 0.2f;
	}

	assert(ok);

	if(u.cobj)
		UpdateUnitPhysics(u, u.target_pos);

	if(send && IsOnline())
	{
		if(IsLocal())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.unit = &u;
			c.id = u.useable->netid;
			c.ile = 3;
		}
		else if(&u == pc->unit)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.id = u.useable->netid;
			c.ile = 0;
		}
	}
}

// ponowne wejœcie na poziom podziemi
void Game::OnReenterLevel(LevelContext& ctx)
{
	// odtwórz skrzynie
	if(ctx.chests)
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
		{
			Chest& chest = **it;

			chest.ani = new AnimeshInstance(aSkrzynia);
		}
	}

	// odtwórz drzwi
	if(ctx.doors)
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;

			// animowany model
			door.ani = new AnimeshInstance(door.door2 ? aDrzwi2 : aDrzwi);
			door.ani->groups[0].speed = 2.f;

			// fizyka
			door.phy = new btCollisionObject;
			door.phy->setCollisionShape(shape_door);
			btTransform& tr = door.phy->getWorldTransform();
			VEC3 pos = door.pos;
			pos.y += 1.319f;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door.rot, 0, 0));
			door.phy->setCollisionFlags(CG_WALL);
			phy_world->addCollisionObject(door.phy);

			// czy otwarte
			if(door.state == Door::Open)
			{
				btVector3& pos = door.phy->getWorldTransform().getOrigin();
				pos.setY(pos.y() - 100.f);
				door.ani->SetToEnd(door.ani->ani->anims[0].name.c_str());
			}
		}
	}
}

void Game::ApplyToTexturePack(TexturePack& tp, cstring diffuse, cstring normal, cstring specular)
{
	tp.diffuse = resMgr.GetLoadedTexture(diffuse);
	if(normal)
		tp.normal = resMgr.GetLoadedTexture(normal);
	else
		tp.normal = nullptr;
	if(specular)
		tp.specular = resMgr.GetLoadedTexture(specular);
	else
		tp.specular = nullptr;
}

void ApplyTexturePackToSubmesh(Animesh::Submesh& sub, TexturePack& tp)
{
	sub.tex = tp.diffuse;
	sub.tex_normal = tp.normal;
	sub.tex_specular = tp.specular;
}

void ApplyDungeonLightToMesh(Animesh& mesh)
{
	for(int i=0; i<mesh.head.n_subs; ++i)
	{
		mesh.subs[i].specular_color = VEC3(1,1,1);
		mesh.subs[i].specular_intensity = 0.2f;
		mesh.subs[i].specular_hardness = 10;
	}
}

void Game::SetDungeonParamsAndTextures(BaseLocation& base)
{
	// ustawienia t³a
	cam.draw_range = base.draw_range;
	fog_params = VEC4(base.fog_range.x, base.fog_range.y, base.fog_range.y-base.fog_range.x, 0);
	fog_color = VEC4(base.fog_color, 1);
	ambient_color = VEC4(base.ambient_color, 1);
	clear_color2 = COLOR_RGB(int(fog_color.x*255), int(fog_color.y*255), int(fog_color.z*255));

	// tekstury podziemi
	if(base.tex_floor)
		ApplyToTexturePack(tFloor[0], base.tex_floor, base.tex_floor_nrm, base.tex_floor_spec);
	else
		tFloor[0] = tFloorBase;
	if(base.tex_wall)
		ApplyToTexturePack(tWall[0], base.tex_wall, base.tex_wall_nrm, base.tex_wall_spec);
	else
		tWall[0] = tWallBase;
	if(base.tex_ceil)
		ApplyToTexturePack(tCeil[0], base.tex_ceil, base.tex_ceil_nrm, base.tex_ceil_spec);
	else
		tCeil[0] = tCeilBase;

	// tekstury schodów / pu³apek
	ApplyTexturePackToSubmesh(aSchodyDol->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyDol->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aSchodyDol2->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyDol2->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aSchodyGora->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aSchodyGora->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aNaDrzwi->subs[0], tWall[0]);
	ApplyTexturePackToSubmesh(g_traps[TRAP_ARROW].mesh->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(g_traps[TRAP_POISON].mesh->subs[0], tFloor[0]);
	ApplyDungeonLightToMesh(*aSchodyDol);
	ApplyDungeonLightToMesh(*aSchodyDol2);
	ApplyDungeonLightToMesh(*aSchodyGora);
	ApplyDungeonLightToMesh(*aNaDrzwi);
	ApplyDungeonLightToMesh(*aNaDrzwi2);
	ApplyDungeonLightToMesh(*g_traps[TRAP_ARROW].mesh);
	ApplyDungeonLightToMesh(*g_traps[TRAP_POISON].mesh);
	
	// druga tekstura
	if(base.tex2 != -1)
	{
		BaseLocation& base2 = g_base_locations[base.tex2];
		if(base2.tex_floor)
			ApplyToTexturePack(tFloor[1], base2.tex_floor, base2.tex_floor_nrm, base2.tex_floor_spec);
		else
			tFloor[1] = tFloor[0];
		if(base2.tex_wall)
			ApplyToTexturePack(tWall[1], base2.tex_wall, base2.tex_wall_nrm, base2.tex_wall_spec);
		else
			tWall[1] = tWall[0];
		if(base2.tex_ceil)
			ApplyToTexturePack(tCeil[1], base2.tex_ceil, base2.tex_ceil_nrm, base2.tex_ceil_spec);
		else
			tCeil[1] = tCeil[0];
		ApplyTexturePackToSubmesh(aNaDrzwi2->subs[0], tWall[1]);
	}
	else
	{
		tFloor[1] = tFloorBase;
		tCeil[1] = tCeilBase;
		tWall[1] = tWallBase;
		ApplyTexturePackToSubmesh(aNaDrzwi2->subs[0], tWall[0]);
	}

	// ustawienia uv podziemi
	bool new_tex_wrap = !IS_SET(base.options, BLO_NO_TEX_WRAP);
	if(new_tex_wrap != dungeon_tex_wrap)
	{
		dungeon_tex_wrap = new_tex_wrap;
		ChangeDungeonTexWrap();
	}
}

void Game::EnterLevel(bool first, bool reenter, bool from_lower, int from_portal, bool from_outside)
{
	if(!first)
		LOG(Format("Entering location '%s' level %d.", location->name.c_str(), dungeon_level+1));

	show_mp_panel = true;
	Inventory::lock_id = LOCK_NO;
	Inventory::lock_give = false;

	InsideLocation* inside = (InsideLocation*)location;
	inside->SetActiveLevel(dungeon_level);
	int days;
	bool need_reset = inside->CheckUpdate(days, worldtime);
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	if(from_portal != -1)
		enter_from = ENTER_FROM_PORTAL+from_portal;
	else if(from_outside)
		enter_from = ENTER_FROM_OUTSIDE;
	else if(from_lower)
		enter_from = ENTER_FROM_DOWN_LEVEL;
	else
		enter_from = ENTER_FROM_UP_LEVEL;

	// ustaw wskaŸniki
	if(!reenter)
		ApplyContext(inside, local_ctx);

	SetDungeonParamsAndTextures(base);

	// czy to pierwsza wizyta?
	if(first)
	{
		if(location->type == L_CAVE)
		{
			LoadingStep(txGeneratingObjects);

			// jaskinia
			// schody w górê
			Object& o = Add1(local_ctx.objects);
			o.mesh = aSchodyGora;
			o.pos = pt_to_pos(lvl.staircase_up);
			o.rot = VEC3(0, dir_to_rot(lvl.staircase_up_dir), 0);
			o.scale = 1;
			o.base = nullptr;

			GenerateCaveObjects();
			if(current_location == quest_mine->target_loc)
				GenerateMine();

			LoadingStep(txGeneratingUnits);
			GenerateCaveUnits();
			GenerateMushrooms();
		}
		else
		{
			// podziemia, wygeneruj schody, drzwi, kratki
			LoadingStep(txGeneratingObjects);
			GenerateDungeonObjects2();
			GenerateDungeonObjects();
			GenerateTraps();

			if(!IS_SET(base.options, BLO_LABIRYNTH))
			{
				LoadingStep(txGeneratingUnits);
				GenerateDungeonUnits();
				GenerateDungeonFood();
				ResetCollisionPointers();
			}
			else
			{
				Obj* obj = FindObject("torch");

				// pochodnia przy œcianie
				{
					Object& o = Add1(lvl.objects);
					o.base = obj;
					o.rot = VEC3(0,random(MAX_ANGLE),0);
					o.scale = 1.f;
					o.mesh = obj->mesh;

					INT2 pt = lvl.GetUpStairsFrontTile();
					if(czy_blokuje2(lvl.map[pt.x - 1 + pt.y*lvl.w].type))
						o.pos = VEC3(2.f*pt.x + obj->size.x + 0.1f, 0.f, 2.f*pt.y + 1.f);
					else if(czy_blokuje2(lvl.map[pt.x + 1 + pt.y*lvl.w].type))
						o.pos = VEC3(2.f*(pt.x + 1) - obj->size.x - 0.1f, 0.f, 2.f*pt.y + 1.f);
					else if(czy_blokuje2(lvl.map[pt.x + (pt.y - 1)*lvl.w].type))
						o.pos = VEC3(2.f*pt.x + 1.f, 0.f, 2.f*pt.y + obj->size.y + 0.1f);
					else if(czy_blokuje2(lvl.map[pt.x + (pt.y + 1)*lvl.w].type))
						o.pos = VEC3(2.f*pt.x + 1.f, 0.f, 2.f*(pt.y + 1) + obj->size.y - 0.1f);

					Light& s = Add1(lvl.lights);
					s.pos = o.pos;
					s.pos.y += obj->centery;
					s.range = 5;
					s.color = VEC3(1.f,0.9f,0.9f);

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tFlare;
					pe->alpha = 0.8f;
					pe->emision_interval = 0.1f;
					pe->emisions = -1;
					pe->life = -1;
					pe->max_particles = 50;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->particle_life = 0.5f;
					pe->pos = s.pos;
					pe->pos_min = VEC3(0,0,0);
					pe->pos_max = VEC3(0,0,0);
					pe->size = IS_SET(obj->flags, OBJ_CAMPFIRE) ? .7f : .5f;
					pe->spawn_min = 1;
					pe->spawn_max = 3;
					pe->speed_min = VEC3(-1,3,-1);
					pe->speed_max = VEC3(1,4,1);
					pe->mode = 1;
					pe->Init();
					local_ctx.pes->push_back(pe);
				}

				// pochodnia w skarbie
				{
					Object& o = Add1(lvl.objects);
					o.base = obj;
					o.rot = VEC3(0,random(MAX_ANGLE),0);
					o.scale = 1.f;
					o.mesh = obj->mesh;
					o.pos = lvl.rooms[0].Center();

					Light& s = Add1(lvl.lights);
					s.pos = o.pos;
					s.pos.y += obj->centery;
					s.range = 5;
					s.color = VEC3(1.f,0.9f,0.9f);

					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = tFlare;
					pe->alpha = 0.8f;
					pe->emision_interval = 0.1f;
					pe->emisions = -1;
					pe->life = -1;
					pe->max_particles = 50;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->particle_life = 0.5f;
					pe->pos = s.pos;
					pe->pos_min = VEC3(0,0,0);
					pe->pos_max = VEC3(0,0,0);
					pe->size = IS_SET(obj->flags, OBJ_CAMPFIRE) ? .7f : .5f;
					pe->spawn_min = 1;
					pe->spawn_max = 3;
					pe->speed_min = VEC3(-1,3,-1);
					pe->speed_max = VEC3(1,4,1);
					pe->mode = 1;
					pe->Init();
					local_ctx.pes->push_back(pe);
				}

				LoadingStep(txGeneratingUnits);
				GenerateLabirynthUnits();
			}
		}
	}
	else if(!reenter)
	{
		LoadingStep(txRegeneratingLevel);

		if(days > 0)
			UpdateLocation(local_ctx, days, base.door_open, need_reset);

		bool respawn_units = true;

		if(location->type == L_CAVE)
		{
			if(current_location == quest_mine->target_loc)
				respawn_units = GenerateMine();
			if(days > 0)
				GenerateMushrooms(min(days, 10));
		}
		else if(days >= 10 && IS_SET(base.options, BLO_LABIRYNTH))
			GenerateDungeonFood();

		if(need_reset)
		{
			// usuñ ¿ywe jednostki
			if(days != 0)
			{
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
				{
					if((*it)->IsAlive())
					{
						delete *it;
						*it = nullptr;
					}
				}
				RemoveNullElements(local_ctx.units);
			}

			// usuñ zawartoœæ skrzyni
			for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawartoœæ skrzyni
			int dlevel = GetDungeonLevel();
			for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
			{
				Chest& chest = **it;
				if(!chest.items.empty())
					continue;
				Room* r = lvl.GetNearestRoom(chest.pos);
				static vector<Chest*> room_chests;
				room_chests.push_back(&chest);
				for(vector<Chest*>::iterator it2 = it+1; it2 != end; ++it2)
				{
					if(r->IsInside((*it2)->pos))
						room_chests.push_back(*it2);
				}
				int gold;
				if(room_chests.size() == 1)
				{
					vector<ItemSlot>& items = room_chests.front()->items;
					GenerateTreasure(dlevel, 10, items, gold);
					InsertItemBare(items, gold_item_ptr, (uint)gold, (uint)gold);
					SortItems(items);
				}
				else
				{
					static vector<ItemSlot> items;
					GenerateTreasure(dlevel, 9+2*room_chests.size(), items, gold);
					SplitTreasure(items, gold, &room_chests[0], room_chests.size());
				}
				room_chests.clear();
			}

			// nowe jednorazowe pu³apki
			RegenerateTraps();
		}

		OnReenterLevel(local_ctx);

		// odtwórz jednostki
		if(respawn_units)
			RespawnUnits();
		RespawnTraps();

		// odtwórz fizykê
		RespawnObjectColliders();

		if(need_reset)
		{
			// nowe jednostki
			if(location->type == L_CAVE)
				GenerateCaveUnits();
			else if(inside->target == LABIRYNTH)
				GenerateLabirynthUnits();
			else
				GenerateDungeonUnits();
		}
	}

	// questowe rzeczy
	if(inside->active_quest && inside->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
	{
		Quest_Event* event = inside->active_quest->GetEvent(current_location);
		if(event)
		{
			if(event->at_level == dungeon_level)
			{
				if(!event->done)
				{
					HandleQuestEvent(event);

					// generowanie orków
					if(current_location == quest_orcs2->target_loc && quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs)
					{
						quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedOrcs;
						UnitData* ud = FindUnitData("q_orkowie_slaby");
						for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
						{
							if(!it->IsCorridor() && rand2()%2 == 0)
							{
								Unit* u = SpawnUnitInsideRoom(*it, *ud, -2, INT2(-999, -999), INT2(-999, -999));
								if(u)
									u->dont_attack = true;
							}
						}

						Unit* u = SpawnUnitInsideRoom(lvl.GetFarRoom(false), *FindUnitData("q_orkowie_kowal"), -2, INT2(-999, -999), INT2(-999, -999));
						if(u)
							u->dont_attack = true;

						vector<ItemSlot>& items = quest_orcs2->wares;
						ParseStockScript(FindStockScript("orc_blacksmith"), 0, false, items);
						SortItems(items);
					}
				}

				location_event_handler = event->location_event_handler;
			}
			else if(inside->active_quest->whole_location_event_handler)
				location_event_handler = event->location_event_handler;
		}
	}

	bool debug_do = DEBUG_BOOL;

	if((first || need_reset) && (rand2()%50 == 0 || (debug_do && Key.Down('C'))) && location->type != L_CAVE && inside->target != LABIRYNTH && !location->active_quest && dungeon_level == 0)
		SpawnHeroesInsideDungeon();

	// stwórz obiekty kolizji
	if(!reenter)
		SpawnDungeonColliders();

	// generuj minimapê
	LoadingStep(txGeneratingMinimap);
	CreateDungeonMinimap();

	// sekret
	if(current_location == secret_where && !inside->HaveDownStairs() && secret_state == SECRET_DROPPED_STONE)
	{
		secret_state = SECRET_GENERATED;
		if(devmode)
			LOG("Generated secret room.");

		Room& r = inside->GetLevelData().rooms[0];

		if(hardcore_mode)
		{
			Object* o = local_ctx.FindObject(FindObject("portal"));

			OutsideLocation* loc = new OutsideLocation;
			loc->active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
			loc->pos = VEC2(-999,-999);
			loc->st = 20;
			loc->name = txHiddenPlace;
			loc->type = L_FOREST;
			loc->image = LI_FOREST;
			int loc_id = AddLocation(loc);

			Portal* portal = new Portal;
			portal->at_level = 2;
			portal->next_portal = nullptr;
			portal->pos = o->pos;
			portal->rot = o->rot.y;
			portal->target = 0;
			portal->target_loc = loc_id;

			inside->portal = portal;
			secret_where2 = loc_id;
		}
		else
		{
			// dodaj kartkê (overkill sprawdzania!)
			const Item* kartka = FindItem("sekret_kartka2");
			assert(kartka);
			Chest* c = local_ctx.FindChestInRoom(r);
			assert(c);
			if(c)
				c->AddItem(kartka, 1, 1);
			else
			{
				Object* o = local_ctx.FindObject(FindObject("portal"));
				assert(0);
				if(o)
				{
					GroundItem* item = new GroundItem;
					item->count = 1;
					item->team_count = 1;
					item->item = kartka;
					item->netid = item_netid_counter++;
					item->pos = o->pos;
					item->rot = random(MAX_ANGLE);
					local_ctx.items->push_back(item);
				}
				else
					SpawnGroundItemInsideRoom(r, kartka);
			}

			secret_state = SECRET_CLOSED;
		}
	}

	// dodaj gracza i jego dru¿ynê
	INT2 spawn_pt;
	VEC3 spawn_pos;
	float spawn_rot;

	if(from_portal == -1)
	{
		if(from_lower)
		{
			spawn_pt = lvl.GetDownStairsFrontTile();
			spawn_rot = dir_to_rot(lvl.staircase_down_dir);
		}
		else
		{
			spawn_pt = lvl.GetUpStairsFrontTile();
			spawn_rot = dir_to_rot(lvl.staircase_up_dir);
		}
		spawn_pos = pt_to_pos(spawn_pt);
	}
	else
	{
		Portal* portal = inside->GetPortal(from_portal);
		spawn_pos = portal->GetSpawnPos();
		spawn_rot = clip(portal->rot+PI);
		spawn_pt = pos_to_pt(spawn_pos);
	}

	AddPlayerTeam(spawn_pos, spawn_rot, reenter, from_outside);
	OpenDoorsByTeam(spawn_pt);
	SetMusic();
	OnEnterLevelOrLocation();
	OnEnterLevel();

	if(!first)
		LOG("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	LOG("Leaving level.");

	if(game_gui)
		game_gui->Reset();

	warp_to_inn.clear();

	if(local_ctx_valid)
	{
		LeaveLevel(local_ctx, clear);
		tmp_ctx_pool.Free(local_ctx.tmp_ctx);
		local_ctx.tmp_ctx = nullptr;
		local_ctx_valid = false;
	}

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if(!*it)
				continue;
			LeaveLevel((*it)->ctx, clear);
			if((*it)->ctx.require_tmp_ctx)
			{
				tmp_ctx_pool.Free((*it)->ctx.tmp_ctx);
				(*it)->ctx.tmp_ctx = nullptr;
			}
		}

		if(!IsLocal())
			DeleteElements(city_ctx->inside_buildings);
	}

	for(vector<Unit*>::iterator it = warp_to_inn.begin(), end = warp_to_inn.end(); it != end; ++it)
		WarpToInn(**it);

	ais.clear();
	RemoveColliders();
	unit_views.clear();
	attached_sounds.clear();
	cam_colliders.clear();

	ClearQuadtree();

	CloseAllPanels();

	cam.Reset();
	player_rot_buf = 0.f;
	selected_unit = nullptr;
	dialog_context.dialog_mode = false;
	inventory_mode = I_NONE;
	before_player = BP_NONE;
}

void Game::LeaveLevel(LevelContext& ctx, bool clear)
{
	// posprz¹taj jednostki
	// usuñ AnimeshInstance i btCollisionShape
	if(IsLocal() && !clear)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			// fizyka
			delete (*it)->cobj->getCollisionShape();
			(*it)->cobj = nullptr;

			// speech bubble
			(*it)->bubble = nullptr;
			(*it)->talking = false;

			// jeœli u¿ywa jakiegoœ obiektu to przesuñ
			if((*it)->useable)
			{
				Unit_StopUsingUseable(ctx, **it);
				(*it)->useable->user = nullptr;
				(*it)->useable = nullptr;
				(*it)->visual_pos = (*it)->pos = (*it)->target_pos;
			}

			if((*it)->bow_instance)
			{
				bow_instances.push_back((*it)->bow_instance);
				(*it)->bow_instance = nullptr;
			}

			// model
			if((*it)->IsAI())
			{
				if((*it)->IsFollower())
				{
					if(!(*it)->IsAlive())
					{
						(*it)->hp = 1.f;
						(*it)->live_state = Unit::ALIVE;
					}
					(*it)->hero->mode = HeroData::Follow;
					(*it)->talking = false;
					(*it)->ani->need_update = true;
					(*it)->ai->Reset();
					*it = nullptr;
				}
				else
				{
					if((*it)->ai->goto_inn && city_ctx)
					{
						// przenieœ do karczmy przy opuszczaniu lokacji
						WarpToInn(**it);
					}
					delete (*it)->ani;
					(*it)->ani = nullptr;
					delete (*it)->ai;
					(*it)->ai = nullptr;
					(*it)->EndEffects();

					if((*it)->useable)
						(*it)->pos = (*it)->target_pos;
				}
			}
			else
			{
				(*it)->talking = false;
				(*it)->ani->need_update = true;
				(*it)->useable = nullptr;
				*it = nullptr;
			}
		}

		// usuñ jednostki które przenios³y siê na inny poziom
		RemoveNullElements(ctx.units);
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it)
				continue;

			if((*it)->cobj)
				delete (*it)->cobj->getCollisionShape();

			if((*it)->bow_instance)
				bow_instances.push_back((*it)->bow_instance);

			// speech bubble
			(*it)->bubble = nullptr;
			(*it)->talking = false;

			// zwolnij EntityInterpolator
			if((*it)->interp)
				interpolators.Free((*it)->interp);

			delete (*it)->ai;
			delete *it;
		}

		ctx.units->clear();
	}

	// usuñ tymczasowe obiekty
	DeleteElements(ctx.explos);
	DeleteElements(ctx.electros);
	ctx.drains->clear();
	ctx.bullets->clear();
	ctx.colliders->clear();
	
	// cz¹steczki
	DeleteElements(ctx.pes);
	DeleteElements(ctx.tpes);

	if(IsLocal())
	{
		// usuñ modele skrzyni
		if(ctx.chests)
		{
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			{
				delete (*it)->ani;
				(*it)->ani = nullptr;
			}
		}

		// usuñ modele drzwi
		if(ctx.doors)
		{
			for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
			{
				Door& door = **it;
				if(door.state == Door::Closing || door.state == Door::Closing2)
					door.state = Door::Closed;
				else if(door.state == Door::Opening || door.state == Door::Opening2)
					door.state = Door::Open;
				delete door.ani;
				door.ani = nullptr;
			}
		}
	}
	else
	{
		// usuñ obiekty
		if(ctx.chests)
			DeleteElements(ctx.chests);
		if(ctx.doors)
			DeleteElements(ctx.doors);
		if(ctx.traps)
			DeleteElements(ctx.traps);
		if(ctx.useables)
			DeleteElements(ctx.useables);
		if(ctx.items)
			DeleteElements(ctx.items);
	}

	// maksymalny rozmiar plamy krwi
	for(vector<Blood>::iterator it = ctx.bloods->begin(), end = ctx.bloods->end(); it != end; ++it)
		it->size = 1.f;
}

void Game::CreateBlood(LevelContext& ctx, const Unit& u, bool fully_created)
{
	if(!tKrewSlad[u.data->blood] || IS_SET(u.data->flags2, F2_BLOODLESS))
		return;

	Blood& b = Add1(ctx.bloods);
	if(u.human_data)
		u.ani->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.ani->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = random(MAX_ANGLE);
	b.size = (fully_created ? 1.f : 0.f);

	if(ctx.have_terrain)
	{
		b.pos.y = terrain->GetH(b.pos) + 0.05f;
		terrain->GetAngle(b.pos.x, b.pos.z, b.normal);
	}
	else
	{
		b.pos.y = u.pos.y+0.05f;
		b.normal = VEC3(0,1,0);
	}
}

void Game::WarpUnit(Unit& unit, const VEC3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	global_col.clear();
	LevelContext& ctx = GetContext(unit);
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&unit, nullptr};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, 2.f+unit_radius, &ignore);

	VEC3 tmp_pos = pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i=0; i<20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			unit.pos = tmp_pos;
			ok = true;
			break;
		}

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*radius;

		if(i < 10)
			radius += 0.25f;
	}

	assert(ok);

	if(ctx.have_terrain)
	{
		if(terrain->IsInside(unit.pos))
			terrain->SetH(unit.pos);
	}

	if(unit.cobj)
		UpdateUnitPhysics(unit, unit.pos);

	unit.visual_pos = unit.pos;

	if(IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(net_changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			GetPlayerInfo(unit.player->id).warping = true;
	}
}

void Game::ProcessUnitWarps()
{
	bool warped_to_arena = false;

	for(vector<UnitWarpData>::iterator it = unit_warp_data.begin(), end = unit_warp_data.end(); it != end; ++it)
	{
		if(it->where == -1)
		{
			if(city_ctx && it->unit->in_building != -1)
			{
				// powróæ na g³ówn¹ mapê
				InsideBuilding& building = *city_ctx->inside_buildings[it->unit->in_building];
				RemoveElement(building.units, it->unit);
				it->unit->in_building = -1;
				it->unit->rot = building.outside_rot;
				WarpUnit(*it->unit, building.outside_spawn);
				local_ctx.units->push_back(it->unit);
			}
			else
			{
				// jednostka opuœci³a podziemia
				if(it->unit->event_handler)
					it->unit->event_handler->HandleUnitEvent(UnitEventHandler::LEAVE, it->unit);
				it->unit->to_remove = true;
				to_remove.push_back(it->unit);
				if(IsOnline())
					Net_RemoveUnit(it->unit);
			}
		}
		else if(it->where == -2)
		{
			// na arene
			InsideBuilding& building = *GetArena();
			RemoveElement(GetContext(*it->unit).units, it->unit);
			it->unit->in_building = building.ctx.building_id;
			VEC3 pos;
			if(!WarpToArea(building.ctx, (it->unit->in_arena == 0 ? building.arena1 : building.arena2), it->unit->GetUnitRadius(), pos, 20))
			{
				// nie uda³o siê, wrzuæ go z areny
				it->unit->in_building = -1;
				it->unit->rot = building.outside_rot;
				WarpUnit(*it->unit, building.outside_spawn);
				local_ctx.units->push_back(it->unit);
				RemoveElement(at_arena, it->unit);
			}
			else
			{
				it->unit->rot = (it->unit->in_arena == 0 ? PI : 0);
				WarpUnit(*it->unit, pos);
				building.units.push_back(it->unit);
				warped_to_arena = true;
			}
		}
		else
		{
			// wejdŸ do budynku
			InsideBuilding& building = *city_ctx->inside_buildings[it->where];
			if(it->unit->in_building == -1)
				RemoveElement(local_ctx.units, it->unit);
			else
				RemoveElement(city_ctx->inside_buildings[it->unit->in_building]->units, it->unit);
			it->unit->in_building = it->where;
			it->unit->rot = PI;
			WarpUnit(*it->unit, building.inside_spawn);
			building.units.push_back(it->unit);
		}

		if(it->unit == pc->unit)
		{
			cam.Reset();
			player_rot_buf = 0.f;

			if(fallback_co == FALLBACK_ARENA)
			{
				pc->unit->frozen = 1;
				fallback_co = FALLBACK_ARENA2;
			}
			else if(fallback_co == FALLBACK_ARENA_EXIT)
			{
				pc->unit->frozen = 0;
				fallback_co = FALLBACK_NONE;
			}
		}
	}

	unit_warp_data.clear();

	if(warped_to_arena)
	{
		VEC3 pt1(0,0,0), pt2(0,0,0);
		int count1=0, count2=0;

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
		{
			if((*it)->in_arena == 0)
			{
				pt1 += (*it)->pos;
				++count1;
			}
			else
			{
				pt2 += (*it)->pos;
				++count2;
			}
		}

		pt1 /= (float)count1;
		pt2 /= (float)count2;

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			(*it)->rot = lookat_angle((*it)->pos, (*it)->in_arena == 0 ? pt2 : pt1);
	}
}

void Game::ApplyContext(ILevel* level, LevelContext& ctx)
{
	assert(level);

	level->ApplyContext(ctx);
	if(ctx.require_tmp_ctx && !ctx.tmp_ctx)
		ctx.SetTmpCtx(tmp_ctx_pool.Get());
}

void Game::UpdateContext(LevelContext& ctx, float dt)
{
	// aktualizuj postacie
	UpdateUnits(ctx, dt);

	if(ctx.lights && lights_dt >= 1.f/60)
		UpdateLights(*ctx.lights);

	// aktualizuj skrzynie
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			(*it)->ani->Update(dt);
	}

	// aktualizuj drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			door.ani->Update(dt);
			if(door.state == Door::Opening || door.state == Door::Opening2)
			{
				if(door.state == Door::Opening)
				{
					if(door.ani->frame_end_info || door.ani->GetProgress() >= 0.25f)
					{
						door.state = Door::Opening2;
						btVector3& pos = door.phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
					}
				}
				if(door.ani->frame_end_info)
					door.state = Door::Open;
			}
			else if(door.state == Door::Closing || door.state == Door::Closing2)
			{
				if(door.state == Door::Closing)
				{
					if(door.ani->frame_end_info || door.ani->GetProgress() <= 0.25f)
					{
						bool blocking = false;

						for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
						{
							if((*it)->IsAlive() && CircleToRotatedRectangle((*it)->pos.x, (*it)->pos.z, (*it)->GetUnitRadius(), door.pos.x, door.pos.z, 0.842f, 0.181f, door.rot))
							{
								blocking = true;
								break;
							}
						}

						if(!blocking)
						{
							door.state = Door::Closing2;
							btVector3& pos = door.phy->getWorldTransform().getOrigin();
							pos.setY(pos.y() + 100.f);
						}
					}
				}
				if(door.ani->frame_end_info)
				{
					if(door.state == Door::Closing2)
						door.state = Door::Closed;
					else
					{
						// nie mo¿na zamknaæ drzwi bo coœ blokuje
						door.state = Door::Opening2;
						door.ani->Play(&door.ani->ani->anims[0], PLAY_ONCE|PLAY_NO_BLEND|PLAY_STOP_AT_END, 0);
						door.ani->frame_end_info = false;
						// mo¿na by daæ lepszy punkt dŸwiêku
						if(sound_volume)
							PlaySound3d(sDoorBudge, door.pos, 2.f, 5.f);
					}
				}
			}
		}
	}

	// aktualizuj pu³apki
	if(ctx.traps && !ctx.traps->empty())
		UpdateTraps(ctx, dt);

	// aktualizuj pociski i efekty czarów
	UpdateBullets(ctx, dt);
	UpdateElectros(ctx, dt);
	UpdateExplosions(ctx, dt);
	UpdateParticles(ctx, dt);
	UpdateDrains(ctx, dt);

	// aktualizuj krew
	for(vector<Blood>::iterator it = ctx.bloods->begin(), end = ctx.bloods->end(); it != end; ++it)
	{
		it->size += dt;
		if(it->size >= 1.f)
			it->size = 1.f;
	}
}

SPAWN_GROUP Game::RandomSpawnGroup(const BaseLocation& base)
{
	int n = rand2()%100;
	int k = base.schance1;
	SPAWN_GROUP sg = SG_LOSOWO;
	if(base.schance1 > 0 && n < k)
		sg = base.sg1;
	else
	{
		k += base.schance2;
		if(base.schance2 > 0 && n < k)
			sg = base.sg2;
		else
		{
			k += base.schance3;
			if(base.schance3 > 0 && n < k)
				sg = base.sg3;
		}
	}

	if(sg == SG_LOSOWO)
	{
		switch(rand2()%10)
		{
		case 0:
		case 1:
			return SG_GOBLINY;
		case 2:
		case 3:
			return SG_ORKOWIE;
		case 4:
		case 5:
			return SG_BANDYCI;
		case 6:
			return SG_MAGOWIE;
		case 7:
			return SG_NEKRO;
		case 8:
			return SG_ZLO;
		case 9:
			return SG_BRAK;
		default:
			assert(0);
			return SG_BRAK;
		}
	}
	else
		return sg;
}

int Game::GetDungeonLevel()
{
	if(location->outside)
		return location->st;
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(inside->IsMultilevel())
			return (int)lerp(max(3.f, float(inside->st)/2), float(inside->st), float(dungeon_level)/(((MultiInsideLocation*)inside)->levels.size()-1));
		else
			return inside->st;
	}
}

int Game::GetDungeonLevelChest()
{
	int level = GetDungeonLevel();
	if(location->spawn == SG_BRAK)
	{
		if(level > 10)
			return 3;
		else if(level > 5)
			return 2;
		else
			return 1;
	}
	else
		return level;
}

void Game::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const VEC3& hitpoint, float range, bool dmg)
{
	if(sound_volume)
	{
		PlaySound3d(GetMaterialSound(mat2, mat), hitpoint, range, 10.f);

		if(mat != MAT_BODY && dmg)
			PlaySound3d(GetMaterialSound(mat2, MAT_BODY), hitpoint, range, 10.f);
	}

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::HIT_SOUND;
		c.pos = hitpoint;
		c.id = mat2;
		c.ile = mat;

		if(mat != MAT_BODY && dmg)
		{
			NetChange& c2 = Add1(net_changes);
			c2.type = NetChange::HIT_SOUND;
			c2.pos = hitpoint;
			c2.id = mat2;
			c2.ile = MAT_BODY; 
		}
	}
}

void Game::LoadingStart(int steps)
{
	load_screen->Reset();
	loading_t.Reset();
	loading_dt = 0.f;
	loading_steps = steps;
	loading_index = 0;
	clear_color = BLACK;
	game_state = GS_LOAD;
	load_screen->visible = true;
	main_menu->visible = false;
	game_gui->visible = false;
}

void Game::LoadingStep(cstring text)
{
	float progress = float(loading_index) / loading_steps;
	if(text)
		load_screen->SetProgress(progress, text);
	else
		load_screen->SetProgress(progress);

	++loading_index;
	loading_dt += loading_t.Tick();
	if(loading_dt >= 1.f/30 || loading_index == loading_steps)
	{
		loading_dt = 0.f;
		DoPseudotick();
		loading_t.Tick();
	}
}

cstring arena_slabi[] = {
	"orcs",
	"goblins",
	"undead",
	"bandits",
	"cave_wolfs",
	"cave_spiders"
};

cstring arena_sredni[] = {
	"orcs",
	"goblins",
	"undead",
	"evils",
	"bandits",
	"mages",
	"golems",
	"mages_n_golems",
	"hunters"
};

cstring arena_silni[] = {
	"orcs",
	"necros",
	"evils",
	"mages",
	"golems",
	"mages_n_golems",
	"crazies"
};

void Game::StartArenaCombat(int level)
{
	assert(in_range(level, 1, 3));

	int ile, lvl;

	switch(rand2()%5)
	{
	case 0:
		ile = 1;
		lvl = level*5+1;
		break;
	case 1:
		ile = 1;
		lvl = level*5;
		break;
	case 2:
		ile = 2;
		lvl = level*5-1;
		break;
	case 3:
		ile = 2;
		lvl = level*5-2;
		break;
	case 4:
		ile = 3;
		lvl = level*5-3;
		break;
	}

	arena_free = false;
	arena_tryb = Arena_Walka;
	arena_etap = Arena_OdliczanieDoPrzeniesienia;
	arena_t = 0.f;
	arena_poziom = level;
	city_ctx->arena_time = worldtime;
	at_arena.clear();

	// dodaj gracza na arenê
	if(current_dialog->is_local)
	{
		fallback_co = FALLBACK_ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ENTER_ARENA;
		c.pc = current_dialog->pc;
		current_dialog->pc->arena_fights++;
		GetPlayerInfo(current_dialog->pc).NeedUpdate();
	}

	current_dialog->pc->unit->frozen = 2;
	current_dialog->pc->unit->in_arena = 0;
	at_arena.push_back(current_dialog->pc->unit);

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHANGE_ARENA_STATE;
		c.unit = current_dialog->pc->unit;
	}

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->frozen || distance2d((*it)->pos, city_ctx->arena_pos) > 5.f)
			continue;
		if((*it)->IsPlayer())
		{
			if((*it)->frozen == 0)
			{
				BreakAction(**it, false, true);

				(*it)->frozen = 2;
				(*it)->in_arena = 0;
				at_arena.push_back(*it);

				(*it)->player->arena_fights++;
				if(IsOnline())
					(*it)->player->stat_flags |= STAT_ARENA_FIGHTS;

				if((*it)->player == pc)
				{
					fallback_co = FALLBACK_ARENA;
					fallback_t = -1.f;
				}
				else
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::ENTER_ARENA;
					c.pc = (*it)->player;
					GetPlayerInfo(c.pc).NeedUpdate();
				}

				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = *it;
			}
		}
		else if((*it)->IsHero() && (*it)->CanFollow())
		{
			(*it)->frozen = 2;
			(*it)->in_arena = 0;
			(*it)->hero->following = current_dialog->pc->unit;
			at_arena.push_back(*it);

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = *it;
			}
		}
	}

	if(at_arena.size() > 3)
	{
		lvl += (at_arena.size()-3)/2+1;
		while(lvl > level*5+2)
		{
			lvl -= 2;
			++ile;
		}
	}

	cstring grupa;
	switch(level)
	{
	default:
	case 1:
		grupa = arena_slabi[rand2()%countof(arena_slabi)];
		SpawnArenaViewers(1);
		break;
	case 2:
		grupa = arena_sredni[rand2()%countof(arena_sredni)];
		SpawnArenaViewers(3);
		break;
	case 3:
		grupa = arena_silni[rand2()%countof(arena_silni)];
		SpawnArenaViewers(5);
		break;
	}

	TmpUnitGroup part;
	part.group = FindUnitGroup(grupa);
	part.total = 0;
	for(auto& entry : part.group->entries)
	{
		if(lvl >= entry.ud->level.x)
		{
			auto& new_entry = Add1(part.entries);
			new_entry.ud = entry.ud;
			new_entry.count = entry.count;
			if(lvl < entry.ud->level.y)
				new_entry.count = max(1, new_entry.count/2);
			part.total += new_entry.count;
		}
	}

	InsideBuilding* arena = GetArena();

	for(int i=0; i<ile; ++i)
	{
		int x = rand2()%part.total, y = 0;
		for(auto& entry : part.entries)
		{
			y += entry.count;
			if(x < y)
			{
				Unit* u = SpawnUnitInsideArea(arena->ctx, arena->arena2, *entry.ud, lvl);
				u->rot = 0.f;
				u->in_arena = 1;
				u->frozen = 2;
				at_arena.push_back(u);

				if(IsOnline())
					Net_SpawnUnit(u);

				break;
			}
		}
	}
}

Unit* Game::SpawnUnitInsideArea(LevelContext& ctx, const BOX2D& area, UnitData& unit, int level)
{
	VEC3 pos;

	if(!WarpToArea(ctx, area, unit.GetRadius(), pos))
		return nullptr;

	return CreateUnitWithAI(ctx, unit, level, nullptr, &pos);
}

bool Game::WarpToArea(LevelContext& ctx, const BOX2D& area, float radius, VEC3& pos, int tries)
{
	for(int i=0; i<tries; ++i)
	{
		pos = area.GetRandomPos3();

		global_col.clear();
		GatherCollisionObjects(ctx, global_col, pos, radius, nullptr);

		if(!Collide(global_col, pos, radius))
			return true;
	}

	return false;
}

void Game::DeleteUnit(Unit* unit)
{
	assert(unit);

	RemoveElement(GetContext(*unit).units, unit);
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
	{
		if(it->unit == unit)
		{
			unit_views.erase(it);
			break;
		}
	}
	if(before_player == BP_UNIT && before_player_ptr.unit == unit)
		before_player = BP_NONE;
	if(unit == selected_target)
		selected_target = nullptr;
	if(unit == selected_unit)
		selected_unit = nullptr;

	if(unit->bubble)
		unit->bubble->unit = nullptr;

	if(unit->bow_instance)
		bow_instances.push_back(unit->bow_instance);

	if(unit->ai)
	{
		RemoveElement(ais, unit->ai);
		delete unit->ai;
	}
	delete unit->cobj->getCollisionShape();
	phy_world->removeCollisionObject(unit->cobj);
	delete unit->cobj;
	delete unit;
}

void Game::DialogTalk(DialogContext& ctx, cstring msg)
{
	assert(msg);

	ctx.dialog_text = msg;
	ctx.dialog_wait = 1.f+float(strlen(ctx.dialog_text))/20;

	int ani;
	if(!ctx.talker->useable && ctx.talker->type == Unit::HUMAN && rand2()%3 != 0)
	{
		ani = rand2()%2+1;
		ctx.talker->ani->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
		ctx.talker->animation = ANI_PLAY;
		ctx.talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	game_gui->AddSpeechBubble(ctx.talker, ctx.dialog_text);

	ctx.pc->Train(TrainWhat::Talk, 0.f, 0);

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::TALK;
		c.unit = ctx.talker;
		c.str = StringPool.Get();
		*c.str = msg;
		c.id = ani;
		c.ile = skip_id_counter++;
		ctx.skip_id = c.ile;
		net_talk.push_back(c.str);
	}
}

void Game::CreateForestMinimap()
{
	D3DLOCKED_RECT lock;
	tMinimap->LockRect(0, &lock, nullptr, 0);

	OutsideLocation* loc = (OutsideLocation*)location;

	for(int y=0; y<OutsideLocation::size; ++y)
	{
		DWORD* pix = (DWORD*)(((byte*)lock.pBits)+lock.Pitch*y);
		for(int x=0; x<OutsideLocation::size; ++x)
		{
			TERRAIN_TILE t = loc->tiles[x+(OutsideLocation::size-1-y)*OutsideLocation::size].t;
			DWORD col;
			if(t == TT_GRASS)
				col = COLOR_RGB(0,128,0);
			else if(t == TT_ROAD)
				col = COLOR_RGB(128,128,128);
			else if(t == TT_SAND)
				col = COLOR_RGB(128,128,64);
			else if(t == TT_GRASS2)
				col = COLOR_RGB(105,128,89);
			else if(t == TT_GRASS3)
				col = COLOR_RGB(127,51,0);
			else
				col = COLOR_RGB(255,0,0);
			if(x < 16 || x > 128-16 || y < 16 || y > 128-16)
			{
				col = ((col & 0xFF) / 2) |
					  ((((col & 0xFF00) >> 8) / 2) << 8) |
					  ((((col & 0xFF0000) >> 16) / 2) << 16) |
					  0xFF000000;
			}

			*pix = col;
			++pix;
		}
	}

	tMinimap->UnlockRect(0);

	game_gui->minimap->minimap_size = OutsideLocation::size;
}

void Game::SpawnOutsideBariers()
{
	const float size = 256.f;
	const float size2 = size/2;
	const float border = 32.f;
	const float border2 = border/2;

	// góra
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size2, border2);
		cobj.w = size2;
		cobj.h = border2;
	}

	// dó³
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size2, size-border2);
		cobj.w = size2;
		cobj.h = border2;
	}

	// lewa
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(border2, size2);
		cobj.w = border2;
		cobj.h = size2;
	}

	// prawa
	{
		CollisionObject& cobj = Add1(local_ctx.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pt = VEC2(size-border2, size2);
		cobj.w = border2;
		cobj.h = size2;
	}
}

bool Game::HaveTeamMember()
{
	return GetActiveTeamSize() > 1;
}

bool Game::HaveTeamMemberNPC()
{
	if(GetActiveTeamSize() < 2)
		return false;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if(!(*it)->IsPlayer())
			return true;
	}
	return false;
}

bool Game::HaveTeamMemberPC()
{
	if(!IsOnline())
		return false;
	if(GetActiveTeamSize() < 2)
		return false;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && *it != pc->unit)
			return true;
	}
	return false;
}

bool Game::IsTeamMember(Unit& unit)
{
	if(unit.IsPlayer())
		return true;
	else if(unit.IsHero())
		return unit.hero->team_member;
	else
		return false;
}

inline int& GetCredit(Unit& u)
{
	if(u.IsPlayer())
		return u.player->credit;
	else
	{
		assert(u.IsFollower());
		return u.hero->credit;
	}
}

int Game::GetPCShare()
{
	int ile_pc = 0, ile_npc = 0;
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer())
			++ile_pc;
		else if(!(*it)->hero->free)
			++ile_npc;
	}
	int r = 100 - ile_npc*10;
	return r/ile_pc;
}

int Game::GetPCShare(int ile_pc, int ile_npc)
{
	int r = 100 - ile_npc*10;
	return r/ile_pc;
}

VEC2 Game::GetMapPosition(Unit& unit)
{
	if(unit.in_building == -1)
		return VEC2(unit.pos.x, unit.pos.z);
	else
	{
		Building* type = city_ctx->inside_buildings[unit.in_building]->type;
		for(CityBuilding& b : city_ctx->buildings)
		{
			if(b.type == type)
				return VEC2(float(b.pt.x*2), float(b.pt.y*2));
		}
	}
	assert(0);
	return VEC2(-1000,-1000);
}

//=============================================================================
// Rozdziela z³oto pomiêdzy cz³onków dru¿yny
//=============================================================================
void Game::AddGold(int ile, vector<Unit*>* units, bool show, cstring msg, float time, bool defmsg)
{
	if(!units)
		units = &active_team;

	// reszta z poprzednich podzia³ów, dodaje siê tylko jak jest wiêksza ni¿ 10 bo inaczej npc nic nie dostaje przy ma³ych kwotach
	int a = team_gold/10;
	if(a)
	{
		ile += a*10;
		team_gold -= a*10;
	}

	if(units->size() == 1)
	{
		Unit& u = *(*units)[0];
		u.gold += ile;
		if(show && u.IsPlayer())
		{
			if(&u == pc->unit)
				AddGameMsg(Format(msg, ile), time);
			else
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::GOLD_MSG;
				c.id = (defmsg ? 1 : 0);
				c.ile = ile;
				c.pc = u.player;
				GetPlayerInfo(u.player->id).update_flags |= (PlayerInfo::UF_NET_CHANGES | PlayerInfo::UF_GOLD);
			}
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != pc->unit)
			{
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.pc = trader->player;
				c.id = u.netid;
				c.ile = u.gold;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
		return;
	}

	int kasa = 0, ile_pc = 0, ile_npc = 0;
	bool credit_info = false;

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
		{
			++ile_pc;
			u.player->on_credit = false;
			u.player->gold_get = 0;
		}
		else
		{
			++ile_npc;
			u.hero->on_credit = false;
			u.hero->gained_gold = false;
		}
	}

	for(int i=0; i<2 && ile > 0; ++i)
	{
		int pc_share, npc_share;
		if(ile_pc > 0)
		{
			pc_share = GetPCShare(ile_pc, ile_npc),
			npc_share = 10;
		}
		else
			npc_share = 100/ile_npc;

		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			int& credit = (u.IsPlayer() ? u.player->credit : u.hero->credit);
			bool& on_credit = (u.IsPlayer() ? u.player->on_credit : u.hero->on_credit);
			if(on_credit)
				continue;

			int zysk = ile * (u.IsHero() ? npc_share : pc_share) / 100;
			if(credit > zysk)
			{
				credit_info = true;
				credit -= zysk;
				on_credit = true;
				if(u.IsPlayer())
					--ile_pc;
				else
					--ile_npc;
			}
			else if(credit)
			{
				credit_info = true;
				zysk -= credit;
				credit = 0;
				u.gold += zysk;
				kasa += zysk;
				if(u.IsPlayer())
					u.player->gold_get += zysk;
				else
					u.hero->gained_gold = true;
			}
			else
			{
				u.gold += zysk;
				kasa += zysk;
				if(u.IsPlayer())
					u.player->gold_get += zysk;
				else
					u.hero->gained_gold = true;
			}
		}

		ile -= kasa;
		kasa = 0;
	}

	team_gold += ile;
	assert(team_gold >= 0);

	if(IsOnline())
	{
		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsPlayer())
			{
				if(u.player != pc)
				{
					if(u.player->gold_get)
					{
						GetPlayerInfo(u.player->id).update_flags |= PlayerInfo::UF_GOLD;
						if(show)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::GOLD_MSG;
							c.id = (defmsg ? 1 : 0);
							c.ile = u.player->gold_get;
							c.pc = u.player;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
				else
				{
					if(show)
						AddGameMsg(Format(msg, pc->gold_get), time);
				}
			}
			else if(u.hero->gained_gold && u.busy == Unit::Busy_Trading)
			{
				Unit* trader = FindPlayerTradingWithUnit(u);
				if(trader != pc->unit)
				{
					NetChangePlayer& c = Add1(net_changes_player);
					c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
					c.pc = trader->player;
					c.id = u.netid;
					c.ile = u.gold;
					GetPlayerInfo(c.pc).NeedUpdate();
				}
			}
		}

		if(credit_info)
			PushNetChange(NetChange::UPDATE_CREDIT);
	}
	else if(show)
		AddGameMsg(Format(msg, pc->gold_get), time);
}

int Game::CalculateQuestReward(int gold)
{
	return gold * (90 + active_team.size()*10) / 100;
}

void Game::AddGoldArena(int ile)
{
	vector<Unit*> v;
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->in_arena == 0)
			v.push_back(*it);
	}

	AddGold(ile * (85 + v.size()*15) / 100, &v, true);
}

void Game::EventTakeItem(int id)
{
	if(id == BUTTON_YES)
	{
		ItemSlot& slot = pc->unit->items[take_item_id];
		pc->credit += slot.item->value/2;
		slot.team_count = 0;

		if(IsLocal())
			CheckCredit(true);
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::TAKE_ITEM_CREDIT;
			c.id = take_item_id;
		}
	}
}

bool Game::IsBetterItem(Unit& unit, const Item* item, int* value)
{
	assert(item && item->IsWearable());

	switch(item->type)
	{
	case IT_WEAPON:
		if(!IS_SET(unit.data->flags, F_MAGE))
			return unit.IsBetterWeapon(item->ToWeapon(), value);
		else 
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > unit.GetWeapon().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_BOW:
		if(!unit.HaveBow())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else
		{
			if(unit.GetBow().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_ARMOR:
		if(!IS_SET(unit.data->flags, F_MAGE))
			return unit.IsBetterArmor(item->ToArmor(), value);
		else
		{
			if(IS_SET(item->flags, ITEM_MAGE) && item->value > unit.GetArmor().value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	case IT_SHIELD:
		if(!unit.HaveShield())
		{
			if(value)
				*value = item->value;
			return true;
		}
		else
		{
			if(unit.GetShield().value < item->value)
			{
				if(value)
					*value = item->value;
				return true;
			}
			else
				return false;
		}
	default:
		assert(0);
		return false;
	}
}

const Item* Game::GetBetterItem(const Item* item)
{
	assert(item);

	auto it = better_items.find(item);
	if(it != better_items.end())
		return it->second;

	return nullptr;
}

void Game::CheckIfLocationCleared()
{
	if(city_ctx)
		return;

	bool czysto = true;
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->IsAlive() && IsEnemy(*pc->unit, **it, true))
		{
			czysto = false;
			break;
		}
	}

	if(czysto)
	{
		bool cleared = false;
		if(!location->outside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
			{
				if(((MultiInsideLocation*)inside)->LevelCleared())
					cleared = true;
			}
			else
				cleared = true;
		}
		else
			cleared = true;

		if(cleared)
		{
			location->state = LS_CLEARED;
			if(location->spawn != SG_BRAK)
			{
				if(location->type == L_CAMP)
					AddNews(Format(txNewsCampCleared, locations[GetNearestSettlement(location->pos)]->name.c_str()));
				else
					AddNews(Format(txNewsLocCleared, location->name.c_str()));
			}
		}

		if(location_event_handler)
			location_event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);
	}
}

void Game::SpawnArenaViewers(int count)
{
	assert(in_range(count, 1, 9));

	vector<Animesh::Point*> points;
	UnitData& ud = *FindUnitData("viewer");
	InsideBuilding* arena = GetArena();
	Animesh* mesh = arena->type->inside_mesh;

	for(vector<Animesh::Point>::iterator it = mesh->attach_points.begin(), end = mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "o_s_viewer_", 11) == 0)
			points.push_back(&*it);
	}

	while(count > 0)
	{
		int id = rand2()%points.size();
		Animesh::Point* pt = points[id];
		points.erase(points.begin()+id);
		VEC3 pos(pt->mat._41+arena->offset.x, pt->mat._42, pt->mat._43+arena->offset.y);
		VEC3 look_at(arena->offset.x, 0, arena->offset.y);
		Unit* u = SpawnUnitNearLocation(arena->ctx, pos, ud, &look_at, -1, 2.f);
		if(u && IsOnline())
			Net_SpawnUnit(u);
		--count;
		u->ai->loc_timer = random(6.f,12.f);
		arena_viewers.push_back(u);
	}
}

void Game::RemoveArenaViewers()
{
	UnitData* ud = FindUnitData("viewer");
	LevelContext& ctx = GetArena()->ctx;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
		{
			(*it)->to_remove = true;
			to_remove.push_back(*it);
			if(IsOnline())
				Net_RemoveUnit(*it);
		}
	}
}

float Game::PlayerAngleY()
{
	//const float c_cam_angle_min = PI+0.1f;
	//const float c_cam_angle_max = PI*1.8f-0.1f;
	const float pt0 = 4.6662526f;

	/*if(rotY < pt0)
	{
		if(rotY < pt0-0.6f)
			return -1.f;
		else
			return lerp(0.f, -1.f, (pt0 - rotY)/0.6f);
	}
	else
	{
		if(rotY > pt0+0.6f)
			return 1.f;
		else
			return lerp(0.f, 1.f, (rotY-pt0)/0.6f);
	}*/

	/*if(rotY < c_cam_angle_min+range/4)
		return -1.f;
	else if(rotY > c_cam_angle_max-range/4)
		return 1.f;
	else
		return lerp(-1.f, 1.f, (rotY - (c_cam_angle_min+range/4)) / (range/2));*/

// 	if(rotY < pt0)
// 	{
// 		return rotY - pt0;
// 	}
	return cam.rot.y - pt0;
}

VEC3 Game::GetExitPos(Unit& u, bool force_border)
{
	const VEC3& pos = u.pos;

	if(location->outside)
	{
		if(u.in_building != -1)
			return VEC3_x0y(city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint());
		else if(city_ctx && !force_border)
		{
			float best_dist, dist;
			int best_index = -1, index = 0;

			for(vector<EntryPoint>::const_iterator it = city_ctx->entry_points.begin(), end = city_ctx->entry_points.end(); it != end; ++it, ++index)
			{
				if(it->exit_area.IsInside(u.pos))
				{
					// unit is already inside exit area, goto outside exit
					best_index = -1;
					break;
				}
				else
				{
					dist = distance(VEC2(u.pos.x, u.pos.z), it->exit_area.Midpoint());
					if(best_index == -1 || dist < best_dist)
					{
						best_dist = dist;
						best_index = index;
					}
				}
			}

			if(best_index != -1)
				return VEC3_x0y(city_ctx->entry_points[best_index].exit_area.Midpoint());
		}

		int best = 0;
		float dist, best_dist;

		// lewa krawêdŸ
		best_dist = abs(pos.x - 32.f);

		// prawa krawêdŸ
		dist = abs(pos.x - (256.f-32.f));
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 1;
		}

		// górna krawêdŸ
		dist = abs(pos.z - 32.f);
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 2;
		}

		// dolna krawêdŸ
		dist = abs(pos.z - (256.f-32.f));
		if(dist < best_dist)
			best = 3;

		switch(best)
		{
		default:
			assert(0);
		case 0:
			return VEC3(32.f,0,pos.z);
		case 1:
			return VEC3(256.f-32.f,0,pos.z);
		case 2:
			return VEC3(pos.x,0,32.f);
		case 3:
			return VEC3(pos.x,0,256.f-32.f);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(dungeon_level == 0 && inside->from_portal)
			return inside->portal->pos;
		INT2& pt = inside->GetLevelData().staircase_up;
		return VEC3(2.f*pt.x+1,0,2.f*pt.y+1);
	}
}

void Game::AttackReaction(Unit& attacked, Unit& attacker)
{
	if(attacker.IsPlayer() && attacked.IsAI() && attacked.in_arena == -1 && !attacked.attack_team)
	{
		if(attacked.data->group == G_CITIZENS)
		{
			if(!bandyta)
			{
				bandyta = true;
				if(IsOnline())
					PushNetChange(NetChange::CHANGE_FLAGS);
			}
		}
		else if(attacked.data->group == G_CRAZIES)
		{
			if(!atak_szalencow)
			{
				atak_szalencow = true;
				if(IsOnline())
					PushNetChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(attacked.dont_attack)
		{
			for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->dont_attack)
				{
					(*it)->dont_attack = false;
					(*it)->ai->change_ai_mode = true;
				}
			}
		}
	}
}

int Game::CanLeaveLocation(Unit& unit)
{
	if(secret_state == SECRET_FIGHT)
		return false;

	if(city_ctx)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
				return 1;

			if(u.IsPlayer())
			{
				if(u.in_building != -1 || distance2d(unit.pos, u.pos) > 8.f)
					return 1;
			}

			for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && distance2d(u.pos, u2.pos) < ALERT_RANGE.x && CanSee(u, u2))
					return 2;
			}
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.busy != Unit::Busy_No || distance2d(unit.pos, u.pos) > 8.f)
				return 1;

			for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && distance2d(u.pos, u2.pos) < ALERT_RANGE.x && CanSee(u, u2))
					return 2;
			}
		}
	}

	return 0;
}

void Game::GenerateTraps()
{
	InsideLocation* inside = (InsideLocation*)location;
	BaseLocation& base = g_base_locations[inside->target];

	if(!IS_SET(base.traps, TRAPS_NORMAL|TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = inside->GetLevelData();

	int szansa;
	INT2 pt(-1000,-1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 10;
		pt = lvl.staircase_up;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size-1)
					szansa = 25;
				else if(dungeon_level == size-2)
					szansa = 15;
				else if(dungeon_level == size-3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<TRAP_TYPE> traps;
	if(IS_SET(base.traps, TRAPS_NORMAL))
	{
		traps.push_back(TRAP_ARROW);
		traps.push_back(TRAP_POISON);
		traps.push_back(TRAP_SPEAR);
	}
	if(IS_SET(base.traps, TRAPS_MAGIC))
		traps.push_back(TRAP_FIREBALL);

	for(int y=1; y<lvl.h-1; ++y)
	{
		for(int x=1; x<lvl.w-1; ++x)
		{
			if(lvl.map[x + y*lvl.w].type == PUSTE
				&& !OR2_EQ(lvl.map[x - 1 + y*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + 1 + y*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA))
			{
				if(rand2()%500 < szansa + max(0, 30-distance(pt, INT2(x,y))))
					CreateTrap(INT2(x,y), traps[rand2()%traps.size()]);
			}
		}
	}
}

void Game::RegenerateTraps()
{
	InsideLocation* inside = (InsideLocation*)location;
	BaseLocation& base = g_base_locations[inside->target];

	if(!IS_SET(base.traps, TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = inside->GetLevelData();

	int szansa;
	INT2 pt(-1000,-1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 0;
		pt = lvl.staircase_up;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size-1)
					szansa = 25;
				else if(dungeon_level == size-2)
					szansa = 15;
				else if(dungeon_level == size-3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<Trap*>& traps = *local_ctx.traps;
	int id = 0, topid = traps.size();

	for(int y=1; y<lvl.h-1; ++y)
	{
		for(int x=1; x<lvl.w-1; ++x)
		{
			if(lvl.map[x + y*lvl.w].type == PUSTE
				&& !OR2_EQ(lvl.map[x - 1 + y*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + 1 + y*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA))
			{
				int s = szansa + max(0, 30-distance(pt, INT2(x,y)));
				if(IS_SET(base.traps, TRAPS_NORMAL))
					s /= 4;
				if(rand2()%500 < s)
				{
					bool ok = false;
					if(id == -1)
						ok = true;
					else if(id == topid)
					{
						id = -1;
						ok = true;
					}
					else
					{
						while(id != topid)
						{
							if(traps[id]->tile.y > y || (traps[id]->tile.y == y && traps[id]->tile.x > x))
							{
								ok = true;
								break;
							}
							else if(traps[id]->tile.x == x && traps[id]->tile.y == y)
							{
								++id;
								break;
							}
							else
								++id;
						}
					}

					if(ok)
						CreateTrap(INT2(x,y), TRAP_FIREBALL);
				}
			}
		}
	}

	if(devmode)
		LOG(Format("Traps: %d", local_ctx.traps->size()));
}

bool SortItemsByValue(const ItemSlot& a, const ItemSlot& b)
{
	return a.item->GetWeightValue() < b.item->GetWeightValue();
}

void Game::SpawnHeroesInsideDungeon()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	Room* p = lvl.GetUpStairsRoom();
	int room_id = lvl.GetRoomId(p);
	int szansa = 23;

	vector<std::pair<Room*, int> > sprawdzone;
	vector<int> ok_room;
	sprawdzone.push_back(std::make_pair(p, room_id));

	while(true)
	{
		p = sprawdzone.back().first;
		for(vector<int>::iterator it = p->connected.begin(), end = p->connected.end(); it != end; ++it)
		{
			room_id = *it;
			bool ok = true;
			for(vector<std::pair<Room*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
			{
				if(room_id == it2->second)
				{
					ok = false;
					break;
				}
			}
			if(ok && (rand2()%20 < szansa || rand2()%3 == 0))
				ok_room.push_back(room_id);
		}

		if(ok_room.empty())
			break;
		else
		{
			room_id = ok_room[rand2()%ok_room.size()];
			ok_room.clear();
			sprawdzone.push_back(std::make_pair(&lvl.rooms[room_id], room_id));
			--szansa;
		}
	}

	// cofnij ich z korytarza
	while(sprawdzone.back().first->IsCorridor())
		sprawdzone.pop_back();

	int gold = 0;
	vector<ItemSlot> items;
	LocalVector<Chest*> chests;

	// pozabijaj jednostki w pokojach, ograb skrzynie
	// trochê to nieefektywne :/
	vector<std::pair<Room*, int> >::iterator end = sprawdzone.end();
	if(rand2()%2 == 0)
		--end;
	for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && IsEnemy(*pc->unit, u))
		{
			for(vector<std::pair<Room*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
			{
				if(it->first->IsInside(u.pos))
				{
					gold += u.gold;
					for(int i=0; i<SLOT_MAX; ++i)
					{
						if(u.slots[i] && u.slots[i]->GetWeightValue() >= 5.f)
						{
							ItemSlot& slot = Add1(items);
							slot.item = u.slots[i];
							slot.count = slot.team_count = 1u;
							u.weight -= u.slots[i]->weight;
							u.slots[i] = nullptr;
						}
					}
					for(vector<ItemSlot>::iterator it3 = u.items.begin(), end3 = u.items.end(); it3 != end3;)
					{
						if(it3->item->GetWeightValue() >= 5.f)
						{
							u.weight -= it3->item->weight * it3->count;
							items.push_back(*it3);
							it3 = u.items.erase(it3);
							end3 = u.items.end();
						}
						else
							++it3;
					}
					u.gold = 0;
					u.live_state = Unit::DEAD;
					u.animation = u.current_animation = ANI_DIE;
					u.SetAnimationAtEnd(NAMES::ani_die);
					u.hp = 0.f;
					CreateBlood(local_ctx, u, true);
					++total_kills;

					// przenieœ fizyke
					btVector3 a_min, a_max;
					u.cobj->getWorldTransform().setOrigin(btVector3(1000,1000,1000));
					u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
					phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);

					if(u.event_handler)
						u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);
					break;
				}
			}
		}
	}
	for(vector<Chest*>::iterator it2 = local_ctx.chests->begin(), end2 = local_ctx.chests->end(); it2 != end2; ++it2)
	{
		for(vector<std::pair<Room*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
		{
			if(it->first->IsInside((*it2)->pos))
			{
				for(vector<ItemSlot>::iterator it3 = (*it2)->items.begin(), end3 = (*it2)->items.end(); it3 != end3;)
				{
					if(it3->item->type == IT_GOLD)
					{
						gold += it3->count;
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else if(it3->item->GetWeightValue() >= 5.f)
					{
						items.push_back(*it3);
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else
						++it3;
				}
				chests->push_back(*it2);
				break;
			}
		}
	}

	// otwórz drzwi pomiêdzy obszarami
	for(vector<std::pair<Room*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
	{
		Room& a = *it2->first,
			&b = lvl.rooms[it2->second];

		// wspólny obszar pomiêdzy pokojami
		int x1 = max(a.pos.x,b.pos.x),
			x2 = min(a.pos.x+a.size.x,b.pos.x+b.size.x),
			y1 = max(a.pos.y,b.pos.y),
			y2 = min(a.pos.y+a.size.y,b.pos.y+b.size.y);

		// szukaj drzwi
		for(int y=y1; y<y2; ++y)
		{
			for(int x=x1; x<x2; ++x)
			{
				Pole& po = lvl.map[x+y*lvl.w];
				if(po.type == DRZWI)
				{
					Door* door = lvl.FindDoor(INT2(x,y));					
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->ani->SetToEnd(&door->ani->ani->anims[0]);
					}
				}
			}
		}
	}

	// stwórz bohaterów
	int ile = random(2,4);
	LocalVector<Unit*> heroes;
	p = sprawdzone.back().first;
	for(int i=0; i<ile; ++i)
	{
		Unit* u = SpawnUnitInsideRoom(*p, GetHero(ClassInfo::GetRandom()), random(2, 15));
		if(u)
			heroes->push_back(u);
		else
			break;
	}

	// sortuj przedmioty wed³ug wartoœci
	std::sort(items.begin(), items.end(), SortItemsByValue);

	// rozdziel z³oto
	int gold_per_hero = gold/heroes->size();
	for(vector<Unit*>::iterator it = heroes->begin(), end = heroes->end(); it != end; ++it)
		(*it)->gold += gold_per_hero;
	gold -= gold_per_hero*heroes->size();
	if(gold)
		heroes->back()->gold += gold;

	// rozdziel przedmioty
	vector<Unit*>::iterator heroes_it = heroes->begin(), heroes_end = heroes->end();
	while(!items.empty())
	{
		ItemSlot& item = items.back();
		for(int i=0, ile=item.count; i<ile; ++i)
		{
			if((*heroes_it)->CanTake(item.item))
			{
				(*heroes_it)->AddItemAndEquipIfNone(item.item);
				--item.count;
				++heroes_it;
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
			else
			{
				// ten heros nie mo¿e wzi¹œæ tego przedmiotu, jest szansa ¿e wzi¹³by jakiœ l¿ejszy i tañszy ale ma³a
				heroes_it = heroes->erase(heroes_it);
				if(heroes->empty())
					break;
				heroes_end = heroes->end();
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
		}
		if(heroes->empty())
			break;
		items.pop_back();
	}
	
	// pozosta³e przedmioty schowaj do skrzyni o ile jest co i gdzie
	if(!chests->empty() && !items.empty())
	{
		chests.Shuffle();
		vector<Chest*>::iterator chest_begin = chests->begin(), chest_end = chests->end(), chest_it = chest_begin;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			(*chest_it)->AddItem(it->item, it->count);
			++chest_it;
			if(chest_it == chest_end)
				chest_it = chest_begin;
		}
	}

	// sprawdŸ czy lokacja oczyszczona (raczej nie jest)
	CheckIfLocationCleared();
}

GroundItem* Game::SpawnGroundItemInsideRoom(Room& room, const Item* item)
{
	assert(item);

	for(int i=0; i<50; ++i)
	{
		VEC3 pos = room.GetRandomPos(0.5f);
		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pos, 0.25f);
		if(!Collide(global_col, pos, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = random(MAX_ANGLE);
			gi->pos = pos;
			gi->item = item;
			gi->netid = item_netid_counter++;
			local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideRadius(const Item* item, const VEC2& pos, float radius, bool try_exact)
{
	assert(item);

	VEC3 pt;
	for(int i=0; i<50; ++i)
	{
		if(!try_exact)
		{
			float a = random(), b = random();
			if(b < a)
				std::swap(a, b);
			pt = VEC3(pos.x+b*radius*cos(2*PI*a/b), 0, pos.y+b*radius*sin(2*PI*a/b));
		}
		else
		{
			try_exact = false;
			pt = VEC3(pos.x,0,pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = random(MAX_ANGLE);
			gi->pos = pt;
			gi->item = item;
			gi->netid = item_netid_counter++;
			local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideRegion(const Item* item, const VEC2& pos, const VEC2& region_size, bool try_exact)
{
	assert(item);
	assert(region_size.x > 0 && region_size.y > 0);

	VEC3 pt;
	for(int i = 0; i<50; ++i)
	{
		if(!try_exact)
			pt = VEC3(pos.x + random(region_size.x), 0, pos.y + random(region_size.y));
		else
		{
			try_exact = false;
			pt = VEC3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = random(MAX_ANGLE);
			gi->pos = pt;
			gi->item = item;
			gi->netid = item_netid_counter++;
			local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

int Game::GetRandomSettlement(const vector<int>& used, int type) const
{
	int id = rand2() % settlements;
	
loop:
	if(type != 0)
	{
		City* city = (City*)locations[id];
		if(type == 1)
		{
			if(city->settlement_type == City::SettlementType::Village)
			{
				id = (id+1) % settlements;
				goto loop;
			}
		}
		else
		{
			if(city->settlement_type == City::SettlementType::City)
			{
				id = (id+1) % settlements;
				goto loop;
			}
		}
	}

	for(vector<int>::const_iterator it = used.begin(), end = used.end(); it != end; ++it)
	{
		if(*it == id)
		{
			id = (id+1) % settlements;
			goto loop;
		}
	}
	
	return id;
}

void Game::InitQuests()
{
	vector<int> used;

	// main quest
	{
		Quest* q = quest_manager.CreateQuest(Q_MAIN);
		q->refid = quest_counter++;
		q->Start();
		unaccepted_quests.push_back(q);
	}

	// goblins
	quest_goblins = new Quest_Goblins;
	quest_goblins->start_loc = GetRandomSettlement(used, 1);
	quest_goblins->refid = quest_counter++;
	quest_goblins->Start();
	unaccepted_quests.push_back(quest_goblins);
	used.push_back(quest_goblins->start_loc);

	// bandits
	quest_bandits = new Quest_Bandits;
	quest_bandits->start_loc = GetRandomSettlement(used, 1);
	quest_bandits->refid = quest_counter++;
	quest_bandits->Start();
	unaccepted_quests.push_back(quest_bandits);
	used.push_back(quest_bandits->start_loc);

	// sawmill
	quest_sawmill = new Quest_Sawmill;
	quest_sawmill->start_loc = GetRandomSettlement(used);
	quest_sawmill->refid = quest_counter++;
	quest_sawmill->Start();
	unaccepted_quests.push_back(quest_sawmill);
	used.push_back(quest_sawmill->start_loc);

	// mine
	quest_mine = new Quest_Mine;
	quest_mine->start_loc = GetRandomSettlement(used);
	quest_mine->target_loc = GetClosestLocation(L_CAVE, locations[quest_mine->start_loc]->pos);
	quest_mine->refid = quest_counter++;
	quest_mine->Start();
	unaccepted_quests.push_back(quest_mine);
	used.push_back(quest_mine->start_loc);

	// mages
	quest_mages = new Quest_Mages;
	quest_mages->start_loc = GetRandomSettlement(used);
	quest_mages->refid = quest_counter++;
	quest_mages->Start();
	unaccepted_quests.push_back(quest_mages);
	used.push_back(quest_mages->start_loc);

	// mages2
	quest_mages2 = new Quest_Mages2;
	quest_mages2->refid = quest_counter++;
	quest_mages2->Start();
	unaccepted_quests.push_back(quest_mages2);
	quest_rumor[P_MAGOWIE2] = true;
	--quest_rumor_counter;

	// orcs
	quest_orcs = new Quest_Orcs;
	quest_orcs->start_loc = GetRandomSettlement(used);
	quest_orcs->refid = quest_counter++;
	quest_orcs->Start();
	unaccepted_quests.push_back(quest_orcs);
	used.push_back(quest_orcs->start_loc);

	// orcs2
	quest_orcs2 = new Quest_Orcs2;
	quest_orcs2->refid = quest_counter++;
	quest_orcs2->Start();
	unaccepted_quests.push_back(quest_orcs2);

	// evil
	quest_evil = new Quest_Evil;
	quest_evil->start_loc = GetRandomSettlement(used);
	quest_evil->refid = quest_counter++;
	quest_evil->Start();
	unaccepted_quests.push_back(quest_evil);
	used.push_back(quest_evil->start_loc);

	// crazies
	quest_crazies = new Quest_Crazies;
	quest_crazies->refid = quest_counter++;
	quest_crazies->Start();
	unaccepted_quests.push_back(quest_crazies);

	// sekret
	secret_state = (FindObject("tomashu_dom")->mesh ? SECRET_NONE : SECRET_OFF);
	GetSecretNote()->desc.clear();
	secret_where = -1;
	secret_where2 = -1;

	// drinking contest
	contest_state = CONTEST_NOT_DONE;
	contest_where = GetRandomSettlement();
	contest_units.clear();
	contest_generated = false;
	contest_winner = nullptr;

	// zawody
	tournament_year = 0;
	tournament_city_year = year;
	tournament_city = GetRandomCity();
	tournament_state = TOURNAMENT_NOT_DONE;
	tournament_units.clear();
	tournament_winner = nullptr;
	tournament_generated = false;

	if(devmode)
	{
		LOG(Format("Quest 'Sawmill' - %s.", locations[quest_sawmill->start_loc]->name.c_str()));
		LOG(Format("Quest 'Mine' - %s, %s.", locations[quest_mine->start_loc]->name.c_str(), locations[quest_mine->target_loc]->name.c_str()));
		LOG(Format("Quest 'Bandits' - %s.", locations[quest_bandits->start_loc]->name.c_str()));
		LOG(Format("Quest 'Mages' - %s.", locations[quest_mages->start_loc]->name.c_str()));
		LOG(Format("Quest 'Orcs' - %s.", locations[quest_orcs->start_loc]->name.c_str()));
		LOG(Format("Quest 'Goblins' - %s.", locations[quest_goblins->start_loc]->name.c_str()));
		LOG(Format("Quest 'Evil' - %s.", locations[quest_evil->start_loc]->name.c_str()));
		LOG(Format("Tournament - %s.", locations[tournament_city]->name.c_str()));
		LOG(Format("Contest - %s.", locations[contest_where]->name.c_str()));
	}
}

void Game::GenerateQuestUnits()
{
	if(quest_sawmill->sawmill_state == Quest_Sawmill::State::None && current_location == quest_sawmill->start_loc)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("artur_drwal"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txArthur;
			quest_sawmill->sawmill_state = Quest_Sawmill::State::GeneratedUnit;
			quest_sawmill->hd_lumberjack.Get(*u->human_data);
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_mine->start_loc && quest_mine->mine_state == Quest_Mine::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("inwestor"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txQuest[272];
			quest_mine->mine_state = Quest_Mine::State::SpawnedInvestor;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_bandits->start_loc && quest_bandits->bandits_state == Quest_Bandits::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("mistrz_agentow"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txQuest[273];
			quest_bandits->bandits_state = Quest_Bandits::State::GeneratedMaster;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_mages->start_loc && quest_mages2->mages_state == Quest_Mages2::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_uczony"), -2);
		assert(u);
		if(u)
		{
			quest_mages2->mages_state = Quest_Mages2::State::GeneratedScholar;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_mages2->mage_loc)
	{
		if(quest_mages2->mages_state == Quest_Mages2::State::TalkedWithCaptain)
		{
			Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				quest_mages2->mages_state = Quest_Mages2::State::GeneratedOldMage;
				quest_mages2->good_mage_name = u->hero->name;
				if(devmode)
					LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
			}
		}
		else if(quest_mages2->mages_state == Quest_Mages2::State::MageLeft)
		{
			Unit* u = SpawnUnitInsideInn(*FindUnitData("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				quest_mages2->scholar = u;
				u->hero->know_name = true;
				u->hero->name = quest_mages2->good_mage_name;
				u->ApplyHumanData(quest_mages2->hd_mage);
				quest_mages2->mages_state = Quest_Mages2::State::MageGeneratedInCity;
				if(devmode)
					LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
			}
		}
	}

	if(current_location == quest_orcs->start_loc && quest_orcs2->orcs_state == Quest_Orcs2::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_orkowie_straznik"));
		assert(u);
		if(u)
		{
			u->StartAutoTalk();
			quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedGuard;
			quest_orcs2->guard = u;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_goblins->start_loc && quest_goblins->goblins_state == Quest_Goblins::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_gobliny_szlachcic"));
		assert(u);
		if(u)
		{
			quest_goblins->nobleman = u;
			quest_goblins->hd_nobleman.Get(*u->human_data);
			u->hero->name = txQuest[274];
			quest_goblins->goblins_state = Quest_Goblins::State::GeneratedNobleman;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(current_location == quest_evil->start_loc && quest_evil->evil_state == Quest_Evil::State::None)
	{
		CityBuilding* b = city_ctx->FindBuilding(content::BG_INN);
		Unit* u = SpawnUnitNearLocation(local_ctx, b->walk_pt, *FindUnitData("q_zlo_kaplan"), nullptr, 10);
		assert(u);
		if(u)
		{
			u->rot = random(MAX_ANGLE);
			u->hero->name = txQuest[275];
			u->StartAutoTalk();
			quest_evil->cleric = u;
			quest_evil->evil_state = Quest_Evil::State::GeneratedCleric;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(bandyta)
		return;

	// sawmill quest
	if(quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
	{
		if(quest_sawmill->days >= 30 && city_ctx)
		{
			quest_sawmill->days = 29;
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_tartak"), &leader->pos, -2, 2.f);
			if(u)
			{
				quest_sawmill->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}
	else if(quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		int ile = quest_sawmill->days / 30;
		if(ile)
		{
			quest_sawmill->days -= ile * 30;
			AddGold(ile*400, nullptr, true);
		}
	}

	if(quest_mine->days >= quest_mine->days_required &&
		((quest_mine->mine_state2 == Quest_Mine::State2::InBuild && quest_mine->mine_state == Quest_Mine::State::Shares) || // inform player about building mine & give gold
		quest_mine->mine_state2 == Quest_Mine::State2::Built || // inform player about possible investment
		quest_mine->mine_state2 == Quest_Mine::State2::InExpand || // inform player about finished mine expanding
		quest_mine->mine_state2 == Quest_Mine::State2::Expanded)) // inform player about finding portal
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
		if(u)
		{
			quest_mine->messenger = u;
			u->StartAutoTalk(true);
		}
	}

	GenerateQuestUnits2(true);

	if(quest_evil->evil_state == Quest_Evil::State::GenerateMage && current_location == quest_evil->mage_loc)
	{
		Unit* u = SpawnUnitInsideInn(*FindUnitData("q_zlo_mag"), -2);
		assert(u);
		if(u)
		{
			quest_evil->evil_state = Quest_Evil::State::GeneratedMage;
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}
}

void Game::GenerateQuestUnits2(bool on_enter)
{
	if(quest_goblins->goblins_state == Quest_Goblins::State::Counting && quest_goblins->days <= 0)
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("q_gobliny_poslaniec"), &leader->pos, -2, 2.f);
		if(u)
		{
			if(IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			quest_goblins->messenger = u;
			u->StartAutoTalk(true);
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft && quest_goblins->days <= 0)
	{
		Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("q_gobliny_mag"), &leader->pos, 5, 2.f);
		if(u)
		{
			if(IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			quest_goblins->messenger = u;
			quest_goblins->goblins_state = Quest_Goblins::State::GeneratedMage;
			u->StartAutoTalk(true);
			if(devmode)
				LOG(Format("Generated quest unit '%s'.", u->GetRealName()));
		}
	}
}

void Game::UpdateQuests(int days)
{
	if(bandyta)
		return;

	RemoveQuestUnits(false);

	int income = 0;

	// sawmill
	if(quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
	{
		quest_sawmill->days += days;
		if(quest_sawmill->days >= 30 && city_ctx && game_state == GS_LEVEL)
		{
			quest_sawmill->days = 29;
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_tartak"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				quest_sawmill->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}
	else if(quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		quest_sawmill->days += days;
		int count = quest_sawmill->days / 30;
		if(count)
		{
			quest_sawmill->days -= count * 30;
			income += count * 400;
		}
	}

	// mine
	if(quest_mine->mine_state2 == Quest_Mine::State2::InBuild)
	{
		quest_mine->days += days;
		if(quest_mine->days >= quest_mine->days_required)
		{
			if(quest_mine->mine_state == Quest_Mine::State::Shares)
			{
				// player invesetd in mine, inform him about finishing
				if(city_ctx && game_state == GS_LEVEL)
				{
					Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
					if(u)
					{
						if(IsOnline())
							Net_SpawnUnit(u);
						AddNews(Format(txMineBuilt, locations[quest_mine->target_loc]->name.c_str()));
						quest_mine->messenger = u;
						u->StartAutoTalk(true);
					}
				}
			}
			else
			{
				// player got gold, don't inform him
				AddNews(Format(txMineBuilt, locations[quest_mine->target_loc]->name.c_str()));
				quest_mine->mine_state2 = Quest_Mine::State2::Built;
				quest_mine->days -= quest_mine->days_required;
				quest_mine->days_required = random(60, 90);
				if(quest_mine->days >= quest_mine->days_required)
					quest_mine->days = quest_mine->days_required - 1;
			}
		}
	}
	else if(quest_mine->mine_state2 == Quest_Mine::State2::Built || quest_mine->mine_state2 == Quest_Mine::State2::InExpand || quest_mine->mine_state2 == Quest_Mine::State2::Expanded)
	{
		// mine is built/in expand/expanded
		// count time to news about expanding/finished expanding/found portal
		quest_mine->days += days;
		if(quest_mine->days >= quest_mine->days_required && city_ctx && game_state == GS_LEVEL)
		{
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("poslaniec_kopalnia"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				quest_mine->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}

	// give gold from mine
	income += quest_mine->GetIncome(days);

	if(income != 0)
		AddGold(income, nullptr, true);

	int stan; // 0 - przed zawodami, 1 - czas na zawody, 2 - po zawodach

	if(month < 8)
		stan = 0;
	else if(month == 8)
	{
		if(day < 20)
			stan = 0;
		else if(day == 20)
			stan = 1;
		else
			stan = 2;
	}
	else
		stan = 2;
	
	switch(stan)
	{
	case 0:
		if(contest_state != CONTEST_NOT_DONE)
		{
			contest_state = CONTEST_NOT_DONE;
			contest_where = GetRandomSettlement(contest_where);
		}
		contest_generated = false;
		contest_units.clear();
		break;
	case 1:
		contest_state = CONTEST_TODAY;
		if(!contest_generated && game_state == GS_LEVEL && current_location == contest_where)
			SpawnDrunkmans();
		break;
	case 2:
		contest_state = CONTEST_DONE;
		contest_generated = false;
		contest_units.clear();
		break;
	}

	//----------------------------
	// mages
	if(quest_mages2->mages_state == Quest_Mages2::State::Counting)
	{
		quest_mages2->days -= days;
		if(quest_mages2->days <= 0)
		{
			// from now golem can be encountered on road
			quest_mages2->mages_state = Quest_Mages2::State::Encounter;
		}
	}

	// orcs
	if(In(quest_orcs2->orcs_state, { Quest_Orcs2::State::CompletedJoined, Quest_Orcs2::State::CampCleared, Quest_Orcs2::State::PickedClass }))
	{
		quest_orcs2->days -= days;
		if(quest_orcs2->days <= 0)
			quest_orcs2->orc->StartAutoTalk();
	}

	// goblins
	if(quest_goblins->goblins_state == Quest_Goblins::State::Counting || quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft)
		quest_goblins->days -= days;

	// crazies
	if(quest_crazies->crazies_state == Quest_Crazies::State::PickedStone)
		quest_crazies->days -= days;

	// zawody
	if(year != tournament_city_year)
	{
		tournament_city_year = year;
		tournament_city = GetRandomCity(tournament_city);
		tournament_master = nullptr;
	}
	if(day == 6 && month == 2 && city_ctx && IS_SET(city_ctx->flags, City::HaveArena) && current_location == tournament_city)
		GenerateTournamentUnits();
	if(month > 2 || (month == 2 && day > 6))
		tournament_year = year;

	if(city_ctx)
		GenerateQuestUnits2(false);
}

void Game::RemoveQuestUnit(UnitData* ud, bool on_leave)
{
	assert(ud);

	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud && (*it)->IsAlive())
		{
			(*it)->to_remove = true;
			to_remove.push_back(*it);
			if(!on_leave && IsOnline())
				Net_RemoveUnit(*it);
			return;
		}
	}

	for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
	{
		for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
		{
			if((*it)->data == ud && (*it)->IsAlive())
			{
				(*it)->to_remove = true;
				to_remove.push_back(*it);
				if(!on_leave && IsOnline())
					Net_RemoveUnit(*it);
				return;
			}
		}
	}
}

void Game::RemoveQuestUnits(bool on_leave)
{
	if(city_ctx)
	{
		if(quest_sawmill->messenger)
		{
			RemoveQuestUnit(FindUnitData("poslaniec_tartak"), on_leave);
			quest_sawmill->messenger = nullptr;
		}

		if(quest_mine->messenger)
		{
			RemoveQuestUnit(FindUnitData("poslaniec_kopalnia"), on_leave);
			quest_mine->messenger = nullptr;
		}

		if(open_location == quest_sawmill->start_loc && quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild && quest_sawmill->build_state == Quest_Sawmill::BuildState::None)
		{
			Unit* u = city_ctx->FindInn()->FindUnit(FindUnitData("artur_drwal"));
			if(u && u->IsAlive())
			{
				u->to_remove = true;
				to_remove.push_back(u);
				quest_sawmill->build_state = Quest_Sawmill::BuildState::LumberjackLeft;
				if(!on_leave && IsOnline())
					Net_RemoveUnit(u);
			}
		}

		if(quest_mages2->scholar && quest_mages2->mages_state == Quest_Mages2::State::ScholarWaits)
		{
			RemoveQuestUnit(FindUnitData("q_magowie_uczony"), on_leave);
			quest_mages2->scholar = nullptr;
			quest_mages2->mages_state = Quest_Mages2::State::Counting;
			quest_mages2->days = random(15, 30);
		}

		if(quest_orcs2->guard && quest_orcs2->orcs_state >= Quest_Orcs2::State::GuardTalked)
		{
			RemoveQuestUnit(FindUnitData("q_orkowie_straznik"), on_leave);
			quest_orcs2->guard = nullptr;
		}
	}

	if(quest_bandits->bandits_state == Quest_Bandits::State::AgentTalked)
	{
		quest_bandits->bandits_state = Quest_Bandits::State::AgentLeft;
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			if(*it == quest_bandits->agent && (*it)->IsAlive())
			{
				(*it)->to_remove = true;
				to_remove.push_back(*it);
				if(!on_leave && IsOnline())
					Net_RemoveUnit(*it);
				break;
			}
		}
		quest_bandits->agent = nullptr;
	}

	if(quest_mages2->mages_state == Quest_Mages2::State::MageLeaving)
	{
		quest_mages2->hd_mage.Set(*quest_mages2->scholar->human_data);
		quest_mages2->scholar = nullptr;
		RemoveQuestUnit(FindUnitData("q_magowie_stary"), on_leave);
		quest_mages2->mages_state = Quest_Mages2::State::MageLeft;
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::MessengerTalked && quest_goblins->messenger)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_poslaniec"), on_leave);
		quest_goblins->messenger = nullptr;
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::GivenBow && quest_goblins->nobleman)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_szlachcic"), on_leave);
		quest_goblins->nobleman = nullptr;
		quest_goblins->goblins_state = Quest_Goblins::State::NoblemanLeft;
		quest_goblins->days = random(15, 30);
	}

	if(quest_goblins->goblins_state == Quest_Goblins::State::MageTalked && quest_goblins->messenger)
	{
		RemoveQuestUnit(FindUnitData("q_gobliny_mag"), on_leave);
		quest_goblins->messenger = nullptr;
		quest_goblins->goblins_state = Quest_Goblins::State::MageLeft;
	}

	if(quest_evil->evil_state == Quest_Evil::State::ClericLeaving)
	{
		quest_evil->cleric->to_remove = true;
		to_remove.push_back(quest_evil->cleric);
		if(!on_leave && IsOnline())
			Net_RemoveUnit(quest_evil->cleric);
		quest_evil->cleric = nullptr;
		quest_evil->evil_state = Quest_Evil::State::ClericLeft;
	}
}

cstring tartak_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"torch",
	"torch_off",
	"firewood"
};
const uint n_tartak_objs = countof(tartak_objs);
Obj* tartak_objs_ptrs[n_tartak_objs];

void Game::GenerateSawmill(bool in_progress)
{
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		delete *it;
	local_ctx.units->clear();
	local_ctx.bloods->clear();

	// wyrównaj teren
	OutsideLocation* outside = (OutsideLocation*)location;
	float* h = outside->h;
	const int _s = outside->size+1;
	vector<INT2> tiles;
	float wys = 0.f;
	for(int y=64-6; y<64+6; ++y)
	{
		for(int x=64-6; x<64+6; ++x)
		{
			if(distance(VEC2(2.f*x+1.f, 2.f*y+1.f), VEC2(128,128)) < 8.f)
			{
				wys += h[x+y*_s];
				tiles.push_back(INT2(x,y));
			}
		}
	}
	wys /= tiles.size();
	for(vector<INT2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		h[it->x+it->y*_s] = wys;
	terrain->Rebuild(true);

	// usuñ obiekty
	for(vector<Object>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end;)
	{
		if(distance2d(it->pos, VEC3(128,0,128)) < 16.f)
		{
			if(it+1 == end)
			{
				local_ctx.objects->pop_back();
				break;
			}
			else
			{
				it->Swap(*(end-1));
				local_ctx.objects->pop_back();
				end = local_ctx.objects->end();
			}
		}
		else
			++it;
	}

	if(!tartak_objs_ptrs[0])
	{
		for(uint i=0; i<n_tartak_objs; ++i)
			tartak_objs_ptrs[i] = FindObject(tartak_objs[i]);
	}

	UnitData& ud = *FindUnitData("artur_drwal");
	UnitData& ud2 = *FindUnitData("drwal");
	
	if(in_progress)
	{
		// artur drwal
		Unit* u = SpawnUnitNearLocation(local_ctx, VEC3(128,0,128), ud, nullptr, -2);
		assert(u);
		u->rot = random(MAX_ANGLE);
		u->hero->name = txArthur;
		u->hero->know_name = true;
		u->ApplyHumanData(quest_sawmill->hd_lumberjack);

		// generuj obiekty
		for(int i=0; i<25; ++i)
		{
			VEC2 pt = random(VEC2(128-16,128-16), VEC2(128+16,128+16));
			Obj* obj = tartak_objs_ptrs[rand2()%n_tartak_objs];
			SpawnObjectNearLocation(local_ctx, obj, pt, random(MAX_ANGLE), 2.f);
		}

		// generuj innych drwali
		int ile = random(5,10);
		for(int i=0; i<ile; ++i)
		{
			Unit* u = SpawnUnitNearLocation(local_ctx, random(VEC3(128-16,0,128-16), VEC3(128+16,0,128+16)), ud2, nullptr, -2);
			if(u)
				u->rot = random(MAX_ANGLE);
		}

		quest_sawmill->build_state = Quest_Sawmill::BuildState::InProgress;
	}
	else
	{
		// budynek
		VEC3 spawn_pt;
		float rot = PI/2*(rand2()%4);
		SpawnObject(local_ctx, FindObject("tartak"), VEC3(128,wys,128), rot, 1.f, &spawn_pt);

		// artur drwal
		Unit* u = SpawnUnitNearLocation(local_ctx, spawn_pt, ud, nullptr, -2);
		assert(u);
		u->rot = rot;
		u->hero->name = txArthur;
		u->hero->know_name = true;
		u->ApplyHumanData(quest_sawmill->hd_lumberjack);

		// obiekty
		for(int i=0; i<25; ++i)
		{
			VEC2 pt = random(VEC2(128-16,128-16), VEC2(128+16,128+16));
			Obj* obj = tartak_objs_ptrs[rand2()%n_tartak_objs];
			SpawnObjectNearLocation(local_ctx, obj, pt, random(MAX_ANGLE), 2.f);
		}
		
		// inni drwale
		int ile = random(5,10);
		for(int i=0; i<ile; ++i)
		{
			Unit* u = SpawnUnitNearLocation(local_ctx, random(VEC3(128-16,0,128-16), VEC3(128+16,0,128+16)), ud2, nullptr, -2);
			if(u)
				u->rot = random(MAX_ANGLE);
		}

		quest_sawmill->build_state = Quest_Sawmill::BuildState::Finished;
	}
}

void Object::Swap(Object& o)
{
	VEC3 tv;

	tv = pos;
	pos = o.pos;
	o.pos = tv;

	tv = rot;
	rot = o.rot;
	o.rot = tv;

	tv.x = scale;
	scale = o.scale;
	o.scale = tv.x;

	void* p;

	p = mesh;
	mesh = o.mesh;
	o.mesh = (Animesh*)p;

	p = base;
	base = o.base;
	o.base = (Obj*)p;

// 	p = ptr;
// 	ptr = o.ptr;
// 	o.ptr = p;
}

bool Game::GenerateMine()
{
	switch(quest_mine->mine_state3)
	{
	case Quest_Mine::State3::None:
		break;
	case Quest_Mine::State3::GeneratedMine:
		if(quest_mine->mine_state2 == Quest_Mine::State2::None)
			return true;
		break;
	case Quest_Mine::State3::GeneratedInBuild:
		if(quest_mine->mine_state2 <= Quest_Mine::State2::InBuild)
			return true;
		break;
	case Quest_Mine::State3::GeneratedBuilt:
		if(quest_mine->mine_state2 <= Quest_Mine::State2::Built)
			return true;
		break;
	case Quest_Mine::State3::GeneratedExpanded:
		if(quest_mine->mine_state2 <= Quest_Mine::State2::Expanded)
			return true;
		break;
	case Quest_Mine::State3::GeneratedPortal:
		if(quest_mine->mine_state2 <= Quest_Mine::State2::FoundPortal)
			return true;
		break;
	default:
		assert(0);
		return true;
	}

	CaveLocation* cave = (CaveLocation*)location;
	InsideLocationLevel& lvl = cave->GetLevelData();

	bool respawn_units = true;

	// usuñ stare jednostki i krew
	if(quest_mine->mine_state3 <= Quest_Mine::State3::GeneratedMine && quest_mine->mine_state2 >= Quest_Mine::State2::InBuild)
	{
		DeleteElements(local_ctx.units);
		DeleteElements(ais);
		local_ctx.units->clear();
		local_ctx.bloods->clear();
		respawn_units = false;
	}

	bool generuj_rude = false;
	int zloto_szansa, powieksz = 0;
	bool rysuj_m = false;

	if(quest_mine->mine_state3 == Quest_Mine::State3::None)
	{
		generuj_rude = true;
		zloto_szansa = 0;
	}

	// pog³êb jaskinie
	if(quest_mine->mine_state2 >= Quest_Mine::State2::InBuild && quest_mine->mine_state3 < Quest_Mine::State3::GeneratedBuilt)
	{
		generuj_rude = true;
		zloto_szansa = 0;
		++powieksz;
		rysuj_m = true;
	}

	// bardziej pog³êb jaskinie
	if(quest_mine->mine_state2 >= Quest_Mine::State2::InExpand && quest_mine->mine_state3 < Quest_Mine::State3::GeneratedExpanded)
	{
		generuj_rude = true;
		zloto_szansa = 4;
		++powieksz;
		rysuj_m = true;
	}

	vector<INT2> nowe;

	for(int i=0; i<powieksz; ++i)
	{
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(lvl.map[x + y*lvl.w].type == SCIANA)
				{
#define A(xx,yy) lvl.map[x+(xx)+(y+(yy))*lvl.w].type
					if(rand2()%2 == 0 && (!czy_blokuje21(A(-1,0)) || !czy_blokuje21(A(1,0)) || !czy_blokuje21(A(0,-1)) || !czy_blokuje21(A(0,1))) &&
						(A(-1,-1) != SCHODY_GORA && A(-1,1) != SCHODY_GORA && A(1,-1) != SCHODY_GORA && A(1,1) != SCHODY_GORA))
					{
						nowe.push_back(INT2(x,y));
					}
#undef A
				}
			}
		}

		// nie potrzebnie dwa razy to robi jeœli powiêksz = 2
		for(vector<INT2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
			lvl.map[it->x + it->y*lvl.w].type = PUSTE;
	}

	// generuj portal
	if(quest_mine->mine_state2 >= Quest_Mine::State2::FoundPortal && quest_mine->mine_state3 < Quest_Mine::State3::GeneratedPortal)
	{
		generuj_rude = true;
		zloto_szansa = 7;
		rysuj_m = true;

		// szukaj dobrego miejsca
		vector<INT2> good_pts;
		for(int y=1; y<lvl.h-5; ++y)
		{
			for(int x=1; x<lvl.w-5; ++x)
			{
				for(int h=0; h<5; ++h)
				{
					for(int w=0; w<5; ++w)
					{
						if(lvl.map[x + w + (y + h)*lvl.w].type != SCIANA)
							goto dalej;
					}
				}

				// jest dobre miejsce
				good_pts.push_back(INT2(x,y));
dalej:
				;
			}
		}

		if(good_pts.empty())
		{
			if(lvl.staircase_up.x / 26 == 1)
			{
				if(lvl.staircase_up.y / 26 == 1)
					good_pts.push_back(INT2(1,1));
				else
					good_pts.push_back(INT2(1,lvl.h-6));
			}
			else
			{
				if(lvl.staircase_up.y / 26 == 1)
					good_pts.push_back(INT2(lvl.w-6,1));
				else
					good_pts.push_back(INT2(lvl.w-6,lvl.h-6));
			}
		}

		INT2 pt = good_pts[rand2()%good_pts.size()];

		// przygotuj pokój
		// BBZBB
		// B___B
		// Z___Z
		// B___B
		// BBZBB
		const INT2 p_blokady[] = {
			INT2(0,0),
			INT2(1,0),
			INT2(3,0),
			INT2(4,0),
			INT2(0,1),
			INT2(4,1),
			INT2(0,3),
			INT2(4,3),
			INT2(0,4),
			INT2(1,4),
			INT2(3,4),
			INT2(4,4)
		};
		const INT2 p_zajete[] = {
			INT2(2,0),
			INT2(0,2),
			INT2(4,2),
			INT2(2,4)
		};
		for(int i=0; i<countof(p_blokady); ++i)
		{
			Pole& p = lvl.map[(pt+p_blokady[i])(lvl.w)];
			p.type = BLOKADA;
			p.flags = 0;
		}
		for(int i=0; i<countof(p_zajete); ++i)
		{
			Pole& p = lvl.map[(pt+p_zajete[i])(lvl.w)];
			p.type = ZAJETE;
			p.flags = 0;
		}

		// dorób wejœcie
		// znajdŸ najbli¿szy pokoju wolny punkt
		const INT2 center(pt.x+2, pt.y+2);
		INT2 closest;
		int best_dist = 999;
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(lvl.map[x + y*lvl.w].type == PUSTE)
				{
					int dist = distance(INT2(x,y), center);
					if(dist < best_dist && dist > 2)
					{
						best_dist = dist;
						closest = INT2(x,y);
					}
				}
			}
		}

		// prowadŸ drogê do œrodka
		// tu mo¿e byæ nieskoñczona pêtla ale nie powinno jej byæ chyba ¿e bêd¹ jakieœ blokady na mapie :3
		INT2 end_pt;
		while(true)
		{
			if(abs(closest.x-center.x) > abs(closest.y-center.y))
			{
po_x:
				if(closest.x > center.x)
				{
					--closest.x;
					Pole& p = lvl.map[closest.x+closest.y*lvl.w];
					if(p.type == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOKADA)
					{
						++closest.x;
						goto po_y;
					}
					else
					{
						p.type = PUSTE;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.x;
					Pole& p = lvl.map[closest.x+closest.y*lvl.w];
					if(p.type == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOKADA)
					{
						--closest.x;
						goto po_y;
					}
					else
					{
						p.type = PUSTE;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
			}
			else
			{
po_y:
				if(closest.y > center.y)
				{
					--closest.y;
					Pole& p = lvl.map[closest.x+closest.y*lvl.w];
					if(p.type == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOKADA)
					{
						++closest.y;
						goto po_x;
					}
					else
					{
						p.type = PUSTE;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
				else
				{
					++closest.y;
					Pole& p = lvl.map[closest.x+closest.y*lvl.w];
					if(p.type == ZAJETE)
					{
						end_pt = closest;
						break;
					}
					else if(p.type == BLOKADA)
					{
						--closest.y;
						goto po_x;
					}
					else
					{
						p.type = PUSTE;
						p.flags = 0;
						nowe.push_back(closest);
					}
				}
			}
		}
		
		// ustaw œciany
		for(int i=0; i<countof(p_blokady); ++i)
		{
			Pole& p = lvl.map[(pt+p_blokady[i])(lvl.w)];
			p.type = SCIANA;
		}
		for(int i=0; i<countof(p_zajete); ++i)
		{
			Pole& p = lvl.map[(pt+p_zajete[i])(lvl.w)];
			p.type = SCIANA;
		}
		for(int y=1; y<4; ++y)
		{
			for(int x=1; x<4; ++x)
			{
				Pole& p = lvl.map[pt.x+x+(pt.y+y)*lvl.w];
				p.type = PUSTE;
				p.flags = Pole::F_DRUGA_TEKSTURA;
			}
		}
		Pole& p = lvl.map[end_pt(lvl.w)];
		p.type = DRZWI;
		p.flags = 0;

		// ustaw pokój
		Room& room = Add1(lvl.rooms);
		room.target = RoomTarget::Portal;
		room.pos = pt;
		room.size = INT2(5, 5);

		// dodaj drzwi, portal, pochodnie
		Obj* portal = FindObject("portal"),
		   * pochodnia = FindObject("torch");

		// drzwi
		{
			Object& o = Add1(local_ctx.objects);
			o.mesh = aNaDrzwi;
			o.pos = VEC3(float(end_pt.x*2)+1,0,float(end_pt.y*2)+1);
			o.scale = 1;
			o.base = nullptr;

			// hack :3
			Room& r2 = Add1(lvl.rooms);
			r2.target = RoomTarget::Corridor;

			if(czy_blokuje2(lvl.map[end_pt.x - 1 + end_pt.y*lvl.w].type))
			{
				o.rot = VEC3(0,0,0);
				if(end_pt.y > center.y)
				{
					o.pos.z -= 0.8229f;
					lvl.At(end_pt+INT2(0,1)).room = 1;
				}
				else
				{
					o.pos.z += 0.8229f;
					lvl.At(end_pt+INT2(0,-1)).room = 1;
				}
			}
			else
			{
				o.rot = VEC3(0,PI/2,0);
				if(end_pt.x > center.x)
				{
					o.pos.x -= 0.8229f;
					lvl.At(end_pt+INT2(1,0)).room = 1;
				}
				else
				{
					o.pos.x += 0.8229f;
					lvl.At(end_pt+INT2(-1,0)).room = 1;
				}
			}

			Door* door = new Door;
			local_ctx.doors->push_back(door);
			door->pt = end_pt;
			door->pos = o.pos;
			door->rot = o.rot.y;
			door->state = Door::Closed;
			door->ani = new AnimeshInstance(aDrzwi);
			door->ani->groups[0].speed = 2.f;
			door->phy = new btCollisionObject;
			door->phy->setCollisionShape(shape_door);
			door->locked = LOCK_MINE;
			door->netid = door_netid_counter++;
			btTransform& tr = door->phy->getWorldTransform();
			VEC3 pos = door->pos;
			pos.y += 1.319f;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door->rot, 0, 0));
			phy_world->addCollisionObject(door->phy);
		}

		// pochodnia
		{
			VEC3 pos(2.f*(pt.x+1), 0, 2.f*(pt.y+1));

			switch(rand2()%4)
			{
			case 0:
				pos.x += pochodnia->r*2;
				pos.z += pochodnia->r*2;
				break;
			case 1:
				pos.x += 6.f-pochodnia->r*2;
				pos.z += pochodnia->r*2;
				break;
			case 2:
				pos.x += pochodnia->r*2;
				pos.z += 6.f-pochodnia->r*2;
				break;
			case 3:
				pos.x += 6.f-pochodnia->r*2;
				pos.z += 6.f-pochodnia->r*2;
				break;
			}

			SpawnObject(local_ctx, pochodnia, pos, random(MAX_ANGLE));
		}

		// portal
		{
			float rot;
			if(end_pt.y == center.y)
			{
				if(end_pt.x > center.x)
					rot = PI*3/2;
				else
					rot = PI/2;
			}
			else
			{
				if(end_pt.y > center.y)
					rot = 0;
				else
					rot = PI;
			}

			rot = clip(rot+PI);

			// obiekt
			const VEC3 pos(2.f*pt.x+5,0,2.f*pt.y+5);
			SpawnObject(local_ctx, portal, pos, rot);

			// lokacja
			SingleInsideLocation* loc = new SingleInsideLocation;
			loc->active_quest = quest_mine;
			loc->target = KOPALNIA_POZIOM;
			loc->from_portal = true;
			loc->name = txAncientArmory;
			loc->pos = VEC2(-999,-999);
			loc->spawn = SG_GOLEMY;
			loc->st = 14;
			loc->type = L_DUNGEON;
			loc->image = LI_DUNGEON;
			int loc_id = AddLocation(loc);
			quest_mine->sub.target_loc = quest_mine->dungeon_loc = loc_id;

			// funkcjonalnoœæ portalu
			cave->portal = new Portal;
			cave->portal->at_level = 0;
			cave->portal->target = 0;
			cave->portal->target_loc = loc_id;
			cave->portal->rot = rot;
			cave->portal->next_portal = nullptr;
			cave->portal->pos = pos;

			// info dla portalu po drugiej stronie
			loc->portal = new Portal;
			loc->portal->at_level = 0;
			loc->portal->target = 0;
			loc->portal->target_loc = current_location;
			loc->portal->next_portal = nullptr;
		}
	}

	if(!nowe.empty())
		regenerate_cave_flags(lvl.map, lvl.w);

	if(rysuj_m && devmode)
		rysuj_mape_konsola(lvl.map, lvl.w, lvl.h);

	// generuj rudê
	if(generuj_rude)
	{
		Obj* iron_ore = FindObject("iron_ore");

		// usuñ star¹ rudê
		if(quest_mine->mine_state3 != Quest_Mine::State3::None)
			DeleteElements(local_ctx.useables);

		// dodaj now¹
		for(int y=1; y<lvl.h-1; ++y)
		{
			for(int x=1; x<lvl.w-1; ++x)
			{
				if(rand2()%3 == 0)
					continue;

#define P(xx,yy) !czy_blokuje21(lvl.map[x-(xx)+(y+(yy))*lvl.w])
#undef S
#define S(xx,yy) lvl.map[x-(xx)+(y+(yy))*lvl.w].type == SCIANA

				// ruda jest generowana dla takich przypadków, w tym obróconych
				//  ### ### ###
				//  _?_ #?_ #?#
				//  ___ #__ #_#
				if(lvl.map[x + y*lvl.w].type == PUSTE && rand2() % 3 != 0 && !IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_DRUGA_TEKSTURA))
				{
					int dir = -1;

					// ma byæ œciana i wolne z ty³u oraz wolne na lewo lub prawo lub zajête z obu stron
					// __#
					// _?#
					// __#
					if(P(-1,0) && S(1,0) && S(1,-1) && S(1,1) && ((P(-1,-1) && P(0,-1)) || (P(-1,1) && P(0,1)) || (S(-1,-1) && S(0,-1) && S(-1,1) && S(0,1))))
					{
						dir = 1;
					}
					// #__
					// #?_
					// #__
					else if(P(1,0) && S(-1,0) && S(-1,1) && S(-1,-1) && ((P(0,-1) && P(1,-1)) || (P(0,1) && P(1,1)) || (S(0,-1) && S(1,-1) && S(0,1) && S(1,1))))
					{
						dir = 3;
					}
					// ### 
					// _?_
					// ___
					else if(P(0,1) && S(0,-1) && S(-1,-1) && S(1,-1) && ((P(-1,0) && P(-1,1)) || (P(1,0) && P(1,1)) || (S(-1,0) && S(-1,1) && S(1,0) && S(1,1))))
					{
						dir = 0;
					}
					// ___
					// _?_
					// ###
					else if(P(0,-1) && S(0,1) && S(-1,1) && S(1,1) && ((P(-1,0) && P(-1,-1)) || (P(1,0) && P(1,-1)) || (S(-1,0) && S(-1,-1) && S(1,0) && S(1,-1))))
					{
						dir = 2;
					}

					if(dir != -1)
					{
						VEC3 pos(2.f*x+1,0,2.f*y+1);

						switch(dir)
						{
						case 0:
							pos.z -= 1.f;
							break;
						case 1:
							pos.x -= 1.f;
							break;
						case 2:
							pos.z += 1.f;
							break;
						case 3:
							pos.x += 1.f;
							break;
						}

						float rot = clip(dir_to_rot(dir)+PI);
						static float radius = max(iron_ore->size.x, iron_ore->size.y) * SQRT_2;

						IgnoreObjects ignore = {0};
						ignore.ignore_blocks = true;
						global_col.clear();
						GatherCollisionObjects(local_ctx, global_col, pos, radius, &ignore);

						BOX2D box(pos.x-iron_ore->size.x, pos.z-iron_ore->size.y, pos.x+iron_ore->size.x, pos.z+iron_ore->size.y);

						if(!Collide(global_col, box, 0.f, rot))
						{
							Useable* u = new Useable;
							u->pos = pos;
							u->rot = rot;
							u->type = (rand2()%10 < zloto_szansa ? U_GOLD_VAIN : U_IRON_VAIN);
							u->user = nullptr;
							u->netid = useable_netid_counter++;
							local_ctx.useables->push_back(u);

							CollisionObject& c = Add1(local_ctx.colliders);
							btCollisionObject* cobj = new btCollisionObject;
							cobj->setCollisionShape(iron_ore->shape);

							btTransform& tr = cobj->getWorldTransform();
							VEC3 zero(0,0,0), pos2;
							D3DXVec3TransformCoord(&pos2, &zero, iron_ore->matrix);
							pos2 += pos;
							tr.setOrigin(ToVector3(pos2));
							tr.setRotation(btQuaternion(rot, 0, 0));

							c.pt = VEC2(pos2.x, pos2.z);
							c.w = iron_ore->size.x;
							c.h = iron_ore->size.y;
							if(not_zero(rot))
							{
								c.type = CollisionObject::RECTANGLE_ROT;
								c.rot = rot;
								c.radius = radius;
							}
							else
								c.type = CollisionObject::RECTANGLE;

							phy_world->addCollisionObject(cobj, CG_WALL);
						}
					}
#undef P
#undef S
				}
			}
		}
	}

	// generuj nowe obiekty
	if(!nowe.empty())
	{
		Obj* kamien = FindObject("rock"),
		   * krzak = FindObject("plant2"),
		   * grzyb = FindObject("mushrooms");

		for(vector<INT2>::iterator it = nowe.begin(), end = nowe.end(); it != end; ++it)
		{
			if(rand2()%10 == 0)
			{
				Obj* obj;
				switch(rand2()%3)
				{
				default:
				case 0:
					obj = kamien;
					break;
				case 1:
					obj = krzak;
					break;
				case 2:
					obj = grzyb;
					break;
				}

				SpawnObject(local_ctx, obj, VEC3(2.f*it->x+random(0.1f,1.9f), 0.f, 2.f*it->y+random(0.1f,1.9f)), random(MAX_ANGLE));
			}
		}
	}

	// generuj jednostki
	bool ustaw = true;

	if(quest_mine->mine_state3 < Quest_Mine::State3::GeneratedInBuild && quest_mine->mine_state2 >= Quest_Mine::State2::InBuild)
	{
		ustaw = false;

		// szef górników na wprost wejœcia
		INT2 pt = lvl.GetUpStairsFrontTile();
		int odl = 1;
		while(lvl.map[pt(lvl.w)].type == PUSTE && odl < 5)
		{
			pt += g_kierunek2[lvl.staircase_up_dir];
			++odl;
		}
		pt -= g_kierunek2[lvl.staircase_up_dir];

		SpawnUnitNearLocation(local_ctx, VEC3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *FindUnitData("gornik_szef"), &VEC3(2.f*lvl.staircase_up.x + 1, 0, 2.f*lvl.staircase_up.y + 1), -2);

		// górnicy
		UnitData& gornik = *FindUnitData("gornik");
		for(int i=0; i<10; ++i)
		{
			for(int j=0; j<15; ++j)
			{
				INT2 tile = cave->GetRandomTile();
				const Pole& p = lvl.At(tile);
				if(p.type == PUSTE && !IS_SET(p.flags, Pole::F_DRUGA_TEKSTURA))
				{
					SpawnUnitNearLocation(local_ctx, VEC3(2.f*tile.x+random(0.4f,1.6f),0,2.f*tile.y+random(0.4f,1.6f)), gornik, nullptr, -2);
					break;
				}
			}
		}
	}

	// ustaw jednostki
	if(!ustaw && quest_mine->mine_state3 >= Quest_Mine::State3::GeneratedInBuild)
	{
		UnitData* gornik = FindUnitData("gornik"),
			* szef_gornikow = FindUnitData("gornik_szef");
		for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
		{
			Unit* u = *it;
			if(u->IsAlive())
			{
				if(u->data == gornik)
				{
					for(int i=0; i<10; ++i)
					{
						INT2 tile = cave->GetRandomTile();
						const Pole& p = lvl.At(tile);
						if(p.type == PUSTE && !IS_SET(p.flags, Pole::F_DRUGA_TEKSTURA))
						{
							WarpUnit(*u, VEC3(2.f*tile.x+random(0.4f,1.6f),0,2.f*tile.y+random(0.4f,1.6f)));
							break;
						}
					}
				}
				else if(u->data == szef_gornikow)
				{
					INT2 pt = lvl.GetUpStairsFrontTile();
					int odl = 1;
					while(lvl.map[pt(lvl.w)].type == PUSTE && odl < 5)
					{
						pt += g_kierunek2[lvl.staircase_up_dir];
						++odl;
					}
					pt -= g_kierunek2[lvl.staircase_up_dir];

					WarpUnit(*u, VEC3(2.f*pt.x+1,0,2.f*pt.y+1));
				}
			}
		}
	}

	switch(quest_mine->mine_state2)
	{
	case Quest_Mine::State2::None:
		quest_mine->mine_state3 = Quest_Mine::State3::GeneratedMine;
		break;
	case Quest_Mine::State2::InBuild:
		quest_mine->mine_state3 = Quest_Mine::State3::GeneratedInBuild;
		break;
	case Quest_Mine::State2::Built:
	case Quest_Mine::State2::CanExpand:
	case Quest_Mine::State2::InExpand:
		quest_mine->mine_state3 = Quest_Mine::State3::GeneratedBuilt;
		break;
	case Quest_Mine::State2::Expanded:
		quest_mine->mine_state3 = Quest_Mine::State3::GeneratedExpanded;
		break;
	case Quest_Mine::State2::FoundPortal:
		quest_mine->mine_state3 = Quest_Mine::State3::GeneratedPortal;
		break;
	default:
		assert(0);
		break;
	}

	return respawn_units;
}

void Game::HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
{
	assert(unit);

	if(event == UnitEventHandler::FALL)
	{
		// jednostka poleg³a w zawodach w piciu :3
		unit->look_target = nullptr;
		unit->busy = Unit::Busy_No;
		unit->event_handler = nullptr;
		RemoveElement(contest_units, unit);

		if(IsOnline() && unit->IsPlayer() && unit->player != pc)
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::LOOK_AT;
			c.pc = unit->player;
			c.id = -1;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}
}

int Game::GetUnitEventHandlerQuestRefid()
{
	// specjalna wartoœæ u¿ywana dla wskaŸnika na Game
	return -2;
}

bool Game::IsTeamNotBusy()
{
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->busy)
			return false;
	}

	return true;
}

// czy ktokolwiek w dru¿ynie rozmawia
// bêdzie u¿ywane w multiplayer
bool Game::IsAnyoneTalking() const
{
	if(IsLocal())
	{
		if(IsOnline())
		{
			for(vector<Unit*>::const_iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->dialog_ctx->dialog_mode)
					return true;
			}
			return false;
		}
		else
			return dialog_context.dialog_mode;
	}
	else
		return anyone_talking;
}

bool Game::FindItemInTeam(const Item* item, int refid, Unit** unit, int* i_index, bool check_npc)
{
	assert(item);

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || check_npc)
		{
			int index = (*it)->FindItem(item, refid);
			if(index != INVALID_IINDEX)
			{
				if(unit)
					*unit = *it;
				if(i_index)
					*i_index = index;
				return true;
			}
		}
	}

	return false;
}

bool Game::RemoveQuestItem(const Item* item, int refid)
{
	Unit* unit;
	int slot_id;

	if(FindItemInTeam(item, refid, &unit, &slot_id))
	{
		RemoveItem(*unit, slot_id, 1);
		return true;
	}
	else
		return false;
}

void Game::EndUniqueQuest()
{
	++unique_quests_completed;
}

Room& Game::GetRoom(InsideLocationLevel& lvl, RoomTarget target, bool down_stairs)
{
	if(target == RoomTarget::None)
		return lvl.GetFarRoom(down_stairs);
	else
	{
		int id = lvl.FindRoomId(target);
		if(id == -1)
		{
			assert(0);
			id = 0;
		}

		return lvl.rooms[id];
	}
}

void Game::AddNews(cstring text)
{
	assert(text);

	News* n = new News;
	n->text = text;
	n->add_time = worldtime;

	news.push_back(n);
}

void Game::UpdateGame2(float dt)
{
	// arena
	if(arena_tryb != Arena_Brak)
	{
		if(arena_etap == Arena_OdliczanieDoPrzeniesienia)
		{
			arena_t += dt*2;
			if(arena_t >= 1.f)
			{
				if(arena_tryb == Arena_Walka)
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						if((*it)->in_arena == 0)
						{
							UnitWarpData& uwd = Add1(unit_warp_data);
							uwd.unit = *it;
							uwd.where = -2;
						}
					}
				}
				else
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						UnitWarpData& uwd = Add1(unit_warp_data);
						uwd.unit = *it;
						uwd.where = -2;
					}

					at_arena[0]->in_arena = 0;
					at_arena[1]->in_arena = 1;

					if(IsOnline())
					{
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = at_arena[0];
						}
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = at_arena[1];
						}
					}
				}

				arena_etap = Arena_OdliczanieDoStartu;
				arena_t = 0.f;
			}
		}
		else if(arena_etap == Arena_OdliczanieDoStartu)
		{
			arena_t += dt;
			if(arena_t >= 2.f)
			{
				if(sound_volume)
					PlaySound2d(sArenaFight);
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ARENA_SOUND;
					c.id = 0;
				}
				arena_etap = Arena_TrwaWalka;
				for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
				{
					(*it)->frozen = 0;
					if((*it)->IsPlayer() && (*it)->player != pc)
					{
						NetChangePlayer& c = Add1(net_changes_player);
						c.type = NetChangePlayer::START_ARENA_COMBAT;
						c.pc = (*it)->player;
						GetPlayerInfo(c.pc).NeedUpdate();
					}
				}
			}
		}
		else if(arena_etap == Arena_TrwaWalka)
		{
			// gadanie przez obserwatorów
			for(vector<Unit*>::iterator it = arena_viewers.begin(), end = arena_viewers.end(); it != end; ++it)
			{
				Unit& u = **it;
				u.ai->loc_timer -= dt;
				if(u.ai->loc_timer <= 0.f)
				{
					u.ai->loc_timer = random(6.f,12.f);

					cstring text;
					if(rand2()%2 == 0)
						text = random_string(txArenaText);
					else
						text = Format(random_string(txArenaTextU), GetRandomArenaHero()->GetRealName());

					UnitTalk(u, text);
				}
			}

			// sprawdŸ ile osób jeszcze ¿yje
			int ile[2] = {0};
			for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			{
				if((*it)->live_state != Unit::DEAD)
					++ile[(*it)->in_arena];
			}

			if(ile[0] == 0 || ile[1] == 0)
			{
				arena_etap = Arena_OdliczanieDoKonca;
				arena_t = 0.f;
				bool victory_sound;
				if(ile[0] == 0)
				{
					arena_wynik = 1;
					if(arena_tryb == Arena_Walka)
						victory_sound = false;
					else
						victory_sound = true;
				}
				else
				{
					arena_wynik = 0;
					victory_sound = true;
				}

				if(sound_volume)
					PlaySound2d(victory_sound ? sArenaWin : sArenaLost);
				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::ARENA_SOUND;
					c.id = victory_sound ? 1 : 2;
				}
			}
		}
		else if(arena_etap == Arena_OdliczanieDoKonca)
		{
			arena_t += dt;
			if(arena_t >= 2.f)
			{
				for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
				{
					(*it)->frozen = 2;
					if((*it)->IsPlayer())
					{
						if((*it)->player == pc)
						{
							fallback_co = FALLBACK_ARENA_EXIT;
							fallback_t = -1.f;
						}
						else
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::EXIT_ARENA;
							c.pc = (*it)->player;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}

				arena_etap = Arena_OdliczanieDoWyjscia;
				arena_t = 0.f;
			}
		}
		else
		{
			arena_t += dt*2;
			if(arena_t >= 1.f)
			{
				if(arena_tryb == Arena_Walka)
				{
					if(arena_wynik == 0)
					{
						int ile;
						switch(arena_poziom)
						{
						case 1:
							ile = 150;
							break;
						case 2:
							ile = 350;
							break;
						case 3:
							ile = 750;
							break;
						}

						AddGoldArena(ile);
					}

					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						if((*it)->in_arena == 0)
						{
							(*it)->frozen = 0;
							(*it)->in_arena = -1;
							if((*it)->hp <= 0.f)
							{
								(*it)->HealPoison();
								(*it)->live_state = Unit::ALIVE;
								(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
								(*it)->action = A_ANIMATION;
								if((*it)->IsAI())
									(*it)->ai->Reset();
								if(IsOnline())
								{
									NetChange& c = Add1(net_changes);
									c.type = NetChange::STAND_UP;
									c.unit = *it;
								}
							}

							UnitWarpData& warp_data = Add1(unit_warp_data);
							warp_data.unit = *it;
							warp_data.where = -1;

							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::CHANGE_ARENA_STATE;
								c.unit = *it;
							}
						}
						else
						{
							to_remove.push_back(*it);
							(*it)->to_remove = true;
							if(IsOnline())
								Net_RemoveUnit(*it);
						}
					}
				}
				else
				{
					for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
					{
						(*it)->frozen = 0;
						(*it)->in_arena = -1;
						if((*it)->hp <= 0.f)
						{
							(*it)->HealPoison();
							(*it)->live_state = Unit::ALIVE;
							(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
							(*it)->action = A_ANIMATION;
							if((*it)->IsAI())
								(*it)->ai->Reset();
							if(IsOnline())
							{
								NetChange& c = Add1(net_changes);
								c.type = NetChange::STAND_UP;
								c.unit = *it;
							}
						}

						UnitWarpData& warp_data = Add1(unit_warp_data);
						warp_data.unit = *it;
						warp_data.where = -1;

						if(IsOnline())
						{
							NetChange& c = Add1(net_changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = *it;
						}
					}

					if(arena_tryb == Arena_Pvp && arena_fighter->IsHero())
					{
						arena_fighter->hero->lost_pvp = (arena_wynik == 0);
						StartDialog2(pvp_player, arena_fighter, FindDialog(IS_SET(arena_fighter->data->flags, F_CRAZY) ? "crazy_pvp" : "hero_pvp"));
					}
				}

				if(arena_tryb != Arena_Zawody)
				{
					RemoveArenaViewers();
					arena_viewers.clear();
					at_arena.clear();
				}
				else
					tournament_state3 = 5;
				arena_tryb = Arena_Brak;
				arena_free = true;
			}
		}
	}

	// zawody
	if(tournament_state != TOURNAMENT_NOT_DONE)
		UpdateTournament(dt);

	// sharing of team items between team members
	UpdateTeamItemShares();

	// zawody w piciu
	UpdateContest(dt);

	// quest bandits
	if(quest_bandits->bandits_state == Quest_Bandits::State::Counting)
	{
		quest_bandits->timer -= dt;
		if(quest_bandits->timer <= 0.f)
		{
			// spawn agent
			Unit* u = SpawnUnitNearLocation(GetContext(*leader), leader->pos, *FindUnitData("agent"), &leader->pos, -2, 2.f);
			if(u)
			{
				if(IsOnline())
					Net_SpawnUnit(u);
				quest_bandits->bandits_state = Quest_Bandits::State::AgentCome;
				quest_bandits->agent = u;
				u->StartAutoTalk(true);
			}
		}
	}

	// quest mages
	if(quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined && current_location == quest_mages2->target_loc)
	{
		quest_mages2->timer += dt;
		if(quest_mages2->timer >= 30.f && quest_mages2->scholar->auto_talk == AutoTalkMode::No)
			quest_mages2->scholar->StartAutoTalk();
	}

	// quest evil
	if(quest_evil->evil_state == Quest_Evil::State::SpawnedAltar && current_location == quest_evil->target_loc)
	{
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			Unit& u = **it;
			if(u.IsStanding() && u.IsPlayer() && distance(u.pos, quest_evil->pos) < 5.f && CanSee(u.pos, quest_evil->pos))
			{
				quest_evil->evil_state = Quest_Evil::State::Summoning;
				if(sound_volume)
					PlaySound2d(sEvil);
				quest_evil->SetProgress(Quest_Evil::Progress::AltarEvent);
				// spawn undead
				InsideLocation* inside = (InsideLocation*)location;
				inside->spawn = SG_NIEUMARLI;
				uint offset = local_ctx.units->size();
				GenerateDungeonUnits();
				if(IsOnline())
				{
					PushNetChange(NetChange::EVIL_SOUND);
					for(uint i=offset, ile=local_ctx.units->size(); i<ile; ++i)
						Net_SpawnUnit(local_ctx.units->at(i));
				}
				break;
			}
		}
	}
	else if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals && !location->outside && location->GetLastLevel() == dungeon_level)
	{
		int d = quest_evil->GetLocId(current_location);
		if(d != -1)
		{
			Quest_Evil::Loc& loc = quest_evil->loc[d];
			if(loc.state != 3)
			{
				Unit* u = quest_evil->cleric;

				if(u->ai->state == AIController::Idle)
				{
					// sprawdŸ czy podszed³ do portalu
					float dist = distance2d(u->pos, loc.pos);
					if(dist < 5.f)
					{
						// podejdŸ
						u->ai->idle_action = AIController::Idle_Move;
						u->ai->idle_data.pos.Build(loc.pos);
						u->ai->timer = 1.f;
						u->hero->mode = HeroData::Wait;

						// zamknij
						if(dist < 2.f)
						{
							quest_evil->timer -= dt;
							if(quest_evil->timer <= 0.f)
							{
								loc.state = Quest_Evil::Loc::State::PortalClosed;
								u->hero->mode = HeroData::Follow;
								u->ai->idle_action = AIController::Idle_None;
								quest_evil->msgs.push_back(Format(txPortalClosed, location->name.c_str()));
								game_gui->journal->NeedUpdate(Journal::Quests, quest_evil->quest_index);
								AddGameMsg3(GMS_JOURNAL_UPDATED);
								u->StartAutoTalk();
								quest_evil->changed = true;
								++quest_evil->closed;
								delete location->portal;
								location->portal = nullptr;
								AddNews(Format(txPortalClosedNews, location->name.c_str()));
								if(IsOnline())
								{
									Net_UpdateQuest(quest_evil->refid);
									PushNetChange(NetChange::CLOSE_PORTAL);
								}
							}
						}
						else
							quest_evil->timer = 1.5f;
					}
					else
						u->hero->mode = HeroData::Follow;
				}
			}
		}
	}

	// sekret
	if(secret_state == SECRET_FIGHT)
	{
		int ile[2] = {0};

		for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
		{
			if((*it)->IsStanding())
				ile[(*it)->in_arena]++;
		}

		if(at_arena[0]->hp < 10.f)
			ile[1] = 0;

		if(ile[0] == 0 || ile[1] == 0)
		{
			// o¿yw wszystkich
			for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			{
				(*it)->in_arena = -1;
				if((*it)->hp <= 0.f)
				{
					(*it)->HealPoison();
					(*it)->live_state = Unit::ALIVE;
					(*it)->ani->Play("wstaje2", PLAY_ONCE|PLAY_PRIO3, 0);
					(*it)->action = A_ANIMATION;
					if((*it)->IsAI())
						(*it)->ai->Reset();
					if(IsOnline())
					{
						NetChange& c = Add1(net_changes);
						c.type = NetChange::STAND_UP;
						c.unit = *it;
					}
				}

				if(IsOnline())
				{
					NetChange& c = Add1(net_changes);
					c.type = NetChange::CHANGE_ARENA_STATE;
					c.unit = *it;
				}
			}

			at_arena[0]->hp = at_arena[0]->hpmax;
			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = at_arena[0];
			}

			if(ile[0])
			{
				// gracz wygra³
				secret_state = SECRET_WIN;
				at_arena[0]->StartAutoTalk();
			}
			else
			{
				// gracz przegra³
				secret_state = SECRET_LOST;
			}

			at_arena.clear();
		}
	}

	// main quest
	if(IsLocal())
	{
		Quest_Main* q = (Quest_Main*)FindQuestById(Q_MAIN);
		if(q && q->state == Quest::Hidden)
		{
			q->timer += dt;
			if(q->timer >= 0.1f)
				q->SetProgress(0);
		}
	}
}

void Game::UpdateContest(float dt)
{
	if(contest_state == CONTEST_STARTING)
	{
		int id;
		InsideBuilding* inn = city_ctx->FindInn(id);
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));

		if(innkeeper.busy == Unit::Busy_No)
		{
			float prev = contest_time;
			contest_time += dt;
			if(prev < 5.f && contest_time >= 5.f)
				UnitTalk(innkeeper, txContestStart);
		}

		if(contest_time >= 15.f && innkeeper.busy != Unit::Busy_Talking)
		{
			contest_state = CONTEST_IN_PROGRESS;
			// zbierz jednostki
			for(vector<Unit*>::iterator it = inn->ctx.units->begin(), end = inn->ctx.units->end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsStanding() && u.IsAI() && !u.event_handler && u.frozen == 0 && u.busy == Unit::Busy_No)
				{
					bool ok = false;
					if(IS_SET(u.data->flags2, F2_CONTEST))
						ok = true;
					else if(IS_SET(u.data->flags2, F2_CONTEST_50))
					{
						if(rand2()%2 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flags3, F3_CONTEST_25))
					{
						if(rand2()%4 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE))
					{
						if(quest_mages2->mages_state < Quest_Mages2::State::MageCured)
							ok = true;
					}

					if(ok)
						contest_units.push_back(*it);
				}
			}
			contest_state2 = 0;

			// patrzenie siê postaci i usuniêcie zajêtych
			bool removed = false;
			for(vector<Unit*>::iterator it = contest_units.begin(), end = contest_units.end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.in_building != id || u.frozen != 0 || !u.IsStanding())
				{
					*it = nullptr;
					removed = true;
				}
				else
				{
					u.busy = Unit::Busy_Yes;
					u.look_target = &innkeeper;
					u.event_handler = this;
					if(u.IsPlayer())
					{
						BreakAction(u, false, true);
						if(u.player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::LOOK_AT;
							c.pc = u.player;
							c.id = innkeeper.netid;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
			}
			if(removed)
				RemoveNullElements(contest_units);

			// jeœli jest za ma³o ludzi
			if(contest_units.size() <= 1u)
			{
				contest_state = CONTEST_FINISH;
				contest_state2 = 3;
				innkeeper.ai->idle_action = AIController::Idle_Rot;
				innkeeper.ai->idle_data.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				UnitTalk(innkeeper, txContestNoPeople);
			}
			else
			{
				innkeeper.ai->idle_action = AIController::Idle_Rot;
				innkeeper.ai->idle_data.rot = innkeeper.ai->start_rot;
				innkeeper.ai->timer = 3.f;
				innkeeper.busy = Unit::Busy_Yes;
				UnitTalk(innkeeper, txContestTalk[0]);
			}
		}
	}
	else if(contest_state == CONTEST_IN_PROGRESS)
	{
		InsideBuilding* inn = city_ctx->FindInn();
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));
		bool talking = true;
		cstring next_text = nullptr, next_drink = nullptr;

		switch(contest_state2)
		{
		case 0:
			next_text = txContestTalk[1];
			break;
		case 1:
			next_text = txContestTalk[2];
			break;
		case 2:
			next_text = txContestTalk[3];
			break;
		case 3:
			next_drink = "beer";
			break;
		case 4:
			talking = false;
			next_text = txContestTalk[4];
			break;
		case 5:
			next_drink = "beer";
			break;
		case 6:
			talking = false;
			next_text = txContestTalk[5];
			break;
		case 7:
			next_drink = "beer";
			break;
		case 8:
			talking = false;
			next_text = txContestTalk[6];
			break;
		case 9:
			next_drink = "vodka";
			break;
		case 10:
			talking = false;
			next_text = txContestTalk[7];
			break;
		case 11:
			next_drink = "vodka";
			break;
		case 12:
			talking = false;
			next_text = txContestTalk[8];
			break;
		case 13:
			next_drink = "vodka";
			break;
		case 14:
			talking = false;
			next_text = txContestTalk[9];
			break;
		case 15:
			next_text = txContestTalk[10];
			break;
		case 16:
			next_drink = "spirit";
			break;
		case 17:
			talking = false;
			next_text = txContestTalk[11];
			break;
		case 18:
			next_drink = "spirit";
			break;
		case 19:
			talking = false;
			next_text = txContestTalk[12];
			break;
		default:
			if((contest_state2-20)%2 == 0)
			{
				if(contest_state2 != 20)
					talking = false;
				next_text = txContestTalk[13];
			}
			else
				next_drink = "spirit";
			break;
		}

		if(talking)
		{
			if(!innkeeper.bubble)
			{
				if(next_text)
					UnitTalk(innkeeper, next_text);
				else
				{
					assert(next_drink);
					contest_time = 0.f;
					const Consumable& drink = FindItem(next_drink)->ToConsumable();
					for(vector<Unit*>::iterator it = contest_units.begin(), end = contest_units.end(); it != end; ++it)
						(*it)->ConsumeItem(drink, true);
				}

				++contest_state2;
			}
		}
		else
		{
			contest_time += dt;
			if(contest_time >= 5.f)
			{
				if(contest_units.size() >= 2)
				{
					assert(next_text);
					UnitTalk(innkeeper, next_text);
					++contest_state2;
				}
				else if(contest_units.size() == 1)
				{
					contest_state = CONTEST_FINISH;
					contest_state2 = 0;
					innkeeper.look_target = contest_units.back();
					AddNews(Format(txContestWinNews, contest_units.back()->GetName()));
					UnitTalk(innkeeper, txContestWin);
				}
				else
				{
					contest_state = CONTEST_FINISH;
					contest_state2 = 1;
					AddNews(txContestNoWinner);
					UnitTalk(innkeeper, txContestNoWinner);
				}
			}
		}
	}
	else if(contest_state == CONTEST_FINISH)
	{
		InsideBuilding* inn = city_ctx->FindInn();
		Unit& innkeeper = *inn->FindUnit(FindUnitData("innkeeper"));

		if(!innkeeper.bubble)
		{
			switch(contest_state2)
			{
			case 0: // wygrana
				contest_state2 = 2;
				UnitTalk(innkeeper, txContestPrize);
				break;
			case 1: // remis
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = nullptr;
				contest_state = CONTEST_DONE;
				contest_generated = false;
				contest_winner = nullptr;
				break;
			case 2: // wygrana2
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = nullptr;
				contest_winner = contest_units.back();
				contest_units.clear();
				contest_state = CONTEST_DONE;
				contest_generated = false;
				contest_winner->look_target = nullptr;
				contest_winner->busy = Unit::Busy_No;
				contest_winner->event_handler = nullptr;
				break;
			case 3: // brak ludzi
				for(vector<Unit*>::iterator it = contest_units.begin(), end = contest_units.end(); it != end; ++it)
				{
					Unit& u = **it;
					u.busy = Unit::Busy_No;
					u.look_target = nullptr;
					u.event_handler = nullptr;
					if(u.IsPlayer())
					{
						BreakAction(u, false, true);
						if(u.player != pc)
						{
							NetChangePlayer& c = Add1(net_changes_player);
							c.type = NetChangePlayer::LOOK_AT;
							c.pc = u.player;
							c.id = -1;
							GetPlayerInfo(c.pc).NeedUpdate();
						}
					}
				}
				contest_state = CONTEST_DONE;
				innkeeper.busy = Unit::Busy_No;
				break;
			}
		}
	}
}

void Game::SetUnitWeaponState(Unit& u, bool wyjmuje, WeaponType co)
{
	if(wyjmuje)
	{
		switch(u.weapon_state)
		{
		case WS_HIDDEN:
			// wyjmij bron
			u.ani->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE|PLAY_PRIO1, 1);
			u.action = A_TAKE_WEAPON;
			u.weapon_taken = co;
			u.weapon_state = WS_TAKING;
			u.animation_state = 0;
			break;
		case WS_HIDING:
			if(u.weapon_hiding == co)
			{
				if(u.animation_state == 0)
				{
					// jeszcze nie schowa³ tej broni, wy³¹cz grupê
					u.action = A_NONE;
					u.weapon_taken = u.weapon_hiding;
					u.weapon_hiding = W_NONE;
					u.weapon_state = WS_TAKEN;
					u.ani->Deactivate(1);
				}
				else
				{
					// schowa³ broñ, zacznij wyci¹gaæ
					u.weapon_taken = u.weapon_hiding;
					u.weapon_hiding = W_NONE;
					u.weapon_state = WS_TAKING;
					CLEAR_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
				}
			}
			else
			{
				// chowa broñ, zacznij wyci¹gaæ
				u.ani->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE|PLAY_PRIO1, 1);
				u.action = A_TAKE_WEAPON;
				u.weapon_taken = co;
				u.weapon_hiding = W_NONE;
				u.weapon_state = WS_TAKING;
				u.animation_state = 0;
			}
			break;
		case WS_TAKING:
		case WS_TAKEN:
			if(u.weapon_taken != co)
			{
				// wyjmuje z³¹ broñ, zacznij wyjmowaæ dobr¹
				// lub
				// powinien mieæ wyjêt¹ broñ, ale nie t¹!
				u.ani->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE|PLAY_PRIO1, 1);
				u.action = A_TAKE_WEAPON;
				u.weapon_taken = co;
				u.weapon_hiding = W_NONE;
				u.weapon_state = WS_TAKING;
				u.animation_state = 0;
			}
			break;
		}
	}
	else // chowa
	{
		switch(u.weapon_state)
		{
		case WS_HIDDEN:
			// schowana to schowana, nie ma co sprawdzaæ czy to ta
			break;
		case WS_HIDING:
			if(u.weapon_hiding != co)
			{
				// chowa z³¹ broñ, zamieñ
				u.weapon_hiding = co;
			}
			break;
		case WS_TAKING:
			if(u.animation_state == 0)
			{
				// jeszcze nie wyj¹³ broni z pasa, po prostu wy³¹cz t¹ grupe
				u.action = A_NONE;
				u.weapon_taken = W_NONE;
				u.weapon_state = WS_HIDDEN;
				u.ani->Deactivate(1);
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				u.weapon_hiding = u.weapon_taken;
				u.weapon_taken = W_NONE;
				u.weapon_state = WS_HIDING;
				u.animation_state = 0;
				SET_BIT(u.ani->groups[1].state, AnimeshInstance::FLAG_BACK);
			}
			break;
		case WS_TAKEN:
			// zacznij chowaæ
			u.ani->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE|PLAY_BACK|PLAY_PRIO1, 1);
			u.weapon_hiding = co;
			u.weapon_taken = W_NONE;
			u.weapon_state = WS_HIDING;
			u.action = A_TAKE_WEAPON;
			u.animation_state = 0;
			break;
		}
	}
}

void Game::AddGameMsg3(GMS id)
{
	cstring text;
	float time = 3.f;
	bool repeat = false;

	switch(id)
	{
	case GMS_IS_LOOTED:
		text = txGmsLooted;
		break;
	case GMS_ADDED_RUMOR:
		repeat = true;
		text = txGmsRumor;
		break;
	case GMS_JOURNAL_UPDATED:
		repeat = true;
		text = txGmsJournalUpdated;
		break;
	case GMS_USED:
		text = txGmsUsed;
		time = 2.f;
		break;
	case GMS_UNIT_BUSY:
		text = txGmsUnitBusy;
		break;
	case GMS_GATHER_TEAM:
		text = txGmsGatherTeam;
		break;
	case GMS_NOT_LEADER:
		text = txGmsNotLeader;
		break;
	case GMS_NOT_IN_COMBAT:
		text = txGmsNotInCombat;
		break;
	case GMS_ADDED_ITEM:
		text = txGmsAddedItem;
		repeat = true;
		break;
	default:
		assert(0);
		return;
	}

	if(repeat)
		AddGameMsg(text, time);
	else
		AddGameMsg2(text, time, id);
}

void Game::UpdatePlayerView()
{
	LevelContext& ctx = GetContext(*pc->unit);
	Unit& u = *pc->unit;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		bool mark = false;
		if(IsEnemy(u, u2))
		{
			if(u2.IsAlive())
				mark = true;
		}
		else if(IsFriend(u, u2))
			mark = true;

		// oznaczanie pobliskich postaci
		if(mark)
		{
			float dist = distance(u.visual_pos, u2.visual_pos);

			if(dist < ALERT_RANGE.x && cam.frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && CanSee(u, u2))
			{
				// dodaj do pobliskich jednostek
				bool jest = false;
				for(vector<UnitView>::iterator it2 = unit_views.begin(), end2 = unit_views.end(); it2 != end2; ++it2)
				{
					if(it2->unit == *it)
					{
						jest = true;
						it2->valid = true;
						it2->last_pos = u2.GetUnitTextPos();
						break;
					}
				}
				if(!jest)
				{
					UnitView& uv = Add1(unit_views);
					uv.valid = true;
					uv.unit = *it;
					uv.time = 0.f;
					uv.last_pos = u2.GetUnitTextPos();
				}
			}
		}
	}
}

void Game::OnCloseInventory()
{
	if(inventory_mode == I_TRADE)
	{
		if(IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			PushNetChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_SHARE || inventory_mode == I_GIVE)
	{
		if(IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			PushNetChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_LOOT_CHEST && IsLocal())
	{
		pc->action_chest->looted = false;
		pc->action_chest->ani->Play(&pc->action_chest->ani->ani->anims[0], PLAY_PRIO1|PLAY_ONCE|PLAY_STOP_AT_END|PLAY_BACK, 0);
		if(sound_volume)
		{
			VEC3 pos = pc->action_chest->pos;
			pos.y += 0.5f;
			PlaySound3d(sChestClose, pos, 2.f, 5.f);
		}
	}

	if(IsOnline() && (inventory_mode == I_LOOT_BODY || inventory_mode == I_LOOT_CHEST))
	{
		if(IsClient())
			PushNetChange(NetChange::STOP_TRADE);
		else if(inventory_mode == I_LOOT_BODY)
			pc->action_unit->busy = Unit::Busy_No;
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHEST_CLOSE;
			c.id = pc->action_chest->netid;
		}
	}

	pc->action = PlayerController::Action_None;
	inventory_mode = I_NONE;
}

// zamyka ekwipunek i wszystkie okna które on móg³by utworzyæ
void Game::CloseInventory(bool do_close)
{
	if(do_close)
		OnCloseInventory();
	inventory_mode = I_NONE;
	if(game_gui)
	{
		game_gui->inventory->Hide();
		game_gui->gp_trade->Hide();
	}
}

void Game::CloseAllPanels(bool close_mp_box)
{
	if(game_gui)
		game_gui->ClosePanels(close_mp_box);
}

bool Game::CanShowEndScreen()
{
	if(IsLocal())
		return !unique_completed_show && unique_quests_completed == UNIQUE_QUESTS && city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
	else
		return unique_completed_show && city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
}

void Game::UpdateGameDialogClient()
{
	if(dialog_context.show_choices)
	{
		if(game_gui->UpdateChoice(dialog_context, dialog_choices.size()))
		{
			dialog_context.show_choices = false;
			dialog_context.dialog_text = "";

			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHOICE;
			c.id = dialog_context.choice_selected;
		}
	}
	else if(dialog_context.dialog_wait > 0.f && dialog_context.skip_id != -1)
	{
		if(KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || KeyPressedReleaseAllowed(GK_ATTACK_USE) ||
			(AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = dialog_context.skip_id;
			dialog_context.skip_id = -1;
		}
	}
}

LevelContext& Game::GetContextFromInBuilding(int in_building)
{
	if(in_building == -1)
		return local_ctx;
	assert(city_ctx);
	return city_ctx->inside_buildings[in_building]->ctx;
}

bool Game::FindQuestItem2(Unit* unit, cstring id, Quest** out_quest, int* i_index, bool not_active)
{
	assert(unit && id);

	if(id[1] == '$')
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(unit->slots[i] && unit->slots[i]->IsQuest())
			{
				Quest* quest = FindQuest(unit->slots[i]->refid, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za³o¿onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = unit->items.begin(), end2 = unit->items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && it2->item->IsQuest())
			{
				Quest* quest = FindQuest(it2->item->refid, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem2(id))
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}
	else
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(unit->slots[i] && unit->slots[i]->IsQuest() && unit->slots[i]->id == id)
			{
				Quest* quest = FindQuest(unit->slots[i]->refid, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = SlotToIIndex(ITEM_SLOT(i));
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}

		// szukaj w nie za³o¿onych
		int index = 0;
		for(vector<ItemSlot>::iterator it2 = unit->items.begin(), end2 = unit->items.end(); it2 != end2; ++it2, ++index)
		{
			if(it2->item && it2->item->IsQuest() && it2->item->id == id)
			{
				Quest* quest = FindQuest(it2->item->refid, !not_active);
				if(quest && (not_active || quest->IsActive()) && quest->IfHaveQuestItem())
				{
					if(i_index)
						*i_index = index;
					if(out_quest)
						*out_quest = quest;
					return true;
				}
			}
		}
	}

	return false;
}

bool Game::Cheat_KillAll(int typ, Unit& unit, Unit* ignore)
{
	if(!in_range(typ, 0, 1))
		return false;

	if(!IsLocal())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHEAT_KILLALL;
		c.id = typ;
		c.unit = ignore;
		return true;
	}

	LevelContext& ctx = GetContext(unit);

	switch(typ)
	{
	case 0:
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && IsEnemy(**it, unit) && *it != ignore)
				GiveDmg(ctx, nullptr, (*it)->hp, **it, nullptr);
		}
		if(city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
			{
				for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
				{
					if((*it)->IsAlive() && IsEnemy(**it, unit) && *it != ignore)
						GiveDmg((*it2)->ctx, nullptr, (*it)->hp, **it, nullptr);
				}
			}
		}
		break;
	case 1:
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && !(*it)->IsPlayer() && *it != ignore)
				GiveDmg(ctx, nullptr, (*it)->hp, **it, nullptr);
		}
		if(city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2; ++it2)
			{
				for(vector<Unit*>::iterator it = (*it2)->units.begin(), end = (*it2)->units.end(); it != end; ++it)
				{
					if((*it)->IsAlive() && !(*it)->IsPlayer() && *it != ignore)
						GiveDmg((*it2)->ctx, nullptr, (*it)->hp, **it, nullptr);
				}
			}
		}
		break;
	}
	
	return true;
}

void Game::Event_Pvp(int id)
{
	dialog_pvp = nullptr;
	if(!pvp_response.ok)
		return;

	if(IsServer())
	{
		if(id == BUTTON_YES)
		{
			// zaakceptuj pvp
			StartPvp(pvp_response.from->player, pvp_response.to);
		}
		else
		{
			// nie akceptuj pvp
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::NO_PVP;
			c.pc = pvp_response.from->player;
			c.id = pvp_response.to->player->id;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}
	else
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::PVP;
		c.unit = pvp_unit;
		if(id == BUTTON_YES)
			c.id = 1;
		else
			c.id = 0;
	}

	pvp_response.ok = false;
}

void Game::Cheat_Reveal()
{
	int index = 0;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		if(*it && (*it)->state == LS_UNKNOWN)
		{
			(*it)->state = LS_KNOWN;
			if(IsOnline())
				Net_ChangeLocationState(index, false);
		}
	}
}

void Game::Cheat_ShowMinimap()
{
	if(!location->outside)
	{
		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

		for(int y=0; y<lvl.h; ++y)
		{
			for(int x=0; x<lvl.w; ++x)
				minimap_reveal.push_back(INT2(x, y));
		}

		UpdateDungeonMinimap(false);

		if(IsLocal() && IsOnline())
			PushNetChange(NetChange::CHEAT_SHOW_MINIMAP);
	}
}

void Game::StartPvp(PlayerController* player, Unit* unit)
{
	arena_free = false;
	arena_tryb = Arena_Pvp;
	arena_etap = Arena_OdliczanieDoPrzeniesienia;
	arena_t = 0.f;
	at_arena.clear();

	// fallback gracza
	if(player == pc)
	{
		fallback_co = FALLBACK_ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ENTER_ARENA;
		c.pc = player;
		GetPlayerInfo(player).NeedUpdate();
	}

	// fallback postaci
	if(unit->IsPlayer())
	{
		if(unit->player == pc)
		{
			fallback_co = FALLBACK_ARENA;
			fallback_t = -1.f;
		}
		else
		{
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::ENTER_ARENA;
			c.pc = unit->player;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}

	// dodaj do areny
	player->unit->frozen = 2;
	player->arena_fights++;
	if(IsOnline())
		player->stat_flags |= STAT_ARENA_FIGHTS;
	at_arena.push_back(player->unit);
	unit->frozen = 2;
	at_arena.push_back(unit);
	if(unit->IsHero())
		unit->hero->following = player->unit;
	else if(unit->IsPlayer())
	{
		unit->player->arena_fights++;
		if(IsOnline())
			unit->player->stat_flags |= STAT_ARENA_FIGHTS;
	}
	pvp_player = player;
	arena_fighter = unit;

	// stwórz obserwatorów na arenie na podstawie poziomu postaci
	player->unit->level = player->unit->CalculateLevel();
	if(unit->IsPlayer())
		unit->level = unit->CalculateLevel();
	int level = max(player->unit->level, unit->level);

	if(level < 7)
		SpawnArenaViewers(1);
	else if(level < 14)
		SpawnArenaViewers(2);
	else
		SpawnArenaViewers(3);
}

void Game::UpdateGameNet(float dt)
{
	if(IsServer())
		UpdateServer(dt);
	else
		UpdateClient(dt);
}

void Game::CheckCredit(bool require_update, bool ignore)
{
	if(active_team.size() > 1)
	{
		int ile = GetCredit(*active_team.front());

		for(vector<Unit*>::iterator it = active_team.begin()+1, end = active_team.end(); it != end; ++it)
		{
			int kredyt = GetCredit(**it);
			if(kredyt < ile)
				ile = kredyt;
		}

		if(ile > 0)
		{
			require_update = true;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
				GetCredit(**it) -= ile;
		}
	}
	else
	{
		pc->unit->gold += team_gold;
		team_gold = 0;
		pc->credit = 0;
	}

	if(!ignore && require_update && IsOnline())
		PushNetChange(NetChange::UPDATE_CREDIT);
}

void Game::StartDialog2(PlayerController* player, Unit* talker, GameDialog* dialog)
{
	assert(player && talker);

	DialogContext& ctx = *player->dialog_ctx;
	assert(!ctx.dialog_mode);

	if(player != pc)
		Net_StartDialog(player, talker);
	StartDialog(ctx, talker, dialog);
}

void Game::UpdateUnitPhysics(Unit& unit, const VEC3& pos)
{
	btVector3 a_min, a_max, bpos(ToVector3(pos));
	bpos.setY(pos.y + max(MIN_H, unit.GetUnitHeight())/2);
	unit.cobj->getWorldTransform().setOrigin(bpos);
	unit.cobj->getCollisionShape()->getAabb(unit.cobj->getWorldTransform(), a_min, a_max);
	phy_broadphase->setAabb(unit.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);
}

void Game::WarpNearLocation(LevelContext& ctx, Unit& unit, const VEC3& pos, float extra_radius, bool allow_exact, int tries)
{
	const float radius = unit.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = {0};
	const Unit* ignore_units[2] = {&unit, nullptr};
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, extra_radius+radius, &ignore);

	VEC3 tmp_pos = pos;
	if(!allow_exact)
	{
		int j = rand2()%poisson_disc_count;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}

	for(int i=0; i<tries; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
			break;

		int j = rand2()%poisson_disc_count;
		tmp_pos = pos;
		tmp_pos.x += POISSON_DISC_2D[j].x*extra_radius;
		tmp_pos.z += POISSON_DISC_2D[j].y*extra_radius;
	}

	unit.pos = tmp_pos;
	MoveUnit(unit, true);
	unit.visual_pos = unit.pos;

	if(unit.cobj)
		UpdateUnitPhysics(unit, unit.pos);
}

void Game::ProcessRemoveUnits()
{
	for(vector<Unit*>::iterator it = to_remove.begin(), end = to_remove.end(); it != end; ++it)
	{
		if((*it)->interp)
			interpolators.Free((*it)->interp);
		DeleteUnit(*it);
	}
	to_remove.clear();
}

/* mode: 0 - normal training
1 - gain 1 point (tutorial)
2 - more points (potion) */
void Game::Train(Unit& unit, bool is_skill, int co, int mode)
{
	int value, *train_points, *train_next;
	if(is_skill)
	{
		if(unit.unmod_stats.skill[co] == SkillInfo::MAX)
		{
			unit.player->sp[co] = unit.player->sn[co];
			return;
		}
		value = unit.unmod_stats.skill[co];
		train_points = &unit.player->sp[co];
		train_next = &unit.player->sn[co];
	}
	else
	{
		if(unit.unmod_stats.attrib[co] == AttributeInfo::MAX)
		{
			unit.player->ap[co] = unit.player->an[co];
			return;
		}
		value = unit.unmod_stats.attrib[co];
		train_points = &unit.player->ap[co];
		train_next = &unit.player->an[co];
	}

	int ile;
	if(mode == 0)
		ile = 10-value/10;
	else if(mode == 1)
		ile = 1;
	else
		ile = 12-value/12;
	
	if(ile >= 1)
	{
		value += ile;
		*train_points /= 2;

		if(is_skill)
		{
			*train_next = GetRequiredSkillPoints(value);
			unit.Set((Skill)co, value);
		}
		else
		{
			*train_next = GetRequiredAttributePoints(value);
			unit.Set((Attribute)co, value);
		}

		if(unit.player->IsLocal())
			ShowStatGain(is_skill, co, ile);
		else
		{
			NetChangePlayer&c = AddChange(NetChangePlayer::GAIN_STAT, unit.player);
			c.id = (is_skill ? 1 : 0);
			c.a = co;
			c.ile = ile;

			NetChangePlayer& c2 = AddChange(NetChangePlayer::STAT_CHANGED, unit.player);
			c2.id = int(is_skill ? ChangedStatType::SKILL : ChangedStatType::ATTRIBUTE);
			c2.a = co;
			c2.ile = value;
		}
	}
	else
	{
		float m;
		if(ile == 0)
			m = 0.5f;
		else if(ile == -1)
			m = 0.25f;
		else
			m = 0.125f;
		float pts = m * *train_next;
		if(is_skill)
			unit.player->TrainMod2((Skill)co, pts);
		else
			unit.player->TrainMod((Attribute)co, pts);
	}
}

void Game::ShowStatGain(bool is_skill, int what, int value)
{
	cstring text, name;
	if(is_skill)
	{
		text = txGainTextSkill;
		name = g_skills[what].name.c_str();
	}
	else
	{
		text = txGainTextAttrib;
		name = g_attributes[what].name.c_str();
	}

	AddGameMsg(Format(text, name, value), 3.f);
}

void Game::ActivateChangeLeaderButton(bool activate)
{
	game_gui->team_panel->bt[2].state = (activate ? Button::NONE : Button::DISABLED);
}

void Game::RespawnTraps()
{
	for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
	{
		Trap& trap = **it;

		trap.state = 0;
		if(trap.base->type == TRAP_SPEAR)
		{
			if(trap.hitted)
				trap.hitted->clear();
			else
				trap.hitted = new vector<Unit*>;
		}
	}
}

// zmienia tylko pozycjê bo ta funkcja jest wywo³ywana przy opuszczaniu miasta
void Game::WarpToInn(Unit& unit)
{
	assert(city_ctx);

	int id;
	InsideBuilding* inn = city_ctx->FindInn(id);

	WarpToArea(inn->ctx, (rand2()%5 == 0 ? inn->arena2 : inn->arena1), unit.GetUnitRadius(), unit.pos, 20);
	unit.visual_pos = unit.pos;
	unit.in_building = id;
}

void Game::PayCredit(PlayerController* player, int ile)
{
	LocalVector<Unit*> _units;
	vector<Unit*>& units = _units;

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if(*it != player->unit)
			units.push_back(*it);
	}

	AddGold(ile, &units, false);

	player->credit -= ile;
	if(player->credit < 0)
	{
		WARN(Format("Player '%s' paid %d credit and now have %d!", player->name.c_str(), ile, player->credit));
		player->credit = 0;
#ifdef _DEBUG
		AddGameMsg("Player has invalid credit!", 5.f);
#endif
	}

	if(IsOnline())
		PushNetChange(NetChange::UPDATE_CREDIT);
}

InsideBuilding* Game::GetArena()
{
	assert(city_ctx);
	for(InsideBuilding* b : city_ctx->inside_buildings)
	{
		if(b->type->group == content::BG_ARENA)
			return b;
	}
	assert(0);
	return nullptr;
}

void Game::CreateSaveImage(cstring filename)
{
	assert(filename);

	SURFACE surf;
	V( tSave->GetSurfaceLevel(0, &surf) );

	// ustaw render target
	if(sSave)
		sCustom = sSave;
	else
		sCustom = surf;

	int old_flags = draw_flags;
	if(game_state == GS_LEVEL)
		draw_flags = (0xFFFFFFFF & ~DF_GUI & ~DF_MENU);
	else
		draw_flags = (0xFFFFFFFF & ~DF_MENU);
	OnDraw(false);
	draw_flags = old_flags;
	sCustom = nullptr;

	// kopiuj teksturê
	if(sSave)
	{
		V( tSave->GetSurfaceLevel(0, &surf) );
		V( device->StretchRect(sSave, nullptr, surf, nullptr, D3DTEXF_NONE) );
		surf->Release();
	}

	// zapisz obrazek
	V( D3DXSaveSurfaceToFile(filename, D3DXIFF_JPG, surf, nullptr, nullptr) );
	surf->Release();

	// przywróc render target
	V( device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf) );
	V( device->SetRenderTarget(0, surf) );
	surf->Release();
}

void Game::PlayerUseUseable(Useable* useable, bool after_action)
{
	Unit& u = *pc->unit;
	Useable& use = *useable;
	BaseUsable& bu = g_base_usables[use.type];

	bool ok = true;
	if(bu.item)
	{
		if(!u.HaveItem(bu.item) && u.slots[SLOT_WEAPON] != bu.item)
		{
			if(use.type == U_CAULDRON)
				AddGameMsg2(txNeedLadle, 2.f, GMS_NEED_LADLE);
			else if(use.type == U_ANVIL)
				AddGameMsg2(txNeedHammer, 2.f, GMS_NEED_HAMMER);
			else if(use.type == U_IRON_VAIN || use.type == U_GOLD_VAIN)
				AddGameMsg2(txNeedPickaxe, 2.f, GMS_NEED_PICKAXE);
			else
				AddGameMsg2(txNeedUnk, 2.f, GMS_NEED_PICKAXE);
			ok = false;
		}
		else if(pc->unit->weapon_state != WS_HIDDEN && (bu.item != &pc->unit->GetWeapon() || pc->unit->HaveShield()))
		{
			if(after_action)
				return;
			u.HideWeapon();
			pc->next_action = NA_USE;
			pc->next_action_useable = &use;
			ok = false;
		}
		else
			u.used_item = bu.item;
	}

	if(ok)
	{
		if(IsLocal())
		{
			u.action = A_ANIMATION2;
			u.animation = ANI_PLAY;
			u.ani->Play(bu.anim, PLAY_PRIO1, 0);
			u.useable = &use;
			u.useable->user = &u;
			u.target_pos = u.pos;
			u.target_pos2 = use.pos;
			if(g_base_usables[use.type].limit_rot == 4)
				u.target_pos2 -= VEC3(sin(use.rot)*1.5f,0,cos(use.rot)*1.5f);
			u.timer = 0.f;
			u.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
			u.use_rot = lookat_angle(u.pos, u.useable->pos);
			before_player = BP_NONE;

			if(IsOnline())
			{
				NetChange& c = Add1(net_changes);
				c.type = NetChange::USE_USEABLE;
				c.unit = &u;
				c.id = u.useable->netid;
				c.ile = 1;
			}
		}
		else
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::USE_USEABLE;
			c.id = before_player_ptr.useable->netid;
			c.ile = 1;
		}
	}
}

SOUND Game::GetTalkSound(Unit& u)
{
	if(IS_SET(u.data->flags2, F2_XAR))
		return sXarTalk;
	else if(u.type == Unit::HUMAN)
		return sTalk[rand2()%4];
	else if(IS_SET(u.data->flags2, F2_ORC_SOUNDS))
		return sOrcTalk;
	else if(IS_SET(u.data->flags2, F2_GOBLIN_SOUNDS))
		return sGoblinTalk;
	else if(IS_SET(u.data->flags2, F2_GOLEM_SOUNDS))
		return sGolemTalk;
	else
		return nullptr;
}

void Game::UnitTalk(Unit& u, cstring text)
{
	assert(text && IsLocal());

	game_gui->AddSpeechBubble(&u, text);

	int ani = 0;
	if(u.type == Unit::HUMAN && rand2()%3 != 0)
	{
		ani = rand2()%2+1;
		u.ani->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE|PLAY_PRIO2, 0);
		u.animation = ANI_PLAY;
		u.action = A_ANIMATION;
	}

	if(IsOnline())
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::TALK;
		c.unit = &u;
		c.str = StringPool.Get();
		*c.str = text;
		c.id = ani;
		c.ile = -1;
		net_talk.push_back(c.str);
	}
}

void Game::OnEnterLocation()
{
	Unit* talker = nullptr;
	cstring text = nullptr;

	// orc talking after entering location
	if(quest_orcs2->orcs_state == Quest_Orcs2::State::ToldAboutCamp && quest_orcs2->target_loc == current_location && quest_orcs2->talked == Quest_Orcs2::Talked::No)
	{
		quest_orcs2->talked = Quest_Orcs2::Talked::AboutCamp;
		talker = quest_orcs2->orc;
		text = txOrcCamp;
	}

	if(!talker)
	{
		TeamInfo info;
		GetTeamInfo(info);
		bool pewna_szansa = false;

		if(info.sane_heroes > 0)
		{
			switch(location->type)
			{
			case L_CITY:
				if(LocationHelper::IsCity(location))
					text = random_string(txAiCity);
				else
					text = random_string(txAiVillage);
				break;
			case L_MOONWELL:
				text = txAiMoonwell;
				break;
			case L_FOREST:
				text = txAiForest;
				break;
			case L_CAMP:
				if(location->state != LS_CLEARED)
				{
					pewna_szansa = true;
					cstring co;

					switch(location->spawn)
					{
					case SG_GOBLINY:
						co = txSGOGoblins;
						break;
					case SG_BANDYCI:
						co = txSGOBandits;
						break;
					case SG_ORKOWIE:
						co = txSGOOrcs;
						break;
					default:
						pewna_szansa = false;
						co = nullptr;
						break;
					}

					if(pewna_szansa)
						text = Format(txAiCampFull, co);
					else
						text = nullptr;
				}
				else
					text = txAiCampEmpty;
				break;
			}

			if(text && (pewna_szansa || rand2()%2 == 0))
				talker = GetRandomSaneHero();
		}
	}

	if(talker)
		UnitTalk(*talker, text);
}

void Game::OnEnterLevel()
{
	Unit* talker = nullptr;
	cstring text = nullptr;

	// cleric talking after entering location
	if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss)
	{
		if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals)
		{
			int d = quest_evil->GetLocId(current_location);
			if(d != -1)
			{
				Quest_Evil::Loc& loc = quest_evil->loc[d];

				if(dungeon_level == location->GetLastLevel())
				{
					if(loc.state < Quest_Evil::Loc::State::TalkedAfterEnterLevel)
					{
						talker = quest_evil->cleric;
						text = txPortalCloseLevel;
						loc.state = Quest_Evil::Loc::State::TalkedAfterEnterLevel;
					}
				}
				else if(dungeon_level == 0 && loc.state == Quest_Evil::Loc::State::None)
				{
					talker = quest_evil->cleric;
					text = txPortalClose;
					loc.state = Quest_Evil::Loc::State::TalkedAfterEnterLocation;
				}
			}
		}
		else if(current_location == quest_evil->target_loc && !quest_evil->told_about_boss)
		{
			quest_evil->told_about_boss = true;
			talker = quest_evil->cleric;
			text = txXarDanger;
		}
	}

	// orc talking after entering level
	if(!talker && (quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs || quest_orcs2->orcs_state == Quest_Orcs2::State::GeneratedOrcs) && current_location == quest_orcs2->target_loc)
	{
		if(dungeon_level == 0)
		{
			if(quest_orcs2->talked < Quest_Orcs2::Talked::AboutBase)
			{
				quest_orcs2->talked = Quest_Orcs2::Talked::AboutBase;
				talker = quest_orcs2->orc;
				text = txGorushDanger;
			}
		}
		else if(dungeon_level == location->GetLastLevel())
		{
			if(quest_orcs2->talked < Quest_Orcs2::Talked::AboutBoss)
			{
				quest_orcs2->talked = Quest_Orcs2::Talked::AboutBoss;
				talker = quest_orcs2->orc;
				text = txGorushCombat;
			}
		}
	}

	// old mage talking after entering location
	if(!talker && (quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined || quest_mages2->mages_state == Quest_Mages2::State::MageRecruited))
	{
		if(quest_mages2->target_loc == current_location)
		{
			if(quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined)
			{
				if(dungeon_level == 0 && quest_mages2->talked == Quest_Mages2::Talked::No)
				{
					quest_mages2->talked = Quest_Mages2::Talked::AboutHisTower;
					text = txMageHere;
				}
			}
			else
			{
				if(dungeon_level == 0)
				{
					if(quest_mages2->talked < Quest_Mages2::Talked::AfterEnter)
					{
						quest_mages2->talked = Quest_Mages2::Talked::AfterEnter;
						text = Format(txMageEnter, quest_mages2->evil_mage_name.c_str());
					}
				}
				else if(dungeon_level == location->GetLastLevel() && quest_mages2->talked < Quest_Mages2::Talked::BeforeBoss)
				{
					quest_mages2->talked = Quest_Mages2::Talked::BeforeBoss;
					text = txMageFinal;
				}
			}
		}

		if(text)
			talker = FindTeamMemberById("q_magowie_stary");
	}

	// default talking about location
	if(!talker && dungeon_level == 0 && (enter_from == ENTER_FROM_OUTSIDE || enter_from >= ENTER_FROM_PORTAL))
	{
		TeamInfo info;
		GetTeamInfo(info);

		if(info.sane_heroes > 0)
		{
			LocalString s;

			switch(location->type)
			{
			case L_DUNGEON:
			case L_CRYPT:
				{
					InsideLocation* inside = (InsideLocation*)location;
					switch(inside->target)
					{
					case HUMAN_FORT:
					case THRONE_FORT:
					case TUTORIAL_FORT:
						s = txAiFort;
						break;
					case DWARF_FORT:
						s = txAiDwarfFort;
						break;
					case MAGE_TOWER:
						s = txAiTower;
						break;
					case KOPALNIA_POZIOM:
						s = txAiArmory;
						break;
					case BANDITS_HIDEOUT:
						s = txAiHideout;
						break;
					case VAULT:
					case THRONE_VAULT:
						s = txAiVault;
						break;
					case HERO_CRYPT:
					case MONSTER_CRYPT:
						s = txAiCrypt;
						break;
					case OLD_TEMPLE:
						s = txAiTemple;
						break;
					case NECROMANCER_BASE:
						s = txAiNecromancerBase;
						break;
					case LABIRYNTH:
						s = txAiLabirynth;
						break;
					}

					if(inside->spawn == SG_BRAK)
						s += txAiNoEnemies;
					else
					{
						cstring co;

						switch(inside->spawn)
						{
						case SG_GOBLINY:
							co = txSGOGoblins;
							break;
						case SG_ORKOWIE:
							co = txSGOOrcs;
							break;
						case SG_BANDYCI:
							co = txSGOBandits;
							break;
						case SG_NIEUMARLI:
						case SG_NEKRO:
						case SG_ZLO:
							co = txSGOUndead;
							break;
						case SG_MAGOWIE:
							co = txSGOMages;
							break;
						case SG_GOLEMY:
							co = txSGOGolems;
							break;
						case SG_MAGOWIE_I_GOLEMY:
							co = txSGOMagesAndGolems;
							break;
						case SG_UNK:
							co = txSGOUnk;
							break;
						case SG_WYZWANIE:
							co = txSGOPowerfull;
							break;
						default:
							co = txSGOEnemies;
							break;
						}

						s += Format(txAiNearEnemies, co);
					}
				}
				break;
			case L_CAVE:
				s = txAiCave;
				break;
			}

			UnitTalk(*GetRandomSaneHero(), s->c_str());
			return;
		}
	}

	if(talker)
		UnitTalk(*talker, text);
}

Unit* Game::FindTeamMemberById(cstring id)
{
	UnitData* ud = FindUnitData(id);
	assert(ud);

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->data == ud)
			return *it;
	}

	assert(0);
	return nullptr;
}

Unit* Game::GetRandomArenaHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || (*it)->IsHero())
			v->push_back(*it);
	}

	return v->at(rand2()%v->size());
}

cstring Game::GetRandomIdleText(Unit& u)
{
	if(IS_SET(u.data->flags3, F3_DRUNK_MAGE) && quest_mages2->mages_state < Quest_Mages2::State::MageCured)
		return random_string(txAiDrunkMageText);

	int n = rand2()%100;
	if(n == 0)
		return random_string(txAiSecretText);

	int type = 1; // 0 - tekst hero, 1 - normalny tekst

	switch(u.data->group)
	{
	case G_CRAZIES:
		if(u.IsTeamMember())
		{
			if(n < 33)
				return random_string(txAiInsaneText);
			else if(n < 66)
				type = 0;
			else
				type = 1;
		}
		else
		{
			if(n < 50)
				return random_string(txAiInsaneText);
			else
				type = 1;
		}
		break;
	case G_CITIZENS:
		if(u.IsTeamMember())
		{
			if(u.in_building >= 0 && (IS_SET(u.data->flags, F_AI_DRUNKMAN) || IS_SET(u.data->flags3, F3_DRUNKMAN_AFTER_CONTEST)))
			{
				int id;
				if(city_ctx->FindInn(id) && id == u.in_building)
				{
					if(IS_SET(u.data->flags, F_AI_DRUNKMAN) || tournament_state != 1)
					{
						if(rand2()%3 == 0)
							return random_string(txAiDrunkText);
					}
					else
						return random_string(txAiDrunkmanText);
				}
			}
			if(n < 10)
				return random_string(txAiHumanText);
			else if(n < 55)
				type = 0;
			else
				type = 1;
		}
		else
			type = 1;
		break;
	case G_BANDITS:
		if(n < 50)
			return random_string(txAiBanditText);
		else
			type = 1;
		break;
	case G_MAGES:
		if(IS_SET(u.data->flags, F_MAGE) && n < 50)
			return random_string(txAiMageText);
		else
			type = 1;
		break;
	case G_GOBLINS:
		if(n < 50 && !IS_SET(u.data->flags2, F2_NOT_GOBLIN))
			return random_string(txAiGoblinText);
		else
			type = 1;
		break;
	case G_ORCS:
		if(n < 50)
			return random_string(txAiOrcText);
		else
			type = 1;
		break;
	}

	if(type == 0)
	{
		if(location->type == L_CITY)
			return random_string(txAiHeroCityText);
		else if(location->outside)
			return random_string(txAiHeroOutsideText);
		else
			return random_string(txAiHeroDungeonText);
	}
	else
	{
		n = rand2()%100;
		if(n < 60)
			return random_string(txAiDefaultText);
		else if(location->outside)
			return random_string(txAiOutsideText);
		else
			return random_string(txAiInsideText);
	}
}

void Game::GetTeamInfo(TeamInfo& info)
{
	info.players = 0;
	info.npcs = 0;
	info.heroes = 0;
	info.sane_heroes = 0;
	info.insane_heroes = 0;
	info.free_members = 0;

	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer())
			++info.players;
		else
		{
			++info.npcs;
			if((*it)->IsHero())
			{
				if((*it)->hero->free)
					++info.free_members;
				else
				{
					++info.heroes;
					if(IS_SET((*it)->data->flags, F_CRAZY))
						++info.insane_heroes;
					else
						++info.sane_heroes;
				}
			}
			else
				++info.free_members;
		}
	}
}

Unit* Game::GetRandomSaneHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsHero() && !IS_SET((*it)->data->flags, F_CRAZY))
			v->push_back(*it);
	}

	return v->at(rand2()%v->size());
}

UnitData* Game::GetRandomHeroData()
{
	cstring id;

	switch(rand2()%8)
	{
	case 0:
	case 1:
	case 2:
		id = "hero_warrior";
		break;
	case 3:
	case 4:
		id = "hero_hunter";
		break;
	case 5:
	case 6:
		id = "hero_rogue";
		break;
	case 7:
	default:
		id = "hero_mage";
		break;
	}

	return FindUnitData(id);
}

void Game::CheckCraziesStone()
{
	quest_crazies->check_stone = false;

	const Item* kamien = FindItem("q_szaleni_kamien");
	if(!FindItemInTeam(kamien, -1, nullptr, nullptr, false))
	{
		// usuñ kamieñ z gry o ile to nie encounter bo i tak jest resetowany
		if(location->type != L_ENCOUNTER)
		{
			if(quest_crazies->target_loc == current_location)
			{
				// jest w dobrym miejscu, sprawdŸ czy w³o¿y³ kamieñ do skrzyni
				if(local_ctx.chests && local_ctx.chests->size() > 0)
				{
					Chest* chest;
					int slot;
					if(local_ctx.FindItemInChest(kamien, &chest, &slot))
					{
						// w³o¿y³ kamieñ, koniec questa
						chest->items.erase(chest->items.begin()+slot);
						quest_crazies->SetProgress(Quest_Crazies::Progress::Finished);
						return;
					}
				}
			}
			
			RemoveItemFromWorld(kamien);
		}

		// dodaj kamieñ przywódcy
		leader->AddItem(kamien, 1, false);
	}

	if(quest_crazies->crazies_state == Quest_Crazies::State::TalkedWithCrazy)
	{
		quest_crazies->crazies_state = Quest_Crazies::State::PickedStone;
		quest_crazies->days = 13;
	}
}

// usuwa podany przedmiot ze œwiata
// u¿ywane w queœcie z kamieniem szaleñców
bool Game::RemoveItemFromWorld(const Item* item)
{
	assert(item);

	if(local_ctx.RemoveItemFromWorld(item))
		return true;

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if((*it)->ctx.RemoveItemFromWorld(item))
				return true;
		}
	}

	return false;
}

const Item* Game::GetRandomItem(int max_value)
{
	int type = rand2()%6;

	LocalVector<const Item*> items;

	switch(type)
	{
	case 0:
		for(Weapon* w : g_weapons)
		{
			if(w->value <= max_value && w->CanBeGenerated())
				items->push_back(w);
		}
		break;
	case 1:
		for(Bow* b : g_bows)
		{
			if(b->value <= max_value && b->CanBeGenerated())
				items->push_back(b);
		}
		break;
	case 2:
		for(Shield* s : g_shields)
		{
			if(s->value <= max_value && s->CanBeGenerated())
				items->push_back(s);
		}
		break;
	case 3:
		for(Armor* a : g_armors)
		{
			if(a->value <= max_value && a->CanBeGenerated())
				items->push_back(a);
		}
		break;
	case 4:
		for(Consumable* c : g_consumables)
		{
			if(c->value <= max_value && c->CanBeGenerated())
				items->push_back(c);
		}
		break;
	case 5:
		for(OtherItem* o : g_others)
		{
			if(o->value <= max_value && o->CanBeGenerated())
				items->push_back(o);
		}
		break;
	}

	return items->at(rand2() % items->size());
}

bool Game::CheckMoonStone(GroundItem* item, Unit& unit)
{
	assert(item);

	if(secret_state == SECRET_NONE && location->type == L_MOONWELL && item->item->id == "krystal" && distance2d(item->pos, VEC3(128.f,0,128.f)) < 1.2f)
	{
		AddGameMsg(txSecretAppear, 3.f);
		secret_state = SECRET_DROPPED_STONE;
		int loc = CreateLocation(L_DUNGEON, VEC2(0,0), -128.f, DWARF_FORT, SG_WYZWANIE, false, 3);
		Location& l = *locations[loc];
		l.st = 18;
		l.active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		secret_where = loc;
		VEC2& cpos = location->pos;
		Item* note = GetSecretNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y>l.pos.y ? 'S' : 'N', (int)abs((cpos.y-l.pos.y)/3), cpos.x>l.pos.x ? 'W' : 'E', (int)abs((cpos.x-l.pos.x)/3));
		unit.AddItem(note);
		delete item;
		if(IsOnline())
			PushNetChange(NetChange::SECRET_TEXT);
		return true;
	}

	return false;
}

UnitData* Game::GetUnitDataFromClass(Class clas, bool crazy)
{
	cstring id = nullptr;

	switch(clas)
	{
	case Class::WARRIOR:
		id = (crazy ? "crazy_warrior" : "hero_warrior");
		break;
	case Class::HUNTER:
		id = (crazy ? "crazy_hunter" : "hero_hunter");
		break;
	case Class::ROGUE:
		id = (crazy ? "crazy_rogue" : "hero_rogue");
		break;
	case Class::MAGE:
		id = (crazy ? "crazy_mage" : "hero_mage");
		break;
	}

	if(id)
		return FindUnitData(id, false);
	else
		return nullptr;
}

int xdif(int a, int b)
{
	if(a == b)
		return 0;
	else if(a < b)
	{
		a += 10;
		if(a >= b)
			return 1;
		a += 25;
		if(a >= b)
			return 2;
		a += 50;
		if(a >= b)
			return 3;
		a += 100;
		if(a >= b)
			return 4;
		a += 150;
		if(a >= b)
			return 5;
		return 6;
	}
	else
	{
		b += 10;
		if(b >= a)
			return -1;
		b += 25;
		if(b >= a)
			return -2;
		b += 50;
		if(b >= a)
			return -3;
		b += 100;
		if(b >= a)
			return -4;
		b += 150;
		if(b >= a)
			return -5;
		return -6;
	}
}

Game::BLOCK_RESULT Game::CheckBlock(Unit& hitted, float angle_dif, float attack_power, float skill, float str)
{
	int k_block;
// 	if(hitted.HaveShield())
// 	{

	// blokowanie tarcz¹
	k_block = xdif((int)hitted.CalculateBlock(&hitted.GetShield()), (int)attack_power);

	k_block += xdif(hitted.Get(Skill::SHIELD), (int)skill);

// 	k_block += hitted.attrib[A_STR]/2;
// 	k_block += hitted.attrib[A_DEX]/4;
// 	k_block += hitted.skill[S_SHIELD];

		// premia za broñ
// 	}
// 	else
// 	{
// 		// blokowanie broni¹
// 		k_block = 0;
// 	}

	if(str > 0.f)
		k_block += xdif(hitted.Get(Attribute::STR), (int)str);

	if(angle_dif > PI/4)
	{
		if(angle_dif > PI/4+PI/8)
			k_block -= 2;
		else
			--k_block;
	}

	k_block += random(-5,5);

	LOG(Format("k_block: %d", k_block));

	/*return BLOCK_PERFECT;*/

	if(k_block > 8)
	{
		LOG("BLOCK_BREAK");
		return BLOCK_BREAK;
	}
	else if(k_block > 4)
	{
		LOG("BLOCK_POOR");
		return BLOCK_POOR;
	}
	else if(k_block > 0)
	{
		LOG("BLOCK_MEDIUM");
		return BLOCK_MEDIUM;
	}
	else if(k_block > -4)
	{
		LOG("BLOCK_GOOD");
		return BLOCK_GOOD;
	}
	else
	{
		LOG("BLOCK_PERFECT");
		return BLOCK_PERFECT;
	}
}

void Game::AddTeamMember(Unit* unit, bool free)
{
	assert(unit && unit->hero);

	// set as team member
	unit->hero->team_member = true;
	unit->hero->free = free;
	unit->hero->mode = HeroData::Follow;

	// add to team list
	if(!free)
	{
		if(active_team.size() == 1u)
			active_team[0]->MakeItemsTeam(false);
		active_team.push_back(unit);
	}
	team.push_back(unit);

	// set items as not team
	unit->MakeItemsTeam(false);

	// update TeamPanel if open
	if(game_gui->team_panel->visible)
		game_gui->team_panel->Changed();

	// send info to other players
	if(IsOnline())
		Net_RecruitNpc(unit);

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::RECRUIT, unit);
}

void Game::RemoveTeamMember(Unit* unit)
{
	assert(unit && unit->hero);

	// set as not team member
	unit->hero->team_member = false;

	// remove from team list
	if(!unit->hero->free)
		RemoveElementOrder(active_team, unit);
	RemoveElementOrder(team, unit);

	// set items as team
	unit->MakeItemsTeam(true);

	// update TeamPanel if open
	if(game_gui->team_panel->visible)
		game_gui->team_panel->Changed();

	// send info to other players
	if(IsOnline())
		Net_KickNpc(unit);

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::KICK, unit);
}

void Game::DropGold(int ile)
{
	pc->unit->gold -= ile;
	if(sound_volume)
		PlaySound2d(sCoins);

	// animacja wyrzucania
	pc->unit->action = A_ANIMATION;
	pc->unit->ani->Play("wyrzuca", PLAY_ONCE|PLAY_PRIO2, 0);
	pc->unit->ani->frame_end_info = false;

	if(IsLocal())
	{
		// stwórz przedmiot
		GroundItem* item = new GroundItem;
		item->item = gold_item_ptr;
		item->count = ile;
		item->team_count = 0;
		item->pos = pc->unit->pos;
		item->pos.x -= sin(pc->unit->rot)*0.25f;
		item->pos.z -= cos(pc->unit->rot)*0.25f;
		item->rot = random(MAX_ANGLE);
		AddGroundItem(GetContext(*pc->unit), item);

		// wyœlij info o animacji
		if(IsServer())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = pc->unit;
		}
	}
	else
	{
		// wyœlij komunikat o wyrzucaniu z³ota
		NetChange& c = Add1(net_changes);
		c.type = NetChange::DROP_GOLD;
		c.id = ile;
	}
}

void Game::AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = nullptr;
	ItemSlot slot;
	switch(Inventory::lock_id)
	{
	case LOCK_NO:
		break;
	case LOCK_MY:
		if(game_gui->inventory->unit == &unit)
			inv = game_gui->inventory;
		break;
	case LOCK_TRADE_MY:
		if(game_gui->inv_trade_mine->unit == &unit)
			inv = game_gui->inv_trade_mine;
		break;
	case LOCK_TRADE_OTHER:
		if(pc->action == PlayerController::Action_LootUnit && game_gui->inv_trade_other->unit == &unit)
			inv = game_gui->inv_trade_other;
		break;
	}
	if(inv && Inventory::lock_index >= 0)
		slot = inv->items->at(Inventory::lock_index);

	// dodaj przedmiot
	unit.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && IsServer())
	{
		if(unit.IsPlayer())
		{
			if(unit.player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::ADD_ITEMS;
				c.pc = unit.player;
				c.item = item;
				c.ile = count;
				c.id = team_count;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
		else
		{
			// sprawdŸ czy ta jednostka nie wymienia siê przedmiotami z graczem
			Unit* u = nullptr;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&unit))
				{
					u = *it;
					break;
				}
			}

			if(u && u->player != pc)
			{
				// wyœlij komunikat do gracza z aktualizacj¹ ekwipunku
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::ADD_ITEMS_TRADER;
				c.pc = u->player;
				c.item = item;
				c.id = unit.netid;
				c.ile = count;
				c.a = team_count;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
	}

	// czy trzeba budowaæ tymczasowy ekwipunek
	int rebuild_id = -1;
	if(&unit == pc->unit)
	{
		if(game_gui->inventory->visible || game_gui->gp_trade->visible)
			rebuild_id = 0;
	}
	else if(game_gui->gp_trade->visible && game_gui->inv_trade_other->unit == &unit)
		rebuild_id = 1;
	if(rebuild_id != -1)
		BuildTmpInventory(rebuild_id);

	// aktualizuj zlockowany przedmiot
	if(inv)
	{		
		while(1)
		{
			ItemSlot& s = inv->items->at(Inventory::lock_index);
			if(s.item == slot.item && s.count >= slot.count)
				break;
			++Inventory::lock_index;
		}
	}
}

void Game::AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = nullptr;
	ItemSlot slot;
	if(Inventory::lock_id == LOCK_TRADE_OTHER && pc->action == PlayerController::Action_LootChest && pc->action_chest == &chest && Inventory::lock_index >= 0)
	{
		inv = game_gui->inv_trade_other;
		slot = inv->items->at(Inventory::lock_index);
	}

	// dodaj przedmiot
	chest.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && chest.looted)
	{
		Unit* u = FindChestUserIfPlayer(&chest);
		if(u)
		{
			// dodaj komunikat o dodaniu przedmiotu do skrzyni
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::ADD_ITEMS_CHEST;
			c.pc = u->player;
			c.item = item;
			c.id = chest.netid;
			c.ile = count;
			c.a = team_count;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
	}

	// czy trzeba przebudowaæ tymczasowy ekwipunek
	if(game_gui->gp_trade->visible && pc->action == PlayerController::Action_LootChest && pc->action_chest == &chest)
		BuildTmpInventory(1);

	// aktualizuj zlockowany przedmiot
	if(inv)
	{
		while(1)
		{
			ItemSlot& s = inv->items->at(Inventory::lock_index);
			if(s.item == slot.item && s.count >= slot.count)
				break;
			++Inventory::lock_index;
		}
	}
}

// zbuduj tymczasowy ekwipunek który ³¹czy slots i items postaci
void Game::BuildTmpInventory(int index)
{
	assert(index == 0 || index == 1);

	vector<int>& ids = tmp_inventory[index];
	const Item** slots;
	vector<ItemSlot>* items;
	int& shift = tmp_inventory_shift[index];
	shift = 0;

	if(index == 0)
	{
		// przedmioty gracza
		slots = pc->unit->slots;
		items = &pc->unit->items;
	}
	else
	{
		// przedmioty innej postaci, w skrzyni
		if(pc->action == PlayerController::Action_LootChest || pc->action == PlayerController::Action_Trade)
			slots = nullptr;
		else
			slots = pc->action_unit->slots;
		items = pc->chest_trade;
	}

	ids.clear();

	// jeœli to postaæ to dodaj za³o¿one przedmioty
	if(slots)
	{
		for(int i=0; i<SLOT_MAX; ++i)
		{
			if(slots[i])
			{
				ids.push_back(-i-1);
				++shift;
			}
		}
	}

	// nie za³o¿one przedmioty
	for(int i=0, ile = (int)items->size(); i<ile; ++i)
		ids.push_back(i);
}

Unit* Game::FindChestUserIfPlayer(Chest* chest)
{
	assert(chest && chest->looted);

	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() && (*it)->player->action == PlayerController::Action_LootChest && (*it)->player->action_chest == chest)
			return *it;
	}

	return nullptr;
}

void Game::RemoveItem(Unit& unit, int i_index, uint count)
{
	// ustal docelowy ekwipunek (o ile jest)
	Inventory* inv = nullptr;
	ItemSlot slot;
	switch(Inventory::lock_id)
	{
	default:
		assert(0);
	case LOCK_NO:
		break;
	case LOCK_MY:
		if(game_gui->inventory->unit == &unit)
			inv = game_gui->inventory;
		break;
	case LOCK_TRADE_MY:
		if(game_gui->inv_trade_mine->unit == &unit)
			inv = game_gui->inv_trade_mine;
		break;
	case LOCK_TRADE_OTHER:
		if(pc->action == PlayerController::Action_LootUnit && game_gui->inv_trade_other->unit == &unit)
			inv = game_gui->inv_trade_other;
		break;
	}
	if(inv && Inventory::lock_index >= 0)
		slot = inv->items->at(Inventory::lock_index);

	// usuñ przedmiot
	bool removed = false;
	if(i_index >= 0)
	{
		ItemSlot& s = unit.items[i_index];
		uint ile = (count == 0 ? s.count : min(s.count, count));
		s.count -= ile;
		if(s.count == 0)
		{
			removed = true;
			unit.items.erase(unit.items.begin()+i_index);
		}
		else if(s.team_count > 0)
			s.team_count -= min(s.team_count, ile);
		unit.weight -= s.item->weight*ile;
	}
	else
	{
		ITEM_SLOT type = IIndexToSlot(i_index);
		unit.weight -= unit.slots[type]->weight;
		unit.slots[type] = nullptr;
		removed = true;

		if(IsServer() && players > 1)
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = &unit;
			c.id = type;
		}
	}

	// komunikat
	if(IsServer())
	{
		if(unit.IsPlayer())
		{
			// dodaj komunikat o dodaniu przedmiotu
			NetChangePlayer& c = Add1(net_changes_player);
			c.type = NetChangePlayer::REMOVE_ITEMS;
			c.pc = unit.player;
			c.id = i_index;
			c.ile = count;
			GetPlayerInfo(c.pc).NeedUpdate();
		}
		else
		{
			Unit* t = nullptr;

			// szukaj gracza który handluje z t¹ postaci¹
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
			{
				if((*it)->IsPlayer() && (*it)->player->IsTradingWith(&unit))
				{
					t = *it;
					break;
				}
			}

			if(t && t->player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(net_changes_player);
				c.type = NetChangePlayer::REMOVE_ITEMS_TRADER;
				c.pc = t->player;
				c.id = unit.netid;
				c.ile = count;
				c.a = i_index;
				GetPlayerInfo(c.pc).NeedUpdate();
			}
		}
	}

	// aktualizuj tymczasowy ekwipunek
	if(pc->unit == &unit)
	{
		if(game_gui->inventory->visible || game_gui->gp_trade->visible)
			BuildTmpInventory(0);
	}
	else if(game_gui->gp_trade->visible && game_gui->inv_trade_other->unit == &unit)
		BuildTmpInventory(1);

	// aktualizuj zlockowany przedmiot
	if(inv && removed)
	{
		if(i_index == Inventory::lock_index)
			Inventory::lock_index = LOCK_REMOVED;
		else if(i_index < Inventory::lock_index)
		{
			while(1)
			{
				ItemSlot& s = inv->items->at(Inventory::lock_index);
				if(s.item == slot.item && s.count == slot.count)
					break;
				--Inventory::lock_index;
			}
		}
	}
}

bool Game::RemoveItem(Unit& unit, const Item* item, uint count)
{
	int i_index = unit.FindItem(item);
	if(i_index == INVALID_IINDEX)
		return false;
	RemoveItem(unit, i_index, count);
	return true;
}

INT2 Game::GetSpawnPoint()
{
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	if(enter_from >= ENTER_FROM_PORTAL)
		return pos_to_pt(inside->GetPortal(enter_from)->GetSpawnPos());
	else if(enter_from == ENTER_FROM_DOWN_LEVEL)
		return lvl.GetDownStairsFrontTile();
	else
		return lvl.GetUpStairsFrontTile();	
}

InsideLocationLevel* Game::TryGetLevelData()
{
	if(location->type == L_DUNGEON || location->type == L_CRYPT || location->type == L_CAVE)
		return &((InsideLocation*)location)->GetLevelData();
	else
		return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item)
{
	assert(item);
	while(true)
	{
		int id = rand2() % lvl.rooms.size();
		if(!lvl.rooms[id].IsCorridor())
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(lvl.rooms[id], item);
			if(item2)
				return item2;
		}
	}
}

Unit* Game::SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& ud, int level, const INT2& pt, const INT2& pt2)
{
	Unit* u = SpawnUnitInsideRoom(room, ud, level, pt, pt2);
	if(u)
		return u;

	LocalVector<int> connected(room.connected);
	connected.Shuffle();

	for(vector<int>::iterator it = connected->begin(), end = connected->end(); it != end; ++it)
	{
		u = SpawnUnitInsideRoom(lvl.rooms[*it], ud, level, pt, pt2);
		if(u)
			return u;
	}

	return nullptr;
}

Unit* Game::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn)
{
	if(!inn)
		inn = city_ctx->FindInn();

	VEC3 pos;
	bool ok = false;
	if(rand2()%5 != 0)
	{
		if(WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20) || 
			WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10))
			ok = true;
	}
	else
	{
		if(WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10) || 
			WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20))
			ok = true;
	}

	if(ok)
	{
		float rot = random(MAX_ANGLE);
		return CreateUnitWithAI(inn->ctx, ud, level, nullptr, &pos, &rot);
	}
	else
		return nullptr;
}

void Game::SpawnDrunkmans()
{
	InsideBuilding* inn = city_ctx->FindInn();
	contest_generated = true;
	UnitData& pijak = *FindUnitData("pijak");
	int ile = random(4,6);
	for(int i=0; i<ile; ++i)
	{
		Unit* u = SpawnUnitInsideInn(pijak, random(2,15), inn);
		if(u)
		{
			u->temporary = true;
			if(IsOnline())
				Net_SpawnUnit(u);
		}
	}
}

void Game::SetOutsideParams()
{
	cam.draw_range = 80.f;
	clear_color2 = WHITE;
	fog_params = VEC4(40, 80, 40, 0);
	fog_color = VEC4(0.9f, 0.85f, 0.8f, 1);
	ambient_color = VEC4(0.5f, 0.5f, 0.5f, 1);
}

void Game::OnEnterLevelOrLocation()
{
	ClearGui(false);
	lights_dt = 1.f;
	autowalk = false;
}

void Game::StartTrade(InventoryMode mode, Unit& unit)
{
	CloseGamePanels();
	inventory_mode = mode;
	Inventory& my = *game_gui->inv_trade_mine;
	Inventory& other = *game_gui->inv_trade_other;

	other.unit = &unit;
	other.items = &unit.items;
	other.slots = unit.slots;

	switch(mode)
	{
	case I_LOOT_BODY:
		my.mode = Inventory::LOOT_MY;
		other.mode = Inventory::LOOT_OTHER;
		other.title = Format("%s - %s", Inventory::txLooting, unit.GetName());
		break;
	case I_SHARE:
		my.mode = Inventory::SHARE_MY;
		other.mode = Inventory::SHARE_OTHER;
		other.title = Format("%s - %s", Inventory::txShareItems, unit.GetName());
		pc->action = PlayerController::Action_ShareItems;
		pc->action_unit = &unit;
		pc->chest_trade = &unit.items;
		break;
	case I_GIVE:
		my.mode = Inventory::GIVE_MY;
		other.mode = Inventory::GIVE_OTHER;
		other.title = Format("%s - %s", Inventory::txGiveItems, unit.GetName());
		pc->action = PlayerController::Action_GiveItems;
		pc->action_unit = &unit;
		pc->chest_trade = &unit.items;
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	game_gui->gp_trade->Show();
}

void Game::StartTrade(InventoryMode mode, vector<ItemSlot>& items, Unit* unit)
{
	CloseGamePanels();
	inventory_mode = mode;
	Inventory& my = *game_gui->inv_trade_mine;
	Inventory& other = *game_gui->inv_trade_other;

	other.items = &items;
	other.slots = nullptr;

	switch(mode)
	{
	case I_LOOT_CHEST:
		my.mode = Inventory::LOOT_MY;
		other.mode = Inventory::LOOT_OTHER;
		other.unit = nullptr;
		other.title = Inventory::txLootingChest;
		break;
	case I_TRADE:
		assert(unit);
		my.mode = Inventory::TRADE_MY;
		other.mode = Inventory::TRADE_OTHER;
		other.unit = unit;
		other.title = Format("%s - %s", Inventory::txTrading, unit->GetName());
		pc->action = PlayerController::Action_Trade;
		pc->action_unit = unit;
		pc->chest_trade = &items;
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	game_gui->gp_trade->Show();
}

UnitData& Game::GetHero(Class clas, bool crazy)
{
	cstring id;

	switch(clas)
	{
	default:
	case Class::WARRIOR:
		id = (crazy ? "crazy_warrior" : "hero_warrior");
		break;
	case Class::HUNTER:
		id = (crazy ? "crazy_hunter" : "hero_hunter");
		break;
	case Class::ROGUE:
		id = (crazy ? "crazy_rogue" : "hero_rogue");
		break;
	case Class::MAGE:
		id = (crazy ? "crazy_mage" : "hero_mage");
		break;
	}

	return *FindUnitData(id);
}

void Game::ShowAcademyText()
{
	if(GUI.GetDialog("academy") == nullptr)
		GUI.SimpleDialog(txQuest[271], world_map, "academy");
	if(IsServer())
		PushNetChange(NetChange::ACADEMY_TEXT);
}

int Game::GetItemPrice(const Item* item, Unit& unit, bool buy)
{
	assert(item);

	int cha = unit.Get(Attribute::CHA);
	float mod;

	if(buy)
	{
		// cha 1 - 1.25
		// cha 50 - 1.0
		// cha 100 - 0.75
		if(cha <= 1)
			mod = 1.25f;
		else if(cha < 50)
			mod = lerp(1.25f, 1.0f, float(cha) / 50);
		else if(cha == 50)
			mod = 1.f;
		else if(cha < 100)
			mod = lerp(1.0f, 0.75f, float(cha - 50) / 50);
		else
			mod = 0.75f;
	}
	else
	{
		// cha 1 - 0.25
		// cha 50 - 0.5
		// cha 100 - 0.75
		if(cha <= 1)
			mod = 0.25f;
		else if(cha < 50)
			mod = lerp(0.25f, 0.5f, float(cha) / 50);
		else if(cha == 50)
			mod = 0.5f;
		else if(cha < 100)
			mod = lerp(0.5f, 0.75f, float(cha - 50) / 50);
		else
			mod = 0.75f;
	}

	int price = int(mod * item->value);
	if(price == 0 && buy)
		price = 1;

	return price;
}

void Game::VerifyObjects()
{
	int errors = 0, e;

	for(Location* l : locations)
	{
		if(!l)
			continue;
		if(l->outside)
		{
			OutsideLocation* outside = (OutsideLocation*)l;
			e = 0;
			VerifyObjects(outside->objects, e);
			if(e > 0)
			{
				ERROR(Format("%d errors in outside location '%s'.", e, outside->name.c_str()));
				errors += e;
			}
			if(l->type == L_CITY)
			{
				City* city = (City*)outside;
				for(InsideBuilding* ib : city->inside_buildings)
				{
					e = 0;
					VerifyObjects(ib->objects, e);
					if(e > 0)
					{
						ERROR(Format("%d errors in city '%s', building '%s'.", e, city->name.c_str(), ib->type->id.c_str()));
						errors += e;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)l;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* m = (MultiInsideLocation*)inside;
				int index = 1;
				for(auto& lvl : m->levels)
				{
					e = 0;
					VerifyObjects(lvl.objects, e);
					if(e > 0)
					{
						ERROR(Format("%d errors in multi inside location '%s' at level %d.", e, m->name.c_str(), index));
						errors += e;
					}
					++index;
				}
			}
			else
			{
				SingleInsideLocation* s = (SingleInsideLocation*)inside;
				e = 0;
				VerifyObjects(s->objects, e);
				if(e > 0)
				{
					ERROR(Format("%d errors in single inside location '%s'.", e, s->name.c_str()));
					errors += e;
				}
			}
		}
	}

	if(errors > 0)
		throw Format("Veryify objects failed with %d errors. Check log for details.", errors);
}

void Game::VerifyObjects(vector<Object>& objects, int& errors)
{
	for(Object& o : objects)
	{
		if(!o.mesh && !o.base)
		{
			ERROR(Format("Broken object at (%g,%g,%g).", o.pos.x, o.pos.y, o.pos.z));
			++errors;
		}
	}
}

void Game::HandleQuestEvent(Quest_Event* event)
{
	assert(event);

	event->done = true;

	Unit* spawned = nullptr, *spawned2 = nullptr;
	InsideLocationLevel* lvl = nullptr;
	InsideLocation* inside = nullptr;
	if(local_ctx.type == LevelContext::Inside)
	{
		inside = (InsideLocation*)location;
		lvl = &inside->GetLevelData();
	}

	// spawn unit
	if(event->unit_to_spawn)
	{
		if(local_ctx.type == LevelContext::Outside)
		{
			if(location->type == L_CITY)
				spawned = SpawnUnitInsideInn(*event->unit_to_spawn, event->unit_spawn_level);
			else
			{
				VEC3 pos(0, 0, 0);
				int count = 0;
				for(Unit* unit : *local_ctx.units)
				{
					pos += unit->pos;
					++count;
				}
				pos /= (float)count;
				spawned = SpawnUnitNearLocation(local_ctx, pos, *event->unit_to_spawn, nullptr, event->unit_spawn_level);
			}
		}
		else
		{
			Room& room = GetRoom(*lvl, event->spawn_unit_room, inside->HaveDownStairs());
			spawned = SpawnUnitInsideRoomOrNear(*lvl, room, *event->unit_to_spawn, event->unit_spawn_level);
		}
		if(!spawned)
			throw "Failed to spawn quest unit!";
		spawned->dont_attack = event->unit_dont_attack;
		if(event->unit_auto_talk)
			spawned->StartAutoTalk();
		spawned->event_handler = event->unit_event_handler;
		if(spawned->event_handler && event->send_spawn_event)
			spawned->event_handler->HandleUnitEvent(UnitEventHandler::SPAWN, spawned);
		if(devmode)
			LOG(Format("Generated unit %s (%g,%g).", event->unit_to_spawn->id.c_str(), spawned->pos.x, spawned->pos.z));

		// mark near units as guards if guarded (only in dungeon)
		if(IS_SET(spawned->data->flags2, F2_GUARDED) && lvl)
		{
			Room& room = GetRoom(*lvl, event->spawn_unit_room, inside->HaveDownStairs());
			for(Unit* unit : *local_ctx.units)
			{
				if(unit != spawned && IsFriend(*unit, *spawned) && lvl->GetRoom(pos_to_pt(unit->pos)) == &room)
				{
					unit->dont_attack = spawned->dont_attack;
					unit->guard_target = spawned;
				}
			}
		}
	}

	// spawn second units (only in dungeon)
	if(event->unit_to_spawn2 && lvl)
	{
		Room* room;
		if(spawned && event->spawn_2_guard_1)
			room = lvl->GetRoom(pos_to_pt(spawned->pos));
		else
			room = &GetRoom(*lvl, event->spawn_unit_room2, inside->HaveDownStairs());
		spawned2 = SpawnUnitInsideRoomOrNear(*lvl, *room, *event->unit_to_spawn2, event->unit_spawn_level2);
		if(!spawned2)
			throw "Failed to spawn quest unit 2!";
		if(devmode)
			LOG(Format("Generated unit %s (%g,%g).", event->unit_to_spawn2->id.c_str(), spawned2->pos.x, spawned2->pos.z));
		if(spawned && event->spawn_2_guard_1)
		{
			spawned2->dont_attack = spawned->dont_attack;
			spawned2->guard_target = spawned;
		}
	}
	
	// spawn item
	switch(event->spawn_item)
	{
	case Quest_Dungeon::Item_GiveStrongest:
		{
			Unit* best = nullptr;
			for(Unit* unit : *local_ctx.units)
			{
				if(unit->IsAlive() && IsEnemy(*unit, *pc->unit) && (!best || unit->level > best->level))
					best = unit;
			}
			assert(best);
			if(best)
			{
				best->AddItem(event->item_to_give[0], 1, true);
				if(devmode)
					LOG(Format("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), best->data->id.c_str(), best->pos.x, best->pos.z));
			}
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned:
		assert(spawned);
		if(spawned)
		{
			spawned->AddItem(event->item_to_give[0], 1, true);
			if(devmode)
				LOG(Format("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned->data->id.c_str(), spawned->pos.x, spawned->pos.z));
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned2:
		assert(spawned2);
		if(spawned2)
		{
			spawned2->AddItem(event->item_to_give[0], 1, true);
			if(devmode)
				LOG(Format("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned2->data->id.c_str(), spawned2->pos.x, spawned2->pos.z));
		}
		break;
	case Quest_Dungeon::Item_OnGround:
		{
			GroundItem* item;
			if(lvl)
				item = SpawnGroundItemInsideAnyRoom(*lvl, event->item_to_give[0]);
			else
			{
				item = SpawnGroundItemInsideRadius(event->item_to_give[0], VEC2(128, 128), 10.f);
				terrain->SetH(item->pos);
			}
			if(devmode)
				LOG(Format("Generated item %s on ground (%g,%g).", event->item_to_give[0]->id.c_str(), item->pos.x, item->pos.z));
		}
		break;
	case Quest_Dungeon::Item_InTreasure:
		if(inside && (inside->type == L_CRYPT || inside->target == LABIRYNTH))
		{
			Chest* chest = nullptr;

			if(inside->type == L_CRYPT)
			{
				Room& room = lvl->rooms[inside->special_room];
				LocalVector2<Chest*> chests;
				for(Chest* chest2 : lvl->chests)
				{
					if(room.IsInside(chest2->pos))
						chests.push_back(chest2);
				}

				if(!chests.empty())
					chest = chests.random_item();
			}
			else
				chest = random_item(lvl->chests);

			assert(chest);
			if(chest)
			{
				chest->AddItem(event->item_to_give[0]);
				if(devmode)
					LOG(Format("Generated item %s in treasure chest (%g,%g).", event->item_to_give[0]->id.c_str(), chest->pos.x, chest->pos.z));
			}
		}
		break;
	case Quest_Dungeon::Item_InChest:
		{
			Chest* chest = local_ctx.GetRandomFarChest(GetSpawnPoint());
			assert(event->item_to_give[0]);
			if(devmode)
			{
				LocalString str = "Addded items (";
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->item_to_give[i])
						break;
					if(i > 0)
						str += ", ";
					chest->AddItem(event->item_to_give[i]);
					str += event->item_to_give[i]->id;
				}
				str += Format(") to chest (%g,%g).", chest->pos.x, chest->pos.z);
				LOG(str.get_ref().c_str());
			}
			else
			{
				for(int i = 0; i < Quest_Dungeon::MAX_ITEMS; ++i)
				{
					if(!event->item_to_give[i])
						break;
					chest->AddItem(event->item_to_give[i]);
				}
			}
			chest->handler = event->chest_event_handler;
		}
		break;
	}

	if(event->callback)
		event->callback();
}
