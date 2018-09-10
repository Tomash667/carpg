#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GameStats.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "ItemScript.h"
#include "RoomType.h"
#include "SaveState.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "Minimap.h"
#include "QuestManager.h"
#include "Quest_Sawmill.h"
#include "Quest_Mine.h"
#include "Quest_Bandits.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Goblins.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Version.h"
#include "LocationHelper.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "Cave.h"
#include "Encounter.h"
#include "GameGui.h"
#include "Console.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "MpBox.h"
#include "GameMessages.h"
#include "AIController.h"
#include "Spell.h"
#include "Team.h"
#include "Action.h"
#include "ItemContainer.h"
#include "Stock.h"
#include "UnitGroup.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "Profiler.h"
#include "Portal.h"
#include "BitStreamFunc.h"
#include "EntityInterpolator.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"
#include "Var.h"
#include "News.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Debug.h"
#include "LocationGeneratorFactory.h"
#include "CaveGenerator.h"
#include "DungeonGenerator.h"
#include "Texture.h"

const int SAVE_VERSION = V_CURRENT;
int LOAD_VERSION;
const int MIN_SUPPORT_LOAD_VERSION = V_0_2_12;

const Vec2 ALERT_RANGE(20.f, 30.f);
const float PICKUP_RANGE = 2.f;
const float TRAP_ARROW_SPEED = 45.f;
const float ARROW_TIMER = 5.f;
const float MIN_H = 1.5f; // hardcoded in GetPhysicsPos
const float TRAIN_KILL_RATIO = 0.1f;
const float SS = 0.25f;//0.25f/8;
const int NN = 64;
extern const int ITEM_IMAGE_SIZE = 64;
const float SMALL_DISTANCE = 0.001f;

Matrix m1, m2, m3, m4;
UINT passes;

PlayerController::Action InventoryModeToActionRequired(InventoryMode imode)
{
	switch(imode)
	{
	case I_NONE:
	case I_INVENTORY:
		return PlayerController::Action_None;
	case I_LOOT_BODY:
		return PlayerController::Action_LootUnit;
	case I_LOOT_CHEST:
		return PlayerController::Action_LootChest;
	case I_TRADE:
		return PlayerController::Action_Trade;
	case I_SHARE:
		return PlayerController::Action_ShareItems;
	case I_GIVE:
		return PlayerController::Action_GiveItems;
	case I_LOOT_CONTAINER:
		return PlayerController::Action_LootContainer;
	default:
		assert(0);
		return PlayerController::Action_None;
	}
}

//=================================================================================================
void Game::BreakUnitAction(Unit& unit, BREAK_ACTION_MODE mode, bool notify, bool allow_animation)
{
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
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
			if(Net::IsLocal())
				AddItem(unit, unit.used_item, 1, unit.used_item_is_team);
			if(mode != BREAK_ACTION_MODE::FALL)
				unit.used_item = nullptr;
		}
		else
			unit.used_item = nullptr;
		unit.mesh_inst->Deactivate(1);
		unit.action = A_NONE;
		break;
	case A_EAT:
		if(unit.animation_state < 2)
		{
			if(Net::IsLocal())
				AddItem(unit, unit.used_item, 1, unit.used_item_is_team);
			if(mode != BREAK_ACTION_MODE::FALL)
				unit.used_item = nullptr;
		}
		else
			unit.used_item = nullptr;
		unit.mesh_inst->Deactivate(1);
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
		unit.mesh_inst->Deactivate(1);
		unit.action = A_NONE;
		break;
	case A_DASH:
		if(unit.animation_state == 1)
		{
			unit.mesh_inst->Deactivate(1);
			unit.mesh_inst->groups[1].blend_max = 0.33f;
		}
		break;
	}

	if(unit.usable && !(unit.player && unit.player->action == PlayerController::Action_LootContainer))
	{
		if(mode == BREAK_ACTION_MODE::INSTANT)
		{
			unit.action = A_NONE;
			unit.SetAnimationAtEnd(NAMES::ani_stand);
			unit.UseUsable(nullptr);
			unit.used_item = nullptr;
			unit.animation = ANI_STAND;
		}
		else
		{
			unit.target_pos2 = unit.target_pos = unit.pos;
			const Item* prev_used_item = unit.used_item;
			Unit_StopUsingUsable(L.GetContext(unit), unit, mode != BREAK_ACTION_MODE::FALL && notify);
			if(prev_used_item && unit.slots[SLOT_WEAPON] == prev_used_item && !unit.HaveShield())
			{
				unit.weapon_state = WS_TAKEN;
				unit.weapon_taken = W_ONE_HANDED;
				unit.weapon_hiding = W_NONE;
			}
			else if(mode == BREAK_ACTION_MODE::FALL)
				unit.used_item = prev_used_item;
			unit.action = A_POSITION;
			unit.animation_state = 0;
		}

		if(Net::IsLocal() && unit.IsAI() && unit.ai->idle_action != AIController::Idle_None)
		{
			unit.ai->idle_action = AIController::Idle_None;
			unit.ai->timer = Random(1.f, 2.f);
		}
	}
	else
	{
		if(!Any(unit.action, A_ANIMATION, A_STAND_UP) || !allow_animation)
			unit.action = A_NONE;
	}

	unit.mesh_inst->frame_end_info = false;
	unit.mesh_inst->frame_end_info2 = false;
	unit.run_attack = false;

	if(unit.IsPlayer())
	{
		PlayerController& player = *unit.player;
		player.next_action = NA_NONE;
		if(&player == pc)
		{
			pc_data.action_ready = false;
			Inventory::lock = nullptr;
			if(inventory_mode > I_INVENTORY)
				CloseInventory();

			if(player.action == PlayerController::Action_Talk)
			{
				if(Net::IsLocal())
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
		else if(Net::IsLocal())
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
	else if(Net::IsLocal())
	{
		unit.ai->potion = -1;
		if(unit.busy == Unit::Busy_Talking)
		{
			DialogContext* ctx = FindDialogContext(&unit);
			if(ctx)
				EndDialog(*ctx);
			unit.busy = Unit::Busy_No;
		}
	}
}

//=================================================================================================
void Game::Draw()
{
	PROFILER_BLOCK("Draw");

	LevelContext& ctx = L.GetContext(*pc->unit);
	bool outside;
	if(ctx.type == LevelContext::Outside)
		outside = true;
	else if(ctx.type == LevelContext::Inside)
		outside = false;
	else if(L.city_ctx->inside_buildings[ctx.building_id]->top > 0.f)
		outside = false;
	else
		outside = true;

	ListDrawObjects(ctx, cam.frustum, outside);
	DrawScene(outside);

	// rysowanie local pathfind map
#ifdef DRAW_LOCAL_PATH
	V(eMesh->SetTechnique(techMeshSimple2));
	V(eMesh->Begin(&passes, 0));
	V(eMesh->BeginPass(0));

	V(eMesh->SetMatrix(hMeshCombined, &cam.matViewProj));
	V(eMesh->CommitChanges());

	SetAlphaBlend(true);
	SetAlphaTest(false);
	SetNoZWrite(false);

	for(vector<std::pair<Vec2, int> >::iterator it = test_pf.begin(), end = test_pf.end(); it != end; ++it)
	{
		Vec3 v[4] = {
			Vec3(it->first.x, 0.1f, it->first.y + SS),
			Vec3(it->first.x + SS, 0.1f, it->first.y + SS),
			Vec3(it->first.x, 0.1f, it->first.y),
			Vec3(it->first.x + SS, 0.1f, it->first.y)
		};

		if(test_pf_outside)
		{
			float h = terrain->GetH(v[0].x, v[0].z) + 0.1f;
			for(int i = 0; i < 4; ++i)
				v[i].y = h;
		}

		Vec4 color;
		switch(it->second)
		{
		case 0:
			color = Vec4(0, 1, 0, 0.5f);
			break;
		case 1:
			color = Vec4(1, 0, 0, 0.5f);
			break;
		case 2:
			color = Vec4(0, 0, 0, 0.5f);
			break;
		}

		V(eMesh->SetVector(hMeshTint, &color));
		V(eMesh->CommitChanges());

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(Vec3));
	}

	V(eMesh->EndPass());
	V(eMesh->End());
#endif
}

//=================================================================================================
void Game::GenerateItemImage(TaskData& task_data)
{
	Item& item = *(Item*)task_data.ptr;
	item.state = ResourceState::Loaded;

	// if item use image, set it as icon
	if(item.tex)
	{
		item.icon = item.tex->tex;
		return;
	}

	// try to find icon using same mesh
	bool use_tex_override = false;
	if(item.type == IT_ARMOR)
		use_tex_override = !item.ToArmor().tex_override.empty();
	ItemTextureMap::iterator it;
	if(!use_tex_override)
	{
		it = item_texture_map.lower_bound(item.mesh);
		if(it != item_texture_map.end() && !(item_texture_map.key_comp()(item.mesh, it->first)))
		{
			item.icon = it->second;
			return;
		}
	}
	else
		it = item_texture_map.end();

	TEX t = TryGenerateItemImage(item);
	item.icon = t;
	if(it != item_texture_map.end())
		item_texture_map.insert(it, ItemTextureMap::value_type(item.mesh, t));
}

//=================================================================================================
TEX Game::TryGenerateItemImage(const Item& item)
{
	TEX t;
	SURFACE out_surface;
	V(device->CreateTexture(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr));
	V(t->GetSurfaceLevel(0, &out_surface));

	while(true)
	{
		SURFACE surf = DrawItemImage(item, tItemRegion, sItemRegion, 0.f);
		HRESULT hr = D3DXLoadSurfaceFromSurface(out_surface, nullptr, nullptr, surf, nullptr, nullptr, D3DX_DEFAULT, 0);
		surf->Release();
		if(hr == D3DERR_DEVICELOST)
			WaitReset();
		else
			break;
	}

	out_surface->Release();
	return t;
}

//=================================================================================================
SURFACE Game::DrawItemImage(const Item& item, TEX tex, SURFACE surface, float rot)
{
	if(IS_SET(ITEM_ALPHA, item.flags))
	{
		SetAlphaBlend(true);
		SetNoZWrite(true);
	}
	else
	{
		SetAlphaBlend(false);
		SetNoZWrite(false);
	}
	SetAlphaTest(false);
	SetNoCulling(false);

	// ustaw render target
	SURFACE surf = nullptr;
	if(surface)
		V(device->SetRenderTarget(0, surface));
	else
	{
		V(tex->GetSurfaceLevel(0, &surf));
		V(device->SetRenderTarget(0, surf));
	}

	// pocz¹tek renderowania
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0));
	V(device->BeginScene());

	const Mesh& mesh = *item.mesh;
	const TexId* tex_override = nullptr;
	if(item.type == IT_ARMOR)
	{
		tex_override = item.ToArmor().GetTextureOverride();
		if(tex_override)
		{
			assert(item.ToArmor().tex_override.size() == mesh.head.n_subs);
		}
	}

	Matrix matWorld = Matrix::RotationY(rot),
		matView = Matrix::CreateLookAt(mesh.head.cam_pos, mesh.head.cam_target, mesh.head.cam_up),
		matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 1.f, 0.1f, 25.f);

	LightData ld;
	ld.pos = mesh.head.cam_pos;
	ld.color = Vec3(1, 1, 1);
	ld.range = 10.f;

	V(eMesh->SetTechnique(techMesh));
	V(eMesh->SetMatrix(hMeshCombined, (D3DXMATRIX*)&(matWorld*matView*matProj)));
	V(eMesh->SetMatrix(hMeshWorld, (D3DXMATRIX*)&matWorld));
	V(eMesh->SetVector(hMeshFogColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetVector(hMeshFogParam, (D3DXVECTOR4*)&Vec4(25.f, 50.f, 25.f, 0)));
	V(eMesh->SetVector(hMeshAmbientColor, (D3DXVECTOR4*)&Vec4(0.5f, 0.5f, 0.5f, 1)));
	V(eMesh->SetVector(hMeshTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(eMesh->SetRawValue(hMeshLights, &ld, 0, sizeof(LightData)));

	V(device->SetVertexDeclaration(vertex_decl[mesh.vertex_decl]));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	UINT passes;
	V(eMesh->Begin(&passes, 0));
	V(eMesh->BeginPass(0));

	for(int i = 0; i < mesh.head.n_subs; ++i)
	{
		const Mesh::Submesh& sub = mesh.subs[i];
		V(eMesh->SetTexture(hMeshTex, mesh.GetTexture(i, tex_override)));
		V(eMesh->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
	}

	V(eMesh->EndPass());
	V(eMesh->End());

	// koniec renderowania
	V(device->EndScene());

	// kopiuj do tekstury
	if(surface)
	{
		V(tex->GetSurfaceLevel(0, &surf));
		V(device->StretchRect(surface, nullptr, surf, nullptr, D3DTEXF_NONE));
	}

	// przywróæ stary render target
	SURFACE backbuffer;
	V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer));
	V(device->SetRenderTarget(0, backbuffer));
	backbuffer->Release();

	return surf;
}

//=================================================================================================
void Game::SetupCamera(float dt)
{
	Unit* target = pc->unit;
	LevelContext& ctx = L.GetContext(*target);

	float rotX;
	if(cam.free_rot)
		rotX = cam.real_rot.x;
	else
		rotX = target->rot;

	cam.UpdateRot(dt, Vec2(rotX, cam.real_rot.y));

	Matrix mat, matProj, matView;
	const Vec3 cam_h(0, target->GetUnitHeight() + 0.2f, 0);
	Vec3 dist(0, -cam.tmp_dist, 0);

	mat = Matrix::Rotation(cam.rot.x, cam.rot.y, 0);
	dist = Vec3::Transform(dist, mat);

	// !!! to => from !!!
	// kamera idzie od g³owy do ty³u
	Vec3 to = target->pos + cam_h;
	float tout, min_tout = 2.f;

	int tx = int(target->pos.x / 2),
		tz = int(target->pos.z / 2);

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)L.location;

		// terrain
		tout = terrain->Raytest(to, to + dist);
		if(tout < min_tout && tout > 0.f)
			min_tout = tout;

		// buildings
		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(OutsideLocation::size - 1, tx + 3),
			maxz = min(OutsideLocation::size - 1, tz + 3);
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				if(outside->tiles[x + z * OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 8.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int minx = max(0, tx - 3),
			minz = max(0, tz - 3),
			maxx = min(lvl.w - 1, tx + 3),
			maxz = min(lvl.h - 1, tz + 3);

		// ceil
		const Plane sufit(0, -1, 0, 4);
		if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
		{
			//tmpvar2 = 1;
			min_tout = tout;
		}

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// dungeon
		for(int z = minz; z <= maxz; ++z)
		{
			for(int x = minx; x <= maxx; ++x)
			{
				Pole& p = lvl.map[x + z * lvl.w];
				if(czy_blokuje2(p.type))
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				else if(IS_SET(p.flags, Pole::F_NISKI_SUFIT))
				{
					const Box box(float(x) * 2, 3.f, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				if(p.type == SCHODY_GORA)
				{
					if(vdSchodyGora->RayToMesh(to, dist, pt_to_pos(lvl.staircase_up), dir_to_rot(lvl.staircase_up_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == SCHODY_DOL)
				{
					if(!lvl.staircase_down_in_wall
						&& vdSchodyDol->RayToMesh(to, dist, pt_to_pos(lvl.staircase_down), dir_to_rot(lvl.staircase_down_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == DRZWI || p.type == OTWOR_NA_DRZWI)
				{
					Vec3 pos(float(x * 2) + 1, 0, float(z * 2) + 1);
					float rot;

					if(czy_blokuje2(lvl.map[x - 1 + z * lvl.w].type))
					{
						rot = 0;
						int mov = 0;
						if(lvl.rooms[lvl.map[x + (z - 1)*lvl.w].room].IsCorridor())
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
						rot = PI / 2;
						int mov = 0;
						if(lvl.rooms[lvl.map[x - 1 + z * lvl.w].room].IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + 1 + z * lvl.w].room].IsCorridor())
							--mov;
						if(mov == 1)
							pos.x += 0.8229f;
						else if(mov == -1)
							pos.x -= 0.8229f;
					}

					if(vdNaDrzwi->RayToMesh(to, dist, pos, rot, tout) && tout < min_tout)
						min_tout = tout;

					Door* door = FindDoor(ctx, Int2(x, z));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						Box box(pos.x, 0.f, pos.z);
						box.v2.y = 1.319f * 2;
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
		// building
		InsideBuilding& building = *L.city_ctx->inside_buildings[ctx.building_id];

		// ceil
		if(building.top > 0.f)
		{
			const Plane sufit(0, -1, 0, 4);
			if(RayToPlane(to, dist, sufit, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}

		// floor
		const Plane podloga(0, 1, 0, 0);
		if(RayToPlane(to, dist, podloga, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;

		// xsphere
		if(building.xsphere_radius > 0.f)
		{
			Vec3 from = to + dist;
			if(RayToSphere(from, -dist, building.xsphere_pos, building.xsphere_radius, tout) && tout > 0.f)
			{
				tout = 1.f - tout;
				if(tout < min_tout)
					min_tout = tout;
			}
		}

		// doors
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box box(door.pos);
				box.v2.y = 1.319f * 2;
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

			if(vdNaDrzwi->RayToMesh(to, dist, door.pos, door.rot, tout) && tout < min_tout)
				min_tout = tout;
		}
	}

	// objects
	for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
	{
		if(it->ptr != CAM_COLLIDER)
			continue;

		if(it->type == CollisionObject::SPHERE)
		{
			if(RayToCylinder(to, to + dist, Vec3(it->pt.x, 0, it->pt.y), Vec3(it->pt.x, 32.f, it->pt.y), it->radius, tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
		else if(it->type == CollisionObject::RECTANGLE)
		{
			Box box(it->pt.x - it->w, 0.f, it->pt.y - it->h, it->pt.x + it->w, 32.f, it->pt.y + it->h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
		else
		{
			float w, h;
			if(Equal(it->rot, PI / 2) || Equal(it->rot, PI * 3 / 2))
			{
				w = it->h;
				h = it->w;
			}
			else
			{
				w = it->w;
				h = it->h;
			}

			Box box(it->pt.x - w, 0.f, it->pt.y - h, it->pt.x + w, 32.f, it->pt.y + h);
			if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
				min_tout = tout;
		}
	}

	// camera colliders
	for(vector<CameraCollider>::iterator it = cam_colliders.begin(), end = cam_colliders.end(); it != end; ++it)
	{
		if(RayToBox(to, dist, it->box, &tout) && tout < min_tout && tout > 0.f)
			min_tout = tout;
	}

	// uwzglêdnienie znear
	if(min_tout > 1.f || pc->noclip)
		min_tout = 1.f;
	else if(min_tout < 0.1f)
		min_tout = 0.1f;

	float real_dist = dist.Length() * min_tout - 0.1f;
	if(real_dist < 0.01f)
		real_dist = 0.01f;
	Vec3 from = to + dist.Normalize() * real_dist;

	cam.Update(dt, from, to);

	float drunk = pc->unit->alcohol / pc->unit->hpmax;
	float drunk_mod = (drunk > 0.1f ? (drunk - 0.1f) / 0.9f : 0.f);

	matView = Matrix::CreateLookAt(cam.from, cam.to);
	matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4 + sin(drunk_anim)*(PI / 16)*drunk_mod,
		GetWindowAspect() * (1.f + sin(drunk_anim) / 10 * drunk_mod), 0.1f, cam.draw_range);
	cam.matViewProj = matView * matProj;
	cam.matViewInv = matView.Inverse();
	cam.center = cam.from;
	cam.frustum.Set(cam.matViewProj);

	// centrum dŸwiêku 3d
	sound_mgr->SetListenerPosition(target->GetHeadSoundPos(), Vec3(sin(target->rot + PI), 0, cos(target->rot + PI)));
}

//=================================================================================================
void Game::LoadShaders()
{
	Info("Loading shaders.");

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
	techGlowMesh = eGlow->GetTechniqueByName("mesh");
	techGlowAni = eGlow->GetTechniqueByName("ani");
	techGrass = eGrass->GetTechniqueByName("grass");
	assert(techMesh && techMeshDir && techMeshSimple && techMeshSimple2 && techMeshExplo && techParticle && techTrail && techSkybox && techTerrain && techArea
		&& techGlowMesh && techGlowAni && techGrass);

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
	assert(hMeshCombined && hMeshWorld && hMeshTex && hMeshFogColor && hMeshFogParam && hMeshTint && hMeshAmbientColor && hMeshLightDir && hMeshLightColor
		&& hMeshLights);

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
	hGlowTex = eGlow->GetParameterByName(nullptr, "texDiffuse");
	assert(hGlowCombined && hGlowBones && hGlowColor && hGlowTex);

	hGrassViewProj = eGrass->GetParameterByName(nullptr, "matViewProj");
	hGrassTex = eGrass->GetParameterByName(nullptr, "texDiffuse");
	hGrassFogColor = eGrass->GetParameterByName(nullptr, "fogColor");
	hGrassFogParams = eGrass->GetParameterByName(nullptr, "fogParam");
	hGrassAmbientColor = eGrass->GetParameterByName(nullptr, "ambientColor");
}

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

	if(Net::IsLocal())
	{
		assert(pc->is_local);
		if(Net::IsServer())
		{
			for(auto pinfo : game_players)
			{
				auto& info = *pinfo;
				if(!info.left)
				{
					assert(info.pc == info.pc->player_info->pc);
					if(info.id != 0)
						assert(!info.pc->is_local);
				}
			}
		}
	}
	else
	{
		assert(pc->is_local && pc == pc->player_info->pc);
	}
#endif

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

	if(in_tutorial && !Net::IsOnline())
		UpdateTutorial();

	drunk_anim = Clip(drunk_anim + dt);
	UpdatePostEffects(dt);

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	L.light_angle = Clip(L.light_angle + dt / 100);

	LevelContext& player_ctx = (pc->unit->in_building == -1 ? L.local_ctx : L.city_ctx->inside_buildings[pc->unit->in_building]->ctx);

	UpdateFallback(dt);

	if(Net::IsLocal() && !in_tutorial)
	{
		// aktualizuj arene/wymiane sprzêtu/zawody w piciu/questy
		UpdateGame2(dt);
	}

	// info o uczoñczeniu wszystkich unikalnych questów
	if(CanShowEndScreen())
	{
		if(Net::IsLocal())
			QM.unique_completed_show = true;
		else
			QM.unique_completed_show = false;

		cstring text;

		if(Net::IsOnline())
		{
			text = txWinMp;
			if(Net::IsServer())
			{
				Net::PushChange(NetChange::GAME_STATS);
				Net::PushChange(NetChange::ALL_QUESTS_COMPLETED);
			}
		}
		else
			text = txWin;

		GUI.SimpleDialog(Format(text, pc->kills, GameStats::Get().total_kills - pc->kills), nullptr);
	}

	// wyzeruj pobliskie jednostki
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
		it->valid = false;

	// licznik otrzymanych obra¿eñ
	pc->last_dmg = 0.f;
	if(Net::IsLocal())
		pc->last_dmg_poison = 0.f;

	if(devmode && GKey.AllowKeyboard())
	{
		if(!L.location->outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			InsideLocationLevel& lvl = inside->GetLevelData();

			if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && inside->HaveUpStairs())
			{
				if(!Key.Down(VK_CONTROL))
				{
					// teleportuj gracza do schodów w górê
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetUpStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.staircase_up_dir);
						WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 0;
					}
				}
				else
				{
					// poziom w górê
					if(Net::IsLocal())
					{
						ChangeLevel(-1);
						return;
					}
					else
					{
						NetChange& c = Add1(Net::changes);
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
					if(Net::IsLocal())
					{
						Int2 tile = lvl.GetDownStairsFrontTile();
						pc->unit->rot = dir_to_rot(lvl.staircase_down_dir);
						WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_WARP_TO_STAIRS;
						c.id = 1;
					}
				}
				else
				{
					// poziom w dó³
					if(Net::IsLocal())
					{
						ChangeLevel(+1);
						return;
					}
					else
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEAT_CHANGE_LEVEL;
						c.id = 1;
					}
				}
			}
		}
		else if(Key.Pressed(VK_OEM_COMMA) && Key.Down(VK_SHIFT) && Key.Down(VK_CONTROL))
		{
			if(Net::IsLocal())
			{
				ExitToMap();
				return;
			}
			else
				Net::PushChange(NetChange::CHEAT_GOTO_MAP);
		}
	}

	// obracanie kamery góra/dó³
	if(!Net::IsLocal() || Team.IsAnyoneAlive())
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
			if(Any(GKey.allow_input, GameKeys::ALLOW_INPUT, GameKeys::ALLOW_MOUSE))
			{
				const float c_cam_angle_min = PI + 0.1f;
				const float c_cam_angle_max = PI * 1.8f - 0.1f;

				int div = (pc->unit->action == A_SHOOT ? 800 : 400);
				cam.real_rot.y += -float(Key.GetMouseDif().y) * mouse_sensitivity_f / div;
				if(cam.real_rot.y > c_cam_angle_max)
					cam.real_rot.y = c_cam_angle_max;
				if(cam.real_rot.y < c_cam_angle_min)
					cam.real_rot.y = c_cam_angle_min;

				if(!cam.free_rot)
				{
					cam.free_rot_key = GKey.KeyDoReturn(GK_ROTATE_CAMERA, &KeyStates::Pressed);
					if(cam.free_rot_key != VK_NONE)
					{
						cam.real_rot.x = Clip(pc->unit->rot + PI);
						cam.free_rot = true;
					}
				}
				else
				{
					if(GKey.KeyUpAllowed(cam.free_rot_key))
						cam.free_rot = false;
					else
						cam.real_rot.x = Clip(cam.real_rot.x + float(Key.GetMouseDif().x) * mouse_sensitivity_f / 400);
				}
			}
			else
				cam.free_rot = false;
		}
	}
	else
	{
		cam.free_rot = false;
		if(cam.real_rot.y > PI + 0.1f)
		{
			cam.real_rot.y -= dt;
			if(cam.real_rot.y < PI + 0.1f)
				cam.real_rot.y = PI + 0.1f;
		}
		else if(cam.real_rot.y < PI + 0.1f)
		{
			cam.real_rot.y += dt;
			if(cam.real_rot.y > PI + 0.1f)
				cam.real_rot.y = PI + 0.1f;
		}
	}

	// przybli¿anie/oddalanie kamery
	if(GKey.AllowMouse())
	{
		if(!dialog_context.dialog_mode || !dialog_context.show_choices || !game_gui->IsMouseInsideDialog())
		{
			cam.dist -= Key.GetMouseWheel();
			cam.dist = Clamp(cam.dist, 0.5f, 6.f);
		}

		if(Key.PressedRelease(VK_MBUTTON))
			cam.dist = 3.5f;
	}

	// umieranie
	if((Net::IsLocal() && !Team.IsAnyoneAlive()) || death_screen != 0)
	{
		if(death_screen == 0)
		{
			Info("Game over: all players died.");
			SetMusic(MusicType::Death);
			CloseAllPanels(true);
			++death_screen;
			death_fade = 0;
			death_solo = (Team.GetTeamSize() == 1u);
			if(Net::IsOnline())
			{
				Net::PushChange(NetChange::GAME_STATS);
				Net::PushChange(NetChange::GAME_OVER);
			}
		}
		else if(death_screen == 1 || death_screen == 2)
		{
			death_fade += dt / 2;
			if(death_fade >= 1.f)
			{
				death_fade = 0;
				++death_screen;
			}
		}
		if(death_screen >= 2 && GKey.AllowKeyboard() && Key.Pressed2Release(VK_ESCAPE, VK_RETURN))
		{
			ExitToMenu();
			return;
		}
	}

	// aktualizuj gracza
	if(pc_data.wasted_key != VK_NONE && Key.Up(pc_data.wasted_key))
		pc_data.wasted_key = VK_NONE;
	if(dialog_context.dialog_mode || pc->unit->look_target || inventory_mode > I_INVENTORY)
	{
		Vec3 pos;
		if(pc->unit->look_target)
		{
			if(pc->unit->look_target->to_remove)
				pc->unit->look_target = nullptr;
			else
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
		else if(inventory_mode == I_LOOT_CONTAINER)
		{
			// TODO: animacja
			assert(pc->action == PlayerController::Action_LootContainer);
			pos = pc->action_container->pos;
			pc->unit->animation = ANI_STAND;
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

		float dir = Vec3::LookAtAngle(pc->unit->pos, pos);

		if(!Equal(pc->unit->rot, dir))
		{
			const float rot_speed = 3.f*dt;
			const float rot_diff = AngleDiff(pc->unit->rot, dir);
			if(ShortestArc(pc->unit->rot, dir) > 0.f)
				pc->unit->animation = ANI_RIGHT;
			else
				pc->unit->animation = ANI_LEFT;
			if(rot_diff < rot_speed)
				pc->unit->rot = dir;
			else
			{
				const float d = Sign(ShortestArc(pc->unit->rot, dir)) * rot_speed;
				pc->unit->rot = Clip(pc->unit->rot + d);
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
				if(!pc->action_unit->IsStanding() || !pc->action_unit->IsIdle())
				{
					// handlarz umar³ / zaatakowany
					CloseInventory();
				}
			}
		}
		else if(dialog_context.dialog_mode && Net::IsLocal())
		{
			if(!dialog_context.talker->IsStanding() || !dialog_context.talker->IsIdle() || dialog_context.talker->to_remove
				|| dialog_context.talker->frozen != FROZEN::NO)
			{
				// rozmówca umar³ / jest usuwany / zaatakowa³ kogoœ
				EndDialog(dialog_context);
			}
		}

		UpdatePlayerView();
		pc_data.before_player = BP_NONE;
		pc_data.rot_buf = 0.f;
		pc_data.autowalk = false;
		pc_data.action_ready = false;
	}
	else if(!IsBlocking(pc->unit->action) && !pc->unit->HaveEffect(EffectId::Stun))
		UpdatePlayer(player_ctx, dt);
	else
	{
		UpdatePlayerView();
		pc_data.before_player = BP_NONE;
		pc_data.rot_buf = 0.f;
		pc_data.autowalk = false;
		pc_data.action_ready = false;
	}

	// aktualizuj ai
	if(!noai && Net::IsLocal())
		UpdateAi(dt);

	// aktualizuj konteksty poziomów
	lights_dt += dt;
	for(LevelContext& ctx : L.ForEachContext())
		UpdateContext(ctx, dt);
	if(lights_dt >= 1.f / 60)
		lights_dt = 0.f;

	// aktualizacja minimapy
	if(minimap_opened_doors)
	{
		for(Unit* unit : Team.active_members)
		{
			if(unit->IsPlayer())
				DungeonReveal(Int2(int(unit->pos.x / 2), int(unit->pos.z / 2)));
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
					if(it + 1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
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
					if(it + 1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
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
	if(Net::IsSingleplayer())
	{
		if(dialog_context.dialog_mode)
			UpdateGameDialog(dialog_context, dt);
	}
	else if(Net::IsServer())
	{
		for(auto info : game_players)
		{
			if(info->left)
				continue;
			DialogContext& ctx = *info->u->player->dialog_ctx;
			if(ctx.dialog_mode)
			{
				if(!ctx.talker->IsStanding() || !ctx.talker->IsIdle() || ctx.talker->to_remove || ctx.talker->frozen != FROZEN::NO)
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
	if(Net::IsLocal())
	{
		if(Net::IsOnline())
			UpdateWarpData(dt);
		L.ProcessUnitWarps();
	}

	// usuñ jednostki
	L.ProcessRemoveUnits(false);

	if(Net::IsOnline())
	{
		UpdateGameNet(dt);
		if(!Net::IsOnline() || game_state != GS_LEVEL)
			return;
	}

	// aktualizacja obrazka obra¿en
	pc->Update(dt);
	if(Net::IsServer())
	{
		for(auto info : game_players)
		{
			if(info->left == PlayerInfo::LEFT_NO && info->pc != pc)
				info->pc->Update(dt, false);
		}
	}

	// aktualizuj kamerê
	SetupCamera(dt);

#ifdef _DEBUG
	if(Net::IsLocal() && arena_free)
	{
		int err_count = 0;
		for(Unit* unit : Team.members)
		{
			for(Unit* unit2 : Team.members)
			{
				if(unit != unit2 && !IsFriend(*unit, *unit2))
				{
					Warn("%s (%d,%d) i %s (%d,%d) are not friends!", unit->data->id.c_str(), unit->in_arena, unit->IsTeamMember() ? 1 : 0,
						unit2->data->id.c_str(), unit2->in_arena, unit2->IsTeamMember() ? 1 : 0);
					++err_count;
				}
			}
		}
		if(err_count)
			AddGameMsg(Format("%d arena friends errors!", err_count), 10.f);
	}
#endif
}

//=================================================================================================
void Game::UpdateFallback(float dt)
{
	if(fallback_type == FALLBACK::NO)
		return;

	if(fallback_t <= 0.f)
	{
		fallback_t += dt * 2;

		if(fallback_t > 0.f)
		{
			switch(fallback_type)
			{
			case FALLBACK::TRAIN:
				if(Net::IsLocal())
				{
					if(fallback_1 == 2)
						QM.quest_tournament->Train(*pc->unit);
					else
						Train(*pc->unit, fallback_1 == 1, fallback_2);
					pc->Rest(10, false);
					if(Net::IsOnline())
						UseDays(pc, 10);
					else
						W.Update(10, World::UM_NORMAL);
				}
				else
				{
					fallback_type = FALLBACK::CLIENT;
					fallback_t = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TRAIN;
					c.id = fallback_1;
					c.ile = fallback_2;
				}
				break;
			case FALLBACK::REST:
				if(Net::IsLocal())
				{
					pc->Rest(fallback_1, true);
					if(Net::IsOnline())
						UseDays(pc, fallback_1);
					else
						W.Update(fallback_1, World::UM_NORMAL);
				}
				else
				{
					fallback_type = FALLBACK::CLIENT;
					fallback_t = 0.f;
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::REST;
					c.id = fallback_1;
				}
				break;
			case FALLBACK::ENTER: // enter/exit building
				L.WarpUnit(pc->unit, fallback_1);
				break;
			case FALLBACK::EXIT:
				ExitToMap();
				break;
			case FALLBACK::CHANGE_LEVEL:
				ChangeLevel(fallback_1);
				break;
			case FALLBACK::USE_PORTAL:
				{
					LeaveLocation(false, false);
					Portal* portal = L.location->GetPortal(fallback_1);
					W.ChangeLevel(portal->target_loc, false);
					// aktualnie mo¿na siê tepn¹æ z X poziomu na 1 zawsze ale ¿eby z X na X to musi byæ odwiedzony
					// np w sekrecie z 3 na 1 i spowrotem do
					int at_level = 0;
					if(L.location->portal)
						at_level = L.location->portal->at_level;
					EnterLocation(at_level, portal->target);
				}
				return;
			case FALLBACK::NONE:
			case FALLBACK::ARENA2:
			case FALLBACK::CLIENT2:
				break;
			case FALLBACK::ARENA:
			case FALLBACK::ARENA_EXIT:
			case FALLBACK::WAIT_FOR_WARP:
			case FALLBACK::CLIENT:
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
		fallback_t += dt * 2;

		if(fallback_t >= 1.f)
		{
			if(Net::IsLocal())
			{
				if(fallback_type != FALLBACK::ARENA2)
				{
					if(fallback_type == FALLBACK::CHANGE_LEVEL || fallback_type == FALLBACK::USE_PORTAL || fallback_type == FALLBACK::EXIT)
					{
						for(Unit* unit : Team.members)
							unit->frozen = FROZEN::NO;
					}
					pc->unit->frozen = FROZEN::NO;
				}
			}
			else if(fallback_type == FALLBACK::CLIENT2)
			{
				pc->unit->frozen = FROZEN::NO;
				Net::PushChange(NetChange::END_FALLBACK);
			}
			fallback_type = FALLBACK::NO;
		}
	}
}

//=================================================================================================
void Game::UpdatePlayer(LevelContext& ctx, float dt)
{
	Unit& u = *pc->unit;

	// unit is on ground
	if(!u.IsStanding())
	{
		pc_data.autowalk = false;
		pc_data.action_ready = false;
		pc_data.rot_buf = 0.f;
		UnitTryStandup(u, dt);
		return;
	}

	// unit is frozen
	if(u.frozen >= FROZEN::YES)
	{
		pc_data.autowalk = false;
		pc_data.action_ready = false;
		pc_data.rot_buf = 0.f;
		if(u.frozen == FROZEN::YES)
			u.animation = ANI_STAND;
		return;
	}

	// using usable
	if(u.usable)
	{
		if(u.action == A_ANIMATION2 && OR2_EQ(u.animation_state, AS_ANIMATION2_USING, AS_ANIMATION2_USING_SOUND))
		{
			if(GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE) || GKey.KeyPressedReleaseAllowed(GK_USE))
				Unit_StopUsingUsable(ctx, u);
		}
		UpdatePlayerView();
		pc_data.rot_buf = 0.f;
		pc_data.action_ready = false;
	}

	u.prev_pos = u.pos;
	u.changed = true;

	bool idle = true, this_frame_run = false;
	int move = 0;

	if(!u.usable)
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

		if(GKey.KeyPressedReleaseAllowed(GK_TOGGLE_WALK))
		{
			pc->always_run = !pc->always_run;
			if(Net::IsClient())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ALWAYS_RUN;
				c.id = (pc->always_run ? 1 : 0);
			}
		}

		int rotate = 0;
		if(GKey.KeyDownAllowed(GK_ROTATE_LEFT))
			--rotate;
		if(GKey.KeyDownAllowed(GK_ROTATE_RIGHT))
			++rotate;
		if(u.frozen == FROZEN::NO)
		{
			bool cancel_autowalk = (GKey.KeyPressedReleaseAllowed(GK_MOVE_FORWARD) || GKey.KeyDownAllowed(GK_MOVE_BACK));
			if(cancel_autowalk)
				pc_data.autowalk = false;
			else if(GKey.KeyPressedReleaseAllowed(GK_AUTOWALK))
				pc_data.autowalk = !pc_data.autowalk;

			if(u.run_attack)
			{
				move = 10;
				if(GKey.KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(GKey.KeyDownAllowed(GK_MOVE_LEFT))
					--move;
			}
			else
			{
				if(GKey.KeyDownAllowed(GK_MOVE_RIGHT))
					++move;
				if(GKey.KeyDownAllowed(GK_MOVE_LEFT))
					--move;
				if(GKey.KeyDownAllowed(GK_MOVE_FORWARD) || pc_data.autowalk)
					move += 10;
				if(GKey.KeyDownAllowed(GK_MOVE_BACK))
					move -= 10;
			}
		}
		else
			pc_data.autowalk = false;

		if(u.action == A_NONE && !u.talking && GKey.KeyPressedReleaseAllowed(GK_YELL))
		{
			if(Net::IsLocal())
				PlayerYell(u);
			else
				Net::PushChange(NetChange::YELL);
		}

		if((GKey.allow_input == GameKeys::ALLOW_INPUT || GKey.allow_input == GameKeys::ALLOW_MOUSE) && !cam.free_rot)
		{
			int div = (pc->unit->action == A_SHOOT ? 800 : 400);
			pc_data.rot_buf *= (1.f - dt * 2);
			pc_data.rot_buf += float(Key.GetMouseDif().x) * mouse_sensitivity_f / div;
			if(pc_data.rot_buf > 0.1f)
				pc_data.rot_buf = 0.1f;
			else if(pc_data.rot_buf < -0.1f)
				pc_data.rot_buf = -0.1f;
		}
		else
			pc_data.rot_buf = 0.f;

		const bool rotating = (rotate != 0 || pc_data.rot_buf != 0.f);

		if(rotating || move)
		{
			// rotating by mouse don't affect idle timer
			if(move || rotate)
				idle = false;

			if(rotating)
			{
				const float rot_speed_dt = u.GetRotationSpeed() * dt;
				const float mouse_rot = Clamp(pc_data.rot_buf, -rot_speed_dt, rot_speed_dt);
				const float val = rot_speed_dt * rotate + mouse_rot;

				pc_data.rot_buf -= mouse_rot;
				u.rot = Clip(u.rot + Clamp(val, -rot_speed_dt, rot_speed_dt));

				if(val > 0)
					u.animation = ANI_RIGHT;
				else if(val < 0)
					u.animation = ANI_LEFT;
			}

			if(move)
			{
				// ustal k¹t i szybkoœæ ruchu
				float angle = u.rot;
				bool run = pc->always_run;
				if(!u.run_attack)
				{
					if(GKey.KeyDownAllowed(GK_WALK))
						run = !run;
					if(!u.CanRun())
						run = false;
				}
				else
					run = true;

				switch(move)
				{
				case 10: // przód
					angle += PI;
					break;
				case -10: // ty³
					run = false;
					break;
				case -1: // lewa
					angle += PI / 2;
					break;
				case 1: // prawa
					angle += PI * 3 / 2;
					break;
				case 9: // lewa góra
					angle += PI * 3 / 4;
					break;
				case 11: // prawa góra
					angle += PI * 5 / 4;
					break;
				case -11: // lewy ty³
					run = false;
					angle += PI / 4;
					break;
				case -9: // prawy ty³
					run = false;
					angle += PI * 7 / 4;
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
				u.prev_speed = Clamp((u.prev_speed + (u.speed - u.prev_speed)*dt * 3), 0.f, u.speed);
				float speed = u.prev_speed * dt;

				u.prev_pos = u.pos;

				const Vec3 dir(sin(angle)*speed, 0, cos(angle)*speed);
				Int2 prev_tile(int(u.pos.x / 2), int(u.pos.z / 2));
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
					if(Net::IsLocal())
						u.player->TrainMove(dt, run);
					else
					{
						train_move += (run ? dt : dt / 10);
						if(train_move >= 1.f)
						{
							--train_move;
							Net::PushChange(NetChange::TRAIN_MOVE);
						}
					}

					// revealing minimap
					if(!L.location->outside)
					{
						Int2 new_tile(int(u.pos.x / 2), int(u.pos.z / 2));
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
			u.prev_speed -= dt * 3;
			if(u.prev_speed < 0)
				u.prev_speed = 0;
		}
	}

	if(u.action == A_NONE || u.action == A_TAKE_WEAPON || u.CanDoWhileUsing())
	{
		if(GKey.KeyPressedReleaseAllowed(GK_TAKE_WEAPON))
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
					if(pc->next_action != NA_NONE)
					{
						pc->next_action = NA_NONE;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}

					switch(u.weapon_state)
					{
					case WS_HIDDEN:
						// broñ jest schowana, zacznij wyjmowaæ
						u.mesh_inst->Play(u.GetTakeWeaponAnimation(bron == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
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
							u.mesh_inst->Deactivate(1);
						}
						else
						{
							// schowa³ broñ za pas, zacznij wyci¹gaæ
							u.weapon_taken = u.weapon_hiding;
							u.weapon_hiding = W_NONE;
							pc->ostatnia = u.weapon_taken;
							u.weapon_state = WS_TAKING;
							u.animation_state = 0;
							CLEAR_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
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
							u.mesh_inst->Deactivate(1);
						}
						else
						{
							// wyj¹³ broñ z pasa, zacznij chowaæ
							u.weapon_hiding = u.weapon_taken;
							u.weapon_taken = W_NONE;
							u.weapon_state = WS_HIDING;
							u.animation_state = 0;
							SET_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
						}
						break;
					case WS_TAKEN:
						// broñ jest wyjêta, zacznij chowaæ
						u.mesh_inst->Play(u.GetTakeWeaponAnimation(bron == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
						u.weapon_hiding = bron;
						u.weapon_taken = W_NONE;
						u.weapon_state = WS_HIDING;
						u.action = A_TAKE_WEAPON;
						u.animation_state = 0;
						break;
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.unit = pc->unit;
						c.id = ((u.weapon_state == WS_HIDDEN || u.weapon_state == WS_HIDING) ? 1 : 0);
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else
				{
					// komunikat o braku broni
					AddGameMsg3(GMS_NEED_WEAPON);
				}
			}
		}
		else if(u.HaveWeapon() && GKey.KeyPressedReleaseAllowed(GK_MELEE_WEAPON))
		{
			idle = false;
			if(u.weapon_state == WS_HIDDEN)
			{
				// broñ schowana, zacznij wyjmowaæ
				u.mesh_inst->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE | PLAY_PRIO1, 1);
				u.weapon_taken = pc->ostatnia = W_ONE_HANDED;
				u.weapon_state = WS_TAKING;
				u.action = A_TAKE_WEAPON;
				u.animation_state = 0;
				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
						u.mesh_inst->Deactivate(1);
					}
					else
					{
						// schowa³ broñ za pas, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKING;
						u.animation_state = 0;
						CLEAR_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
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

				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}
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
						u.mesh_inst->Play(u.GetTakeWeaponAnimation(true), PLAY_ONCE | PLAY_PRIO1, 1);
						pc->ostatnia = u.weapon_taken = W_ONE_HANDED;
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->ostatnia = u.weapon_taken = W_ONE_HANDED;
						u.weapon_hiding = W_BOW;
						u.weapon_state = WS_HIDING;
						u.animation_state = 0;
						SET_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
					}
				}

				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
					u.mesh_inst->Play(NAMES::ani_take_bow, PLAY_BACK | PLAY_ONCE | PLAY_PRIO1, 1);

					if(pc->next_action != NA_NONE)
					{
						pc->next_action = NA_NONE;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}
		else if(u.HaveBow() && GKey.KeyPressedReleaseAllowed(GK_RANGED_WEAPON))
		{
			idle = false;
			if(u.weapon_state == WS_HIDDEN)
			{
				// broñ schowana, zacznij wyjmowaæ
				u.weapon_taken = pc->ostatnia = W_BOW;
				u.weapon_state = WS_TAKING;
				u.action = A_TAKE_WEAPON;
				u.animation_state = 0;
				u.mesh_inst->Play(NAMES::ani_take_bow, PLAY_ONCE | PLAY_PRIO1, 1);

				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
						u.mesh_inst->Deactivate(1);
					}
					else
					{
						// schowa³ ³uk, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->ostatnia = u.weapon_taken;
						u.weapon_state = WS_TAKING;
						u.animation_state = 0;
						CLEAR_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
					}
				}
				else
				{
					// chowa broñ, dodaj info ¿eby wyj¹³ broñ
					u.weapon_taken = W_BOW;
				}

				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
						u.mesh_inst->Play(NAMES::ani_take_bow, PLAY_ONCE | PLAY_PRIO1, 1);
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->ostatnia = u.weapon_taken = W_BOW;
						u.weapon_hiding = W_ONE_HANDED;
						u.weapon_state = WS_HIDING;
						u.animation_state = 0;
						SET_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
					}
				}

				if(pc->next_action != NA_NONE)
				{
					pc->next_action = NA_NONE;
					if(Net::IsClient())
						Net::PushChange(NetChange::SET_NEXT_ACTION);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
					u.mesh_inst->Play(u.GetTakeWeaponAnimation(true), PLAY_BACK | PLAY_ONCE | PLAY_PRIO1, 1);
					pc->ostatnia = u.weapon_taken = W_BOW;
					u.weapon_hiding = W_ONE_HANDED;
					u.weapon_state = WS_HIDING;
					u.animation_state = 0;
					u.action = A_TAKE_WEAPON;

					if(pc->next_action != NA_NONE)
					{
						pc->next_action = NA_NONE;
						if(Net::IsClient())
							Net::PushChange(NetChange::SET_NEXT_ACTION);
					}

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.unit = pc->unit;
						c.id = 1;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
			}
		}

		if(GKey.KeyPressedReleaseAllowed(GK_POTION) && !Equal(u.hp, u.hpmax))
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
				AddGameMsg3(GMS_NO_POTION);
		}
	} // allow_input == ALLOW_INPUT || allow_input == ALLOW_KEYBOARD

	if(u.usable)
		return;

	// sprawdŸ co jest przed graczem oraz stwórz listê pobliskich wrogów
	pc_data.before_player = BP_NONE;
	float dist, best_dist = 3.0f, best_dist_target = 3.0f;
	pc_data.selected_target = nullptr;

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

		dist = Vec3::Distance2d(u.visual_pos, u2.visual_pos);

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
		{
			//if(!(*it)->looted)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_CHEST);
		}
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
	for(vector<Usable*>::iterator it = ctx.usables->begin(), end = ctx.usables->end(); it != end; ++it)
	{
		if(!(*it)->user)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_USABLE);
	}

	if(best_dist < best_dist_target && pc_data.before_player == BP_UNIT && pc_data.before_player_ptr.unit->IsStanding())
	{
		pc_data.selected_target = pc_data.before_player_ptr.unit;
#ifdef DRAW_LOCAL_PATH
		if(Key.Down('K'))
			marked = pc_data.selected_target;
#endif
	}

	// u¿yj czegoœ przed graczem
	if(u.frozen == FROZEN::NO && pc_data.before_player != BP_NONE && !pc_data.action_ready
		&& (GKey.KeyPressedReleaseAllowed(GK_USE) || (u.IsNotFighting() && GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE))))
	{
		idle = false;
		if(pc_data.before_player == BP_UNIT)
		{
			Unit* u2 = pc_data.before_player_ptr.unit;

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
					AddGameMsg3(GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// nie mo¿na okradaæ sojuszników
					AddGameMsg3(GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->in_arena != -1)
					AddGameMsg3(GMS_DONT_LOOT_ARENA);
				else if(Net::IsLocal())
				{
					if(Net::IsOnline() && u2->busy == Unit::Busy_Looted)
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
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::LOOT_UNIT;
					c.id = u2->netid;
					pc->action = PlayerController::Action_LootUnit;
					pc->action_unit = u2;
					pc->chest_trade = &u2->items;
				}
			}
			else if(u2->IsAI() && u2->IsIdle() && u2->in_arena == -1 && u2->data->dialog && !IsEnemy(u, *u2))
			{
				if(Net::IsLocal())
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
						pc_data.before_player = BP_NONE;
					}
				}
				else
				{
					// wiadomoœæ o rozmowie do serwera
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::TALK;
					c.id = u2->netid;
					pc->action = PlayerController::Action_Talk;
					pc->action_unit = u2;
					predialog.clear();
				}
			}
		}
		else if(pc_data.before_player == BP_CHEST)
		{
			// pl¹drowanie skrzyni
			if(pc->unit->action != A_NONE)
			{
			}
			else if(Net::IsLocal())
			{
				if(pc_data.before_player_ptr.chest->looted)
				{
					// ktoœ ju¿ zajmuje siê t¹ skrzyni¹
					AddGameMsg3(GMS_IS_LOOTED);
				}
				else
				{
					// rozpocznij ograbianie skrzyni
					pc->action = PlayerController::Action_LootChest;
					pc->action_chest = pc_data.before_player_ptr.chest;
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
					pc->action_chest->mesh_inst->Play(&pc->action_chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END, 0);
					sound_mgr->PlaySound3d(sChestOpen, pc->action_chest->GetCenter(), 2.f, 5.f);

					// event handler
					if(pc_data.before_player_ptr.chest->handler)
						pc_data.before_player_ptr.chest->handler->HandleChestEvent(ChestEventHandler::Opened);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHEST_OPEN;
						c.id = pc->action_chest->netid;
					}
				}
			}
			else
			{
				// wyœlij wiadomoœæ o pl¹drowaniu skrzyni
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::LOOT_CHEST;
				c.id = pc_data.before_player_ptr.chest->netid;
				pc->action = PlayerController::Action_LootChest;
				pc->action_chest = pc_data.before_player_ptr.chest;
				pc->chest_trade = &pc->action_chest->items;
			}
		}
		else if(pc_data.before_player == BP_DOOR)
		{
			// otwieranie/zamykanie drzwi
			Door* door = pc_data.before_player_ptr.door;
			if(door->state == Door::Closed)
			{
				// otwieranie drzwi
				if(door->locked == LOCK_NONE)
				{
					if(!L.location->outside)
						minimap_opened_doors = true;
					door->state = Door::Opening;
					door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
					door->mesh_inst->frame_end_info = false;
					if(Rand() % 2 == 0)
						sound_mgr->PlaySound3d(sDoor[Rand() % 3], door->GetCenter(), 2.f, 5.f);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
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

					Vec3 center = door->GetCenter();
					if(key && pc->unit->HaveItem(Item::Get(key)))
					{
						sound_mgr->PlaySound3d(sUnlock, center, 2.f, 5.f);
						AddGameMsg3(GMS_UNLOCK_DOOR);
						if(!L.location->outside)
							minimap_opened_doors = true;
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
						door->mesh_inst->frame_end_info = false;
						if(Rand() % 2 == 0)
							sound_mgr->PlaySound3d(sDoor[Rand() % 3], center, 2.f, 5.f);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::USE_DOOR;
							c.id = door->netid;
							c.ile = 0;
						}
					}
					else
					{
						AddGameMsg3(GMS_NEED_KEY);
						sound_mgr->PlaySound3d(sDoorClosed[Rand() % 2], center, 2.f, 5.f);
					}
				}
			}
			else
			{
				// zamykanie drzwi
				door->state = Door::Closing;
				door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND | PLAY_BACK, 0);
				door->mesh_inst->frame_end_info = false;
				if(Rand() % 2 == 0)
				{
					SOUND snd;
					if(Rand() % 2 == 0)
						snd = sDoorClose;
					else
						snd = sDoor[Rand() % 3];
					sound_mgr->PlaySound3d(snd, door->GetCenter(), 2.f, 5.f);
				}
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::USE_DOOR;
					c.id = door->netid;
					c.ile = 1;
				}
			}
		}
		else if(pc_data.before_player == BP_ITEM)
		{
			// podnieœ przedmiot
			GroundItem& item = *pc_data.before_player_ptr.item;
			if(u.action == A_NONE)
			{
				bool u_gory = (item.pos.y > u.pos.y + 0.5f);

				u.action = A_PICKUP;
				u.animation = ANI_PLAY;
				u.mesh_inst->Play(u_gory ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				u.mesh_inst->frame_end_info = false;

				if(Net::IsLocal())
				{
					AddItem(u, item);

					if(item.item->type == IT_GOLD)
						sound_mgr->PlaySound2d(sCoins);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::PICKUP_ITEM;
						c.unit = pc->unit;
						c.ile = (u_gory ? 1 : 0);

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::REMOVE_ITEM;
						c2.id = pc_data.before_player_ptr.item->netid;
					}

					DeleteElement(ctx.items, pc_data.before_player_ptr.item);
					pc_data.before_player = BP_NONE;
				}
				else
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::PICKUP_ITEM;
					c.id = item.netid;

					pc_data.picking_item = &item;
					pc_data.picking_item_state = 1;
				}
			}
		}
		else if(u.action == A_NONE)
			PlayerUseUsable(pc_data.before_player_ptr.usable, false);
	}

	if(pc_data.before_player == BP_UNIT)
		pc_data.selected_unit = pc_data.before_player_ptr.unit;
	else
		pc_data.selected_unit = nullptr;

	// atak
	if(u.weapon_state == WS_TAKEN && !pc_data.action_ready)
	{
		idle = false;
		if(u.weapon_taken == W_ONE_HANDED)
		{
			if(u.action == A_ATTACK)
			{
				if(u.animation_state == 0)
				{
					if(GKey.KeyUpAllowed(pc->action_key))
					{
						// release attack
						u.attack_power = u.mesh_inst->groups[1].time / u.GetAttackFrame(0);
						u.animation_state = 1;
						u.mesh_inst->groups[1].speed = u.attack_power + u.GetAttackSpeed();
						u.attack_power += 1.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Attack;
							c.f[1] = u.mesh_inst->groups[1].speed;
						}

						if(Net::IsLocal())
							u.player->Train(TrainWhat::AttackStart, 0.f, 0);
					}
				}
				else if(u.animation_state == 2)
				{
					byte k = GKey.KeyDoReturn(GK_ATTACK_USE, &KeyStates::Down);
					if(k != VK_NONE && u.stamina > 0)
					{
						// prepare next attack
						u.action = A_ATTACK;
						u.attack_id = u.GetRandomAttack();
						u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
						u.mesh_inst->groups[1].speed = u.GetPowerAttackSpeed();
						pc->action_key = k;
						u.animation_state = 0;
						u.run_attack = false;
						u.hitted = false;
						u.timer = 0.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PrepareAttack;
							c.f[1] = u.mesh_inst->groups[1].speed;
						}

						if(Net::IsLocal())
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina);
					}
				}
			}
			else if(u.action == A_BLOCK)
			{
				if(GKey.KeyUpAllowed(pc->action_key))
				{
					// stop blocking
					u.action = A_NONE;
					u.mesh_inst->frame_end_info2 = false;
					u.mesh_inst->Deactivate(1);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StopBlock;
						c.f[1] = 1.f;
					}
				}
				else if(!u.mesh_inst->groups[1].IsBlending() && u.HaveShield())
				{
					if(GKey.KeyPressedUpAllowed(GK_ATTACK_USE) && u.stamina > 0)
					{
						// shield bash
						u.action = A_BASH;
						u.animation_state = 0;
						u.mesh_inst->Play(NAMES::ani_bash, PLAY_ONCE | PLAY_PRIO1 | PLAY_RESTORE, 1);
						u.mesh_inst->groups[1].speed = 2.f;
						u.mesh_inst->frame_end_info2 = false;
						u.hitted = false;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Bash;
							c.f[1] = 2.f;
						}

						if(Net::IsLocal())
						{
							u.player->Train(TrainWhat::BashStart, 0.f, 0);
							u.RemoveStamina(Unit::STAMINA_BASH_ATTACK);
						}
					}
				}
			}
			else if(u.action == A_NONE && u.frozen == FROZEN::NO && !GKey.KeyDownAllowed(GK_BLOCK))
			{
				byte k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &KeyStates::Down, pc_data.wasted_key);
				if(k != VK_NONE && u.stamina > 0)
				{
					u.action = A_ATTACK;
					u.attack_id = u.GetRandomAttack();
					u.mesh_inst->Play(NAMES::ani_attacks[u.attack_id], PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
					if(this_frame_run)
					{
						// running attack
						u.mesh_inst->groups[1].speed = u.GetAttackSpeed();
						u.animation_state = 1;
						u.run_attack = true;
						u.attack_power = 1.5f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_RunningAttack;
							c.f[1] = u.mesh_inst->groups[1].speed;
						}

						if(Net::IsLocal())
						{
							u.player->Train(TrainWhat::AttackStart, 0.f, 0);
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina * 1.5f);
						}
					}
					else
					{
						// prepare attack
						u.mesh_inst->groups[1].speed = u.GetPowerAttackSpeed();
						pc->action_key = k;
						u.animation_state = 0;
						u.run_attack = false;
						u.timer = 0.f;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_PrepareAttack;
							c.f[1] = u.mesh_inst->groups[1].speed;
						}

						if(Net::IsLocal())
							u.RemoveStamina(u.GetWeapon().GetInfo().stamina);
					}
					u.hitted = false;
				}
			}
			if(u.frozen == FROZEN::NO && u.HaveShield() && !u.run_attack && (u.action == A_NONE || u.action == A_ATTACK))
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
					byte k = GKey.KeyDoReturnIgnore(GK_BLOCK, &KeyStates::Down, pc_data.wasted_key);
					if(k != VK_NONE)
					{
						// start blocking
						u.action = A_BLOCK;
						u.mesh_inst->Play(NAMES::ani_block, PLAY_PRIO1 | PLAY_STOP_AT_END | PLAY_RESTORE, 1);
						u.mesh_inst->groups[1].blend_max = (oks == 2 ? 0.33f : u.GetBlockSpeed());
						pc->action_key = k;
						u.animation_state = 0;

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = pc->unit;
							c.id = AID_Block;
							c.f[1] = u.mesh_inst->groups[1].blend_max;
						}
					}
				}
			}
		}
		else
		{
			// shoting from bow
			if(u.action == A_SHOOT)
			{
				if(u.animation_state == 0 && GKey.KeyUpAllowed(pc->action_key))
				{
					u.animation_state = 1;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_Shoot;
						c.f[1] = 1.f;
					}
				}
			}
			else if(u.frozen == FROZEN::NO)
			{
				byte k = GKey.KeyDoReturnIgnore(GK_ATTACK_USE, &KeyStates::Down, pc_data.wasted_key);
				if(k != VK_NONE && u.stamina > 0)
				{
					float speed = u.GetBowAttackSpeed();
					u.mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO1 | PLAY_ONCE | PLAY_RESTORE, 1);
					u.mesh_inst->groups[1].speed = speed;
					u.action = A_SHOOT;
					u.animation_state = 0;
					u.hitted = false;
					pc->action_key = k;
					u.bow_instance = GetBowInstance(u.GetBow().mesh);
					u.bow_instance->Play(&u.bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND | PLAY_RESTORE, 0);
					u.bow_instance->groups[0].speed = speed;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::ATTACK;
						c.unit = pc->unit;
						c.id = AID_StartShoot;
						c.f[1] = speed;
					}

					if(Net::IsLocal())
						u.RemoveStamina(Unit::STAMINA_BOW_ATTACK);
				}
			}
		}
	}

	// action
	if(!pc_data.action_ready)
	{
		if(u.frozen == FROZEN::NO && u.action == A_NONE && GKey.KeyPressedReleaseAllowed(GK_ACTION) && pc->CanUseAction() && !in_tutorial)
		{
			pc_data.action_ready = true;
			pc_data.action_ok = false;
			pc_data.action_rot = 0.f;
		}
	}
	else
	{
		auto& action = pc->GetAction();
		if(action.area == Action::LINE && IS_SET(action.flags, Action::F_PICK_DIR))
		{
			// adjust action dir
			switch(move)
			{
			case 0: // none
			case 10: // forward
				pc_data.action_rot = 0.f;
				break;
			case -10: // backward
				pc_data.action_rot = PI;
				break;
			case -1: // left
				pc_data.action_rot = -PI / 2;
				break;
			case 1: // right
				pc_data.action_rot = PI / 2;
				break;
			case 9: // left forward
				pc_data.action_rot = -PI / 4;
				break;
			case 11: // right forward
				pc_data.action_rot = PI / 4;
				break;
			case -11: // left backward
				pc_data.action_rot = -PI * 3 / 4;
				break;
			case -9: // prawy ty³
				pc_data.action_rot = PI * 3 / 4;
				break;
			}
		}

		pc_data.wasted_key = GKey.KeyDoReturn(GK_ATTACK_USE, &KeyStates::PressedRelease);
		if(pc_data.wasted_key != VK_NONE)
		{
			if(pc_data.action_ok)
				UseAction(pc, false);
			else
				pc_data.wasted_key = VK_NONE;
		}
		else
		{
			pc_data.wasted_key = GKey.KeyDoReturn(GK_BLOCK, &KeyStates::PressedRelease);
			if(pc_data.wasted_key != VK_NONE)
				pc_data.action_ready = false;
		}
	}

	// animacja idle
	if(idle && u.action == A_NONE)
	{
		pc->idle_timer += dt;
		if(pc->idle_timer >= 4.f)
		{
			if(u.animation == ANI_LEFT || u.animation == ANI_RIGHT)
				pc->idle_timer = Random(2.f, 3.f);
			else
			{
				int id = Rand() % u.data->idles->anims.size();
				pc->idle_timer = Random(0.f, 0.5f);
				u.mesh_inst->Play(u.data->idles->anims[id].c_str(), PLAY_ONCE, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				u.mesh_inst->frame_end_info = false;
				u.animation = ANI_IDLE;

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::IDLE;
					c.unit = pc->unit;
					c.id = id;
				}
			}
		}
	}
	else
		pc->idle_timer = Random(0.f, 0.5f);
}

//=================================================================================================
void Game::UseAction(PlayerController* p, bool from_server, const Vec3* pos)
{
	if(p == pc && from_server)
		return;

	auto& action = p->GetAction();
	if(action.sound)
		PlayAttachedSound(*p->unit, action.sound->sound, action.sound_dist);

	if(!from_server)
	{
		p->UseActionCharge();
		if(action.stamina_cost > 0)
			p->unit->RemoveStamina(action.stamina_cost);
	}

	Vec3 action_point;
	if(strcmp(action.id, "dash") == 0 || strcmp(action.id, "bull_charge") == 0)
	{
		bool dash = (strcmp(action.id, "dash") == 0);
		p->unit->action = A_DASH;
		if(Net::IsLocal() || !from_server)
		{
			p->unit->use_rot = Clip(pc_data.action_rot + p->unit->rot + PI);
			action_point = Vec3(pc_data.action_rot, 0, 0);
		}
		else if(!from_server)
		{
			assert(pos);
			p->unit->use_rot = Clip(pos->x + p->unit->rot + PI);
		}
		p->unit->animation = ANI_RUN;
		p->unit->current_animation = ANI_RUN;
		p->unit->mesh_inst->Play(NAMES::ani_run, PLAY_PRIO1 | PLAY_NO_BLEND, 0);
		if(dash)
		{
			p->unit->animation_state = 0;
			p->unit->timer = 0.33f;
			p->unit->speed = action.area_size.x / 0.33f;
			p->unit->mesh_inst->groups[0].speed = 3.f;
		}
		else
		{
			p->unit->animation_state = 1;
			p->unit->timer = 0.5f;
			p->unit->speed = action.area_size.x / 0.5f;
			p->unit->mesh_inst->groups[0].speed = 2.5f;
			p->action_targets.clear();
			p->unit->mesh_inst->groups[1].blend_max = 0.1f;
			p->unit->mesh_inst->Play("charge", PLAY_PRIO1, 1);
		}
	}
	else if(strcmp(action.id, "summon_wolf") == 0)
	{
		// cast animation
		p->unit->action = A_CAST;
		p->unit->attack_id = -1;
		p->unit->animation_state = 0;
		p->unit->mesh_inst->frame_end_info2 = false;
		p->unit->mesh_inst->Play("cast", PLAY_ONCE | PLAY_PRIO1, 1);

		if(Net::IsLocal())
		{
			// despawn old
			Unit* existing_unit = L.FindUnit([=](Unit* u) { return u->summoner == p->unit; });
			if(existing_unit)
			{
				RemoveTeamMember(existing_unit);
				RemoveUnit(existing_unit);
			}

			// spawn new
			Vec3 spawn_pos;
			if(p == pc)
				spawn_pos = pc_data.action_point;
			else
			{
				assert(pos);
				spawn_pos = *pos;
			}
			auto unit = SpawnUnitNearLocation(L.GetContext(*p->unit), spawn_pos, *UnitData::Get("white_wolf_sum"), nullptr, p->unit->level);
			if(unit)
			{
				unit->summoner = p->unit;
				unit->in_arena = p->unit->in_arena;
				if(unit->in_arena != -1)
					at_arena.push_back(unit);
				if(Net::IsServer())
					Net_SpawnUnit(unit);
				AddTeamMember(unit, true);
				SpawnUnitEffect(*unit);
			}
		}
		else if(!from_server)
			action_point = pc_data.action_point;
	}

	if(Net::IsOnline())
	{
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PLAYER_ACTION;
			c.unit = p->unit;
		}
		else if(!from_server)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::PLAYER_ACTION;
			c.pos = action_point;
		}
	}

	if(p == pc)
		pc_data.action_ready = false;
}

//=================================================================================================
void Game::SpawnUnitEffect(Unit& unit)
{
	Vec3 real_pos = unit.pos;
	real_pos.y += 1.f;
	sound_mgr->PlaySound3d(sSummon, real_pos, 1.5f);

	ParticleEmitter* pe = new ParticleEmitter;
	pe->tex = tSpawn;
	pe->emision_interval = 0.1f;
	pe->life = 5.f;
	pe->particle_life = 0.5f;
	pe->emisions = 5;
	pe->spawn_min = 10;
	pe->spawn_max = 15;
	pe->max_particles = 15 * 5;
	pe->pos = unit.pos;
	pe->speed_min = Vec3(-1, 0, -1);
	pe->speed_max = Vec3(1, 1, 1);
	pe->pos_min = Vec3(-0.75f, 0, -0.75f);
	pe->pos_max = Vec3(0.75f, 1.f, 0.75f);
	pe->size = 0.3f;
	pe->op_size = POP_LINEAR_SHRINK;
	pe->alpha = 0.5f;
	pe->op_alpha = POP_LINEAR_SHRINK;
	pe->mode = 0;
	pe->Init();
	L.GetContext(unit).pes->push_back(pe);
}

//=================================================================================================
void Game::PlayerCheckObjectDistance(Unit& u, const Vec3& pos, void* ptr, float& best_dist, BeforePlayer type)
{
	assert(ptr);

	if(pos.y < u.pos.y - 0.5f || pos.y > u.pos.y + 2.f)
		return;

	float dist = Vec3::Distance2d(u.pos, pos);
	if(dist < PICKUP_RANGE && dist < best_dist)
	{
		float angle = AngleDiff(Clip(u.rot + PI / 2), Clip(-Vec3::Angle2d(u.pos, pos)));
		assert(angle >= 0.f);
		if(angle < PI / 4)
		{
			if(type == BP_CHEST)
			{
				Chest* chest = (Chest*)ptr;
				if(AngleDiff(Clip(chest->rot - PI / 2), Clip(-Vec3::Angle2d(u.pos, pos))) > PI / 2)
					return;
			}
			else if(type == BP_USABLE)
			{
				Usable* use = (Usable*)ptr;
				auto& bu = *use->base;
				if(IS_SET(bu.use_flags, BaseUsable::CONTAINER))
				{
					float allowed_dif;
					switch(bu.limit_rot)
					{
					default:
					case 0:
						allowed_dif = PI * 2;
						break;
					case 1:
						allowed_dif = PI / 2;
						break;
					case 2:
						allowed_dif = PI / 4;
						break;
					}
					if(AngleDiff(Clip(use->rot - PI / 2), Clip(-Vec3::Angle2d(u.pos, pos))) > allowed_dif)
						return;
				}
			}
			dist += angle;
			if(dist < best_dist)
			{
				best_dist = dist;
				pc_data.before_player_ptr.any = ptr;
				pc_data.before_player = type;
			}
		}
	}
}

//=================================================================================================
int Game::CheckMove(Vec3& _pos, const Vec3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	Vec3 new_pos = _pos + _dir;
	Vec3 gather_pos = _pos + _dir / 2;
	float gather_radius = _dir.Length() + _radius;
	global_col.clear();

	IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { _me, nullptr };
	ignore.ignored_units = (const Unit**)ignored;
	GatherCollisionObjects(L.GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ po x
	Vec3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
}

//=================================================================================================
int Game::CheckMovePhase(Vec3& _pos, const Vec3& _dir, float _radius, Unit* _me, bool* is_small)
{
	assert(_radius > 0.f && _me);

	Vec3 new_pos = _pos + _dir;
	Vec3 gather_pos = _pos + _dir / 2;
	float gather_radius = _dir.Length() + _radius;

	global_col.clear();

	IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { _me, nullptr };
	ignore.ignored_units = (const Unit**)ignored;
	ignore.ignore_objects = true;
	GatherCollisionObjects(L.GetContext(*_me), global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!Collide(global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ po x
	Vec3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// jeœli zablokowa³ siê w innej jednostce to wyjdŸ z niej
	if(Collide(global_col, _pos, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 4;
	}

	// nie ma drogi
	return 0;
}

//=================================================================================================
void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const Vec3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x - _radius) / 2)),
		maxx = int((_pos.x + _radius) / 2),
		minz = max(0, int((_pos.z - _radius) / 2)),
		maxz = int((_pos.z + _radius) / 2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(L.location->outside)
		{
			City* city = (City*)L.location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					POLE co = lvl.map[x + z * lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
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
	Vec3 pos;
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
			}
			while(1);

			radius = (*it)->GetUnitRadius();
			pos = (*it)->GetColliderPos();
			if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = Vec2(pos.x, pos.z);
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
			Vec3 pos = (*it)->GetColliderPos();
			if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
			{
				CollisionObject& co = Add1(_objects);
				co.pt = Vec2(pos.x, pos.z);
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
				co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = 0.842f;
				co.h = 0.181f;
				co.rot = (*it)->rot;
			}
		}
	}
}

//=================================================================================================
void Game::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const Box2d& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x / 2)),
		maxx = int(_box.v2.x / 2),
		minz = max(0, int(_box.v1.y / 2)),
		maxz = int(_box.v2.y / 2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(L.location->outside)
		{
			City* city = (City*)L.location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					POLE co = lvl.map[x + z * lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
							co.check = &Game::CollideWithStairs;
							co.check_rect = &Game::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(co == SCHODY_GORA)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.check = &Game::CollideWithStairs;
						co.check_rect = &Game::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
					}
				}
			}
		}
	}

	Vec2 rectpos = _box.Midpoint(),
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
			}
			while(1);

			radius = (*it)->GetUnitRadius();
			if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
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
				co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
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
				if(RectangleToRectangle(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
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
				if(RectangleToRectangle(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
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

//=================================================================================================
bool Game::Collide(const vector<CollisionObject>& _objects, const Vec3& _pos, float _radius)
{
	assert(_radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(Distance(_pos.x, _pos.z, it->pt.x, it->pt.y) <= it->radius + _radius)
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

//=================================================================================================
bool Game::Collide(const vector<CollisionObject>& _objects, const Box2d& _box, float _margin)
{
	Box2d box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pt.x, it->pt.y, it->radius + _margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
				r1.size.x = it->w + _margin;
				r1.size.y = it->h + _margin;
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

//=================================================================================================
bool Game::Collide(const vector<CollisionObject>& _objects, const Box2d& _box, float margin, float _rot)
{
	if(!NotZero(_rot))
		return Collide(_objects, _box, margin);

	Box2d box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	Vec2 rectpos = box.Midpoint(),
		rectsize = box.Size() / 2;

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

//=================================================================================================
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
	if(Net::IsLocal())
	{
		// this vars are useless for clients, don't increase ref counter
		ctx.talker->busy = Unit::Busy_Talking;
		ctx.talker->look_target = ctx.pc->unit;
	}
	ctx.update_news = true;
	ctx.update_locations = 1;
	ctx.pc->action = PlayerController::Action_Talk;
	ctx.pc->action_unit = talker;
	ctx.not_active = false;
	ctx.choices.clear();
	ctx.can_skip = true;
	ctx.dialog = dialog ? dialog : talker->data->dialog;
	ctx.force_end = false;
	ctx.negate_if = false;

	if(Net::IsLocal())
	{
		// dŸwiêk powitania
		SOUND snd = GetTalkSound(*ctx.talker);
		if(snd)
		{
			PlayAttachedSound(*ctx.talker, snd, 2.f, 5.f);
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
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

//=================================================================================================
void Game::EndDialog(DialogContext& ctx)
{
	ctx.choices.clear();
	ctx.prev.clear();
	ctx.dialog_mode = false;

	if(ctx.talker->busy == Unit::Busy_Trading)
	{
		if(!ctx.is_local)
		{
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::END_DIALOG;
		}

		return;
	}

	ctx.talker->busy = Unit::Busy_No;
	ctx.talker->look_target = nullptr;
	ctx.talker = nullptr;
	ctx.pc->action = PlayerController::Action_None;

	if(!ctx.is_local)
	{
		NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
		c.type = NetChangePlayer::END_DIALOG;
	}
}

//=================================================================================================
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

//							WEAPON	BOW		SHIELD	ARMOR	LETTER	POTION	OTHER	BOOK	GOLD
bool merchant_buy[] = { true,	true,	true,	true,	true,	true,	true,	true,	false };
bool blacksmith_buy[] = { true,	true,	true,	true,	false,	false,	false,	false,	false };
bool alchemist_buy[] = { false,	false,	false,	false,	false,	true,	false,	false,	false };
bool innkeeper_buy[] = { false,	false,	false,	false,	false,	true,	false,	false,	false };
bool foodseller_buy[] = { false,	false,	false,	false,	false,	true,	false,	false,	false };

//=================================================================================================
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

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
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
				if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG)
					|| GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG)
					|| (GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
					skip = true;
				else
				{
					pc_data.wasted_key = GKey.KeyDoReturn(GK_ATTACK_USE, &KeyStates::PressedRelease);
					if(pc_data.wasted_key != VK_NONE)
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

	if(ctx.force_end)
	{
		EndDialog(ctx);
		return;
	}

	int if_level = ctx.dialog_level;

	while(1)
	{
		DialogEntry& de = *(ctx.dialog->code.data() + ctx.dialog_pos);

		switch(de.type)
		{
		case DTF_CHOICE:
			if(if_level == ctx.dialog_level)
			{
				cstring text = ctx.GetText((int)de.msg);
				if(text[0] == '$')
				{
					if(strncmp(text, "$player", 7) == 0)
					{
						int id = int(text[7] - '1');
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos + 1, near_players_str[id].c_str(), ctx.dialog_level + 1));
					}
					else
						ctx.choices.push_back(DialogChoice(ctx.dialog_pos + 1, "!Broken choice!", ctx.dialog_level + 1));
				}
				else
					ctx.choices.push_back(DialogChoice(ctx.dialog_pos + 1, text, ctx.dialog_level + 1));
			}
			++if_level;
			break;
		case DTF_END_CHOICE:
		case DTF_END_IF:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			--if_level;
			break;
		case DTF_END:
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
		case DTF_END2:
			if(if_level == ctx.dialog_level)
			{
				EndDialog(ctx);
				return;
			}
			break;
		case DTF_SHOW_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				ctx.show_choices = true;
				if(ctx.is_local)
				{
					ctx.choice_selected = 0;
					dialog_cursor_pos = Int2(-1, -1);
					game_gui->UpdateScrollbar(ctx.choices.size());
				}
				else
				{
					ctx.choice_selected = -1;
					NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
					c.type = NetChangePlayer::SHOW_DIALOG_CHOICES;
				}
				return;
			}
			break;
		case DTF_RESTART:
			if(if_level == ctx.dialog_level)
				ctx.dialog_pos = -1;
			break;
		case DTF_TRADE:
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
					ctx.pc->chest_trade = &QM.quest_orcs2->wares;
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
					NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
					c.type = NetChangePlayer::START_TRADE;
					c.id = t->netid;

					trader_buy = old_trader_buy;
				}

				ctx.pc->Train(TrainWhat::Trade, 0.f, 0);

				return;
			}
			break;
		case DTF_TALK:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.GetText((int)de.msg);
				DialogTalk(ctx, msg);
				++ctx.dialog_pos;
				return;
			}
			break;
		case DTF_TALK2:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.GetText((int)de.msg);

				static string str_part;
				ctx.dialog_s_text.clear();

				for(uint i = 0, len = strlen(msg); i < len; ++i)
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
		case DTF_SPECIAL:
			if(ctx.dialog_level == if_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				if(ExecuteGameDialogSpecial(ctx, msg, if_level))
					return;
			}
			break;
		case DTF_SET_QUEST_PROGRESS:
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
		case DTF_IF_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				bool ok = (ctx.dialog_quest->IsActive() && ctx.dialog_quest->IsTimedout());
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_RAND:
			if(if_level == ctx.dialog_level)
			{
				bool ok = (Rand() % int(de.msg) == 0);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_ONCE:
			if(if_level == ctx.dialog_level)
			{
				bool ok = ctx.dialog_once;
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
				{
					ctx.dialog_once = false;
					++ctx.dialog_level;
				}
			}
			++if_level;
			break;
		case DTF_DO_ONCE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_once = false;
			break;
		case DTF_ELSE:
			if(if_level == ctx.dialog_level)
				--ctx.dialog_level;
			else if(if_level == ctx.dialog_level + 1)
				++ctx.dialog_level;
			break;
		case DTF_CHECK_QUEST_TIMEOUT:
			if(if_level == ctx.dialog_level)
			{
				Quest* quest = QM.FindQuest(L.location_index, (QuestType)(int)de.msg);
				if(quest && quest->IsActive() && quest->IsTimedout())
				{
					ctx.dialog_once = false;
					StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_FAIL), if_level, quest);
				}
			}
			break;
		case DTF_IF_HAVE_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				bool ok = FindQuestItem2(ctx.pc->unit, msg, nullptr, nullptr, ctx.not_active);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
				ctx.not_active = false;
			}
			++if_level;
			break;
		case DTF_DO_QUEST_ITEM:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				Quest* quest;
				if(FindQuestItem2(ctx.pc->unit, msg, &quest, nullptr))
					StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
			}
			break;
		case DTF_IF_QUEST_PROGRESS:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				bool ok = (ctx.dialog_quest->prog == (int)de.msg);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_QUEST_PROGRESS_RANGE:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				int x = int(de.msg) & 0xFFFF;
				int y = (int(de.msg) & 0xFFFF0000) >> 16;
				assert(y > x);
				bool ok = InRange(ctx.dialog_quest->prog, x, y);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_NEED_TALK:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				bool negate = false;
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					negate = true;
				}

				for(Quest* quest : QM.quests)
				{
					bool ok = (quest->IsActive() && quest->IfNeedTalk(msg));
					if(negate)
						ok = !ok;
					if(ok)
					{
						++ctx.dialog_level;
						break;
					}
				}
			}
			++if_level;
			break;
		case DTF_DO_QUEST:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				for(Quest* quest : QM.quests)
				{
					if(quest->IsActive() && quest->IfNeedTalk(msg))
					{
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}
			}
			break;
		case DTF_ESCAPE_CHOICE:
			if(if_level == ctx.dialog_level)
				ctx.dialog_esc = (int)ctx.choices.size() - 1;
			break;
		case DTF_IF_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				bool ok = ExecuteGameDialogSpecialIf(ctx, msg);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_CHOICES:
			if(if_level == ctx.dialog_level)
			{
				bool ok = (ctx.choices.size() == (int)de.msg);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_DO_QUEST2:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();

				for(Quest* quest : QM.quests)
				{
					if(quest->IfNeedTalk(msg))
					{
						StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_NEXT), if_level, quest);
						break;
					}
				}

				if(ctx.dialog_pos != -1)
				{
					for(Quest* quest : QM.unaccepted_quests)
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
		case DTF_IF_HAVE_ITEM:
			if(if_level == ctx.dialog_level)
			{
				const Item* item = (const Item*)de.msg;
				bool ok = ctx.pc->unit->HaveItem(item);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_IF_QUEST_EVENT:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				bool ok = ctx.dialog_quest->IfQuestEvent();
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_END_OF_DIALOG:
			assert(0);
			throw Format("Broken dialog '%s'.", ctx.dialog->id.c_str());
		case DTF_NOT_ACTIVE:
			ctx.not_active = true;
			break;
		case DTF_IF_QUEST_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				bool ok = ctx.dialog_quest->SpecialIf(ctx, msg);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		case DTF_QUEST_SPECIAL:
			if(if_level == ctx.dialog_level)
			{
				assert(ctx.dialog_quest);
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				ctx.dialog_quest->Special(ctx, msg);
			}
			break;
		case DTF_NOT:
			if(if_level == ctx.dialog_level)
				ctx.negate_if = true;
			break;
		case DTF_SCRIPT:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				SM.SetContext(ctx.pc, ctx.talker);
				SM.RunScript(msg);
			}
			break;
		case DTF_IF_SCRIPT:
			if(if_level == ctx.dialog_level)
			{
				cstring msg = ctx.dialog->strs[(int)de.msg].c_str();
				SM.SetContext(ctx.pc, ctx.talker);
				bool ok = SM.RunIfScript(msg);
				if(ctx.negate_if)
				{
					ctx.negate_if = false;
					ok = !ok;
				}
				if(ok)
					++ctx.dialog_level;
			}
			++if_level;
			break;
		default:
			assert(0 && "Unknown dialog type!");
			break;
		}

		++ctx.dialog_pos;
	}
}

bool Game::ExecuteGameDialogSpecial(DialogContext& ctx, cstring msg, int& if_level)
{
	if(QM.HandleSpecial(ctx, msg))
		return false;

	if(strcmp(msg, "burmistrz_quest") == 0)
	{
		bool have_quest = true;
		if(L.city_ctx->quest_mayor == CityQuestState::Failed)
		{
			DialogTalk(ctx, RandomString(txMayorQFailed));
			++ctx.dialog_pos;
			return true;
		}
		else if(W.GetWorldtime() - L.city_ctx->quest_mayor_time > 30 || L.city_ctx->quest_mayor_time == -1)
		{
			if(L.city_ctx->quest_mayor == CityQuestState::InProgress)
			{
				Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Mayor);
				DeleteElement(QM.unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			L.city_ctx->quest_mayor_time = W.GetWorldtime();
			L.city_ctx->quest_mayor = CityQuestState::InProgress;

			Quest* quest = QM.GetMayorQuest();
			if(quest)
			{
				// add new quest
				quest->refid = QM.quest_counter++;
				quest->Start();
				QM.unaccepted_quests.push_back(quest);
				StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
			}
			else
				have_quest = false;
		}
		else if(L.city_ctx->quest_mayor == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie ?
			Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Mayor);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
			}
			else
			{
				quest = QM.FindQuest(L.location_index, QuestType::Mayor);
				if(quest)
				{
					DialogTalk(ctx, RandomString(txQuestAlreadyGiven));
					++ctx.dialog_pos;
					return true;
				}
				else
					have_quest = false;
			}
		}
		else
			have_quest = false;
		if(!have_quest)
		{
			DialogTalk(ctx, RandomString(txMayorNoQ));
			++ctx.dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "dowodca_quest") == 0)
	{
		bool have_quest = true;
		if(L.city_ctx->quest_captain == CityQuestState::Failed)
		{
			DialogTalk(ctx, RandomString(txCaptainQFailed));
			++ctx.dialog_pos;
			return true;
		}
		else if(W.GetWorldtime() - L.city_ctx->quest_captain_time > 30 || L.city_ctx->quest_captain_time == -1)
		{
			if(L.city_ctx->quest_captain == CityQuestState::InProgress)
			{
				Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Captain);
				DeleteElement(QM.unaccepted_quests, quest);
			}

			// jest nowe zadanie (mo¿e), czas starego min¹³
			L.city_ctx->quest_captain_time = W.GetWorldtime();
			L.city_ctx->quest_captain = CityQuestState::InProgress;

			Quest* quest = QM.GetCaptainQuest();
			if(quest)
			{
				// add new quest
				quest->refid = QM.quest_counter++;
				quest->Start();
				QM.unaccepted_quests.push_back(quest);
				StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
			}
			else
				have_quest = false;
		}
		else if(L.city_ctx->quest_captain == CityQuestState::InProgress)
		{
			// ju¿ ma przydzielone zadanie
			Quest* quest = QM.FindUnacceptedQuest(L.location_index, QuestType::Captain);
			if(quest)
			{
				// quest nie zosta³ zaakceptowany
				StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
			}
			else
			{
				quest = QM.FindQuest(L.location_index, QuestType::Captain);
				if(quest)
				{
					DialogTalk(ctx, RandomString(txQuestAlreadyGiven));
					++ctx.dialog_pos;
					return true;
				}
				else
					have_quest = false;
			}
		}
		else
			have_quest = false;
		if(!have_quest)
		{
			DialogTalk(ctx, RandomString(txCaptainNoQ));
			++ctx.dialog_pos;
			return true;
		}
	}
	else if(strcmp(msg, "przedmiot_quest") == 0)
	{
		if(ctx.talker->quest_refid == -1)
		{
			Quest* quest = QM.GetAdventurerQuest();
			quest->refid = QM.quest_counter++;
			ctx.talker->quest_refid = quest->refid;
			quest->Start();
			QM.unaccepted_quests.push_back(quest);
			StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
		}
		else
		{
			Quest* quest = QM.FindUnacceptedQuest(ctx.talker->quest_refid);
			StartNextDialog(ctx, quest->GetDialog(QUEST_DIALOG_START), if_level, quest);
		}
	}
	else if(strcmp(msg, "arena_combat1") == 0 || strcmp(msg, "arena_combat2") == 0 || strcmp(msg, "arena_combat3") == 0)
		StartArenaCombat(msg[strlen("arena_combat")] - '0');
	else if(strcmp(msg, "rest1") == 0 || strcmp(msg, "rest5") == 0 || strcmp(msg, "rest10") == 0 || strcmp(msg, "rest30") == 0)
	{
		// rest in inn
		int days, cost;
		if(strcmp(msg, "rest1") == 0)
		{
			days = 1;
			cost = 5;
		}
		else if(strcmp(msg, "rest5") == 0)
		{
			days = 5;
			cost = 20;
		}
		else if(strcmp(msg, "rest10") == 0)
		{
			days = 10;
			cost = 35;
		}
		else
		{
			days = 30;
			cost = 100;
		}

		// does player have enough gold?
		if(ctx.pc->unit->gold < cost)
		{
			// restart dialog
			ctx.dialog_s_text = Format(txNeedMoreGold, cost - ctx.pc->unit->gold);
			DialogTalk(ctx, ctx.dialog_s_text.c_str());
			ctx.dialog_pos = 0;
			ctx.dialog_level = 0;
			return true;
		}

		// give gold and freeze
		ctx.pc->unit->ModGold(-cost);
		ctx.pc->unit->frozen = FROZEN::YES;
		if(ctx.is_local)
		{
			fallback_type = FALLBACK::REST;
			fallback_t = -1.f;
			fallback_1 = days;
		}
		else
		{
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::REST;
			c.id = days;
		}
	}
	else if(strcmp(msg, "gossip") == 0 || strcmp(msg, "gossip_drunk") == 0)
	{
		bool pijak = (strcmp(msg, "gossip_drunk") == 0);
		if(!pijak && (Rand() % 3 == 0 || (Key.Down(VK_SHIFT) && devmode)))
		{
			int co = Rand() % 3;
			if(QM.quest_rumor_counter != 0 && Rand() % 2 == 0)
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
			const vector<Location*>& locations = W.GetLocations();
			switch(co)
			{
			case 0:
				// info o nieznanej lokacji
				{
					int id = Rand() % locations.size();
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
						loc.SetKnown();
						Location& cloc = *L.location;
						cstring s_daleko;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
						if(dist <= 300)
							s_daleko = txNear;
						else if(dist <= 500)
							s_daleko = "";
						else if(dist <= 700)
							s_daleko = txFar;
						else
							s_daleko = txVeryFar;

						ctx.dialog_s_text = Format(RandomString(txLocationDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), loc.name.c_str());
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						++ctx.dialog_pos;

						return true;
					}
					else
					{
						DialogTalk(ctx, RandomString(txAllDiscovered));
						++ctx.dialog_pos;
						return true;
					}
				}
				break;
			case 1:
				// info o obozie
				{
					Location* new_camp = nullptr;
					static vector<Location*> camps;
					for(vector<Location*>::const_iterator it = locations.begin(), end = locations.end(); it != end; ++it)
					{
						if(*it && (*it)->type == L_CAMP && (*it)->state != LS_HIDDEN)
						{
							camps.push_back(*it);
							if((*it)->state == LS_UNKNOWN && !new_camp)
								new_camp = *it;
						}
					}

					if(!new_camp && !camps.empty())
						new_camp = camps[Rand() % camps.size()];

					camps.clear();

					if(new_camp)
					{
						Location& loc = *new_camp;
						Location& cloc = *L.location;
						cstring s_daleko;
						float dist = Vec2::Distance(loc.pos, cloc.pos);
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
						case SG_ORCS:
							co = txSGOOrcs;
							break;
						case SG_BANDITS:
							co = txSGOBandits;
							break;
						case SG_GOBLINS:
							co = txSGOGoblins;
							break;
						default:
							assert(0);
							co = txSGOEnemies;
							break;
						}

						loc.SetKnown();

						ctx.dialog_s_text = Format(RandomString(txCampDiscovered), s_daleko, GetLocationDirName(cloc.pos, loc.pos), co);
						DialogTalk(ctx, ctx.dialog_s_text.c_str());
						++ctx.dialog_pos;
						return true;
					}
					else
					{
						DialogTalk(ctx, RandomString(txAllCampDiscovered));
						++ctx.dialog_pos;
						return true;
					}
				}
				break;
			case 2:
				// plotka o quescie
				if(QM.quest_rumor_counter == 0)
				{
					DialogTalk(ctx, RandomString(txNoQRumors));
					++ctx.dialog_pos;
					return true;
				}
				else
				{
					co = Rand() % P_MAX;
					while(QM.quest_rumor[co])
						co = (co + 1) % P_MAX;
					--QM.quest_rumor_counter;
					QM.quest_rumor[co] = true;

					switch(co)
					{
					case P_TARTAK:
						ctx.dialog_s_text = Format(txRumorQ[0], locations[QM.quest_sawmill->start_loc]->name.c_str());
						break;
					case P_KOPALNIA:
						ctx.dialog_s_text = Format(txRumorQ[1], locations[QM.quest_mine->start_loc]->name.c_str());
						break;
					case P_ZAWODY_W_PICIU:
						ctx.dialog_s_text = txRumorQ[2];
						break;
					case P_BANDYCI:
						ctx.dialog_s_text = Format(txRumorQ[3], locations[QM.quest_bandits->start_loc]->name.c_str());
						break;
					case P_MAGOWIE:
						ctx.dialog_s_text = Format(txRumorQ[4], locations[QM.quest_mages->start_loc]->name.c_str());
						break;
					case P_MAGOWIE2:
						ctx.dialog_s_text = txRumorQ[5];
						break;
					case P_ORKOWIE:
						ctx.dialog_s_text = Format(txRumorQ[6], locations[QM.quest_orcs->start_loc]->name.c_str());
						break;
					case P_GOBLINY:
						ctx.dialog_s_text = Format(txRumorQ[7], locations[QM.quest_goblins->start_loc]->name.c_str());
						break;
					case P_ZLO:
						ctx.dialog_s_text = Format(txRumorQ[8], locations[QM.quest_evil->start_loc]->name.c_str());
						break;
					default:
						assert(0);
						return true;
					}

					game_gui->journal->AddRumor(ctx.dialog_s_text.c_str());
					DialogTalk(ctx, ctx.dialog_s_text.c_str());
					++ctx.dialog_pos;
					return true;
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
				uint co = Rand() % ile;
				if(co < countof(txRumor))
					plotka = txRumor[co];
				else
					plotka = txRumorD[co - countof(txRumor)];
			}
			while(ctx.ostatnia_plotka == plotka);
			ctx.ostatnia_plotka = plotka;

			static string str, str_part;
			str.clear();
			for(uint i = 0, len = strlen(plotka); i < len; ++i)
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
			return true;
		}
	}
	else if(strncmp(msg, "train_", 6) == 0)
	{
		const int cost = 200;
		cstring s = msg + 6;
		bool is_skill;
		int what;

		auto attrib = Attribute::Find(s);
		if(attrib)
		{
			is_skill = false;
			what = (int)attrib->attrib_id;
		}
		else
		{
			auto skill = Skill::Find(s);
			if(skill)
			{
				is_skill = true;
				what = (int)skill->skill_id;
			}
			else
			{
				assert(0);
				is_skill = false;
				what = (int)AttributeId::STR;
			}
		}

		// does player have enough gold?
		if(ctx.pc->unit->gold < cost)
		{
			ctx.dialog_s_text = Format(txNeedMoreGold, cost - ctx.pc->unit->gold);
			DialogTalk(ctx, ctx.dialog_s_text.c_str());
			ctx.force_end = true;
			return true;
		}

		// give gold and freeze
		ctx.pc->unit->ModGold(-cost);
		ctx.pc->unit->frozen = FROZEN::YES;
		if(ctx.is_local)
		{
			fallback_type = FALLBACK::TRAIN;
			fallback_t = -1.f;
			fallback_1 = (is_skill ? 1 : 0);
			fallback_2 = what;
		}
		else
		{
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::TRAIN;
			c.id = (is_skill ? 1 : 0);
			c.ile = what;
		}
	}
	else if(strcmp(msg, "near_loc") == 0)
	{
		const vector<Location*>& locations = W.GetLocations();
		if(ctx.update_locations == 1)
		{
			ctx.active_locations.clear();
			const Vec2& world_pos = W.GetWorldPos();

			int index = 0;
			for(Location* loc : locations)
			{
				if(loc && loc->type != L_CITY && loc->type != L_ACADEMY && Vec2::Distance(loc->pos, world_pos) <= 150.f && loc->state != LS_HIDDEN)
					ctx.active_locations.push_back(std::pair<int, bool>(index, loc->state == LS_UNKNOWN));
				++index;
			}

			if(!ctx.active_locations.empty())
			{
				std::random_shuffle(ctx.active_locations.begin(), ctx.active_locations.end(), MyRand);
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
			return true;
		}
		else if(ctx.active_locations.empty())
		{
			DialogTalk(ctx, txAllNearLoc);
			++ctx.dialog_pos;
			return true;
		}

		int id = ctx.active_locations.back().first;
		ctx.active_locations.pop_back();
		Location& loc = *locations[id];
		loc.SetKnown();
		ctx.dialog_s_text = Format(txNearLoc, GetLocationDirName(W.GetWorldPos(), loc.pos), loc.name.c_str());
		if(loc.spawn == SG_NONE)
		{
			if(loc.type != L_CAVE && loc.type != L_FOREST && loc.type != L_MOONWELL)
				ctx.dialog_s_text += RandomString(txNearLocEmpty);
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
			ctx.dialog_s_text += Format(RandomString(txNearLocEnemy), jacy, g_spawn_groups[loc.spawn].name);
		}

		DialogTalk(ctx, ctx.dialog_s_text.c_str());
		++ctx.dialog_pos;
		return true;
	}
	else if(strcmp(msg, "tell_name") == 0)
		ctx.talker->RevealName(false);
	else if(strcmp(msg, "hero_about") == 0)
	{
		Class clas = ctx.talker->GetClass();
		if(clas < Class::MAX)
			DialogTalk(ctx, ClassInfo::classes[(int)clas].about.c_str());
		++ctx.dialog_pos;
		return true;
	}
	else if(strcmp(msg, "recruit") == 0)
	{
		int cost = ctx.talker->hero->JoinCost();
		ctx.pc->unit->ModGold(-cost);
		ctx.talker->gold += cost;
		AddTeamMember(ctx.talker, false);
		ctx.talker->temporary = false;
		Team.free_recruit = false;
		ctx.talker->hero->SetupMelee();
		if(Net::IsOnline() && !ctx.is_local)
			ctx.pc->player_info->UpdateGold();
	}
	else if(strcmp(msg, "recruit_free") == 0)
	{
		AddTeamMember(ctx.talker, false);
		Team.free_recruit = false;
		ctx.talker->temporary = false;
		ctx.talker->hero->SetupMelee();
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
		ctx.talker->ai->loc_timer = Random(5.f, 10.f);
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
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::START_GIVE;
		}
		return true;
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
			NetChangePlayer& c = Add1(ctx.pc->player_info->changes);
			c.type = NetChangePlayer::START_SHARE;
		}
		return true;
	}
	else if(strcmp(msg, "kick_npc") == 0)
	{
		RemoveTeamMember(ctx.talker);
		if(L.city_ctx)
			ctx.talker->hero->mode = HeroData::Wander;
		else
			ctx.talker->hero->mode = HeroData::Leave;
		ctx.talker->hero->credit = 0;
		ctx.talker->ai->city_wander = true;
		ctx.talker->ai->loc_timer = Random(5.f, 10.f);
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
	else if(strcmp(msg, "attack") == 0)
	{
		// ta komenda jest zbyt ogólna, jeœli bêdzie kilka takich grup to wystarczy ¿e jedna tego u¿yje to wszyscy zaatakuj¹, nie obs³uguje te¿ budynków
		if(ctx.talker->data->group == G_CRAZIES)
		{
			if(!Team.crazies_attack)
			{
				Team.crazies_attack = true;
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CHANGE_FLAGS;
				}
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->dont_attack && IsEnemy(**it, *Team.leader, true))
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
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HAIR_COLOR;
			c.unit = ctx.pc->unit;
		}
	}
	else if(strcmp(msg, "random_hair") == 0)
	{
		if(Rand() % 2 == 0)
		{
			Vec4 kolor = ctx.pc->unit->human_data->hair_color;
			do
			{
				ctx.pc->unit->human_data->hair_color = g_hair_colors[Rand() % n_hair_colors];
			}
			while(kolor.Equal(ctx.pc->unit->human_data->hair_color));
		}
		else
		{
			Vec4 kolor = ctx.pc->unit->human_data->hair_color;
			do
			{
				ctx.pc->unit->human_data->hair_color = Vec4(Random(0.f, 1.f), Random(0.f, 1.f), Random(0.f, 1.f), 1.f);
			}
			while(kolor.Equal(ctx.pc->unit->human_data->hair_color));
		}
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
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
	else if(strcmp(msg, "news") == 0)
	{
		if(ctx.update_news)
		{
			ctx.update_news = false;
			ctx.active_news = W.GetNews();
			if(ctx.active_news.empty())
			{
				DialogTalk(ctx, RandomString(txNoNews));
				++ctx.dialog_pos;
				return true;
			}
		}

		if(ctx.active_news.empty())
		{
			DialogTalk(ctx, RandomString(txAllNews));
			++ctx.dialog_pos;
			return true;
		}

		int id = Rand() % ctx.active_news.size();
		News* news = ctx.active_news[id];
		ctx.active_news.erase(ctx.active_news.begin() + id);

		DialogTalk(ctx, news->text.c_str());
		++ctx.dialog_pos;
		return true;
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
		for(Unit* unit : Team.active_members)
		{
			if(unit->IsPlayer() && unit->player != ctx.pc && Vec3::Distance2d(unit->pos, L.city_ctx->arena_pos) < 5.f)
				near_players.push_back(unit);
		}
		near_players_str.resize(near_players.size());
		for(uint i = 0, size = near_players.size(); i != size; ++i)
			near_players_str[i] = Format(txPvpWith, near_players[i]->player->name.c_str());
	}
	else if(strncmp(msg, "pvp", 3) == 0)
	{
		int id = int(msg[3] - '1');
		Unit* u = near_players[id];
		if(Vec3::Distance2d(u->pos, L.city_ctx->arena_pos) > 5.f)
		{
			ctx.dialog_s_text = Format(txPvpTooFar, u->player->name.c_str());
			DialogTalk(ctx, ctx.dialog_s_text.c_str());
			++ctx.dialog_pos;
			return true;
		}
		else
		{
			if(u->player->is_local)
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
				NetChangePlayer& c = Add1(near_players[id]->player->player_info->changes);
				c.type = NetChangePlayer::PVP;
				c.id = ctx.pc->id;
			}

			pvp_response.ok = true;
			pvp_response.from = ctx.pc->unit;
			pvp_response.to = u;
			pvp_response.timer = 0.f;
		}
	}
	else if(strcmp(msg, "crazy_give_item") == 0)
	{
		crazy_give_item = GetRandomItem(100);
		ctx.pc->unit->AddItem2(crazy_give_item, 1u, 0u);
	}
	else
	{
		Warn("DTF_SPECIAL: %s", msg);
		assert(0);
	}

	return false;
}

bool Game::ExecuteGameDialogSpecialIf(DialogContext& ctx, cstring msg)
{
	bool result;
	if(QM.HandleSpecialIf(ctx, msg, result))
		return result;

	if(strcmp(msg, "arena_combat") == 0)
		return !W.IsSameWeek(L.city_ctx->arena_time);
	else if(strcmp(msg, "have_team") == 0)
		return Team.HaveTeamMember();
	else if(strcmp(msg, "have_pc_team") == 0)
		return Team.HaveOtherPlayer();
	else if(strcmp(msg, "have_npc_team") == 0)
		return Team.HaveActiveNpc();
	else if(strcmp(msg, "is_drunk") == 0)
		return IS_SET(ctx.talker->data->flags, F_AI_DRUNKMAN) && ctx.talker->in_building != -1;
	else if(strcmp(msg, "is_team_member") == 0)
		return Team.IsTeamMember(*ctx.talker);
	else if(strcmp(msg, "is_not_known") == 0)
	{
		assert(ctx.talker->IsHero());
		return !ctx.talker->hero->know_name;
	}
	else if(strcmp(msg, "is_inside_dungeon") == 0)
		return L.local_ctx.type == LevelContext::Inside;
	else if(strcmp(msg, "is_team_full") == 0)
		return Team.GetActiveTeamSize() >= Team.GetMaxSize();
	else if(strcmp(msg, "can_join") == 0)
		return ctx.pc->unit->gold >= ctx.talker->hero->JoinCost();
	else if(strcmp(msg, "is_inside_town") == 0)
		return L.city_ctx != nullptr;
	else if(strcmp(msg, "is_free") == 0)
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->mode == HeroData::Wander;
	}
	else if(strcmp(msg, "is_not_free") == 0)
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->mode != HeroData::Wander;
	}
	else if(strcmp(msg, "is_not_follow") == 0)
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->mode != HeroData::Follow;
	}
	else if(strcmp(msg, "is_not_waiting") == 0)
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->mode != HeroData::Wait;
	}
	else if(strcmp(msg, "is_safe_location") == 0)
	{
		if(L.city_ctx)
			return true;
		else if(L.location->outside)
		{
			if(L.location->state == LS_CLEARED)
				return true;
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				if(multi->IsLevelClear())
				{
					if(L.dungeon_level == 0)
					{
						if(!multi->from_portal)
							return true;
					}
					else
						return true;
				}
			}
			else if(L.location->state == LS_CLEARED)
				return true;
		}
	}
	else if(strcmp(msg, "is_near_arena") == 0)
		return L.city_ctx && IS_SET(L.city_ctx->flags, City::HaveArena) && Vec3::Distance2d(ctx.talker->pos, L.city_ctx->arena_pos) < 5.f;
	else if(strcmp(msg, "is_lost_pvp") == 0)
	{
		assert(ctx.talker->IsHero());
		return ctx.talker->hero->lost_pvp;
	}
	else if(strcmp(msg, "is_healthy") == 0)
		return ctx.talker->GetHpp() >= 0.75f;
	else if(strcmp(msg, "is_arena_free") == 0)
		return arena_free;
	else if(strcmp(msg, "is_bandit") == 0)
		return Team.is_bandit;
	else if(strcmp(msg, "is_ginger") == 0)
		return ctx.pc->unit->human_data->hair_color.Equal(g_hair_colors[8]);
	else if(strcmp(msg, "is_bald") == 0)
		return ctx.pc->unit->human_data->hair == -1;
	else if(strcmp(msg, "is_camp") == 0)
		return target_loc_is_camp;
	else if(strcmp(msg, "taken_guards_reward") == 0)
	{
		Var& var = SM.GetVar("guards_enc_reward");
		if(var != true)
		{
			AddGold(250, nullptr, true);
			var = true;
			return true;
		}
	}
	else if(strcmp(msg, "dont_have_quest") == 0)
		return ctx.talker->quest_refid == -1;
	else if(strcmp(msg, "have_unaccepted_quest") == 0)
		return QM.FindUnacceptedQuest(ctx.talker->quest_refid);
	else if(strcmp(msg, "have_completed_quest") == 0)
	{
		Quest* q = QM.FindQuest(ctx.talker->quest_refid, false);
		if(q && !q->IsActive())
			return true;
	}
	else if(strcmp(msg, "is_free_recruit") == 0)
		return ctx.talker->level < 6 && Team.free_recruit;
	else if(strcmp(msg, "have_unique_quest") == 0)
	{
		if(((QM.quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted || QM.quest_orcs2->orcs_state == Quest_Orcs2::State::OrcJoined)
			&& QM.quest_orcs->start_loc == L.location_index)
			|| (QM.quest_mages2->mages_state >= Quest_Mages2::State::TalkedWithCaptain
				&& QM.quest_mages2->mages_state < Quest_Mages2::State::Completed
				&& QM.quest_mages2->start_loc == L.location_index))
			return true;
	}
	else if(strcmp(msg, "is_not_mage") == 0)
		return ctx.talker->GetClass() != Class::MAGE;
	else if(strcmp(msg, "prefer_melee") == 0)
		return ctx.talker->hero->melee;
	else if(strcmp(msg, "is_leader") == 0)
		return ctx.pc->unit == Team.leader;
	else if(strncmp(msg, "have_player", 11) == 0)
	{
		int id = int(msg[11] - '1');
		if(id < (int)near_players.size())
			return true;
	}
	else if(strcmp(msg, "waiting_for_pvp") == 0)
		return pvp_response.ok;
	else if(strcmp(msg, "in_city") == 0)
		return L.city_ctx != nullptr;
	else
	{
		Warn("DTF_IF_SPECIAL: %s", msg);
		assert(0);
	}

	return false;
}

//=============================================================================
// Ruch jednostki, ustawia pozycje Y dla terenu, opuszczanie lokacji
//=============================================================================
void Game::MoveUnit(Unit& unit, bool warped, bool dash)
{
	if(L.location->outside)
	{
		if(unit.in_building == -1)
		{
			if(terrain->IsInside(unit.pos))
			{
				terrain->SetH(unit.pos);
				if(warped)
					return;
				if(unit.IsPlayer() && WantExitLevel() && unit.frozen == FROZEN::NO && !dash)
				{
					bool allow_exit = false;

					if(L.city_ctx && IS_SET(L.city_ctx->flags, City::HaveExit))
					{
						for(vector<EntryPoint>::const_iterator it = L.city_ctx->entry_points.begin(), end = L.city_ctx->entry_points.end(); it != end; ++it)
						{
							if(it->exit_area.IsInside(unit.pos))
							{
								if(!IsLeader())
									AddGameMsg3(GMS_NOT_LEADER);
								else
								{
									if(Net::IsLocal())
									{
										auto result = CanLeaveLocation(unit);
										if(result == CanLeaveLocationResult::Yes)
										{
											allow_exit = true;
											W.SetTravelDir(unit.pos);
										}
										else
											AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										Net_LeaveLocation(WHERE_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(L.location_index != QM.quest_secret->where2
						&& (unit.pos.x < 33.f || unit.pos.x > 256.f - 33.f || unit.pos.z < 33.f || unit.pos.z > 256.f - 33.f))
					{
						if(!IsLeader())
							AddGameMsg3(GMS_NOT_LEADER);
						else if(!Net::IsLocal())
							Net_LeaveLocation(WHERE_OUTSIDE);
						else
						{
							auto result = CanLeaveLocation(unit);
							if(result == CanLeaveLocationResult::Yes)
							{
								allow_exit = true;
								W.SetTravelDir(unit.pos);
							}
							else
								AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
					}

					if(allow_exit)
					{
						fallback_type = FALLBACK::EXIT;
						fallback_t = -1.f;
						for(Unit* unit : Team.members)
							unit->frozen = FROZEN::YES;
						if(Net::IsOnline())
							Net::PushChange(NetChange::LEAVE_LOCATION);
					}
				}
			}
			else
				unit.pos.y = 0.f;

			if(warped)
				return;

			if(unit.IsPlayer() && L.location->type == L_CITY && WantExitLevel() && unit.frozen == FROZEN::NO && !dash)
			{
				int id = 0;
				for(vector<InsideBuilding*>::iterator it = L.city_ctx->inside_buildings.begin(), end = L.city_ctx->inside_buildings.end(); it != end; ++it, ++id)
				{
					if((*it)->enter_area.IsInside(unit.pos))
					{
						if(Net::IsLocal())
						{
							// wejdŸ do budynku
							fallback_type = FALLBACK::ENTER;
							fallback_t = -1.f;
							fallback_1 = id;
							unit.frozen = FROZEN::YES;
						}
						else
						{
							// info do serwera
							fallback_type = FALLBACK::WAIT_FOR_WARP;
							fallback_t = -1.f;
							unit.frozen = FROZEN::YES;
							NetChange& c = Add1(Net::changes);
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
			City* city = (City*)L.location;
			InsideBuilding& building = *city->inside_buildings[unit.in_building];

			if(unit.IsPlayer() && building.exit_area.IsInside(unit.pos) && WantExitLevel() && unit.frozen == FROZEN::NO && !dash)
			{
				if(Net::IsLocal())
				{
					// opuœæ budynek
					fallback_type = FALLBACK::ENTER;
					fallback_t = -1.f;
					fallback_1 = -1;
					unit.frozen = FROZEN::YES;
				}
				else
				{
					// info do serwera
					fallback_type = FALLBACK::WAIT_FOR_WARP;
					fallback_t = -1.f;
					unit.frozen = FROZEN::YES;
					Net::PushChange(NetChange::EXIT_BUILDING);
				}
			}
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		Int2 pt = pos_to_pt(unit.pos);

		if(pt == lvl.staircase_up)
		{
			Box2d box;
			switch(lvl.staircase_up_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z - 2.f*lvl.staircase_up.y) / 2;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y + 1.4f, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x - 2.f*lvl.staircase_up.x) / 2;
				box = Box2d(2.f*lvl.staircase_up.x + 1.4f, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.staircase_up.y - unit.pos.z) / 2 + 1.f;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*lvl.staircase_up.y + 0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.staircase_up.x - unit.pos.x) / 2 + 1.f;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*lvl.staircase_up.x + 0.6f, 2.f*(lvl.staircase_up.y + 1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == FROZEN::NO && !dash)
			{
				if(IsLeader())
				{
					if(Net::IsLocal())
					{
						auto result = CanLeaveLocation(unit);
						if(result == CanLeaveLocationResult::Yes)
						{
							fallback_type = FALLBACK::CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = -1;
							for(Unit* unit : Team.members)
								unit->frozen = FROZEN::YES;
							if(Net::IsOnline())
								Net::PushChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
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
			Box2d box;
			switch(lvl.staircase_down_dir)
			{
			case 0:
				unit.pos.y = (unit.pos.z - 2.f*lvl.staircase_down.y)*-1.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y + 1.4f, 2.f*(lvl.staircase_down.x + 1), 2.f*(lvl.staircase_down.y + 1));
				break;
			case 1:
				unit.pos.y = (unit.pos.x - 2.f*lvl.staircase_down.x)*-1.f;
				box = Box2d(2.f*lvl.staircase_down.x + 1.4f, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x + 1), 2.f*(lvl.staircase_down.y + 1));
				break;
			case 2:
				unit.pos.y = (2.f*lvl.staircase_down.y - unit.pos.z)*-1.f - 2.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x + 1), 2.f*lvl.staircase_down.y + 0.6f);
				break;
			case 3:
				unit.pos.y = (2.f*lvl.staircase_down.x - unit.pos.x)*-1.f - 2.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*lvl.staircase_down.x + 0.6f, 2.f*(lvl.staircase_down.y + 1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == FROZEN::NO && !dash)
			{
				if(IsLeader())
				{
					if(Net::IsLocal())
					{
						auto result = CanLeaveLocation(unit);
						if(result == CanLeaveLocationResult::Yes)
						{
							fallback_type = FALLBACK::CHANGE_LEVEL;
							fallback_t = -1.f;
							fallback_1 = +1;
							for(Unit* unit : Team.members)
								unit->frozen = FROZEN::YES;
							if(Net::IsOnline())
								Net::PushChange(NetChange::LEAVE_LOCATION);
						}
						else
							AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
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
	if(unit.frozen == FROZEN::NO && unit.IsPlayer() && WantExitLevel() && !dash)
	{
		Portal* portal = L.location->portal;
		int index = 0;
		while(portal)
		{
			if(portal->target_loc != -1 && Vec3::Distance2d(unit.pos, portal->pos) < 2.f)
			{
				if(CircleToRotatedRectangle(unit.pos.x, unit.pos.z, unit.GetUnitRadius(), portal->pos.x, portal->pos.z, 0.67f, 0.1f, portal->rot))
				{
					if(IsLeader())
					{
						if(Net::IsLocal())
						{
							auto result = CanLeaveLocation(unit);
							if(result == CanLeaveLocationResult::Yes)
							{
								fallback_type = FALLBACK::USE_PORTAL;
								fallback_t = -1.f;
								fallback_1 = index;
								for(Unit* unit : Team.members)
									unit->frozen = FROZEN::YES;
								if(Net::IsOnline())
									Net::PushChange(NetChange::LEAVE_LOCATION);
							}
							else
								AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							Net_LeaveLocation(WHERE_PORTAL + index);
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
		if(Net::IsLocal() || &unit != pc->unit || interpolate_timer <= 0.f)
		{
			unit.visual_pos = unit.pos;
			unit.changed = true;
		}
		UpdateUnitPhysics(unit, unit.pos);
	}
}

//=================================================================================================
bool Game::CollideWithStairs(const CollisionObject& _co, const Vec3& _pos, float _radius) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check == &Game::CollideWithStairs && !L.location->outside && _radius > 0.f);

	InsideLocation* inside = (InsideLocation*)L.location;
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
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y - 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 1)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x - 0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	if(dir != 2)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y + 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != 3)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x + 0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
bool Game::CollideWithStairsRect(const CollisionObject& _co, const Box2d& _box) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check_rect == &Game::CollideWithStairsRect && !L.location->outside);

	InsideLocation* inside = (InsideLocation*)L.location;
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
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y - 1.f, _co.pt.x + 1.f, _co.pt.y - 0.9f))
			return true;
	}

	if(dir != 1)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y - 1.f, _co.pt.x - 0.9f, _co.pt.y + 1.f))
			return true;
	}

	if(dir != 2)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y + 0.9f, _co.pt.x + 1.f, _co.pt.y + 1.f))
			return true;
	}

	if(dir != 3)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x + 0.9f, _co.pt.y - 1.f, _co.pt.x + 1.f, _co.pt.y + 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
uint Game::TestGameData(bool major)
{
	string str;
	uint errors = 0;
	auto& mesh_mgr = ResourceManager::Get<Mesh>();

	Info("Test: Checking items...");

	// bronie
	for(Weapon* weapon : Weapon::weapons)
	{
		const Weapon& w = *weapon;
		if(!w.mesh)
		{
			Error("Test: Weapon %s: missing mesh %s.", w.id.c_str(), w.mesh_id.c_str());
			++errors;
		}
		else
		{
			mesh_mgr.LoadMetadata(w.mesh);
			Mesh::Point* pt = w.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				Error("Test: Weapon %s: no hitbox in mesh %s.", w.id.c_str(), w.mesh_id.c_str());
				++errors;
			}
			else if(!pt->size.IsPositive())
			{
				Error("Test: Weapon %s: invalid hitbox %g, %g, %g in mesh %s.", w.id.c_str(), pt->size.x, pt->size.y, pt->size.z, w.mesh_id.c_str());
				++errors;
			}
		}
	}

	// tarcze
	for(Shield* shield : Shield::shields)
	{
		const Shield& s = *shield;
		if(!s.mesh)
		{
			Error("Test: Shield %s: missing mesh %s.", s.id.c_str(), s.mesh_id.c_str());
			++errors;
		}
		else
		{
			mesh_mgr.LoadMetadata(s.mesh);
			Mesh::Point* pt = s.mesh->FindPoint("hit");
			if(!pt || !pt->IsBox())
			{
				Error("Test: Shield %s: no hitbox in mesh %s.", s.id.c_str(), s.mesh_id.c_str());
				++errors;
			}
			else if(!pt->size.IsPositive())
			{
				Error("Test: Shield %s: invalid hitbox %g, %g, %g in mesh %s.", s.id.c_str(), pt->size.x, pt->size.y, pt->size.z, s.mesh_id.c_str());
				++errors;
			}
		}
	}

	// postacie
	Info("Test: Checking units...");
	for(UnitData* ud_ptr : UnitData::units)
	{
		UnitData& ud = *ud_ptr;
		str.clear();

		// przedmioty postaci
		if(ud.item_script)
			ud.item_script->Test(str, errors);

		// czary postaci
		if(ud.spells)
			TestUnitSpells(*ud.spells, str, errors);

		// ataki
		if(ud.frames)
		{
			if(ud.frames->extra)
			{
				if(InRange(ud.frames->attacks, 1, NAMES::max_attacks))
				{
					for(int i = 0; i < ud.frames->attacks; ++i)
					{
						if(!InRange(0.f, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end, 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i + 1, ud.frames->extra->e[i].start, ud.frames->extra->e[i].end);
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
				if(InRange(ud.frames->attacks, 1, 3))
				{
					for(int i = 0; i < ud.frames->attacks; ++i)
					{
						if(!InRange(0.f, ud.frames->t[F_ATTACK1_START + i * 2], ud.frames->t[F_ATTACK1_END + i * 2], 1.f))
						{
							str += Format("\tInvalid values in attack %d (%g, %g).\n", i + 1, ud.frames->t[F_ATTACK1_START + i * 2],
								ud.frames->t[F_ATTACK1_END + i * 2]);
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
			if(!InRange(ud.frames->t[F_BASH], 0.f, 1.f))
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
				Mesh& a = *ud.mesh;
				mesh_mgr.Load(ud.mesh);

				for(uint i = 0; i < NAMES::n_ani_base; ++i)
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
					for(uint i = 0; i < NAMES::n_points; ++i)
					{
						if(!a.GetPoint(NAMES::points[i]))
						{
							str += Format("\tMissing attachment point '%s'.\n", NAMES::points[i]);
							++errors;
						}
					}

					for(uint i = 0; i < NAMES::n_ani_humanoid; ++i)
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
					for(int i = 0; i < min(ud.frames->attacks, NAMES::max_attacks); ++i)
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
					for(const string& s : ud.idles->anims)
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
			Error("Test: Unit %s:\n%s", ud.id.c_str(), str.c_str());
	}

	return errors;
}

//=================================================================================================
void Game::TestUnitSpells(const SpellList& _spells, string& _errors, uint& _count)
{
	for(int i = 0; i < 3; ++i)
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

//=================================================================================================
Unit* Game::CreateUnit(UnitData& base, int level, Human* human_data, Unit* test_unit, bool create_physics, bool custom)
{
	Unit* u;
	if(test_unit)
		u = test_unit;
	else
		u = new Unit;

	// unit data
	u->data = &base;
	u->human_data = nullptr;
	u->pos = Vec3(0, 0, 0);
	u->rot = 0.f;
	u->used_item = nullptr;
	u->live_state = Unit::ALIVE;
	for(int i = 0; i < SLOT_MAX; ++i)
		u->slots[i] = nullptr;
	u->action = A_NONE;
	u->weapon_taken = W_NONE;
	u->weapon_hiding = W_NONE;
	u->weapon_state = WS_HIDDEN;
	if(level == -2)
		u->level = base.level.Random();
	else if(level == -3)
		u->level = base.level.Clamp(L.location->st);
	else
		u->level = base.level.Clamp(level);
	u->player = nullptr;
	u->ai = nullptr;
	u->speed = u->prev_speed = 0.f;
	u->invisible = false;
	u->hurt_timer = 0.f;
	u->talking = false;
	u->usable = nullptr;
	u->in_building = -1;
	u->frozen = FROZEN::NO;
	u->in_arena = -1;
	u->run_attack = false;
	u->event_handler = nullptr;
	u->to_remove = false;
	u->temporary = false;
	u->quest_refid = -1;
	u->bubble = nullptr;
	u->busy = Unit::Busy_No;
	u->interp = nullptr;
	u->dont_attack = false;
	u->assist = false;
	u->auto_talk = AutoTalkMode::No;
	u->attack_team = false;
	u->last_bash = 0.f;
	u->guard_target = nullptr;
	u->alcohol = 0.f;
	u->moved = false;

	u->fake_unit = true; // to prevent sending hp changed message set temporary as fake unit
	u->data->GetStatProfile().Set(u->level, u->unmod_stats.attrib, u->unmod_stats.skill);
	u->CalculateStats();
	u->hp = u->hpmax = u->CalculateMaxHp();
	u->stamina = u->stamina_max = u->CalculateMaxStamina();
	u->fake_unit = false;

	// items
	u->weight = 0;
	u->CalculateLoad();
	if(!custom && base.item_script)
	{
		ParseItemScript(*u, base.item_script);
		SortItems(u->items);
		u->RecalculateWeight();
		if(!ResourceManager::Get().IsLoadScreen())
		{
			for(auto slot : u->slots)
			{
				if(slot)
					PreloadItem(slot);
			}
			for(auto& slot : u->items)
				PreloadItem(slot.item);
		}
	}

	// gold
	float t;
	if(base.level.x == base.level.y)
		t = 1.f;
	else
		t = float(u->level - base.level.x) / (base.level.y - base.level.x);
	u->gold = Int2::Lerp(base.gold, base.gold2, t).Random();

	if(!test_unit)
	{
		// mesh, human details
		if(base.type == UNIT_TYPE::HUMAN)
		{
			if(human_data)
				u->human_data = human_data;
			else
			{
#define HEX(h) Vec4(1.f/256*(((h)&0xFF0000)>>16), 1.f/256*(((h)&0xFF00)>>8), 1.f/256*((h)&0xFF), 1.f)
				u->human_data = new Human;
				u->human_data->beard = Rand() % MAX_BEARD - 1;
				u->human_data->hair = Rand() % MAX_HAIR - 1;
				u->human_data->mustache = Rand() % MAX_MUSTACHE - 1;
				u->human_data->height = Random(0.9f, 1.1f);
				if(IS_SET(base.flags2, F2_OLD))
					u->human_data->hair_color = HEX(0xDED5D0);
				else if(IS_SET(base.flags, F_CRAZY))
					u->human_data->hair_color = Vec4(RandomPart(8), RandomPart(8), RandomPart(8), 1.f);
				else if(IS_SET(base.flags, F_GRAY_HAIR))
					u->human_data->hair_color = g_hair_colors[Rand() % 4];
				else if(IS_SET(base.flags, F_TOMASHU))
				{
					u->human_data->beard = 4;
					u->human_data->mustache = -1;
					u->human_data->hair = 0;
					u->human_data->hair_color = g_hair_colors[0];
					u->human_data->height = 1.1f;
				}
				else
					u->human_data->hair_color = g_hair_colors[Rand() % n_hair_colors];
#undef HEX
			}
		}

		u->CreateMesh(Unit::CREATE_MESH::NORMAL);

		// hero data
		if(IS_SET(base.flags, F_HERO))
		{
			u->hero = new HeroData;
			u->hero->Init(*u);
		}
		else
			u->hero = nullptr;

		// boss music
		if(IS_SET(u->data->flags2, F2_BOSS))
			W.AddBossLevel(Int2(L.location_index, L.dungeon_level));

		// physics
		if(create_physics)
			CreateUnitPhysics(*u);
		else
			u->cobj = nullptr;
	}

	if(Net::IsServer())
		u->netid = Unit::netid_counter++;

	return u;
}

//=================================================================================================
void GiveItem(Unit& unit, const int*& ps, int count)
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
			unit.AddItemAndEquipIfNone(lis.Get(Random(level, unit.level)));
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
				unit.AddItemAndEquipIfNone(lis.Get(Random(l, level)));
		}
	}

	++ps;
}

//=================================================================================================
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

void Game::ParseItemScript(Unit& unit, const ItemScript* script)
{
	assert(script);

	const int* ps = script->code.data();
	int a, b, depth = 0, depth_if = 0;

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
				b = Rand() % a;
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
			if(depth == depth_if && Rand() % 100 < a)
				GiveItem(unit, ps, 1);
			else
				SkipItem(ps, 1);
			break;
		case PS_CHANCE2:
			a = *ps;
			++ps;
			if(depth == depth_if)
			{
				if(Rand() % 100 < a)
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
			if(depth == depth_if && Rand() % 100 < a)
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
			else if(depth == depth_if + 1)
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
			a = Random(a, b);
			if(depth == depth_if && a > 0)
				GiveItem(unit, ps, a);
			else
				SkipItem(ps, 1);
			break;
		default:
			assert(0);
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
			if(u1.IsAI() && u1.IsDontAttack())
				return false;
			if(u2.IsAI() && u2.IsDontAttack())
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
				return Team.is_bandit || u1.WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_CRAZIES)
		{
			if(g2 == G_CITIZENS)
				return true;
			else if(g2 == G_TEAM)
				return Team.crazies_attack || u1.WantAttackTeam();
			else
				return true;
		}
		else if(g1 == G_TEAM)
		{
			if(u2.WantAttackTeam())
				return true;
			else if(g2 == G_CITIZENS)
				return Team.is_bandit;
			else if(g2 == G_CRAZIES)
				return Team.crazies_attack;
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
			else if(u2.IsAI() && !Team.is_bandit && u2.IsAssist())
				return true;
			else
				return false;
		}
		else if(u2.IsTeamMember())
		{
			if(u1.IsAI() && !Team.is_bandit && u1.IsAssist())
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

	Int2 tile1(int(u1.pos.x / 2), int(u1.pos.z / 2)),
		tile2(int(u2.pos.x / 2), int(u2.pos.z / 2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = L.GetContext(u1);

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)L.location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].mode >= TM_BUILDING_BLOCK
					&& LineToRectangle(u1.pos, u2.pos, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(czy_blokuje2(lvl.map[x + y * lvl.w].type) && LineToRectangle(u1.pos, u2.pos, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y * lvl.w].type == DRZWI)
				{
					Door* door = FindDoor(ctx, Int2(x, y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						Box2d box(door->pos.x, door->pos.z);
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

			Box2d box(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box2d box(door.pos.x, door.pos.z);
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

bool Game::CanSee(const Vec3& v1, const Vec3& v2)
{
	Int2 tile1(int(v1.x / 2), int(v1.z / 2)),
		tile2(int(v2.x / 2), int(v2.z / 2));

	if(tile1 == tile2)
		return true;

	LevelContext& ctx = L.local_ctx;

	if(ctx.type == LevelContext::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)L.location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].mode >= TM_BUILDING_BLOCK && LineToRectangle(v1, v2, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
			}
		}
	}
	else if(ctx.type == LevelContext::Inside)
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(czy_blokuje2(lvl.map[x + y * lvl.w].type) && LineToRectangle(v1, v2, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y * lvl.w].type == DRZWI)
				{
					Door* door = FindDoor(ctx, Int2(x, y));
					if(door && door->IsBlocking())
					{
						// 0.842f, 1.319f, 0.181f
						Box2d box(door->pos.x, door->pos.z);
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

			Box2d box(it->pt.x-it->w, it->pt.y-it->h, it->pt.x+it->w, it->pt.y+it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		// kolizje z drzwiami
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box2d box(door.pos.x, door.pos.z);
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

bool Game::CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, Vec3& hitpoint)
{
	// atak broni¹ lub naturalny

	Mesh::Point* hitbox, *point;

	if(unit.mesh_inst->mesh->head.n_groups > 1)
	{
		Mesh* mesh = unit.GetWeapon().mesh;
		if(!mesh)
			return false;
		hitbox = mesh->FindPoint("hit");
		point = unit.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
		assert(point);
	}
	else
	{
		point = nullptr;
		hitbox = unit.mesh_inst->mesh->GetPoint(Format("hitbox%d", unit.attack_id + 1));
		if(!hitbox)
			hitbox = unit.mesh_inst->mesh->FindPoint("hitbox");
	}

	assert(hitbox);

	return CheckForHit(ctx, unit, hitted, *hitbox, point, hitpoint);
}

// Sprawdza czy jest kolizja hitboxa z jak¹œ postaci¹
// S¹ dwie opcje:
//  - _bone to punkt "bron", _hitbox to hitbox z bronii
//  - _bone jest nullptr, _hitbox jest na modelu postaci
// Na drodze testów ustali³em ¿e najlepiej dzia³a gdy stosuje siê sam¹ funkcjê OrientedBoxToOrientedBox
bool Game::CheckForHit(LevelContext& ctx, Unit& _unit, Unit*& _hitted, Mesh::Point& _hitbox, Mesh::Point* _bone, Vec3& _hitpoint)
{
	assert(_hitted && _hitbox.IsBox());

	//Box box1, box2;

	// ustaw koœci
	if(_unit.human_data)
		_unit.mesh_inst->SetupBones(&_unit.human_data->mat_scale[0]);
	else
		_unit.mesh_inst->SetupBones();

	// oblicz macierz hitbox

	// transformacja postaci
	m1 = Matrix::RotationY(_unit.rot) * Matrix::Translation(_unit.pos); // m1 (World) = Rot * Pos

	if(_bone)
	{
		// m1 = BoxMatrix * PointMatrix * BoneMatrix * UnitRot * UnitPos
		m1 = _hitbox.mat * (_bone->mat * _unit.mesh_inst->mat_bones[_bone->bone] * m1);
	}
	else
	{
		m1 = _hitbox.mat * _unit.mesh_inst->mat_bones[_hitbox.bone] * m1;
	}

	// m1 to macierz hitboxa
	// teraz mo¿emy stworzyæ AABBOX wokó³ broni
	//CreateAABBOX(box1, m1);

	// przy okazji stwórz obrócony Box
	/*Obbox obox1, obox2;
	D3DXMatrixIdentity(&obox2.rot);
	D3DXVec3TransformCoord(&obox1.pos, &Vec3(0,0,0), &m1);
	obox1.size = _hitbox.size*2;
	obox1.rot = m1;
	obox1.rot._11 = obox2.rot._22 = obox2.rot._33 = 1.f;

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		Box box2;
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
				Vec3 dir = _hitpoint - (*it)->pos;
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
	Oob a, b;
	m1._11 = m1._22 = m1._33 = 1.f;
	a.c = Vec3::TransformZero(m1);
	a.e = _hitbox.size;
	a.u[0] = Vec3(m1._11, m1._12, m1._13);
	a.u[1] = Vec3(m1._21, m1._22, m1._23);
	a.u[2] = Vec3(m1._31, m1._32, m1._33);
	b.u[0] = Vec3(1, 0, 0);
	b.u[1] = Vec3(0, 1, 0);
	b.u[2] = Vec3(0, 0, 1);

	// szukaj kolizji
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if(*it == &_unit || !(*it)->IsAlive() || Vec3::Distance((*it)->pos, _unit.pos) > 5.f || IsFriend(_unit, **it))
			continue;

		Box box2;
		(*it)->GetBox(box2);
		b.c = box2.Midpoint();
		b.e = box2.Size() / 2;

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

			int ile = min(Random(pe.spawn_min, pe.spawn_max), pe.max_particles - pe.alive);
			vector<Particle>::iterator it2 = pe.particles.begin();

			for(int i = 0; i < ile; ++i)
			{
				while(it2->exists)
					++it2;

				Particle& p = *it2;
				p.exists = true;
				p.gravity = 9.81f;
				p.life = pe.particle_life;
				p.pos = pe.pos + Vec3::Random(pe.pos_min, pe.pos_max);
				p.speed = Vec3::Random(pe.speed_min, pe.speed_max);
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
	Vec3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(ctx, unit, hitted, hitpoint))
		return ATTACK_NOT_HIT;

	return DoGenericAttack(ctx, unit, *hitted, hitpoint, unit.CalculateAttack()*unit.attack_power, unit.GetDmgType(), false);
}

void Game::GiveDmg(LevelContext& ctx, Unit* giver, float dmg, Unit& taker, const Vec3* hitpoint, int dmg_flags)
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
			pe->pos.y += taker.GetUnitHeight() * 2.f / 3;
		}
		pe->speed_min = Vec3(-1, 0, -1);
		pe->speed_max = Vec3(1, 1, 1);
		pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
		pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
		pe->size = 0.3f;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 0.9f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 0;
		pe->Init();
		ctx.pes->push_back(pe);

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
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
		taker.player->Train(TrainWhat::TakeDamage, min(dmg, taker.hp) / taker.hpmax, (giver ? giver->level : -1));

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
			PlayAttachedSound(taker, taker.data->sounds->sound[SOUND_PAIN]->sound, 2.f, 15.f);
			taker.hurt_timer = Random(1.f, 1.5f);
			if(IS_SET(dmg_flags, DMG_NO_BLOOD))
				taker.hurt_timer += 1.f;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::HURT_SOUND;
				c.unit = &taker;
			}
		}

		// immortality
		if(taker.hp < 1.f)
			taker.hp = 1.f;

		// send update hp
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
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

		// update effects and mouth moving
		if(u.IsAlive())
		{
			u.UpdateEffects(dt);

			if(Net::IsLocal())
			{
				// hurt sound timer since last hit, timer since last stun (to prevent stunlock)
				u.hurt_timer -= dt;
				u.last_bash -= dt;

				if(u.moved)
				{
					// unstuck units after being force moved (by bull charge)
					static vector<Unit*> targets;
					targets.clear();
					float t;
					bool in_dash = false;
					ContactTest(u.cobj, [&](btCollisionObject* obj, bool second)
					{
						if(!second)
						{
							int flags = obj->getCollisionFlags();
							if(!IS_SET(flags, CG_UNIT))
								return false;
							else
							{
								if(obj->getUserPointer() == &u)
									return false;
								return true;
							}
						}
						else
						{
							Unit* target = (Unit*)obj->getUserPointer();
							if(target->action == A_DASH)
								in_dash = true;
							targets.push_back(target);
							return true;
						}
					}, true);

					if(!in_dash)
					{
						if(targets.empty())
							u.moved = false;
						else
						{
							Vec3 center(0, 0, 0);
							for(auto target : targets)
							{
								center += u.pos - target->pos;
								target->moved = true;
							}
							center /= (float)targets.size();
							center.y = 0;
							center.Normalize();
							center *= dt;
							LineTest(u.cobj->getCollisionShape(), u.GetPhysicsPos(), center, [&](btCollisionObject* obj, bool)
							{
								int flags = obj->getCollisionFlags();
								if(IS_SET(flags, CG_TERRAIN | CG_UNIT))
									return LT_IGNORE;
								return LT_COLLIDE;
							}, t);
							if(t == 1.f)
							{
								u.pos += center;
								MoveUnit(u, false, true);
							}
						}
					}
				}
				if(u.IsStanding() && u.talking)
				{
					u.talk_timer += dt;
					u.mesh_inst->need_update = true;
				}
			}
		}

		// zmieñ podstawow¹ animacjê
		if(u.animation != u.current_animation)
		{
			u.changed = true;
			switch(u.animation)
			{
			case ANI_WALK:
				u.mesh_inst->Play(NAMES::ani_move, PLAY_PRIO1 | PLAY_RESTORE, 0);
				if(!Net::IsClient())
					u.mesh_inst->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_WALK_TYL:
				u.mesh_inst->Play(NAMES::ani_move, PLAY_BACK | PLAY_PRIO1 | PLAY_RESTORE, 0);
				if(!Net::IsClient())
					u.mesh_inst->groups[0].speed = u.GetWalkSpeed() / u.data->walk_speed;
				break;
			case ANI_RUN:
				u.mesh_inst->Play(NAMES::ani_run, PLAY_PRIO1 | PLAY_RESTORE, 0);
				if(!Net::IsClient())
					u.mesh_inst->groups[0].speed = u.GetRunSpeed() / u.data->run_speed;
				break;
			case ANI_LEFT:
				u.mesh_inst->Play(NAMES::ani_left, PLAY_PRIO1 | PLAY_RESTORE, 0);
				if(!Net::IsClient())
					u.mesh_inst->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_RIGHT:
				u.mesh_inst->Play(NAMES::ani_right, PLAY_PRIO1 | PLAY_RESTORE, 0);
				if(!Net::IsClient())
					u.mesh_inst->groups[0].speed = u.GetRotationSpeed() / u.data->rot_speed;
				break;
			case ANI_STAND:
				u.mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				break;
			case ANI_BATTLE:
				u.mesh_inst->Play(NAMES::ani_battle, PLAY_PRIO1, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				break;
			case ANI_BATTLE_BOW:
				u.mesh_inst->Play(NAMES::ani_battle_bow, PLAY_PRIO1, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				break;
			case ANI_DIE:
				u.mesh_inst->Play(NAMES::ani_die, PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				break;
			case ANI_PLAY:
				break;
			case ANI_IDLE:
				break;
			case ANI_KNEELS:
				u.mesh_inst->Play("kleka", PLAY_STOP_AT_END | PLAY_ONCE | PLAY_PRIO3, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				break;
			default:
				assert(0);
				break;
			}
			u.current_animation = u.animation;
		}

		// aktualizuj animacjê
		u.mesh_inst->Update(dt);

		// koniec animacji idle
		if(u.animation == ANI_IDLE && u.mesh_inst->frame_end_info)
		{
			u.mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
			u.mesh_inst->groups[0].speed = 1.f;
			u.animation = ANI_STAND;
		}

		// zmieñ stan z umiera na umar³ i stwórz krew (chyba ¿e tylko upad³)
		if(!u.IsStanding())
		{
			if(u.mesh_inst->frame_end_info)
			{
				if(u.live_state == Unit::DYING)
				{
					u.live_state = Unit::DEAD;
					CreateBlood(ctx, u);
					if(u.summoner != nullptr && Net::IsLocal())
					{
						RemoveTeamMember(&u);
						u.action = A_DESPAWN;
						u.timer = Random(2.5f, 5.f);
						u.summoner = nullptr;
					}
				}
				else if(u.live_state == Unit::FALLING)
					u.live_state = Unit::FALL;
				u.mesh_inst->frame_end_info = false;
			}
			if(u.action != A_POSITION && u.action != A_DESPAWN)
			{
				u.UpdateStaminaAction();
				continue;
			}
		}

		// aktualizuj akcjê
		switch(u.action)
		{
		case A_NONE:
			break;
		case A_TAKE_WEAPON:
			if(u.weapon_state == WS_TAKING)
			{
				if(u.animation_state == 0 && (u.mesh_inst->GetProgress2() >= u.data->frames->t[F_TAKE_WEAPON] || u.mesh_inst->frame_end_info2))
					u.animation_state = 1;
				if(u.mesh_inst->frame_end_info2)
				{
					u.weapon_state = WS_TAKEN;
					if(u.usable)
					{
						u.action = A_ANIMATION2;
						u.animation_state = AS_ANIMATION2_USING;
					}
					else
						u.action = A_NONE;
					u.mesh_inst->Deactivate(1);
					u.mesh_inst->frame_end_info2 = false;
				}
			}
			else
			{
				// chowanie broni
				if(u.animation_state == 0 && (u.mesh_inst->GetProgress2() <= u.data->frames->t[F_TAKE_WEAPON] || u.mesh_inst->frame_end_info2))
					u.animation_state = 1;
				if(u.weapon_taken != W_NONE && (u.animation_state == 1 || u.mesh_inst->frame_end_info2))
				{
					u.mesh_inst->Play(u.GetTakeWeaponAnimation(u.weapon_taken == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
					u.weapon_state = WS_TAKING;
					u.weapon_hiding = W_NONE;
					u.animation_state = 1;
					u.mesh_inst->frame_end_info2 = false;
					u.animation_state = 0;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.unit = &u;
						c.id = 0;
						c.type = NetChange::TAKE_WEAPON;
					}
				}
				else if(u.mesh_inst->frame_end_info2)
				{
					u.weapon_state = WS_HIDDEN;
					u.weapon_hiding = W_NONE;
					u.action = A_NONE;
					u.mesh_inst->Deactivate(1);
					u.mesh_inst->frame_end_info2 = false;

					if(&u == pc->unit)
					{
						switch(pc->next_action)
						{
							// unequip item
						case NA_REMOVE:
							if(u.slots[pc->next_action_data.slot])
								game_gui->inventory->RemoveSlotItem(pc->next_action_data.slot);
							break;
							// equip item after unequiping old one
						case NA_EQUIP:
							{
								int index = pc->GetNextActionItemIndex();
								if(index != -1)
									game_gui->inventory->EquipSlotItem(index);
							}
							break;
							// drop item after hiding it
						case NA_DROP:
							if(u.slots[pc->next_action_data.slot])
								game_gui->inventory->DropSlotItem(pc->next_action_data.slot);
							break;
							// use consumable
						case NA_CONSUME:
							{
								int index = pc->GetNextActionItemIndex();
								if(index != -1)
									game_gui->inventory->ConsumeItem(index);
							}
							break;
							//  use usable
						case NA_USE:
							if(!pc->next_action_data.usable->user)
								PlayerUseUsable(pc->next_action_data.usable, true);
							break;
							// sell equipped item
						case NA_SELL:
							if(u.slots[pc->next_action_data.slot])
								game_gui->inv_trade_mine->SellSlotItem(pc->next_action_data.slot);
							break;
							// put equipped item in container
						case NA_PUT:
							if(u.slots[pc->next_action_data.slot])
								game_gui->inv_trade_mine->PutSlotItem(pc->next_action_data.slot);
							break;
							// give equipped item
						case NA_GIVE:
							if(u.slots[pc->next_action_data.slot])
								game_gui->inv_trade_mine->GiveSlotItem(pc->next_action_data.slot);
							break;
						}

						if(pc->next_action != NA_NONE)
						{
							pc->next_action = NA_NONE;
							if(Net::IsClient())
								Net::PushChange(NetChange::SET_NEXT_ACTION);
						}

						if(u.action == A_NONE && u.usable && !u.usable->container)
						{
							u.action = A_ANIMATION2;
							u.animation_state = AS_ANIMATION2_USING;
						}
					}
					else if(Net::IsLocal() && u.IsAI() && u.ai->potion != -1)
					{
						u.ConsumeItem(u.ai->potion);
						u.ai->potion = -1;
					}
				}
			}
			break;
		case A_SHOOT:
			u.stamina_timer = Unit::STAMINA_RESTORE_TIMER;
			if(u.animation_state == 0)
			{
				if(u.mesh_inst->GetProgress2() > 20.f / 40)
					u.mesh_inst->groups[1].time = 20.f / 40 * u.mesh_inst->groups[1].anim->length;
			}
			else if(u.animation_state == 1)
			{
				if(Net::IsLocal() && !u.hitted && u.mesh_inst->GetProgress2() > 20.f / 40)
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
						u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
					else
						u.mesh_inst->SetupBones();

					Mesh::Point* point = u.mesh_inst->mesh->GetPoint(NAMES::point_weapon);
					assert(point);

					m2 = point->mat * u.mesh_inst->mat_bones[point->bone] * (Matrix::RotationY(u.rot) * Matrix::Translation(u.pos));

					b.attack = u.CalculateAttack(&u.GetBow());
					b.rot = Vec3(PI / 2, u.rot + PI, 0);
					b.pos = Vec3::TransformZero(m2);
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
							b.yspeed = PlayerAngleY() * 36;
						else
							b.yspeed = u.player->player_info->yspeed;
						u.player->Train(TrainWhat::BowStart, 0.f, 0);
					}
					else
					{
						b.yspeed = u.ai->shoot_yspeed;
						if(u.ai->state == AIController::Idle && u.ai->idle_action == AIController::Idle_TrainBow)
							b.attack = -100.f;
					}

					// losowe odchylenie
					int sk = u.Get(SkillId::BOW);
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
							odchylenie_x = PI / 16;
							odchylenie_y = 5.f;
						}
						else if(sk < 20)
						{
							szansa = 80;
							odchylenie_x = PI / 20;
							odchylenie_y = 4.f;
						}
						else if(sk < 30)
						{
							szansa = 60;
							odchylenie_x = PI / 26;
							odchylenie_y = 3.f;
						}
						else if(sk < 40)
						{
							szansa = 40;
							odchylenie_x = PI / 34;
							odchylenie_y = 2.f;
						}
						else
						{
							szansa = 20;
							odchylenie_x = PI / 48;
							odchylenie_y = 1.f;
						}

						if(Rand() % 100 < szansa)
							b.rot.y += RandomNormalized(odchylenie_x);
						if(Rand() % 100 < szansa)
							b.yspeed += RandomNormalized(odchylenie_y);
					}

					b.rot.y = Clip(b.rot.y);

					TrailParticleEmitter* tpe = new TrailParticleEmitter;
					tpe->fade = 0.3f;
					tpe->color1 = Vec4(1, 1, 1, 0.5f);
					tpe->color2 = Vec4(1, 1, 1, 0);
					tpe->Init(50);
					ctx.tpes->push_back(tpe);
					b.trail = tpe;

					TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
					tpe2->fade = 0.3f;
					tpe2->color1 = Vec4(1, 1, 1, 0.5f);
					tpe2->color2 = Vec4(1, 1, 1, 0);
					tpe2->Init(50);
					ctx.tpes->push_back(tpe2);
					b.trail2 = tpe2;

					sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, 2.f, 8.f);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::SHOOT_ARROW;
						c.unit = &u;
						c.pos = b.start_pos;
						c.f[0] = b.rot.y;
						c.f[1] = b.yspeed;
						c.f[2] = b.rot.x;
						c.extra_f = b.speed;
					}
				}
				if(u.mesh_inst->GetProgress2() > 20.f / 40)
					u.animation_state = 2;
			}
			else if(u.mesh_inst->GetProgress2() > 35.f / 40)
			{
				u.animation_state = 3;
				if(u.mesh_inst->frame_end_info2)
				{
				koniec_strzelania:
					u.mesh_inst->Deactivate(1);
					u.mesh_inst->frame_end_info2 = false;
					u.action = A_NONE;
					assert(u.bow_instance);
					bow_instances.push_back(u.bow_instance);
					u.bow_instance = nullptr;
					if(Net::IsLocal() && u.IsAI())
					{
						float v = 1.f - float(u.Get(SkillId::BOW)) / 100;
						u.ai->next_attack = Random(v / 2, v);
					}
					break;
				}
			}
			if(!u.mesh_inst)
			{
				// fix na skutek, nie na przyczynê ;(
#ifdef _DEBUG
				Warn("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", u.GetName(), u.live_state, u.action, u.animation,
					u.current_animation, u.animation_state);
				AddGameMsg("Unit don't have shooting animation!", 5.f);
#endif
				goto koniec_strzelania;
			}
			u.bow_instance->groups[0].time = min(u.mesh_inst->groups[1].time, u.bow_instance->groups[0].anim->length);
			u.bow_instance->need_update = true;
			break;
		case A_ATTACK:
			u.stamina_timer = Unit::STAMINA_RESTORE_TIMER;
			if(u.animation_state == 0)
			{
				int index = u.mesh_inst->mesh->head.n_groups == 1 ? 0 : 1;
				float t = u.GetAttackFrame(0);
				float p = u.mesh_inst->GetProgress(index);
				if(p > t)
				{
					if(Net::IsLocal() && u.IsAI())
					{
						u.mesh_inst->groups[index].speed = 1.f + u.GetAttackSpeed();
						u.attack_power = 2.f;
						++u.animation_state;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ATTACK;
							c.unit = &u;
							c.id = AID_Attack;
							c.f[1] = u.mesh_inst->groups[index].speed;
						}
					}
					else
						u.mesh_inst->groups[index].time = t * u.mesh_inst->groups[index].anim->length;
				}
				else if(u.IsPlayer() && Net::IsLocal())
				{
					float dif = p - u.timer;
					float stamina_to_remove_total = u.GetWeapon().GetInfo().stamina / 2;
					float stamina_used = dif / t * stamina_to_remove_total;
					u.timer = p;
					u.RemoveStamina(stamina_used);
				}
			}
			else
			{
				int index = u.mesh_inst->mesh->head.n_groups == 1 ? 0 : 1;
				if(u.animation_state == 1 && u.mesh_inst->GetProgress(index) > u.GetAttackFrame(0))
				{
					if(Net::IsLocal() && !u.hitted && u.mesh_inst->GetProgress(index) >= u.GetAttackFrame(1))
					{
						ATTACK_RESULT result = DoAttack(ctx, u);
						if(result != ATTACK_NOT_HIT)
							u.hitted = true;
					}
					if(u.mesh_inst->GetProgress(index) >= u.GetAttackFrame(2) || u.mesh_inst->GetEndResult(index))
					{
						// koniec mo¿liwego ataku
						u.animation_state = 2;
						u.mesh_inst->groups[index].speed = 1.f;
						u.run_attack = false;
					}
				}
				if(u.animation_state == 2 && u.mesh_inst->GetEndResult(index))
				{
					u.run_attack = false;
					if(index == 0)
					{
						u.animation = ANI_BATTLE;
						u.current_animation = ANI_STAND;
					}
					else
					{
						u.mesh_inst->Deactivate(1);
						u.mesh_inst->frame_end_info2 = false;
					}
					u.action = A_NONE;
					if(Net::IsLocal() && u.IsAI())
					{
						float v = 1.f - float(u.Get(SkillId::ONE_HANDED_WEAPON)) / 100;
						u.ai->next_attack = Random(v / 2, v);
					}
				}
			}
			break;
		case A_BLOCK:
			break;
		case A_BASH:
			u.stamina_timer = Unit::STAMINA_RESTORE_TIMER;
			if(u.animation_state == 0)
			{
				if(u.mesh_inst->GetProgress2() >= u.data->frames->t[F_BASH])
					u.animation_state = 1;
			}
			if(Net::IsLocal() && u.animation_state == 1 && !u.hitted)
			{
				if(DoShieldSmash(ctx, u))
					u.hitted = true;
			}
			if(u.mesh_inst->frame_end_info2)
			{
				u.action = A_NONE;
				u.mesh_inst->frame_end_info2 = false;
				u.mesh_inst->Deactivate(1);
			}
			break;
		case A_DRINK:
			{
				float p = u.mesh_inst->GetProgress2();
				if(p >= 28.f / 52.f && u.animation_state == 0)
				{
					PlayUnitSound(u, sGulp);
					u.animation_state = 1;
					if(Net::IsLocal())
						u.ApplyConsumableEffect(u.used_item->ToConsumable());
				}
				if(p >= 49.f / 52.f && u.animation_state == 1)
				{
					u.animation_state = 2;
					u.used_item = nullptr;
				}
				if(u.mesh_inst->frame_end_info2)
				{
					if(u.usable)
					{
						u.animation_state = AS_ANIMATION2_USING;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.mesh_inst->frame_end_info2 = false;
					u.mesh_inst->Deactivate(1);
				}
			}
			break;
		case A_EAT:
			{
				float p = u.mesh_inst->GetProgress2();
				if(p >= 32.f / 70 && u.animation_state == 0)
				{
					u.animation_state = 1;
					PlayUnitSound(u, sEat);
				}
				if(p >= 48.f / 70 && u.animation_state == 1)
				{
					u.animation_state = 2;
					if(Net::IsLocal())
						u.ApplyConsumableEffect(u.used_item->ToConsumable());
				}
				if(p >= 60.f / 70 && u.animation_state == 2)
				{
					u.animation_state = 3;
					u.used_item = nullptr;
				}
				if(u.mesh_inst->frame_end_info2)
				{
					if(u.usable)
					{
						u.animation_state = AS_ANIMATION2_USING;
						u.action = A_ANIMATION2;
					}
					else
						u.action = A_NONE;
					u.mesh_inst->frame_end_info2 = false;
					u.mesh_inst->Deactivate(1);
				}
			}
			break;
		case A_PAIN:
			if(u.mesh_inst->mesh->head.n_groups == 2)
			{
				if(u.mesh_inst->frame_end_info2)
				{
					u.action = A_NONE;
					u.mesh_inst->frame_end_info2 = false;
					u.mesh_inst->Deactivate(1);
				}
			}
			else if(u.mesh_inst->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_BATTLE;
				u.mesh_inst->frame_end_info = false;
			}
			break;
		case A_CAST:
			if(u.mesh_inst->mesh->head.n_groups == 2)
			{
				if(Net::IsLocal() && u.animation_state == 0 && u.mesh_inst->GetProgress2() >= u.data->frames->t[F_CAST])
				{
					u.animation_state = 1;
					CastSpell(ctx, u);
				}
				if(u.mesh_inst->frame_end_info2)
				{
					u.action = A_NONE;
					u.mesh_inst->frame_end_info2 = false;
					u.mesh_inst->Deactivate(1);
				}
			}
			else
			{
				if(Net::IsLocal() && u.animation_state == 0 && u.mesh_inst->GetProgress() >= u.data->frames->t[F_CAST])
				{
					u.animation_state = 1;
					CastSpell(ctx, u);
				}
				if(u.mesh_inst->frame_end_info)
				{
					u.action = A_NONE;
					u.animation = ANI_BATTLE;
					u.mesh_inst->frame_end_info = false;
				}
			}
			break;
		case A_ANIMATION:
			if(u.mesh_inst->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_STAND;
				u.current_animation = (Animation)-1;
			}
			break;
		case A_ANIMATION2:
			{
				bool allow_move = true;
				if(Net::IsServer())
				{
					if(u.IsPlayer() && &u != pc->unit)
						allow_move = false;
				}
				else if(Net::IsClient())
				{
					if(!u.IsPlayer() || &u != pc->unit)
						allow_move = false;
				}
				if(u.animation_state == AS_ANIMATION2_MOVE_TO_ENDPOINT)
				{
					u.timer += dt;
					if(allow_move && u.timer >= 0.5f)
					{
						u.action = A_NONE;
						u.visual_pos = u.pos = u.target_pos;
						u.changed = true;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::USE_USABLE;
							c.unit = &u;
							c.id = u.usable->netid;
							c.ile = USE_USABLE_END;
						}
						if(Net::IsLocal())
							u.UseUsable(nullptr);
						break;
					}

					if(allow_move)
					{
						// przesuñ postaæ
						u.visual_pos = u.pos = Vec3::Lerp(u.target_pos2, u.target_pos, u.timer * 2);

						// obrót
						float target_rot = Vec3::LookAtAngle(u.target_pos, u.usable->pos);
						float dif = AngleDiff(u.rot, target_rot);
						if(NotZero(dif))
						{
							const float rot_speed = u.GetRotationSpeed() * 2 * dt;
							const float arc = ShortestArc(u.rot, target_rot);

							if(dif <= rot_speed)
								u.rot = target_rot;
							else
								u.rot = Clip(u.rot + Sign(arc) * rot_speed);
						}

						u.changed = true;
					}
				}
				else
				{
					BaseUsable& bu = *u.usable->base;

					if(u.animation_state > AS_ANIMATION2_MOVE_TO_OBJECT)
					{
						// odtwarzanie dŸwiêku
						if(bu.sound)
						{
							if(u.mesh_inst->GetProgress() >= bu.sound_timer)
							{
								if(u.animation_state == AS_ANIMATION2_USING)
								{
									u.animation_state = AS_ANIMATION2_USING_SOUND;
									sound_mgr->PlaySound3d(bu.sound->sound, u.GetCenter(), 2.f, 5.f);
									if(Net::IsServer())
									{
										NetChange& c = Add1(Net::changes);
										c.type = NetChange::USABLE_SOUND;
										c.unit = &u;
									}
								}
							}
							else if(u.animation_state == AS_ANIMATION2_USING_SOUND)
								u.animation_state = AS_ANIMATION2_USING;
						}
					}
					else if(Net::IsLocal() || &u == pc->unit)
					{
						// ustal docelowy obrót postaci
						float target_rot;
						if(bu.limit_rot == 0)
							target_rot = u.rot;
						else if(bu.limit_rot == 1)
						{
							float rot1 = Clip(u.use_rot + PI / 2),
								dif1 = AngleDiff(rot1, u.usable->rot),
								rot2 = Clip(u.usable->rot + PI),
								dif2 = AngleDiff(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.usable->rot;
							else
								target_rot = rot2;
						}
						else if(bu.limit_rot == 2)
							target_rot = u.usable->rot;
						else if(bu.limit_rot == 3)
						{
							float rot1 = Clip(u.use_rot + PI),
								dif1 = AngleDiff(rot1, u.usable->rot),
								rot2 = Clip(u.usable->rot + PI),
								dif2 = AngleDiff(rot1, rot2);

							if(dif1 < dif2)
								target_rot = u.usable->rot;
							else
								target_rot = rot2;
						}
						else
							target_rot = u.usable->rot + PI;
						target_rot = Clip(target_rot);

						// obrót w strone obiektu
						const float dif = AngleDiff(u.rot, target_rot);
						const float rot_speed = u.GetRotationSpeed() * 2;
						if(allow_move && NotZero(dif))
						{
							const float rot_speed_dt = rot_speed * dt;
							if(dif <= rot_speed_dt)
								u.rot = target_rot;
							else
							{
								const float arc = ShortestArc(u.rot, target_rot);
								u.rot = Clip(u.rot + Sign(arc) * rot_speed_dt);
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
								u.visual_pos = u.pos = Vec3::Lerp(u.target_pos, u.target_pos2, u.timer * 2);
								u.changed = true;
								global_col.clear();
								float my_radius = u.GetUnitRadius();
								bool ok = true;
								for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
								{
									if(&u == *it2 || !(*it2)->IsStanding())
										continue;

									float radius = (*it2)->GetUnitRadius();
									if(Distance((*it2)->pos.x, (*it2)->pos.z, u.pos.x, u.pos.z) <= radius + my_radius)
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
			if(Net::IsClient())
			{
				if(u.player != pc)
					break;
			}
			else
			{
				if(u.player && u.player != pc)
					break;
			}
			u.timer += dt;
			if(u.animation_state == 1)
			{
				// obs³uga animacji cierpienia
				if(u.mesh_inst->mesh->head.n_groups == 2)
				{
					if(u.mesh_inst->frame_end_info2 || u.timer >= 0.5f)
					{
						u.mesh_inst->frame_end_info2 = false;
						u.mesh_inst->Deactivate(1);
						u.animation_state = 2;
					}
				}
				else if(u.mesh_inst->frame_end_info || u.timer >= 0.5f)
				{
					u.animation = ANI_BATTLE;
					u.mesh_inst->frame_end_info = false;
					u.animation_state = 2;
				}
			}
			if(u.timer >= 0.5f)
			{
				u.action = A_NONE;
				u.visual_pos = u.pos = u.target_pos;

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::USE_USABLE;
					c.unit = &u;
					c.id = u.usable->netid;
					c.ile = USE_USABLE_END;
				}

				if(Net::IsLocal())
					u.UseUsable(nullptr);
			}
			else
				u.visual_pos = u.pos = Vec3::Lerp(u.target_pos2, u.target_pos, u.timer * 2);
			u.changed = true;
			break;
		case A_PICKUP:
			if(u.mesh_inst->frame_end_info)
			{
				u.action = A_NONE;
				u.animation = ANI_STAND;
				u.current_animation = (Animation)-1;
			}
			break;
		case A_DASH:
			// update unit dash/bull charge
			{
				assert(u.player); // todo
				float dt_left = min(dt, u.timer);
				float t;
				const float eps = 0.05f;
				float len = u.speed * dt_left;
				Vec3 dir(sin(u.use_rot)*(len + eps), 0, cos(u.use_rot)*(len + eps));
				Vec3 dir_normal = dir.Normalized();
				bool ok = true;
				Vec3 from = u.GetPhysicsPos();

				if(u.animation_state == 0)
				{
					// dash
					LineTest(u.cobj->getCollisionShape(), from, dir, [&u](btCollisionObject* obj, bool)
					{
						int flags = obj->getCollisionFlags();
						if(IS_SET(flags, CG_TERRAIN))
							return LT_IGNORE;
						if(IS_SET(flags, CG_UNIT) && obj->getUserPointer() == &u)
							return LT_IGNORE;
						return LT_COLLIDE;
					}, t);
				}
				else
				{
					// bull charge, do line test and find targets
					static vector<Unit*> targets;
					targets.clear();
					LineTest(u.cobj->getCollisionShape(), from, dir, [&](btCollisionObject* obj, bool second)
					{
						int flags = obj->getCollisionFlags();
						if(!second)
						{
							if(IS_SET(flags, CG_TERRAIN))
								return LT_IGNORE;
							if(IS_SET(flags, CG_UNIT) && obj->getUserPointer() == &u)
								return LT_IGNORE;
						}
						else
						{
							if(IS_SET(obj->getCollisionFlags(), CG_UNIT))
							{
								Unit* unit = (Unit*)obj->getUserPointer();
								targets.push_back(unit);
								return LT_IGNORE;
							}
						}
						return LT_COLLIDE;
					}, t, nullptr, true);

					// check all hitted
					if(Net::IsLocal())
					{
						float move_angle = Angle(0, 0, dir.x, dir.z);
						Vec3 dir_left(sin(u.use_rot + PI / 2)*len, 0, cos(u.use_rot + PI / 2)*len);
						Vec3 dir_right(sin(u.use_rot - PI / 2)*len, 0, cos(u.use_rot - PI / 2)*len);
						for(auto unit : targets)
						{
							// deal damage/stun
							bool move_forward = true;
							if(!IsFriend(*unit, u))
							{
								if(!u.player->IsHit(unit))
								{
									GiveDmg(ctx, &u, 100.f + u.Get(AttributeId::STR) * 2, *unit);
									if(!unit->IsAlive())
										continue;
									else
										u.player->action_targets.push_back(unit);
								}
								unit->ApplyStun(1.5f);
							}
							else
								move_forward = false;

							auto unit_clbk = [unit](btCollisionObject* obj, bool)
							{
								int flags = obj->getCollisionFlags();
								if(IS_SET(flags, CG_TERRAIN | CG_UNIT))
									return LT_IGNORE;
								return LT_COLLIDE;
							};

							// try to push forward
							if(move_forward)
							{
								Vec3 move_dir = unit->pos - u.pos;
								move_dir.y = 0;
								move_dir.Normalize();
								move_dir += dir_normal;
								move_dir.Normalize();
								move_dir *= len;
								float t;
								LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), move_dir, unit_clbk, t);
								if(t == 1.f)
								{
									unit->moved = true;
									unit->pos += move_dir;
									MoveUnit(*unit, false, true);
									continue;
								}
							}

							// try to push, left or right
							float angle = Angle(u.pos.x, u.pos.z, unit->pos.x, unit->pos.z);
							float best_dir = ShortestArc(move_angle, angle);
							bool inner_ok = false;
							for(int i = 0; i < 2; ++i)
							{
								float t;
								Vec3& actual_dir = (best_dir < 0 ? dir_left : dir_right);
								LineTest(unit->cobj->getCollisionShape(), unit->GetPhysicsPos(), actual_dir, unit_clbk, t);
								if(t == 1.f)
								{
									inner_ok = true;
									unit->moved = true;
									unit->pos += actual_dir;
									MoveUnit(*unit, false, true);
									break;
								}
							}
							if(!inner_ok)
								ok = false;
						}
					}
				}

				// move if there is free space
				float actual_len = (len + eps) * t - eps;
				if(actual_len > 0 && ok)
				{
					u.pos += Vec3(sin(u.use_rot), 0, cos(u.use_rot)) * actual_len;
					MoveUnit(u, false, true);
				}

				// end dash
				u.timer -= dt;
				if(u.timer <= 0 || t < 1.f || !ok)
				{
					if(u.animation_state == 1)
					{
						u.mesh_inst->Deactivate(1);
						u.mesh_inst->groups[1].blend_max = 0.33f;
					}
					u.action = A_NONE;
					u.mesh_inst->groups[0].speed = u.GetRunSpeed() / u.data->run_speed;
				}
			}
			break;
		case A_DESPAWN:
			u.timer -= dt;
			if(u.timer <= 0.f)
				RemoveUnit(&u);
			break;
		case A_PREPARE:
			assert(Net::IsClient());
			break;
		case A_STAND_UP:
			if(u.mesh_inst->frame_end_info)
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

		u.UpdateStaminaAction();
	}
}

// dzia³a tylko dla cz³onków dru¿yny!
void Game::UpdateUnitInventory(Unit& u, bool notify)
{
	bool changes = false;
	int index = 0;
	const Item* prev_slots[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
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

		if(Net::IsOnline() && players > 1 && notify)
		{
			for(int i = 0; i < SLOT_MAX; ++i)
			{
				if(u.slots[i] != prev_slots[i])
				{
					NetChange& c = Add1(Net::changes);
					c.unit = &u;
					c.type = NetChange::CHANGE_EQUIPMENT;
					c.id = i;
				}
			}
		}
	}
}

bool Game::DoShieldSmash(LevelContext& ctx, Unit& attacker)
{
	assert(attacker.HaveShield());

	Vec3 hitpoint;
	Unit* hitted;
	Mesh* mesh = attacker.GetShield().mesh;

	if(!mesh)
		return false;

	if(!CheckForHit(ctx, attacker, hitted, *mesh->FindPoint("hit"), attacker.mesh_inst->mesh->GetPoint(NAMES::point_shield), hitpoint))
		return false;

	if(!IS_SET(hitted->data->flags, F_DONT_SUFFER) && hitted->last_bash <= 0.f)
	{
		hitted->last_bash = 1.f + float(hitted->Get(AttributeId::END)) / 50.f;

		BreakUnitAction(*hitted);

		if(hitted->action != A_POSITION)
			hitted->action = A_PAIN;
		else
			hitted->animation_state = 1;

		if(hitted->mesh_inst->mesh->head.n_groups == 2)
		{
			hitted->mesh_inst->frame_end_info2 = false;
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
			hitted->mesh_inst->groups[1].speed = 1.f;
		}
		else
		{
			hitted->mesh_inst->frame_end_info = false;
			hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
			hitted->mesh_inst->groups[0].speed = 1.f;
			hitted->animation = ANI_PLAY;
		}

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.unit = hitted;
			c.type = NetChange::STUNNED;
		}
	}

	DoGenericAttack(ctx, attacker, *hitted, hitpoint, attacker.CalculateShieldAttack(), DMG_BLUNT, true);

	return true;
}

Vec4 Game::GetFogColor()
{
	return fog_color;
}

Vec4 Game::GetFogParams()
{
	if(cl_fog)
		return fog_params;
	else
		return Vec4(cam.draw_range, cam.draw_range + 1, 1, 0);
}

Vec4 Game::GetAmbientColor()
{
	if(!cl_lighting)
		return Vec4(1, 1, 1, 1);
	//return Vec4(0.1f,0.1f,0.1f,1);
	return ambient_color;
}

Vec4 Game::GetLightDir()
{
	// 	Vec3 light_dir(1, 1, 1);
	// 	D3DXVec3Normalize(&light_dir, &light_dir);
	// 	return Vec4(light_dir, 1);

	Vec3 light_dir(sin(L.light_angle), 2.f, cos(L.light_angle));
	light_dir.Normalize();
	return Vec4(light_dir, 1);
}

Vec4 Game::GetLightColor()
{
	return Vec4(1, 1, 1, 1);
	//return Vec4(0.5f,0.5f,0.5f,1);
}

struct BulletCallback : public btCollisionWorld::ConvexResultCallback
{
	BulletCallback(btCollisionObject* ignore) : target(nullptr), ignore(ignore)
	{
		CLEAR_BIT(m_collisionFilterMask, CG_BARRIER);
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
	{
		assert(m_closestHitFraction >= convexResult.m_hitFraction);

		if(convexResult.m_hitCollisionObject == ignore)
			return 1.f;

		m_closestHitFraction = convexResult.m_hitFraction;
		hitpoint = convexResult.m_hitPointLocal;
		target = (btCollisionObject*)convexResult.m_hitCollisionObject;
		return 0.f;
	}

	btCollisionObject* ignore, *target;
	btVector3 hitpoint;
};

void Game::UpdateBullets(LevelContext& ctx, float dt)
{
	bool deletions = false;

	for(vector<Bullet>::iterator it = ctx.bullets->begin(), end = ctx.bullets->end(); it != end; ++it)
	{
		// update position
		Vec3 prev_pos = it->pos;
		it->pos += Vec3(sin(it->rot.y)*it->speed, it->yspeed, cos(it->rot.y)*it->speed) * dt;
		if(it->spell && it->spell->type == Spell::Ball)
			it->yspeed -= 10.f*dt;

		// update particles
		if(it->pe)
			it->pe->pos = it->pos;
		if(it->trail)
		{
			Vec3 pt1 = it->pos;
			pt1.y += 0.05f;
			Vec3 pt2 = it->pos;
			pt2.y -= 0.05f;
			it->trail->Update(0, &pt1, &pt2);

			pt1 = it->pos;
			pt1.x += sin(it->rot.y + PI / 2)*0.05f;
			pt1.z += cos(it->rot.y + PI / 2)*0.05f;
			pt2 = it->pos;
			pt2.x -= sin(it->rot.y + PI / 2)*0.05f;
			pt2.z -= cos(it->rot.y + PI / 2)*0.05f;
			it->trail2->Update(0, &pt1, &pt2);
		}

		// remove bullet on timeout
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
			continue;
		}

		// do contact test
		btCollisionShape* shape;
		if(!it->spell)
			shape = shape_arrow;
		else
			shape = it->spell->shape;
		assert(shape->isConvex());

		btTransform tr_from, tr_to;
		tr_from.setOrigin(ToVector3(prev_pos));
		tr_from.setRotation(btQuaternion(it->rot.y, it->rot.x, it->rot.z));
		tr_to.setOrigin(ToVector3(it->pos));
		tr_to.setRotation(tr_from.getRotation());

		BulletCallback callback(it->owner ? it->owner->cobj : nullptr);
		phy_world->convexSweepTest((btConvexShape*)shape, tr_from, tr_to, callback);
		if(!callback.hasHit())
			continue;

		Vec3 hitpoint = ToVec3(callback.hitpoint);
		Unit* hitted = nullptr;
		if(IS_SET(callback.target->getCollisionFlags(), CG_UNIT))
			hitted = (Unit*)callback.target->getUserPointer();

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

		if(hitted)
		{
			if(!Net::IsLocal())
				continue;

			if(!it->spell)
			{
				if(it->owner && IsFriend(*it->owner, *hitted) || it->attack < -50.f)
				{
					// friendly fire
					if(hitted->action == A_BLOCK && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, 2.f, 10.f);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.ile = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, 2.f, false);
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
				float angle_dif = AngleDiff(it->rot.y, hitted->rot);
				float backstab_mod;
				if(it->backstab == 0)
					backstab_mod = 0.25f;
				if(it->backstab == 1)
					backstab_mod = 0.5f;
				else
					backstab_mod = 0.75f;
				if(IS_SET(hitted->data->flags2, F2_BACKSTAB_RES))
					backstab_mod /= 2;
				m += angle_dif / PI * backstab_mod;

				// modyfikator obra¿eñ
				dmg *= m;
				float base_dmg = dmg;

				if(hitted->action == A_BLOCK && angle_dif < PI * 2 / 5)
				{
					float block = hitted->CalculateBlock() * hitted->mesh_inst->groups[1].GetBlendT() * (1.f - angle_dif / (PI * 2 / 5));
					float stamina = min(block, dmg);
					dmg -= block;
					hitted->RemoveStaminaBlock(stamina);

					MATERIAL_TYPE mat = hitted->GetShield().material;
					sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, 2.f, 10.f);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HIT_SOUND;
						c.id = MAT_IRON;
						c.ile = mat;
						c.pos = hitpoint;
					}

					if(hitted->IsPlayer())
					{
						// player blocked bullet, train shield
						hitted->player->Train(TrainWhat::BlockBullet, base_dmg / hitted->hpmax, it->level);
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
					hitted->player->Train(TrainWhat::TakeDamageArmor, base_dmg / hitted->hpmax, it->level);

				// hit sound
				PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, 2.f, dmg > 0.f);

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
					float v = dmg / hitted->hpmax;
					if(hitted->hp - dmg < 0.f && !hitted->IsImmortal())
						v = max(TRAIN_KILL_RATIO, v);
					if(v > 1.f)
						v = 1.f;
					it->owner->player->Train(TrainWhat::BowAttack, v, hitted->level);
				}

				GiveDmg(ctx, it->owner, dmg, *hitted, &hitpoint, 0);

				// apply poison
				if(it->poison_attack > 0.f && !IS_SET(hitted->data->flags, F_POISON_RES))
				{
					Effect& e = Add1(hitted->effects);
					e.power = it->poison_attack / 5;
					e.time = 5.f;
					e.effect = EffectId::Poison;
				}
			}
			else
			{
				// trafienie w postaæ z czara
				if(it->owner && IsFriend(*it->owner, *hitted))
				{
					// frendly fire
					SpellHitEffect(ctx, *it, hitpoint, hitted);

					// dŸwiêk trafienia w postaæ
					if(hitted->action == A_BLOCK && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, 2.f, 10.f);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.ile = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, 2.f, false);
					continue;
				}

				if(hitted->IsAI())
					AI_HitReaction(*hitted, it->start_pos);

				float dmg = it->attack;
				if(it->owner)
					dmg += it->owner->level * it->spell->dmg_bonus;
				float angle_dif = AngleDiff(it->rot.y, hitted->rot);
				float base_dmg = dmg;

				if(hitted->action == A_BLOCK && angle_dif < PI * 2 / 5)
				{
					float block = hitted->CalculateBlock() * hitted->mesh_inst->groups[1].GetBlendT() * (1.f - angle_dif / (PI * 2 / 5));
					float stamina = min(dmg, block);
					dmg -= block / 2;
					hitted->RemoveStaminaBlock(stamina);

					if(hitted->IsPlayer())
					{
						// player blocked spell, train him
						hitted->player->Train(TrainWhat::BlockBullet, base_dmg / hitted->hpmax, it->level);
					}

					if(dmg < 0)
					{
						// blocked by shield
						SpellHitEffect(ctx, *it, hitpoint, hitted);
						continue;
					}
				}

				GiveDmg(ctx, it->owner, dmg, *hitted, &hitpoint, !IS_SET(it->spell->flags, Spell::Poison) ? DMG_MAGICAL : 0);

				// apply poison
				if(IS_SET(it->spell->flags, Spell::Poison) && !IS_SET(hitted->data->flags, F_POISON_RES))
				{
					Effect& e = Add1(hitted->effects);
					e.power = dmg / 5;
					e.time = 5.f;
					e.effect = EffectId::Poison;
				}

				// apply spell effect
				SpellHitEffect(ctx, *it, hitpoint, hitted);
			}
		}
		else
		{
			// trafiono w obiekt
			if(!it->spell)
			{
				sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, 2.f, 10.f);

				ParticleEmitter* pe = new ParticleEmitter;
				pe->tex = tIskra;
				pe->emision_interval = 0.01f;
				pe->life = 5.f;
				pe->particle_life = 0.5f;
				pe->emisions = 1;
				pe->spawn_min = 10;
				pe->spawn_max = 15;
				pe->max_particles = 15;
				pe->pos = hitpoint;
				pe->speed_min = Vec3(-1, 0, -1);
				pe->speed_max = Vec3(1, 1, 1);
				pe->pos_min = Vec3(-0.1f, -0.1f, -0.1f);
				pe->pos_max = Vec3(0.1f, 0.1f, 0.1f);
				pe->size = 0.3f;
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 0.9f;
				pe->op_alpha = POP_LINEAR_SHRINK;
				pe->mode = 0;
				pe->Init();
				ctx.pes->push_back(pe);

				if(Net::IsLocal() && in_tutorial && callback.target)
				{
					void* ptr = callback.target->getUserPointer();
					if((ptr == tut_shield || ptr == tut_shield2) && tut_state == 12)
					{
						Train(*pc->unit, true, (int)SkillId::BOW, 1);
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
						for(vector<Door*>::iterator it = L.local_ctx.doors->begin(), end = L.local_ctx.doors->end(); it != end; ++it)
						{
							if((*it)->locked == LOCK_TUTORIAL + unlock)
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
				SpellHitEffect(ctx, *it, hitpoint, nullptr);
			}
		}
	}

	if(deletions)
		RemoveElements(ctx.bullets, [](const Bullet& b) { return b.remove; });
}

void Game::SpawnDungeonColliders()
{
	assert(!L.location->outside);

	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Pole* m = lvl.map;
	int w = lvl.w,
		h = lvl.h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(czy_blokuje2(m[x + y * w]) && (!czy_blokuje2(m[x - 1 + (y - 1)*w]) || !czy_blokuje2(m[x + (y - 1)*w]) || !czy_blokuje2(m[x + 1 + (y - 1)*w]) ||
				!czy_blokuje2(m[x - 1 + y * w]) || !czy_blokuje2(m[x + 1 + y * w]) ||
				!czy_blokuje2(m[x - 1 + (y + 1)*w]) || !czy_blokuje2(m[x + (y + 1)*w]) || !czy_blokuje2(m[x + 1 + (y + 1)*w])))
			{
				SpawnDungeonCollider(Vec3(2.f*x + 1.f, 2.f, 2.f*y + 1.f));
			}
		}
	}

	// lewa/prawa œciana
	for(int i = 1; i < h - 1; ++i)
	{
		// lewa
		if(czy_blokuje2(m[i*w]) && !czy_blokuje2(m[1 + i * w]))
			SpawnDungeonCollider(Vec3(1.f, 2.f, 2.f*i + 1.f));

		// prawa
		if(czy_blokuje2(m[i*w + w - 1]) && !czy_blokuje2(m[w - 2 + i * w]))
			SpawnDungeonCollider(Vec3(2.f*(w - 1) + 1.f, 2.f, 2.f*i + 1.f));
	}

	// przednia/tylna œciana
	for(int i = 1; i < lvl.w - 1; ++i)
	{
		// przednia
		if(czy_blokuje2(m[i + (h - 1)*w]) && !czy_blokuje2(m[i + (h - 2)*w]))
			SpawnDungeonCollider(Vec3(2.f*i + 1.f, 2.f, 2.f*(h - 1) + 1.f));

		// tylna
		if(czy_blokuje2(m[i]) && !czy_blokuje2(m[i + w]))
			SpawnDungeonCollider(Vec3(2.f*i + 1.f, 2.f, 1.f));
	}

	// up stairs
	if(inside->HaveUpStairs())
	{
		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_schody);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
		cobj->getWorldTransform().setOrigin(btVector3(2.f*lvl.staircase_up.x + 1.f, 0.f, 2.f*lvl.staircase_up.y + 1.f));
		cobj->getWorldTransform().setRotation(btQuaternion(dir_to_rot(lvl.staircase_up_dir), 0, 0));
		phy_world->addCollisionObject(cobj, CG_BUILDING);
	}

	// room floors/ceilings
	dungeon_shape_pos.clear();
	dungeon_shape_index.clear();
	int index = 0;

	if((inside->type == L_DUNGEON && inside->target == LABIRYNTH) || inside->type == L_CAVE)
	{
		const float h = Room::HEIGHT;
		for(int x = 0; x < 16; ++x)
		{
			for(int y = 0; y < 16; ++y)
			{
				// floor
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, 0, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, 0, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, 0, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, 0, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_index.push_back(index);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 3);
				index += 4;

				// ceil
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, h, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, h, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, h, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, h, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_index.push_back(index);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 3);
				dungeon_shape_index.push_back(index + 1);
				index += 4;
			}
		}
	}

	for(Room& room : lvl.rooms)
	{
		// floor
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, 0, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), 0, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, 0, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), 0, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_index.push_back(index);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 3);
		index += 4;

		// ceil
		const float h = (room.IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, h, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), h, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, h, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), h, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_index.push_back(index);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 3);
		dungeon_shape_index.push_back(index + 1);
		index += 4;
	}

	delete dungeon_shape;
	delete dungeon_shape_data;

	dungeon_shape_data = new btTriangleIndexVertexArray(dungeon_shape_index.size() / 3, dungeon_shape_index.data(), sizeof(int) * 3,
		dungeon_shape_pos.size(), (btScalar*)dungeon_shape_pos.data(), sizeof(Vec3));
	dungeon_shape = new btBvhTriangleMeshShape(dungeon_shape_data, true);

	obj_dungeon = new btCollisionObject;
	obj_dungeon->setCollisionShape(dungeon_shape);
	obj_dungeon->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	phy_world->addCollisionObject(obj_dungeon, CG_BUILDING);
}

void Game::SpawnDungeonCollider(const Vec3& pos)
{
	auto cobj = new btCollisionObject;
	cobj->setCollisionShape(shape_wall);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	cobj->getWorldTransform().setOrigin(ToVector3(pos));
	phy_world->addCollisionObject(cobj, CG_BUILDING);
}

void Game::RemoveColliders()
{
	if(phy_world)
		phy_world->Reset();

	DeleteElements(shapes);
}

void Game::CreateCollisionShapes()
{
	const float size = 256.f;
	const float border = 32.f;

	shape_wall = new btBoxShape(btVector3(1.f, 2.f, 1.f));
	shape_low_ceiling = new btBoxShape(btVector3(1.f, 0.5f, 1.f));
	shape_ceiling = new btStaticPlaneShape(btVector3(0.f, -1.f, 0.f), 4.f);
	shape_floor = new btStaticPlaneShape(btVector3(0.f, 1.f, 0.f), 0.f);
	shape_door = new btBoxShape(btVector3(0.842f, 1.319f, 0.181f));
	shape_block = new btBoxShape(btVector3(1.f, 4.f, 1.f));
	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f, 2.f, 0.1f));
	shape_schody_c[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f, 2.f, 0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f, 2.f, 1.f));
	shape_schody_c[1] = b;
	tr.setOrigin(btVector3(-0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	shape_schody = s;
	shape_summon = new btCylinderShape(btVector3(1.5f / 2, 0.75f, 1.5f / 2));
	shape_barrier = new btBoxShape(btVector3(size / 2, 40.f, border / 2));

	Mesh::Point* point = aArrow->FindPoint("Empty");
	assert(point && point->IsBox());
	shape_arrow = new btBoxShape(ToVector3(point->size));
}

Vec3 Game::PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const
{
	if(bullet_speed == 0.f)
		return target.GetCenter();
	else
	{
		Vec3 vel = target.pos - target.prev_pos;
		vel *= 60;

		float a = bullet_speed * bullet_speed - vel.Dot2d();
		float b = -2 * vel.Dot2d(target.pos - me.pos);
		float c = -(target.pos - me.pos).Dot2d();

		float delta = b * b - 4 * a*c;
		// brak rozwi¹zania, nie mo¿e trafiæ wiêc strzel w aktualn¹ pozycjê
		if(delta < 0)
			return target.GetCenter();

		Vec3 pos = target.pos + ((b + std::sqrt(delta)) / (2 * a)) * Vec3(vel.x, 0, vel.z);
		pos.y += target.GetUnitHeight() / 2;
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

bool Game::CanShootAtLocation(const Vec3& from, const Vec3& to) const
{
	BulletRaytestCallback2 callback;
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);
	return callback.clear;
}

bool Game::CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const
{
	BulletRaytestCallback callback(&me, ptr);
	phy_world->rayTest(btVector3(me.pos.x, me.pos.y + 1.f, me.pos.z), btVector3(to.x, to.y + 1.f, to.z), callback);
	return callback.clear;
}

void Game::SpawnTerrainCollider()
{
	if(terrain_shape)
		delete terrain_shape;

	terrain_shape = new btHeightfieldTerrainShape(OutsideLocation::size + 1, OutsideLocation::size + 1, terrain->GetHeightMap(), 1.f, 0.f, 10.f, 1, PHY_FLOAT, false);
	terrain_shape->setLocalScaling(btVector3(2.f, 1.f, 2.f));

	obj_terrain = new btCollisionObject;
	obj_terrain->setCollisionShape(terrain_shape);
	obj_terrain->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_TERRAIN);
	obj_terrain->getWorldTransform().setOrigin(btVector3(float(OutsideLocation::size), 5.f, float(OutsideLocation::size)));
	phy_world->addCollisionObject(obj_terrain, CG_TERRAIN);
}

void Game::GenerateDungeonObjects()
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	static vector<Chest*> room_chests;
	static vector<Vec3> on_wall;
	static vector<Int2> blocks;
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

		AddRoomColliders(lvl, *it, blocks);

		// choose room type
		RoomType* rt;
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
				Int2 pt;
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
						Object* o = L.local_ctx.FindObject(BaseObject::Get("portal"));
						if(o)
							pt = pos_to_pt(o->pos);
						else
							pt = it->CenterTile();
					}
				}

				for(int y = -1; y <= 1; ++y)
				{
					for(int x = -1; x <= 1; ++x)
						blocks.push_back(Int2(pt.x + x, pt.y + y));
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

		// try to spawn all objects
		for(uint i = 0; i < rt->count && fail > 0; ++i)
		{
			bool is_group = false;
			BaseObject* base = BaseObject::Get(rt->objs[i].id, &is_group);
			int count = rt->objs[i].count.Random();

			for(int j = 0; j < count && fail > 0; ++j)
			{
				auto e = GenerateDungeonObject(lvl, *it, base, on_wall, blocks, flags);
				if(!e)
				{
					if(IS_SET(base->flags, OBJ_IMPORTANT))
						--j;
					--fail;
					continue;
				}

				if(e.type == ObjectEntity::CHEST)
					room_chests.push_back(e);

				if(IS_SET(base->flags, OBJ_REQUIRED))
					wymagany_obiekt = true;

				if(is_group)
					base = BaseObject::Get(rt->objs[i].id);
			}
		}

		if(wymagany && wymagany_obiekt && it->target == RoomTarget::None)
			wymagany = false;

		if(!room_chests.empty())
		{
			bool extra = IS_SET(rt->flags, RT_TREASURE);
			GenerateDungeonTreasure(room_chests, chest_lvl, extra);
			room_chests.clear();
		}

		on_wall.clear();
		blocks.clear();
	}

	if(wymagany)
		throw "Failed to generate required object!";

	if(L.local_ctx.chests->empty())
	{
		// niech w podziemiach bêdzie minimum 1 skrzynia
		BaseObject* base = BaseObject::Get("chest");
		for(int i = 0; i < 100; ++i)
		{
			on_wall.clear();
			blocks.clear();
			Room& r = lvl.rooms[Rand() % lvl.rooms.size()];
			if(r.target == RoomTarget::None)
			{
				AddRoomColliders(lvl, r, blocks);

				auto e = GenerateDungeonObject(lvl, r, base, on_wall, blocks, flags);
				if(e)
				{
					GenerateDungeonTreasure(*L.local_ctx.chests, chest_lvl);
					break;
				}
			}
		}
	}
}

Game::ObjectEntity Game::GenerateDungeonObject(InsideLocationLevel& lvl, Room& room, BaseObject* base, vector<Vec3>& on_wall, vector<Int2>& blocks, int flags)
{
	Vec3 pos;
	float rot;
	Vec2 shift;

	if(base->type == OBJ_CYLINDER)
	{
		shift.x = base->r + base->extra_dist;
		shift.y = base->r + base->extra_dist;
	}
	else
		shift = base->size + Vec2(base->extra_dist, base->extra_dist);

	if(IS_SET(base->flags, OBJ_NEAR_WALL))
	{
		Int2 tile;
		int dir;
		if(!lvl.GetRandomNearWallTile(room, tile, dir, IS_SET(base->flags, OBJ_ON_WALL)))
			return nullptr;

		rot = dir_to_rot(dir);

		if(dir == 2 || dir == 3)
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f) + 2, 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f) + 2);
		else
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f), 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f));

		if(IS_SET(base->flags, OBJ_ON_WALL))
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

			for(vector<Vec3>::iterator it2 = on_wall.begin(), end2 = on_wall.end(); it2 != end2; ++it2)
			{
				float dist = Vec3::Distance2d(*it2, pos);
				if(dist < 2.f)
					return nullptr;
			}
		}
		else
		{
			switch(dir)
			{
			case 0:
				pos.x += Random(0.2f, 1.8f);
				break;
			case 1:
				pos.z += Random(0.2f, 1.8f);
				break;
			case 2:
				pos.x -= Random(0.2f, 1.8f);
				break;
			case 3:
				pos.z -= Random(0.2f, 1.8f);
				break;
			}
		}
	}
	else if(IS_SET(base->flags, OBJ_IN_MIDDLE))
	{
		rot = PI / 2 * (Rand() % 4);
		pos = room.Center();
		switch(Rand() % 4)
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
		rot = Random(MAX_ANGLE);
		float margin = (base->type == OBJ_CYLINDER ? base->r : max(base->size.x, base->size.y));
		if(!room.GetRandomPos(margin, pos))
			return nullptr;
	}

	if(IS_SET(base->flags, OBJ_HIGH))
		pos.y += 1.5f;

	if(base->type == OBJ_HITBOX)
	{
		// sprawdŸ kolizje z blokami
		if(!IS_SET(base->flags, OBJ_NO_PHYSICS))
		{
			if(NotZero(rot))
			{
				RotRect r1, r2;
				r1.center.x = pos.x;
				r1.center.y = pos.z;
				r1.rot = rot;
				r1.size = shift;
				r2.rot = 0;
				r2.size = Vec2(1, 1);
				for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
				{
					r2.center.x = 2.f*b_it->x + 1.f;
					r2.center.y = 2.f*b_it->y + 1.f;
					if(RotatedRectanglesCollision(r1, r2))
						return nullptr;
				}
			}
			else
			{
				for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
				{
					if(RectangleToRectangle(pos.x - shift.x, pos.z - shift.y, pos.x + shift.x, pos.z + shift.y,
						2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x + 1), 2.f*(b_it->y + 1)))
						return nullptr;
				}
			}
		}

		// sprawdŸ kolizje z innymi obiektami
		IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
		if(!global_col.empty() && Collide(global_col, Box2d(pos.x - shift.x, pos.z - shift.y, pos.x + shift.x, pos.z + shift.y), 0.8f, rot))
			return nullptr;
	}
	else
	{
		// sprawdŸ kolizje z blokami
		if(!IS_SET(base->flags, OBJ_NO_PHYSICS))
		{
			for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
			{
				if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x + 1.f, 2.f*b_it->y + 1.f, 1.f, 1.f))
					return nullptr;
			}
		}

		// sprawdŸ kolizje z innymi obiektami
		IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pos, base->r, &ignore);
		if(!global_col.empty() && Collide(global_col, pos, base->r + 0.8f))
			return nullptr;
	}

	if(IS_SET(base->flags, OBJ_ON_WALL))
		on_wall.push_back(pos);

	return SpawnObjectEntity(L.local_ctx, base, pos, rot, 1.f, flags);
}

void Game::AddRoomColliders(InsideLocationLevel& lvl, Room& room, vector<Int2>& blocks)
{
	// add colliding blocks
	for(int x = 0; x < room.size.x; ++x)
	{
		// top
		POLE co = lvl.map[room.pos.x + x + (room.pos.y + room.size.y - 1)*lvl.w].type;
		if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 2));
		}
		else if(co == SCIANA || co == BLOKADA_SCIANA)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));

		// bottom
		co = lvl.map[room.pos.x + x + room.pos.y*lvl.w].type;
		if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + 1));
		}
		else if(co == SCIANA || co == BLOKADA_SCIANA)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
	}
	for(int y = 0; y < room.size.y; ++y)
	{
		// left
		POLE co = lvl.map[room.pos.x + (room.pos.y + y)*lvl.w].type;
		if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
		{
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + 1, room.pos.y + y));
		}
		else if(co == SCIANA || co == BLOKADA_SCIANA)
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));

		// right
		co = lvl.map[room.pos.x + room.size.x - 1 + (room.pos.y + y)*lvl.w].type;
		if(co == PUSTE || co == KRATKA || co == KRATKA_PODLOGA || co == KRATKA_SUFIT || co == DRZWI || co == OTWOR_NA_DRZWI)
		{
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + room.size.x - 2, room.pos.y + y));
		}
		else if(co == SCIANA || co == BLOKADA_SCIANA)
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
	}
}

void Game::GenerateDungeonTreasure(vector<Chest*>& chests, int level, bool extra)
{
	int gold;
	if(chests.size() == 1)
	{
		vector<ItemSlot>& items = chests.front()->items;
		GenerateTreasure(level, 10, items, gold, extra);
		InsertItemBare(items, Item::gold, (uint)gold, (uint)gold);
		SortItems(items);
	}
	else
	{
		static vector<ItemSlot> items;
		GenerateTreasure(level, 9 + 2 * chests.size(), items, gold, extra);
		SplitTreasure(items, gold, &chests[0], chests.size());
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

void Game::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold, bool extra)
{
	assert(InRange(level, 1, 20));

	int value = Random(wartosc_skarbu[level - 1], wartosc_skarbu[level]);

	items.clear();

	const Item* item;
	uint count;

	for(int tries = 0; tries < _count; ++tries)
	{
		switch(Rand() % IT_MAX_GEN)
		{
		case IT_WEAPON:
			item = Weapon::weapons[Rand() % Weapon::weapons.size()];
			count = 1;
			break;
		case IT_ARMOR:
			item = Armor::armors[Rand() % Armor::armors.size()];
			count = 1;
			break;
		case IT_BOW:
			item = Bow::bows[Rand() % Bow::bows.size()];
			count = 1;
			break;
		case IT_SHIELD:
			item = Shield::shields[Rand() % Shield::shields.size()];
			count = 1;
			break;
		case IT_CONSUMABLE:
			item = Consumable::consumables[Rand() % Consumable::consumables.size()];
			count = Random(2, 5);
			break;
		case IT_OTHER:
			item = OtherItem::others[Rand() % OtherItem::others.size()];
			count = Random(1, 4);
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

	if(extra)
		InsertItemBare(items, ItemList::GetItem("treasure"));

	gold = value + level * 5;
}

void Game::SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count)
{
	assert(gold >= 0 && count > 0 && chests);

	while(!items.empty())
	{
		for(int i = 0; i < count && !items.empty(); ++i)
		{
			chests[i]->items.push_back(items.back());
			items.pop_back();
		}
	}

	int ile = gold / count - 1;
	if(ile < 0)
		ile = 0;
	gold -= ile * count;

	for(int i = 0; i < count; ++i)
	{
		if(i == count - 1)
			ile += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(Item::gold, ile, ile);
		SortItems(chests[i]->items);
	}
}

Unit* Game::SpawnUnitInsideRoom(Room &p, UnitData &unit, int level, const Int2& stairs_pt, const Int2& stairs_down_pt)
{
	const float radius = unit.GetRadius();
	Vec3 stairs_pos(2.f*stairs_pt.x + 1.f, 0.f, 2.f*stairs_pt.y + 1.f);

	for(int i = 0; i < 10; ++i)
	{
		Vec3 pt = p.GetRandomPos(radius);

		if(Vec3::Distance(stairs_pos, pt) < 10.f)
			continue;

		Int2 my_pt = Int2(int(pt.x / 2), int(pt.y / 2));
		if(my_pt == stairs_down_pt)
			continue;

		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pt, radius, nullptr);

		if(!Collide(global_col, pt, radius))
		{
			float rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(L.local_ctx, unit, level, nullptr, &pt, &rot);
		}
	}

	return nullptr;
}

Unit* Game::SpawnUnitNearLocation(LevelContext& ctx, const Vec3 &pos, UnitData &unit, const Vec3* look_at, int level, float extra_radius)
{
	const float radius = unit.GetRadius();

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, pos, extra_radius + radius, nullptr);

	Vec3 tmp_pos = pos;

	for(int i = 0; i < 10; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
		{
			float rot;
			if(look_at)
				rot = Vec3::LookAtAngle(tmp_pos, *look_at);
			else
				rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(ctx, unit, level, nullptr, &tmp_pos, &rot);
		}

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
	}

	return nullptr;
}

Unit* Game::CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level, Human* human_data, const Vec3* pos, const float* rot, AIController** ai)
{
	Unit* u = CreateUnit(unit, level, human_data);
	u->in_building = ctx.building_id;

	if(pos)
	{
		if(ctx.type == LevelContext::Outside)
		{
			Vec3 pt = *pos;
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

const Int2 g_kierunekk[4] = {
	Int2(0,-1),
	Int2(-1,0),
	Int2(0,1),
	Int2(1,0)
};

void Game::ChangeLevel(int where)
{
	assert(where == 1 || where == -1);

	Info(where == 1 ? "Changing level to lower." : "Changing level to upper.");

	L.event_handler = nullptr;
	UpdateDungeonMinimap(false);

	if(!in_tutorial && QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && QM.quest_crazies->crazies_state < Quest_Crazies::State::End)
		QM.quest_crazies->CheckStone();

	if(Net::IsOnline() && players > 1)
	{
		int level = L.dungeon_level;
		if(where == -1)
			--level;
		else
			++level;
		if(level >= 0)
		{
			packet_data.resize(4);
			packet_data[0] = ID_CHANGE_LEVEL;
			packet_data[1] = (byte)L.location_index;
			packet_data[2] = (byte)level;
			packet_data[3] = 0;
			int ack = peer->Send((cstring)&packet_data[0], 4, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			StreamWrite(packet_data.data(), 3, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
			for(auto info : game_players)
			{
				if(info->id == my_id)
					info->state = PlayerInfo::IN_GAME;
				else
				{
					info->state = PlayerInfo::WAITING_FOR_RESPONSE;
					info->ack = ack;
					info->timer = 5.f;
				}
			}
		}

		Net_FilterServerChanges();
	}

	if(where == -1)
	{
		// poziom w górê
		if(L.dungeon_level == 0)
		{
			if(in_tutorial)
			{
				TutEvent(3);
				fallback_type = FALLBACK::CLIENT;
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

			MultiInsideLocation* inside = (MultiInsideLocation*)L.location;
			LeaveLevel();
			--L.dungeon_level;
			inside->SetActiveLevel(L.dungeon_level);
			LocationGenerator* loc_gen = loc_gen_factory->Get(inside);
			loc_gen->Init(false, false, L.dungeon_level);
			L.enter_from = ENTER_FROM_DOWN_LEVEL;
			EnterLevel(loc_gen);
		}
	}
	else
	{
		MultiInsideLocation* inside = (MultiInsideLocation*)L.location;

		int steps = 3; // txLevelDown, txGeneratingMinimap, txLoadingComplete
		if(L.dungeon_level + 1 >= inside->generated)
			steps += 3; // txGeneratingMap, txGeneratingObjects, txGeneratingUnits
		else
			++steps; // txRegeneratingLevel

		LoadingStart(steps);
		LoadingStep(txLevelDown);

		// poziom w dó³
		LeaveLevel();
		++L.dungeon_level;

		inside->SetActiveLevel(L.dungeon_level);

		LocationGenerator* loc_gen = loc_gen_factory->Get(inside);
		loc_gen->Init(false, false, L.dungeon_level);

		// czy to pierwsza wizyta?
		if(L.dungeon_level >= inside->generated)
		{
			if(next_seed != 0)
			{
				Srand(next_seed);
				next_seed = 0;
			}
			else if(force_seed != 0 && force_seed_all)
				Srand(force_seed);

			inside->generated = L.dungeon_level + 1;
			inside->infos[L.dungeon_level].seed = RandVal();

			Info("Generating location '%s', seed %u.", L.location->name.c_str(), RandVal());
			Info("Generating dungeon, level %d, target %d.", L.dungeon_level + 1, inside->target);

			LoadingStep(txGeneratingMap);

			loc_gen->first = true;
			loc_gen->Generate();
		}

		L.enter_from = ENTER_FROM_UP_LEVEL;
		EnterLevel(loc_gen);
	}

	L.location->last_visit = W.GetWorldtime();
	CheckIfLocationCleared();
	bool loaded_resources = RequireLoadingResources(L.location, nullptr);
	LoadResources(txLoadingComplete, false);

	SetMusic();

	if(Net::IsOnline() && players > 1)
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		net_stream.Reset();
		PrepareLevelData(net_stream, loaded_resources);
		Info("Generated location packet: %d.", net_stream.GetNumberOfBytesUsed());
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

	Info("Randomness integrity: %d", RandVal());
}

void Game::AddPlayerTeam(const Vec3& pos, float rot, bool reenter, bool hide_weapon)
{
	for(Unit* unit : Team.members)
	{
		Unit& u = *unit;

		if(!reenter)
		{
			L.local_ctx.units->push_back(&u);
			CreateUnitPhysics(u);
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
		u.mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		u.mesh_inst->groups[0].speed = 1.f;
		BreakUnitAction(u);
		u.SetAnimationAtEnd();
		if(u.in_building != -1)
		{
			if(reenter)
			{
				RemoveElement(L.GetContext(u).units, &u);
				L.local_ctx.units->push_back(&u);
			}
			u.in_building = -1;
		}

		if(u.IsAI())
		{
			u.ai->state = AIController::Idle;
			u.ai->idle_action = AIController::Idle_None;
			u.ai->target = nullptr;
			u.ai->alert_target = nullptr;
			u.ai->timer = Random(2.f, 5.f);
		}

		WarpNearLocation(L.local_ctx, u, pos, L.city_ctx ? 4.f : 2.f, true, 20);
		u.visual_pos = u.pos;

		if(!L.location->outside)
			DungeonReveal(Int2(int(u.pos.x / 2), int(u.pos.z / 2)));

		if(u.interp)
			u.interp->Reset(u.pos, u.rot);
	}
}

void Game::OpenDoorsByTeam(const Int2& pt)
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	for(Unit* unit : Team.members)
	{
		Int2 unit_pt = pos_to_pt(unit->pos);
		if(FindPath(L.local_ctx, unit_pt, pt, tmp_path))
		{
			for(vector<Int2>::iterator it2 = tmp_path.begin(), end2 = tmp_path.end(); it2 != end2; ++it2)
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
						door->mesh_inst->SetToEnd(&door->mesh_inst->mesh->anims[0]);
					}
				}
			}
		}
		else
			Warn("OpenDoorsByTeam: Can't find path from unit %s (%d,%d) to spawn point (%d,%d).", unit->data->id.c_str(), unit_pt.x, unit_pt.y, pt.x, pt.y);
	}
}

void Game::ExitToMap()
{
	// zamknij gui layer
	CloseAllPanels();

	clear_color = Color::Black;
	game_state = GS_WORLDMAP;
	if(L.is_open && L.location->type == L_ENCOUNTER)
		LeaveLocation();

	W.ExitToMap();
	SetMusic(MusicType::Travel);

	if(Net::IsServer())
		Net::PushChange(NetChange::EXIT_TO_MAP);

	show_mp_panel = true;
	world_map->visible = true;
	game_gui->visible = false;
}

// wbrew nazwie tworzy te¿ ogieñ :3
void Game::RespawnObjectColliders(bool spawn_pes)
{
	for(LevelContext& ctx : L.ForEachContext())
	{
		int flags = SOE_DONT_CREATE_LIGHT;
		if(!spawn_pes)
			flags |= SOE_DONT_SPAWN_PARTICLES;

		// dotyczy tylko pochodni
		if(ctx.type == LevelContext::Inside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			BaseLocation& base = g_base_locations[inside->target];
			if(IS_SET(base.options, BLO_MAGIC_LIGHT))
				flags |= SOE_MAGIC_LIGHT;
		}

		for(vector<Object*>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
		{
			Object& obj = **it;
			BaseObject* base_obj = obj.base;

			if(!base_obj)
				continue;

			if(IS_SET(base_obj->flags, OBJ_BUILDING))
			{
				float rot = obj.rot.y;
				int roti;
				if(Equal(rot, 0))
					roti = 0;
				else if(Equal(rot, PI / 2))
					roti = 1;
				else if(Equal(rot, PI))
					roti = 2;
				else if(Equal(rot, PI * 3 / 2))
					roti = 3;
				else
				{
					assert(0);
					roti = 0;
					rot = 0.f;
				}

				ProcessBuildingObjects(ctx, nullptr, nullptr, base_obj->mesh, nullptr, rot, roti, obj.pos, nullptr, nullptr, true);
			}
			else
				SpawnObjectExtras(ctx, base_obj, obj.pos, obj.rot.y, &obj, obj.scale, flags);
		}

		if(ctx.chests)
		{
			BaseObject* chest = BaseObject::Get("chest");
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
				SpawnObjectExtras(ctx, chest, (*it)->pos, (*it)->rot, nullptr, 1.f, flags);
		}

		for(vector<Usable*>::iterator it = ctx.usables->begin(), end = ctx.usables->end(); it != end; ++it)
			SpawnObjectExtras(ctx, (*it)->base, (*it)->pos, (*it)->rot, *it, 1.f, flags);
	}
}

void Game::SetRoomPointers()
{
	for(uint i = 0; i < n_base_locations; ++i)
	{
		BaseLocation& base = g_base_locations[i];

		if(base.schody.id)
			base.schody.room = FindRoomType(base.schody.id);

		if(base.wymagany.id)
			base.wymagany.room = FindRoomType(base.wymagany.id);

		if(base.rooms)
		{
			base.room_total = 0;
			for(uint j = 0; j < base.room_count; ++j)
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
		return sBody[Rand() % 5];
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
	assert(sound);

	if(!sound_mgr->CanPlaySound())
		return;

	Vec3 pos = unit.GetHeadSoundPos();
	FMOD::Channel* channel = sound_mgr->CreateChannel(sound, pos, smin);
	AttachedSound& s = Add1(attached_sounds);
	s.channel = channel;
	s.unit = &unit;
}

void Game::StopAllSounds()
{
	sound_mgr->StopSounds();
	attached_sounds.clear();
}

Game::ATTACK_RESULT Game::DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float start_dmg, int dmg_type, bool bash)
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
	float angle_dif = AngleDiff(Clip(attacker.rot + PI), hitted.rot);
	float backstab_mod = 0.25f;
	if(IS_SET(attacker.data->flags, F2_BACKSTAB))
		backstab_mod += 0.25f;
	if(attacker.HaveWeapon() && IS_SET(attacker.GetWeapon().flags, ITEM_BACKSTAB))
		backstab_mod += 0.25f;
	if(IS_SET(hitted.data->flags2, F2_BACKSTAB_RES))
		backstab_mod /= 2;
	m += angle_dif / PI * backstab_mod;

	// apply modifiers
	float dmg = start_dmg * m;
	float base_dmg = dmg;

	// calculate defense
	float armor_def = hitted.CalculateArmorDefense(),
		dex_def = hitted.CalculateDexterityDefense(),
		base_def = hitted.CalculateBaseDefense();

	// blocking
	if(hitted.action == A_BLOCK && angle_dif < PI / 2)
	{
		// reduce damage
		float block = hitted.CalculateBlock() * hitted.mesh_inst->groups[1].GetBlendT();
		float stamina = min(dmg, block);
		if(IS_SET(attacker.data->flags2, F2_IGNORE_BLOCK))
			block *= 2.f / 3;
		if(attacker.attack_power >= 1.9f)
			stamina *= 4.f / 3;
		dmg -= block;
		hitted.RemoveStaminaBlock(stamina);

		// play sound
		MATERIAL_TYPE hitted_mat = hitted.GetShield().material;
		MATERIAL_TYPE weapon_mat = (!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.ile = hitted_mat;
		}
		sound_mgr->PlaySound3d(GetMaterialSound(weapon_mat, hitted_mat), hitpoint, 2.f, 10.f);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, base_dmg / hitted.hpmax, attacker.level);

		// pain animation & break blocking
		if(hitted.stamina < 0)
		{
			BreakUnitAction(hitted);

			if(!IS_SET(hitted.data->flags, F_DONT_SUFFER))
			{
				if(hitted.action != A_POSITION)
					hitted.action = A_PAIN;
				else
					hitted.animation_state = 1;

				if(hitted.mesh_inst->mesh->head.n_groups == 2)
				{
					hitted.mesh_inst->frame_end_info2 = false;
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
				}
				else
				{
					hitted.mesh_inst->frame_end_info = false;
					hitted.mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
					hitted.mesh_inst->groups[0].speed = 1.f;
					hitted.animation = ANI_PLAY;
				}
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
		base_def += hitted.CalculateBlock() / 20;
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
			case SkillId::LIGHT_ARMOR:
				armor_def *= 0.25f;
				break;
			case SkillId::MEDIUM_ARMOR:
				armor_def *= 0.5f;
				break;
			case SkillId::HEAVY_ARMOR:
				armor_def *= 0.75f;
				break;
			}
		}
		clean_hit = true;
	}

	// armor defense
	dmg -= (armor_def + dex_def + base_def);

	// hit sound
	PlayHitSound(!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material, hitted.GetBodyMaterial(), hitpoint, 2.f, dmg > 0.f);

	// train player armor skill
	if(hitted.IsPlayer())
		hitted.player->Train(TrainWhat::TakeDamageArmor, base_dmg / hitted.hpmax, attacker.level);

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
			ratio = max(TRAIN_KILL_RATIO, dmgf / hitted.hpmax);
		else
			ratio = dmgf / hitted.hpmax;
		attacker.player->Train(bash ? TrainWhat::BashHit : TrainWhat::AttackHit, ratio, hitted.level);
	}

	GiveDmg(ctx, &attacker, dmg, hitted, &hitpoint);

	// apply poison
	if(IS_SET(attacker.data->flags, F_POISON_ATTACK) && !IS_SET(hitted.data->flags, F_POISON_RES))
	{
		Effect& e = Add1(hitted.effects);
		e.power = dmg / 10;
		e.time = 5.f;
		e.effect = EffectId::Poison;
	}

	return clean_hit ? ATTACK_CLEAN_HIT : ATTACK_HIT;
}

void Game::CastSpell(LevelContext& ctx, Unit& u)
{
	if(u.player)
		return;

	Spell& spell = *u.data->spells->spell[u.attack_id];

	Mesh::Point* point = u.mesh_inst->mesh->GetPoint(NAMES::point_cast);
	assert(point);

	if(u.human_data)
		u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.mesh_inst->SetupBones();

	m2 = point->mat * u.mesh_inst->mat_bones[point->bone] * (Matrix::RotationY(u.rot) * Matrix::Translation(u.pos));

	Vec3 coord = Vec3::TransformZero(m2);

	if(spell.type == Spell::Ball || spell.type == Spell::Point)
	{
		int ile = 1;
		if(IS_SET(spell.flags, Spell::Triple))
			ile = 3;

		for(int i = 0; i < ile; ++i)
		{
			Bullet& b = Add1(ctx.bullets);

			b.level = u.level + u.CalculateMagicPower();
			b.backstab = 0;
			b.pos = coord;
			b.attack = float(spell.dmg);
			b.rot = Vec3(0, Clip(u.rot + PI + Random(-0.05f, 0.05f)), 0);
			b.mesh = spell.mesh;
			b.tex = spell.tex;
			b.tex_size = spell.size;
			b.speed = spell.speed;
			b.timer = spell.range / (spell.speed - 1);
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
				float dist = Vec3::Distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + Random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h / t + (10.f*t) / 2;
			}
			else if(spell.type == Spell::Point)
			{
				float dist = Vec3::Distance2d(u.pos, u.target_pos);
				float t = dist / spell.speed;
				float h = (u.target_pos.y + Random(-0.5f, 0.5f)) - b.pos.y;
				b.yspeed = h / t;
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
				pe->speed_min = Vec3(-1, -1, -1);
				pe->speed_max = Vec3(1, 1, 1);
				pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
				pe->pos_max = Vec3(spell.size, spell.size, spell.size);
				pe->size = spell.size_particle;
				pe->op_size = POP_LINEAR_SHRINK;
				pe->alpha = 1.f;
				pe->op_alpha = POP_LINEAR_SHRINK;
				pe->mode = 1;
				pe->Init();
				ctx.pes->push_back(pe);
				b.pe = pe;
			}

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
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

			Vec3 hitpoint;
			Unit* hitted;

			u.target_pos.y += Random(-0.5f, 0.5f);
			Vec3 dir = u.target_pos - coord;
			dir.Normalize();
			Vec3 target = coord + dir * spell.range;

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

			if(Net::IsOnline())
			{
				e->netid = Electro::netid_counter++;

				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CREATE_ELECTRO;
				c.e_id = e->netid;
				c.pos = e->lines[0].pts.front();
				memcpy(c.f, &e->lines[0].pts.back(), sizeof(Vec3));
			}
		}
		else if(IS_SET(spell.flags, Spell::Drain))
		{
			Vec3 hitpoint;
			Unit* hitted;

			u.target_pos.y += Random(-0.5f, 0.5f);

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
					drain.pe->speed_min = Vec3(-3, 0, -3);
					drain.pe->speed_max = Vec3(3, 3, 3);

					u.hp += float(spell.dmg);
					if(u.hp > u.hpmax)
						u.hp = u.hpmax;

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::UPDATE_HP;
						c.unit = drain.to;

						NetChange& c2 = Add1(Net::changes);
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
				if((*it)->live_state == Unit::DEAD
					&& !IsEnemy(u, **it)
					&& IS_SET((*it)->data->flags, F_UNDEAD)
					&& Vec3::Distance(u.target_pos, (*it)->pos) < 0.5f)
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
					pe->pos.y += u2.GetUnitHeight() / 2;
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::RAISE_EFFECT;
						c.pos = pe->pos;

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::STAND_UP;
						c2.unit = &u2;

						NetChange& c3 = Add1(Net::changes);
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
				if(!IsEnemy(u, **it) && !IS_SET((*it)->data->flags, F_UNDEAD) && Vec3::Distance(u.target_pos, (*it)->pos) < 0.5f)
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
					pe->pos.y += u2.GetUnitHeight() / 2;
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					ctx.pes->push_back(pe);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HEAL_EFFECT;
						c.pos = pe->pos;

						NetChange& c3 = Add1(Net::changes);
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
		sound_mgr->PlaySound3d(spell.sound_cast, coord, spell.sound_cast_dist.x, spell.sound_cast_dist.y);
		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPELL_SOUND;
			c.spell = &spell;
			c.pos = coord;
		}
	}
}

void Game::SpellHitEffect(LevelContext& ctx, Bullet& bullet, const Vec3& pos, Unit* hitted)
{
	Spell& spell = *bullet.spell;

	// dŸwiêk
	if(spell.sound_hit)
		sound_mgr->PlaySound3d(spell.sound_hit, pos, spell.sound_hit_dist.x, spell.sound_hit_dist.y);

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
		pe->pos = pos;
		pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
		pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
		pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
		pe->pos_max = Vec3(spell.size, spell.size, spell.size);
		pe->size = spell.size / 2;
		pe->op_size = POP_LINEAR_SHRINK;
		pe->alpha = 1.f;
		pe->op_alpha = POP_LINEAR_SHRINK;
		pe->mode = 1;
		pe->Init();
		//pe->gravity = true;
		ctx.pes->push_back(pe);
	}

	// wybuch
	if(Net::IsLocal() && spell.tex_explode && IS_SET(spell.flags, Spell::Explode))
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

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
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

		float dmg = e.dmg * Lerp(1.f, 0.1f, e.size / e.sizemax);

		if(Net::IsLocal())
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
					Box box;
					(*it2)->GetBox(box);

					if(SphereToBox(e.pos, e.size, box))
					{
						// zadaj obra¿enia
						GiveDmg(ctx, e.owner, dmg, **it2, nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
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
		if(index == ctx.explos->size() - 1)
			ctx.explos->pop_back();
		else
		{
			std::iter_swap(ctx.explos->begin() + index, ctx.explos->end() - 1);
			ctx.explos->pop_back();
		}
	}
}

void Game::UpdateTraps(LevelContext& ctx, float dt)
{
	const bool is_local = Net::IsLocal();
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
				// check if someone is step on it
				bool trigger = false;
				if(is_local)
				{
					for(Unit* unit : *ctx.units)
					{
						if(unit->IsStanding() && !IS_SET(unit->data->flags, F_SLIGHT)
							&& CircleToCircle(trap.pos.x, trap.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
						{
							trigger = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					trigger = true;
					trap.trigger = false;
				}

				if(trigger)
				{
					sound_mgr->PlaySound3d(trap.base->sound->sound, trap.pos, 1.f, 4.f);
					trap.state = 1;
					trap.time = Random(0.5f, 0.75f);

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			else if(trap.state == 1)
			{
				// count timer to spears come out
				bool trigger = false;
				if(is_local)
				{
					trap.time -= dt;
					if(trap.time <= 0.f)
						trigger = true;
				}
				else if(trap.trigger)
				{
					trigger = true;
					trap.trigger = false;
				}

				if(trigger)
				{
					trap.state = 2;
					trap.time = 0.f;

					sound_mgr->PlaySound3d(trap.base->sound2->sound, trap.pos, 2.f, 8.f);

					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			else if(trap.state == 2)
			{
				// wychodzenie w³óczni
				bool end = false;
				trap.time += dt;
				if(trap.time >= 0.27f)
				{
					trap.time = 0.27f;
					end = true;
				}

				trap.obj2.pos.y = trap.obj.pos.y - 2.f + 2.f*(trap.time / 0.27f);

				if(is_local)
				{
					for(Unit* unit : *ctx.units)
					{
						if(!unit->IsAlive())
							continue;
						if(CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
						{
							bool found = false;
							for(Unit* unit2 : *trap.hitted)
							{
								if(unit == unit2)
								{
									found = true;
									break;
								}
							}

							if(!found)
							{
								// hit unit with spears
								float dmg = float(trap.base->dmg),
									def = unit->CalculateDefense();

								int mod = ObliczModyfikator(DMG_PIERCE, unit->data->flags);
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
								sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, unit->GetBodyMaterial()), unit->pos + Vec3(0, 1.f, 0), 2.f, 8.f);

								// train player armor skill
								if(unit->IsPlayer())
									unit->player->Train(TrainWhat::TakeDamageArmor, base_dmg / unit->hpmax, 4);

								// obra¿enia
								if(dmg > 0)
									GiveDmg(ctx, nullptr, dmg, *unit);

								trap.hitted->push_back(unit);
							}
						}
					}
				}

				if(end)
				{
					trap.state = 3;
					if(is_local)
						trap.hitted->clear();
					trap.time = 1.f;
				}
			}
			else if(trap.state == 3)
			{
				// count timer to hide spears
				trap.time -= dt;
				if(trap.time <= 0.f)
				{
					trap.state = 4;
					trap.time = 1.5f;
					sound_mgr->PlaySound3d(trap.base->sound3->sound, trap.pos, 1.f, 4.f);
				}
			}
			else if(trap.state == 4)
			{
				// hiding spears
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
				// spears are hidden, wait until unit moves away to reactivate
				bool reactivate;
				if(is_local)
				{
					reactivate = true;
					for(Unit* unit : *ctx.units)
					{
						if(!IS_SET(unit->data->flags, F_SLIGHT)
							&& CircleToCircle(trap.obj2.pos.x, trap.obj2.pos.z, trap.base->rw, unit->pos.x, unit->pos.z, unit->GetUnitRadius()))
						{
							reactivate = false;
							break;
						}
					}
				}
				else
				{
					if(trap.trigger)
					{
						reactivate = true;
						trap.trigger = false;
					}
					else
						reactivate = false;
				}

				if(reactivate)
				{
					trap.state = 0;
					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::TRIGGER_TRAP;
						c.id = trap.netid;
					}
				}
			}
			break;
		case TRAP_ARROW:
		case TRAP_POISON:
			if(trap.state == 0)
			{
				// check if someone is step on it
				bool trigger = false;
				if(is_local)
				{
					for(Unit* unit : *ctx.units)
					{
						if(unit->IsStanding() && !IS_SET(unit->data->flags, F_SLIGHT) &&
							CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							trigger = true;
							break;
						}
					}
				}
				else if(trap.trigger)
				{
					trigger = true;
					trap.trigger = false;
				}

				if(trigger)
				{
					// someone step on trap, shoot arrow
					trap.state = is_local ? 1 : 2;
					trap.time = Random(5.f, 7.5f);
					sound_mgr->PlaySound3d(trap.base->sound->sound, trap.pos, 1.f, 4.f);

					if(is_local)
					{
						Bullet& b = Add1(ctx.bullets);
						b.level = 4;
						b.backstab = 0;
						b.attack = float(trap.base->dmg);
						b.mesh = aArrow;
						b.pos = Vec3(2.f*trap.tile.x + trap.pos.x - float(int(trap.pos.x / 2) * 2) + Random(-trap.base->rw, trap.base->rw) - 1.2f*g_kierunek2[trap.dir].x,
							Random(0.5f, 1.5f),
							2.f*trap.tile.y + trap.pos.z - float(int(trap.pos.z / 2) * 2) + Random(-trap.base->h, trap.base->h) - 1.2f*g_kierunek2[trap.dir].y);
						b.start_pos = b.pos;
						b.rot = Vec3(0, dir_to_rot(trap.dir), 0);
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
						tpe->color1 = Vec4(1, 1, 1, 0.5f);
						tpe->color2 = Vec4(1, 1, 1, 0);
						tpe->Init(50);
						ctx.tpes->push_back(tpe);
						b.trail = tpe;

						TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
						tpe2->fade = 0.3f;
						tpe2->color1 = Vec4(1, 1, 1, 0.5f);
						tpe2->color2 = Vec4(1, 1, 1, 0);
						tpe2->Init(50);
						ctx.tpes->push_back(tpe2);
						b.trail2 = tpe2;

						sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, 2.f, 8.f);

						if(Net::IsServer())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::SHOOT_ARROW;
							c.unit = nullptr;
							c.pos = b.start_pos;
							c.f[0] = b.rot.y;
							c.f[1] = 0.f;
							c.f[2] = 0.f;
							c.extra_f = b.speed;

							NetChange& c2 = Add1(Net::changes);
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
				// check if units leave trap
				bool empty;
				if(is_local)
				{
					empty = true;
					for(Unit* unit : *ctx.units)
					{
						if(!IS_SET(unit->data->flags, F_SLIGHT) &&
							CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
						{
							empty = false;
							break;
						}
					}
				}
				else
				{
					if(trap.trigger)
					{
						empty = true;
						trap.trigger = false;
					}
					else
						empty = false;
				}

				if(empty)
				{
					trap.state = 0;
					if(Net::IsServer())
					{
						NetChange& c = Add1(Net::changes);
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

				bool trigger = false;
				for(Unit* unit : *ctx.units)
				{
					if(unit->IsStanding()
						&& CircleToRectangle(unit->pos.x, unit->pos.z, unit->GetUnitRadius(), trap.pos.x, trap.pos.z, trap.base->rw, trap.base->h))
					{
						trigger = true;
						break;
					}
				}

				if(trigger)
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

					sound_mgr->PlaySound3d(fireball->sound_hit, explo->pos, fireball->sound_hit_dist.x, fireball->sound_hit_dist.y);

					ctx.explos->push_back(explo);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CREATE_EXPLOSION;
						c.spell = fireball;
						c.pos = explo->pos;

						NetChange& c2 = Add1(Net::changes);
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
		if(index == ctx.traps->size() - 1)
			ctx.traps->pop_back();
		else
		{
			std::iter_swap(ctx.traps->begin() + index, ctx.traps->end() - 1);
			ctx.traps->pop_back();
		}
	}
}

struct TrapLocation
{
	Int2 pt;
	int dist, dir;
};

Trap* Game::CreateTrap(Int2 pt, TRAP_TYPE type, bool timed)
{
	Trap* t = new Trap;
	Trap& trap = *t;
	L.local_ctx.traps->push_back(t);

	auto& base = BaseTrap::traps[type];
	trap.base = &base;
	trap.hitted = nullptr;
	trap.state = 0;
	trap.pos = Vec3(2.f*pt.x + Random(trap.base->rw, 2.f - trap.base->rw), 0.f, 2.f*pt.y + Random(trap.base->h, 2.f - trap.base->h));
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.netid = Trap::netid_counter++;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		trap.obj.rot = Vec3(0, 0, 0);

		static vector<TrapLocation> possible;

		InsideLocationLevel& lvl = ((InsideLocation*)L.location)->GetLevelData();

		// ustal tile i dir
		for(int i = 0; i < 4; ++i)
		{
			bool ok = false;
			int j;

			for(j = 1; j <= 10; ++j)
			{
				if(czy_blokuje2(lvl.map[pt.x + g_kierunek2[i].x*j + (pt.y + g_kierunek2[i].y*j)*lvl.w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + g_kierunek2[i] * j;

				if(CanShootAtLocation(Vec3(trap.pos.x + (2.f*j - 1.2f)*g_kierunek2[i].x, 1.f, trap.pos.z + (2.f*j - 1.2f)*g_kierunek2[i].y),
					Vec3(trap.pos.x, 1.f, trap.pos.z)))
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
			{
				std::sort(possible.begin(), possible.end(), [](TrapLocation& pt1, TrapLocation& pt2)
				{
					return abs(pt1.dist - 5) < abs(pt2.dist - 5);
				});
			}

			trap.tile = possible[0].pt;
			trap.dir = possible[0].dir;

			possible.clear();
		}
		else
		{
			L.local_ctx.traps->pop_back();
			delete t;
			return nullptr;
		}
	}
	else if(type == TRAP_SPEAR)
	{
		trap.obj.rot = Vec3(0, Random(MAX_ANGLE), 0);
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
		trap.obj.rot = Vec3(0, PI / 2 * (Rand() % 4), 0);
		trap.obj.base = &obj_alpha;
	}

	if(timed)
	{
		trap.state = -1;
		trap.time = 2.f;
	}

	return &trap;
}

void Game::PreloadTraps(vector<Trap*>& traps)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& sound_mgr = ResourceManager::Get<Sound>();

	for(Trap* trap : traps)
	{
		auto& base = *trap->base;
		if(base.state != ResourceState::NotLoaded)
			continue;

		if(base.mesh)
			mesh_mgr.AddLoadTask(base.mesh);
		if(base.mesh2)
			mesh_mgr.AddLoadTask(base.mesh2);
		if(base.sound)
			sound_mgr.AddLoadTask(base.sound);
		if(base.sound2)
			sound_mgr.AddLoadTask(base.sound2);
		if(base.sound3)
			sound_mgr.AddLoadTask(base.sound3);

		base.state = ResourceState::Loaded;
	}
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

bool Game::RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted)
{
	BulletRaytestCallback3 callback(ignore);
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hit)
	{
		hitpoint = from + (to - from) * callback.fraction;
		hitted = callback.hitted;
		return true;
	}
	else
		return false;

}

struct ConvexCallback : public btCollisionWorld::ConvexResultCallback
{
	delegate<Game::LINE_TEST_RESULT(btCollisionObject*, bool)> clbk;
	vector<float>* t_list;
	float end_t, closest;
	bool use_clbk2, end = false;

	ConvexCallback(delegate<Game::LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, vector<float>* t_list, bool use_clbk2) : clbk(clbk), t_list(t_list), use_clbk2(use_clbk2),
		end(false), end_t(1.f), closest(1.1f)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk((btCollisionObject*)proxy0->m_clientObject, false) == Game::LT_COLLIDE;
	}

	float addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace)
	{
		if(use_clbk2)
		{
			auto result = clbk((btCollisionObject*)convexResult.m_hitCollisionObject, true);
			if(result == Game::LT_IGNORE)
				return m_closestHitFraction;
			else if(result == Game::LT_END)
			{
				if(end_t > convexResult.m_hitFraction)
				{
					closest = min(closest, convexResult.m_hitFraction);
					m_closestHitFraction = convexResult.m_hitFraction;
					end_t = convexResult.m_hitFraction;
				}
				end = true;
				return 1.f;
			}
			else if(end && convexResult.m_hitFraction > end_t)
				return 1.f;
		}
		closest = min(closest, convexResult.m_hitFraction);
		if(t_list)
			t_list->push_back(convexResult.m_hitFraction);
		// return value is unused
		return 1.f;
	}
};

bool Game::LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
	vector<float>* t_list, bool use_clbk2, float* end_t)
{
	assert(shape->isConvex());

	btTransform t_from, t_to;
	t_from.setIdentity();
	t_from.setOrigin(ToVector3(from));
	//t_from.getBasis().setRotation(ToQuaternion(Quat::CreateFromYawPitchRoll(rot, 0, 0)));
	t_to.setIdentity();
	t_to.setOrigin(ToVector3(dir) + t_from.getOrigin());
	//t_to.setBasis(t_from.getBasis());

	ConvexCallback callback(clbk, t_list, use_clbk2);

	phy_world->convexSweepTest((btConvexShape*)shape, t_from, t_to, callback);

	bool has_hit = (callback.closest <= 1.f);
	t = min(callback.closest, 1.f);
	if(end_t)
	{
		if(callback.end)
			*end_t = callback.end_t;
		else
			*end_t = 1.f;
	}
	return has_hit;
}

struct ContactTestCallback : public btCollisionWorld::ContactResultCallback
{
	btCollisionObject* obj;
	delegate<bool(btCollisionObject*, bool)> clbk;
	bool use_clbk2, hit;

	ContactTestCallback(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2) : obj(obj), clbk(clbk), use_clbk2(use_clbk2), hit(false)
	{
	}

	bool needsCollision(btBroadphaseProxy* proxy0) const override
	{
		return clbk((btCollisionObject*)proxy0->m_clientObject, false);
	}

	float addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap,
		int partId1, int index1)
	{
		if(use_clbk2)
		{
			const btCollisionObject* cobj;
			if(colObj0Wrap->m_collisionObject == obj)
				cobj = colObj1Wrap->m_collisionObject;
			else
				cobj = colObj0Wrap->m_collisionObject;
			if(!clbk((btCollisionObject*)cobj, true))
				return 1.f;
		}
		hit = true;
		return 0.f;
	}
};

bool Game::ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2)
{
	ContactTestCallback callback(obj, clbk, use_clbk2);
	phy_world->contactTest(obj, callback);
	return callback.hit;
}

void Game::UpdateElectros(LevelContext& ctx, float dt)
{
	uint index = 0;
	for(vector<Electro*>::iterator it = ctx.electros->begin(), end = ctx.electros->end(); it != end; ++it, ++index)
	{
		Electro& e = **it;

		for(vector<ElectroLine>::iterator it2 = e.lines.begin(), end2 = e.lines.end(); it2 != end2; ++it2)
			it2->t += dt;

		if(!Net::IsLocal())
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
					GiveDmg(ctx, e.owner, e.dmg, *e.hitted.back(), nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
				}

				if(e.spell->sound_hit)
					sound_mgr->PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist.x, e.spell->sound_hit_dist.y);

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
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = Vec3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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

						float dist = Vec3::Distance((*it2)->pos, e.hitted.back()->pos);
						if(dist <= 5.f)
							targets.push_back(std::pair<Unit*, float>(*it2, dist));
					}

					if(!targets.empty())
					{
						if(targets.size() > 1)
						{
							std::sort(targets.begin(), targets.end(), [](const std::pair<Unit*, float>& target1, const std::pair<Unit*, float>& target2)
							{
								return target1.second < target2.second;
							});
						}

						Unit* target = nullptr;
						float dist;

						for(vector<std::pair<Unit*, float> >::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
						{
							Vec3 hitpoint;
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
							e.dmg = min(e.dmg / 2, Lerp(e.dmg, e.dmg / 5, dist / 5));
							e.valid = true;
							e.hitted.push_back(target);
							Vec3 from = e.lines.back().pts.back();
							Vec3 to = target->GetCenter();
							e.AddLine(from, to);

							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
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

				if(e.spell->sound_hit)
					sound_mgr->PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist.x, e.spell->sound_hit_dist.y);

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
					pe->speed_min = Vec3(-1.5f, -1.5f, -1.5f);
					pe->speed_max = Vec3(1.5f, 1.5f, 1.5f);
					pe->pos_min = Vec3(-e.spell->size, -e.spell->size, -e.spell->size);
					pe->pos_max = Vec3(e.spell->size, e.spell->size, e.spell->size);
					pe->size = e.spell->size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					//pe->gravity = true;
					ctx.pes->push_back(pe);
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
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
		if(index == ctx.electros->size() - 1)
			ctx.electros->pop_back();
		else
		{
			std::iter_swap(ctx.electros->begin() + index, ctx.electros->end() - 1);
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
		Vec3 center = d.to->GetCenter();

		if(d.pe->manual_delete == 2)
		{
			delete d.pe;
			_to_remove.push_back(index);
		}
		else
		{
			for(vector<Particle>::iterator it2 = d.pe->particles.begin(), end2 = d.pe->particles.end(); it2 != end2; ++it2)
				it2->pos = Vec3::Lerp(it2->pos, center, d.t / 1.5f);
		}
	}

	// usuñ
	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == ctx.drains->size() - 1)
			ctx.drains->pop_back();
		else
		{
			std::iter_swap(ctx.drains->begin() + index, ctx.drains->end() - 1);
			ctx.drains->pop_back();
		}
	}
}

void Game::UpdateAttachedSounds(float dt)
{
	uint index = 0;
	for(vector<AttachedSound>::iterator it = attached_sounds.begin(), end = attached_sounds.end(); it != end; ++it, ++index)
	{
		if(it->unit)
		{
			if(it->unit->to_remove)
			{
				it->unit = nullptr;
				if(!sound_mgr->IsPlaying(it->channel))
					_to_remove.push_back(index);
			}
			else if(!sound_mgr->UpdateChannelPosition(it->channel, it->unit->GetHeadSoundPos()))
			{
				it->unit = nullptr;
				_to_remove.push_back(index);
			}
		}
		else
		{
			if(!sound_mgr->IsPlaying(it->channel))
				_to_remove.push_back(index);
		}
	}

	while(!_to_remove.empty())
	{
		index = _to_remove.back();
		_to_remove.pop_back();
		if(index == attached_sounds.size() - 1)
			attached_sounds.pop_back();
		else
		{
			std::iter_swap(attached_sounds.begin() + index, attached_sounds.end() - 1);
			attached_sounds.pop_back();
		}
	}
}

vector<Unit*> Unit::refid_table;
vector<std::pair<Unit**, int> > Unit::refid_request;
vector<ParticleEmitter*> ParticleEmitter::refid_table;
vector<TrailParticleEmitter*> TrailParticleEmitter::refid_table;
vector<Usable*> Usable::refid_table;
vector<UsableRequest> Usable::refid_request;

void Game::BuildRefidTables()
{
	// jednostki i u¿ywalne
	Unit::refid_table.clear();
	Usable::refid_table.clear();
	for(Location* loc : W.GetLocations())
	{
		if(loc)
			loc->BuildRefidTables();
	}

	// cz¹steczki
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();
	if(L.is_open)
	{
		for(LevelContext& ctx : L.ForEachContext())
			ctx.BuildRefidTables();
	}
}

void Game::ClearGameVarsOnNewGameOrLoad()
{
	inventory_mode = I_NONE;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	death_screen = 0;
	koniec_gry = false;
	minimap_reveal.clear();
	game_gui->minimap->city = nullptr;
	team_shares.clear();
	team_share_id = -1;
	draw_flags = 0xFFFFFFFF;
	unit_views.clear();
	game_gui->journal->Reset();
	arena_viewers.clear();
	debug_info = false;
	debug_info2 = false;
	dialog_enc = nullptr;
	dialog_pvp = nullptr;
	game_gui->visible = false;
	Inventory::lock = nullptr;
	world_map->picked_location = -1;
	post_effects.clear();
	grayout = 0.f;
	cam.Reset();
	lights_dt = 1.f;
	pc_data.Reset();
	SM.Reset();
	L.Reset();

#ifdef DRAW_LOCAL_PATH
	marked = nullptr;
#endif

	// odciemnianie na pocz¹tku
	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;
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
	cam.real_rot = Vec2(0, 4.2875104f);
	cam.dist = 3.5f;
	game_speed = 1.f;
	L.dungeon_level = 0;
	QM.Reset();
	W.OnNewGame();
	GameStats::Get().Reset();
	Team.Reset();
	dont_wander = false;
	arena_fighter = nullptr;
	pc_data.picking_item_state = 0;
	arena_tryb = Arena_Brak;
	L.is_open = false;
	arena_free = true;
	game_gui->PositionPanels();
	ClearGui(true);
	game_gui->mp_box->visible = Net::IsOnline();
	drunk_anim = 0.f;
	L.light_angle = Random(PI * 2);
	cam.Reset();
	pc_data.rot_buf = 0.f;
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

// czyszczenie gry
void Game::ClearGame()
{
	Info("Clearing game.");

	draw_batch.Clear();

	LeaveLocation(true, false);

	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP) && !L.is_open && Net::IsLocal() && !was_client)
	{
		for(Unit* unit : Team.members)
		{
			if(unit->bow_instance)
				bow_instances.push_back(unit->bow_instance);
			if(unit->interp)
				EntityInterpolator::Pool.Free(unit->interp);

			delete unit->ai;
			delete unit;
		}

		prev_game_state = GS_LOAD;
	}

	if(!net_talk.empty())
		StringPool.Free(net_talk);

	// usuñ lokalizacje
	L.is_open = false;
	L.city_ctx = nullptr;

	// usuñ quest
	QM.Cleanup();
	DeleteElements(quest_items);

	W.Reset();

	ClearGui(true);
}

cstring Game::FormatString(DialogContext& ctx, const string& str_part)
{
	cstring result;
	if(QM.HandleFormatString(str_part, result))
		return result;

	if(str_part == "burmistrzem")
		return LocationHelper::IsCity(L.location) ? "burmistrzem" : "so³tysem";
	else if(str_part == "mayor")
		return LocationHelper::IsCity(L.location) ? "mayor" : "soltys";
	else if(str_part == "rcitynhere")
		return W.GetRandomSettlement(L.location_index)->name.c_str();
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
		return Format("%d", ctx.team_share_item->value / 2);
	}
	else if(str_part == "player_name")
		return current_dialog->pc->name.c_str();
	else if(str_part == "rhero")
	{
		static string str;
		GenerateHeroName(ClassInfo::GetRandom(), Rand() % 4 == 0, str);
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

void Game::AddGameMsg(cstring msg, float time)
{
	game_gui->game_messages->AddMessage(msg, time, 0);
}

void Game::AddGameMsg2(cstring msg, float time, int id)
{
	game_gui->game_messages->AddMessageIfNotExists(msg, time, id);
}

void Game::UpdateDungeonMinimap(bool send)
{
	if(minimap_reveal.empty())
		return;

	TextureLock lock(tMinimap);
	InsideLocationLevel& lvl = ((InsideLocation*)L.location)->GetLevelData();

	for(vector<Int2>::iterator it = minimap_reveal.begin(), end = minimap_reveal.end(); it != end; ++it)
	{
		Pole& p = lvl.map[it->x + (lvl.w - it->y - 1)*lvl.w];
		SET_BIT(p.flags, Pole::F_ODKRYTE);
		uint* pix = lock[it->y] + it->x;
		if(OR2_EQ(p.type, SCIANA, BLOKADA_SCIANA))
			*pix = Color(100, 100, 100);
		else if(p.type == DRZWI)
			*pix = Color(127, 51, 0);
		else
			*pix = Color(220, 220, 240);
	}

	if(Net::IsLocal())
	{
		if(send)
		{
			if(Net::IsOnline())
				minimap_reveal_mp.insert(minimap_reveal_mp.end(), minimap_reveal.begin(), minimap_reveal.end());
		}
		else
			minimap_reveal_mp.clear();
	}

	minimap_reveal.clear();
}

Door* Game::FindDoor(LevelContext& ctx, const Int2& pt)
{
	for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
	{
		if((*it)->pt == pt)
			return *it;
	}

	return nullptr;
}

void Game::AddGroundItem(LevelContext& ctx, GroundItem* item)
{
	assert(item);

	if(ctx.type == LevelContext::Outside)
		terrain->SetH(item->pos);
	ctx.items->push_back(item);

	if(Net::IsOnline())
	{
		item->netid = GroundItem::netid_counter++;
		NetChange& c = Add1(Net::changes);
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
		if(item->ToArmor().skill != SkillId::LIGHT_ARMOR)
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
		if(L.location->outside)
			return L.location->name.c_str();
		else
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
				return Format(txLocationText, L.location->name.c_str(), L.dungeon_level + 1);
			else
				return L.location->name.c_str();
		}
	}
	else
		return Format(txLocationTextMap, W.GetCurrentLocation()->name.c_str());
}

void Game::Unit_StopUsingUsable(LevelContext& ctx, Unit& u, bool send)
{
	u.animation = ANI_STAND;
	u.animation_state = AS_ANIMATION2_MOVE_TO_ENDPOINT;
	u.timer = 0.f;
	u.used_item = nullptr;

	const float unit_radius = u.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &u, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, u.pos, 2.f + unit_radius, &ignore);

	Vec3 tmp_pos = u.target_pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			if(i != 0 && ctx.have_terrain)
				tmp_pos.y = terrain->GetH(tmp_pos);
			u.target_pos = tmp_pos;
			ok = true;
			break;
		}

		tmp_pos = u.target_pos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

		if(i < 10)
			radius += 0.2f;
	}

	assert(ok);

	if(u.cobj)
		UpdateUnitPhysics(u, u.target_pos);

	if(send && Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.unit = &u;
		c.id = u.usable->netid;
		c.ile = USE_USABLE_STOP;
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

			chest.mesh_inst = new MeshInstance(aChest);
		}
	}

	// odtwórz drzwi
	if(ctx.doors)
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;

			// animowany model
			door.mesh_inst = new MeshInstance(door.door2 ? aDoor2 : aDoor);
			door.mesh_inst->groups[0].speed = 2.f;

			// fizyka
			door.phy = new btCollisionObject;
			door.phy->setCollisionShape(shape_door);
			door.phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
			btTransform& tr = door.phy->getWorldTransform();
			Vec3 pos = door.pos;
			pos.y += 1.319f;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door.rot, 0, 0));
			phy_world->addCollisionObject(door.phy, CG_DOOR);

			// czy otwarte
			if(door.state == Door::Open)
			{
				btVector3& pos = door.phy->getWorldTransform().getOrigin();
				pos.setY(pos.y() - 100.f);
				door.mesh_inst->SetToEnd(door.mesh_inst->mesh->anims[0].name.c_str());
			}
		}
	}
}

void ApplyTexturePackToSubmesh(Mesh::Submesh& sub, TexturePack& tp)
{
	sub.tex = tp.diffuse;
	sub.tex_normal = tp.normal;
	sub.tex_specular = tp.specular;
}

void ApplyDungeonLightToMesh(Mesh& mesh)
{
	for(int i = 0; i < mesh.head.n_subs; ++i)
	{
		mesh.subs[i].specular_color = Vec3(1, 1, 1);
		mesh.subs[i].specular_intensity = 0.2f;
		mesh.subs[i].specular_hardness = 10;
	}
}

void Game::ApplyLocationTexturePack(TexturePack& floor, TexturePack& wall, TexturePack& ceil, LocationTexturePack& tex)
{
	ApplyLocationTexturePack(floor, tex.floor, tFloorBase);
	ApplyLocationTexturePack(wall, tex.wall, tWallBase);
	ApplyLocationTexturePack(ceil, tex.ceil, tCeilBase);
}

void Game::ApplyLocationTexturePack(TexturePack& pack, LocationTexturePack::Entry& e, TexturePack& pack_def)
{
	if(e.tex)
	{
		pack.diffuse = e.tex;
		pack.normal = e.tex_normal;
		pack.specular = e.tex_specular;
	}
	else
		pack = pack_def;

	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask(pack.diffuse);
	if(pack.normal)
		tex_mgr.AddLoadTask(pack.normal);
	if(pack.specular)
		tex_mgr.AddLoadTask(pack.specular);
}

void Game::SetDungeonParamsAndTextures(BaseLocation& base)
{
	// ustawienia t³a
	cam.draw_range = base.draw_range;
	fog_params = Vec4(base.fog_range.x, base.fog_range.y, base.fog_range.y - base.fog_range.x, 0);
	fog_color = Vec4(base.fog_color, 1);
	ambient_color = Vec4(base.ambient_color, 1);
	clear_color2 = Color(int(fog_color.x * 255), int(fog_color.y * 255), int(fog_color.z * 255));

	// tekstury podziemi
	ApplyLocationTexturePack(tFloor[0], tWall[0], tCeil[0], base.tex);

	// druga tekstura
	if(base.tex2 != -1)
	{
		BaseLocation& base2 = g_base_locations[base.tex2];
		ApplyLocationTexturePack(tFloor[1], tWall[1], tCeil[1], base2.tex);
	}
	else
	{
		tFloor[1] = tFloor[0];
		tCeil[1] = tCeil[0];
		tWall[1] = tWall[0];
	}

	// ustawienia uv podziemi
	bool new_tex_wrap = !IS_SET(base.options, BLO_NO_TEX_WRAP);
	if(new_tex_wrap != dungeon_tex_wrap)
	{
		dungeon_tex_wrap = new_tex_wrap;
		ChangeDungeonTexWrap();
	}
}

void Game::SetDungeonParamsToMeshes()
{
	// tekstury schodów / pu³apek
	ApplyTexturePackToSubmesh(aStairsDown->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsDown->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aStairsDown2->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsDown2->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aStairsUp->subs[0], tFloor[0]);
	ApplyTexturePackToSubmesh(aStairsUp->subs[2], tWall[0]);
	ApplyTexturePackToSubmesh(aDoorWall->subs[0], tWall[0]);
	ApplyDungeonLightToMesh(*aStairsDown);
	ApplyDungeonLightToMesh(*aStairsDown2);
	ApplyDungeonLightToMesh(*aStairsUp);
	ApplyDungeonLightToMesh(*aDoorWall);
	ApplyDungeonLightToMesh(*aDoorWall2);

	// apply texture/lighting to trap to make it same texture as dungeon
	if(BaseTrap::traps[TRAP_ARROW].mesh->state == ResourceState::Loaded)
	{
		ApplyTexturePackToSubmesh(BaseTrap::traps[TRAP_ARROW].mesh->subs[0], tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_ARROW].mesh);
	}
	if(BaseTrap::traps[TRAP_POISON].mesh->state == ResourceState::Loaded)
	{
		ApplyTexturePackToSubmesh(BaseTrap::traps[TRAP_POISON].mesh->subs[0], tFloor[0]);
		ApplyDungeonLightToMesh(*BaseTrap::traps[TRAP_POISON].mesh);
	}

	// druga tekstura
	ApplyTexturePackToSubmesh(aDoorWall2->subs[0], tWall[1]);
}

void Game::EnterLevel(LocationGenerator* loc_gen)
{
	if(!loc_gen->first)
		Info("Entering location '%s' level %d.", L.location->name.c_str(), L.dungeon_level + 1);

	show_mp_panel = true;
	Inventory::lock = nullptr;

	loc_gen->OnEnter();

	OnEnterLevelOrLocation();
	OnEnterLevel();

	if(!loc_gen->first)
		Info("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	Info("Leaving level.");

	if(game_gui)
		game_gui->Reset();

	for(vector<Unit*>::iterator it = warp_to_inn.begin(), end = warp_to_inn.end(); it != end; ++it)
		WarpToInn(**it);
	warp_to_inn.clear();

	if(L.is_open)
	{
		for(LevelContext& ctx : L.ForEachContext())
		{
			LeaveLevel(ctx, clear);
			if(ctx.require_tmp_ctx)
			{
				L.tmp_ctx_pool.Free(ctx.tmp_ctx);
				ctx.tmp_ctx = nullptr;
			}
		}
		if(L.city_ctx && !Net::IsLocal())
			DeleteElements(L.city_ctx->inside_buildings);
		L.is_open = false;
	}

	ais.clear();
	RemoveColliders();
	unit_views.clear();
	StopAllSounds();
	cam_colliders.clear();

	ClearQuadtree();

	CloseAllPanels();

	cam.Reset();
	pc_data.rot_buf = 0.f;
	pc_data.selected_unit = nullptr;
	dialog_context.dialog_mode = false;
	inventory_mode = I_NONE;
	pc_data.before_player = BP_NONE;
}

void Game::LeaveLevel(LevelContext& ctx, bool clear)
{
	// cleanup units
	if(Net::IsLocal() && !clear)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			Unit& unit = **it;

			// physics
			delete unit.cobj->getCollisionShape();
			unit.cobj = nullptr;

			// speech bubble
			unit.bubble = nullptr;
			unit.talking = false;

			// if using object, end it and move to free space
			if(unit.usable)
			{
				Unit_StopUsingUsable(ctx, **it);
				unit.UseUsable(nullptr);
				unit.visual_pos = unit.pos = unit.target_pos;
			}

			if(unit.bow_instance)
			{
				bow_instances.push_back(unit.bow_instance);
				unit.bow_instance = nullptr;
			}

			// mesh
			if(unit.IsAI())
			{
				if(unit.IsFollower())
				{
					if(!unit.IsAlive())
					{
						unit.hp = 1.f;
						unit.live_state = Unit::ALIVE;
					}
					unit.hero->mode = HeroData::Follow;
					unit.talking = false;
					unit.mesh_inst->need_update = true;
					unit.ai->Reset();
					*it = nullptr;
				}
				else
				{
					if(unit.live_state == Unit::DYING)
					{
						unit.live_state = Unit::DEAD;
						unit.mesh_inst->SetToEnd();
						CreateBlood(ctx, unit, true);
					}
					if(unit.ai->goto_inn && L.city_ctx)
					{
						// warp to inn if unit wanted to go there
						WarpToInn(**it);
					}
					delete unit.mesh_inst;
					unit.mesh_inst = nullptr;
					delete unit.ai;
					unit.ai = nullptr;
					unit.EndEffects();
				}
			}
			else
			{
				unit.talking = false;
				unit.mesh_inst->need_update = true;
				unit.usable = nullptr;
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

			Unit& unit = **it;

			if(unit.cobj)
				delete unit.cobj->getCollisionShape();

			if(unit.bow_instance)
				bow_instances.push_back(unit.bow_instance);

			unit.bubble = nullptr;
			unit.talking = false;

			if(unit.interp)
				EntityInterpolator::Pool.Free(unit.interp);

			delete unit.ai;
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

	if(Net::IsLocal())
	{
		// usuñ modele skrzyni
		if(ctx.chests)
		{
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			{
				delete (*it)->mesh_inst;
				(*it)->mesh_inst = nullptr;
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
				delete door.mesh_inst;
				door.mesh_inst = nullptr;
			}
		}
	}
	else
	{
		// usuñ obiekty
		if(ctx.objects)
			DeleteElements(ctx.objects);
		if(ctx.chests)
			DeleteElements(ctx.chests);
		if(ctx.doors)
			DeleteElements(ctx.doors);
		if(ctx.traps)
			DeleteElements(ctx.traps);
		if(ctx.usables)
			DeleteElements(ctx.usables);
		if(ctx.items)
			DeleteElements(ctx.items);
	}

	if(!clear)
	{
		// maksymalny rozmiar plamy krwi
		for(vector<Blood>::iterator it = ctx.bloods->begin(), end = ctx.bloods->end(); it != end; ++it)
			it->size = 1.f;
	}
}

void Game::CreateBlood(LevelContext& ctx, const Unit& u, bool fully_created)
{
	if(!tKrewSlad[u.data->blood] || IS_SET(u.data->flags2, F2_BLOODLESS))
		return;

	Blood& b = Add1(ctx.bloods);
	if(u.human_data)
		u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.mesh_inst->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = Random(MAX_ANGLE);
	b.size = (fully_created ? 1.f : 0.f);

	if(ctx.have_terrain)
	{
		b.pos.y = terrain->GetH(b.pos) + 0.05f;
		terrain->GetAngle(b.pos.x, b.pos.z, b.normal);
	}
	else
	{
		b.pos.y = u.pos.y + 0.05f;
		b.normal = Vec3(0, 1, 0);
	}
}

void Game::WarpUnit(Unit& unit, const Vec3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	BreakUnitAction(unit, BREAK_ACTION_MODE::INSTANT, false, true);

	global_col.clear();
	LevelContext& ctx = L.GetContext(unit);
	IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, 2.f + unit_radius, &ignore);

	Vec3 tmp_pos = pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			unit.pos = tmp_pos;
			ok = true;
			break;
		}

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

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

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->player_info->warping = true;
	}
}

void Game::UpdateContext(LevelContext& ctx, float dt)
{
	// aktualizuj postacie
	UpdateUnits(ctx, dt);

	if(ctx.lights && lights_dt >= 1.f / 60)
		UpdateLights(*ctx.lights);

	// aktualizuj skrzynie
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			(*it)->mesh_inst->Update(dt);
	}

	// aktualizuj drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			door.mesh_inst->Update(dt);
			if(door.state == Door::Opening || door.state == Door::Opening2)
			{
				if(door.state == Door::Opening)
				{
					if(door.mesh_inst->frame_end_info || door.mesh_inst->GetProgress() >= 0.25f)
					{
						door.state = Door::Opening2;
						btVector3& pos = door.phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
					}
				}
				if(door.mesh_inst->frame_end_info)
					door.state = Door::Open;
			}
			else if(door.state == Door::Closing || door.state == Door::Closing2)
			{
				if(door.state == Door::Closing)
				{
					if(door.mesh_inst->frame_end_info || door.mesh_inst->GetProgress() <= 0.25f)
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
				if(door.mesh_inst->frame_end_info)
				{
					if(door.state == Door::Closing2)
						door.state = Door::Closed;
					else
					{
						// nie mo¿na zamknaæ drzwi bo coœ blokuje
						door.state = Door::Opening2;
						door.mesh_inst->Play(&door.mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_NO_BLEND | PLAY_STOP_AT_END, 0);
						door.mesh_inst->frame_end_info = false;
						// mo¿na by daæ lepszy punkt dŸwiêku
						sound_mgr->PlaySound3d(sDoorBudge, door.pos, 2.f, 5.f);
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

int Game::GetDungeonLevel()
{
	if(L.location->outside)
		return L.location->st;
	else
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		if(inside->IsMultilevel())
			return (int)Lerp(max(3.f, float(inside->st) / 2), float(inside->st), float(L.dungeon_level) / (((MultiInsideLocation*)inside)->levels.size() - 1));
		else
			return inside->st;
	}
}

int Game::GetDungeonLevelChest()
{
	int level = GetDungeonLevel();
	if(L.location->spawn == SG_NONE)
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

void Game::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const Vec3& hitpoint, float range, bool dmg)
{
	// sounds
	sound_mgr->PlaySound3d(GetMaterialSound(mat2, mat), hitpoint, range, 10.f);
	if(mat != MAT_BODY && dmg)
		sound_mgr->PlaySound3d(GetMaterialSound(mat2, MAT_BODY), hitpoint, range, 10.f);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::HIT_SOUND;
		c.pos = hitpoint;
		c.id = mat2;
		c.ile = mat;

		if(mat != MAT_BODY && dmg)
		{
			NetChange& c2 = Add1(Net::changes);
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
	loading_cap = 0.66f;
	loading_steps = steps;
	loading_index = 0;
	clear_color = Color::Black;
	game_state = GS_LOAD;
	load_screen->visible = true;
	main_menu->visible = false;
	game_gui->visible = false;
	ResourceManager::Get().PrepareLoadScreen(loading_cap);
}

void Game::LoadingStep(cstring text, int end)
{
	float progress;
	if(end == 0)
		progress = float(loading_index) / loading_steps * loading_cap;
	else if(end == 1)
		progress = loading_cap;
	else
		progress = 1.f;
	load_screen->SetProgressOptional(progress, text);

	if(end != 2)
	{
		++loading_index;
		if(end == 1)
			assert(loading_index == loading_steps);
	}
	if(end != 1)
	{
		loading_dt += loading_t.Tick();
		if(loading_dt >= 1.f / 30 || end == 2)
		{
			loading_dt = 0.f;
			DoPseudotick();
			loading_t.Tick();
		}
	}
}

// Preload resources and start load screen if required
void Game::LoadResources(cstring text, bool worldmap)
{
	LoadingStep(nullptr, 1);

	PreloadResources(worldmap);

	// check if there is anything to load
	auto& res_mgr = ResourceManager::Get();
	if(res_mgr.HaveTasks())
	{
		Info("Loading new resources (%d).", res_mgr.GetLoadTasksCount());
		res_mgr.StartLoadScreen(txLoadingResources);

		// apply mesh instance for newly loaded meshes
		for(auto& unit_mesh : units_mesh_load)
		{
			Unit::CREATE_MESH mode;
			if(unit_mesh.second)
				mode = Unit::CREATE_MESH::ON_WORLDMAP;
			else if(mp_load && Net::IsClient())
				mode = Unit::CREATE_MESH::AFTER_PRELOAD;
			else
				mode = Unit::CREATE_MESH::NORMAL;
			unit_mesh.first->CreateMesh(mode);
		}
		units_mesh_load.clear();
	}
	else
	{
		Info("Nothing new to load.");
		res_mgr.CancelLoadScreen();
	}

	// spawn blood for units that are dead and their mesh just loaded
	for(Unit* unit : blood_to_spawn)
		CreateBlood(L.GetContext(*unit), *unit, true);
	blood_to_spawn.clear();

	// finished
	if((Net::IsLocal() || !mp_load_worldmap) && !L.location->outside)
		SetDungeonParamsToMeshes();
	LoadingStep(text, 2);
}

// set if passed, returns prev value
bool Game::RequireLoadingResources(Location* loc, bool* to_set)
{
	bool result;
	if(loc->GetLastLevel() == 0)
	{
		result = loc->loaded_resources;
		if(to_set)
			loc->loaded_resources = *to_set;
	}
	else
	{
		MultiInsideLocation* multi = (MultiInsideLocation*)loc;
		auto& info = multi->infos[L.dungeon_level];
		result = info.loaded_resources;
		if(to_set)
			info.loaded_resources = *to_set;
	}
	return result;
}

// When there is something new to load, add task to load it when entering location etc
void Game::PreloadResources(bool worldmap)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();

	if(Net::IsLocal())
		items_load.clear();

	if(!worldmap)
	{
		// load units - units respawn so need to check everytime...
		PreloadUnits(*L.local_ctx.units);
		// some traps respawn
		if(L.local_ctx.traps)
			PreloadTraps(*L.local_ctx.traps);

		// preload items, this info is sent by server so no need to redo this by clients (and it will be less complete)
		if(Net::IsLocal())
		{
			for(auto ground_item : *L.local_ctx.items)
				items_load.insert(ground_item->item);
			for(auto chest : *L.local_ctx.chests)
				PreloadItems(chest->items);
			for(auto usable : *L.local_ctx.usables)
			{
				if(usable->container)
					PreloadItems(usable->container->items);
			}
			PreloadItems(chest_merchant);
			PreloadItems(chest_blacksmith);
			PreloadItems(chest_alchemist);
			PreloadItems(chest_innkeeper);
			PreloadItems(chest_food_seller);
			auto quest = (Quest_Orcs2*)QM.FindQuestById(Q_ORCS2);
			if(quest)
				PreloadItems(quest->wares);
		}

		if(L.city_ctx)
		{
			for(auto ib : L.city_ctx->inside_buildings)
			{
				PreloadUnits(ib->units);
				if(Net::IsLocal())
				{
					for(auto ground_item : ib->items)
						items_load.insert(ground_item->item);
					for(auto usable : ib->usables)
					{
						if(usable->container)
							PreloadItems(usable->container->items);
					}
				}
			}
		}

		bool new_value = true;
		if(RequireLoadingResources(L.location, &new_value) == false)
		{
			// load music
			if(!sound_mgr->IsMusicDisabled())
				LoadMusic(GetLocationMusic(), false, true);

			// load objects
			for(auto obj : *L.local_ctx.objects)
				mesh_mgr.AddLoadTask(obj->mesh);

			// load usables
			PreloadUsables(*L.local_ctx.usables);

			if(L.city_ctx)
			{
				// load buildings
				for(auto& b : L.city_ctx->buildings)
				{
					auto& type = *b.type;
					if(type.state == ResourceState::NotLoaded)
					{
						if(type.mesh)
							mesh_mgr.AddLoadTask(type.mesh);
						if(type.inside_mesh)
							mesh_mgr.AddLoadTask(type.inside_mesh);
						type.state = ResourceState::Loaded;
					}
				}

				for(auto ib : L.city_ctx->inside_buildings)
				{
					// load building objects
					for(auto obj : ib->objects)
						mesh_mgr.AddLoadTask(obj->mesh);

					// load building usables
					PreloadUsables(ib->usables);

					// load units inside building
					PreloadUnits(ib->units);
				}
			}
		}
	}

	for(const Item* item : items_load)
		PreloadItem(item);
}

void Game::PreloadUsables(vector<Usable*>& usables)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& sound_mgr = ResourceManager::Get<Sound>();

	for(auto u : usables)
	{
		auto base = u->base;
		if(base->state == ResourceState::NotLoaded)
		{
			if(base->variants)
			{
				for(uint i = 0; i < base->variants->entries.size(); ++i)
					mesh_mgr.AddLoadTask(base->variants->entries[i].mesh);
			}
			else
				mesh_mgr.AddLoadTask(base->mesh);
			if(base->sound)
				sound_mgr.AddLoadTask(base->sound);
			base->state = ResourceState::Loaded;
		}
	}
}

void Game::PreloadUnits(vector<Unit*>& units)
{
	for(Unit* unit : units)
		PreloadUnit(unit);
}

void Game::PreloadUnit(Unit* unit)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();
	auto& tex_mgr = ResourceManager::Get<Texture>();
	auto& sound_mgr = ResourceManager::Get<Sound>();

	if(Net::IsLocal())
	{
		for(uint i = 0; i < SLOT_MAX; ++i)
		{
			if(unit->slots[i])
				items_load.insert(unit->slots[i]);
		}
		PreloadItems(unit->items);
	}

	auto& data = *unit->data;
	if(data.state == ResourceState::Loaded)
		return;

	if(data.mesh)
		mesh_mgr.AddLoadTask(data.mesh);

	for(int i = 0; i < SOUND_MAX; ++i)
	{
		if(data.sounds->sound[i])
			sound_mgr.AddLoadTask(data.sounds->sound[i]);
	}

	if(data.tex)
	{
		for(TexId& ti : data.tex->textures)
		{
			if(ti.tex)
				tex_mgr.AddLoadTask(ti.tex);
		}
	}
}

void Game::PreloadItems(vector<ItemSlot>& items)
{
	for(auto& slot : items)
		items_load.insert(slot.item);
}

void Game::PreloadItem(const Item* citem)
{
	Item& item = *(Item*)citem;
	if(item.state == ResourceState::Loaded)
		return;

	if(ResourceManager::Get().IsLoadScreen())
	{
		if(item.state != ResourceState::Loading)
		{
			if(item.type == IT_ARMOR)
			{
				Armor& armor = item.ToArmor();
				if(!armor.tex_override.empty())
				{
					auto& tex_mgr = ResourceManager::Get<Texture>();
					for(TexId& ti : armor.tex_override)
					{
						if(!ti.id.empty())
							ti.tex = tex_mgr.AddLoadTask(ti.id);
					}
				}
			}
			else if(item.type == IT_BOOK)
			{
				Book& book = item.ToBook();
				ResourceManager::Get<Texture>().AddLoadTask(book.scheme->tex);
			}

			if(item.tex)
				ResourceManager::Get<Texture>().AddLoadTask(item.tex, &item, TaskCallback(this, &Game::GenerateItemImage), true);
			else
				ResourceManager::Get<Mesh>().AddLoadTask(item.mesh, &item, TaskCallback(this, &Game::GenerateItemImage), true);

			item.state = ResourceState::Loading;
		}
	}
	else
	{
		// instant loading
		if(item.type == IT_ARMOR)
		{
			Armor& armor = item.ToArmor();
			if(!armor.tex_override.empty())
			{
				auto& tex_mgr = ResourceManager::Get<Texture>();
				for(TexId& ti : armor.tex_override)
				{
					if(!ti.id.empty())
						tex_mgr.Load(ti.tex);
				}
			}
		}
		else if(item.type == IT_BOOK)
		{
			Book& book = item.ToBook();
			ResourceManager::Get<Texture>().Load(book.scheme->tex);
		}

		if(item.tex)
		{
			ResourceManager::Get<Texture>().Load(item.tex);
			item.icon = item.tex->tex;
		}
		else
		{
			ResourceManager::Get<Mesh>().Load(item.mesh);
			TaskData task;
			task.ptr = &item;
			GenerateItemImage(task);
		}

		item.state = ResourceState::Loaded;
	}
}

void Game::VerifyResources()
{
	for(auto item : *L.local_ctx.items)
		VerifyItemResources(item->item);
	for(auto obj : *L.local_ctx.objects)
		assert(obj->mesh->state == ResourceState::Loaded);
	for(auto unit : *L.local_ctx.units)
		VerifyUnitResources(unit);
	for(auto u : *L.local_ctx.usables)
	{
		auto base = u->base;
		assert(base->state == ResourceState::Loaded);
		if(base->sound)
			assert(base->sound->IsLoaded());
	}
	if(L.local_ctx.traps)
	{
		for(auto trap : *L.local_ctx.traps)
		{
			assert(trap->base->state == ResourceState::Loaded);
			if(trap->base->mesh)
				assert(trap->base->mesh->IsLoaded());
			if(trap->base->mesh2)
				assert(trap->base->mesh2->IsLoaded());
			if(trap->base->sound)
				assert(trap->base->sound->IsLoaded());
			if(trap->base->sound2)
				assert(trap->base->sound2->IsLoaded());
			if(trap->base->sound3)
				assert(trap->base->sound3->IsLoaded());
		}
	}
	if(L.city_ctx)
	{
		for(auto ib : L.city_ctx->inside_buildings)
		{
			for(auto item : ib->items)
				VerifyItemResources(item->item);
			for(auto obj : ib->objects)
				assert(obj->mesh->state == ResourceState::Loaded);
			for(auto unit : ib->units)
				VerifyUnitResources(unit);
			for(auto u : ib->usables)
			{
				auto base = u->base;
				assert(base->state == ResourceState::Loaded);
				if(base->sound)
					assert(base->sound->IsLoaded());
			}
		}
	}
}

void Game::VerifyUnitResources(Unit* unit)
{
	assert(unit->data->state == ResourceState::Loaded);
	if(unit->data->mesh)
		assert(unit->data->mesh->IsLoaded());
	if(unit->data->sounds)
	{
		for(int i = 0; i < SOUND_MAX; ++i)
		{
			if(unit->data->sounds->sound[i])
				assert(unit->data->sounds->sound[i]->IsLoaded());
		}
	}
	if(unit->data->tex)
	{
		for(auto& tex : unit->data->tex->textures)
		{
			if(tex.tex)
				assert(tex.tex->IsLoaded());
		}
	}

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(unit->slots[i])
			VerifyItemResources(unit->slots[i]);
	}
	for(auto& slot : unit->items)
		VerifyItemResources(slot.item);
}

void Game::VerifyItemResources(const Item* item)
{
	assert(item->state == ResourceState::Loaded);
	if(item->tex)
		assert(item->tex->IsLoaded());
	if(item->mesh)
		assert(item->mesh->IsLoaded());
	assert(item->icon);
}

void Game::StartArenaCombat(int level)
{
	assert(InRange(level, 1, 3));

	int ile, lvl;

	switch(Rand() % 5)
	{
	case 0:
		ile = 1;
		lvl = level * 5 + 1;
		break;
	case 1:
		ile = 1;
		lvl = level * 5;
		break;
	case 2:
		ile = 2;
		lvl = level * 5 - 1;
		break;
	case 3:
		ile = 2;
		lvl = level * 5 - 2;
		break;
	case 4:
		ile = 3;
		lvl = level * 5 - 3;
		break;
	}

	arena_free = false;
	arena_tryb = Arena_Walka;
	arena_etap = Arena_OdliczanieDoPrzeniesienia;
	arena_t = 0.f;
	arena_poziom = level;
	L.city_ctx->arena_time = W.GetWorldtime();
	at_arena.clear();

	// dodaj gracza na arenê
	if(current_dialog->is_local)
	{
		fallback_type = FALLBACK::ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(current_dialog->pc->player_info->changes);
		c.type = NetChangePlayer::ENTER_ARENA;
		current_dialog->pc->arena_fights++;
	}

	current_dialog->pc->unit->frozen = FROZEN::YES;
	current_dialog->pc->unit->in_arena = 0;
	at_arena.push_back(current_dialog->pc->unit);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHANGE_ARENA_STATE;
		c.unit = current_dialog->pc->unit;
	}

	for(Unit* unit : Team.members)
	{
		if(unit->frozen != FROZEN::NO || Vec3::Distance2d(unit->pos, L.city_ctx->arena_pos) > 5.f)
			continue;
		if(unit->IsPlayer())
		{
			if(unit->frozen == FROZEN::NO)
			{
				BreakUnitAction(*unit, BREAK_ACTION_MODE::NORMAL, true, true);

				unit->frozen = FROZEN::YES;
				unit->in_arena = 0;
				at_arena.push_back(unit);

				unit->player->arena_fights++;
				if(Net::IsOnline())
					unit->player->stat_flags |= STAT_ARENA_FIGHTS;

				if(unit->player == pc)
				{
					fallback_type = FALLBACK::ARENA;
					fallback_t = -1.f;
				}
				else
				{
					NetChangePlayer& c = Add1(unit->player->player_info->changes);
					c.type = NetChangePlayer::ENTER_ARENA;
				}

				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = unit;
			}
		}
		else if(unit->IsHero() && unit->CanFollow())
		{
			unit->frozen = FROZEN::YES;
			unit->in_arena = 0;
			unit->hero->following = current_dialog->pc->unit;
			at_arena.push_back(unit);

			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::CHANGE_ARENA_STATE;
				c.unit = unit;
			}
		}
	}

	if(at_arena.size() > 3)
	{
		lvl += (at_arena.size() - 3) / 2 + 1;
		while(lvl > level * 5 + 2)
		{
			lvl -= 2;
			++ile;
		}
	}

	cstring list_id;
	switch(level)
	{
	default:
	case 1:
		list_id = "arena_easy";
		SpawnArenaViewers(1);
		break;
	case 2:
		list_id = "arena_medium";
		SpawnArenaViewers(3);
		break;
	case 3:
		list_id = "arena_hard";
		SpawnArenaViewers(5);
		break;
	}

	auto list = UnitGroupList::Get(list_id);
	auto group = list->groups[Rand() % list->groups.size()];

	TmpUnitGroup part;
	part.group = group;
	part.total = 0;
	for(auto& entry : part.group->entries)
	{
		if(lvl >= entry.ud->level.x)
		{
			auto& new_entry = Add1(part.entries);
			new_entry.ud = entry.ud;
			new_entry.count = entry.count;
			if(lvl < entry.ud->level.y)
				new_entry.count = max(1, new_entry.count / 2);
			part.total += new_entry.count;
		}
	}

	InsideBuilding* arena = GetArena();

	for(int i = 0; i < ile; ++i)
	{
		int x = Rand() % part.total, y = 0;
		for(auto& entry : part.entries)
		{
			y += entry.count;
			if(x < y)
			{
				Unit* u = SpawnUnitInsideArea(arena->ctx, arena->arena2, *entry.ud, lvl);
				u->rot = 0.f;
				u->in_arena = 1;
				u->frozen = FROZEN::YES;
				at_arena.push_back(u);

				if(Net::IsOnline())
					Net_SpawnUnit(u);

				break;
			}
		}
	}
}

Unit* Game::SpawnUnitInsideArea(LevelContext& ctx, const Box2d& area, UnitData& unit, int level)
{
	Vec3 pos;

	if(!WarpToArea(ctx, area, unit.GetRadius(), pos))
		return nullptr;

	return CreateUnitWithAI(ctx, unit, level, nullptr, &pos);
}

bool Game::WarpToArea(LevelContext& ctx, const Box2d& area, float radius, Vec3& pos, int tries)
{
	for(int i = 0; i < tries; ++i)
	{
		pos = area.GetRandomPos3();

		global_col.clear();
		GatherCollisionObjects(ctx, global_col, pos, radius, nullptr);

		if(!Collide(global_col, pos, radius))
			return true;
	}

	return false;
}

void Game::RemoveUnit(Unit* unit, bool notify)
{
	assert(unit);
	if(unit->action == A_DESPAWN)
		SpawnUnitEffect(*unit);
	unit->to_remove = true;
	L.to_remove.push_back(unit);
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->netid;
	}
}

void Game::DeleteUnit(Unit* unit)
{
	assert(unit);

	if(game_state != GS_WORLDMAP)
	{
		RemoveElement(L.GetContext(*unit).units, unit);
		for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
		{
			if(it->unit == unit)
			{
				unit_views.erase(it);
				break;
			}
		}
		if(pc_data.before_player == BP_UNIT && pc_data.before_player_ptr.unit == unit)
			pc_data.before_player = BP_NONE;
		if(unit == pc_data.selected_target)
			pc_data.selected_target = nullptr;
		if(unit == pc_data.selected_unit)
			pc_data.selected_unit = nullptr;
		if(pc->action == PlayerController::Action_LootUnit && pc->action_unit == unit)
			BreakUnitAction(*pc->unit);

		if(unit->player && Net::IsLocal())
		{
			switch(unit->player->action)
			{
			case PlayerController::Action_LootChest:
				{
					// close chest
					unit->player->action_chest->looted = false;
					unit->player->action_chest->mesh_inst->Play(&unit->player->action_chest->mesh_inst->mesh->anims[0],
						PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
					sound_mgr->PlaySound3d(sChestClose, unit->player->action_chest->GetCenter(), 2.f, 5.f);
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CHEST_CLOSE;
					c.id = unit->player->action_chest->netid;
				}
				break;
			case PlayerController::Action_LootUnit:
				unit->player->action_unit->busy = Unit::Busy_No;
				break;
			case PlayerController::Action_Trade:
			case PlayerController::Action_Talk:
			case PlayerController::Action_GiveItems:
			case PlayerController::Action_ShareItems:
				unit->player->action_unit->busy = Unit::Busy_No;
				unit->player->action_unit->look_target = nullptr;
				break;
			case PlayerController::Action_LootContainer:
				unit->UseUsable(nullptr);
				break;
			}
		}

		if(QM.quest_contest->state >= Quest_Contest::CONTEST_STARTING)
			RemoveElementTry(QM.quest_contest->units, unit);
		if(!arena_free)
		{
			RemoveElementTry(at_arena, unit);
			if(arena_fighter == unit)
				arena_fighter = nullptr;
		}

		if(unit->usable)
			unit->usable->user = nullptr;

		if(unit->bubble)
			unit->bubble->unit = nullptr;

		if(unit->bow_instance)
			bow_instances.push_back(unit->bow_instance);
	}

	if(unit->interp)
		EntityInterpolator::Pool.Free(unit->interp);

	if(unit->ai)
	{
		RemoveElement(ais, unit->ai);
		delete unit->ai;
	}

	if(unit->cobj)
	{
		delete unit->cobj->getCollisionShape();
		phy_world->removeCollisionObject(unit->cobj);
		delete unit->cobj;
	}

	unit->look_target = nullptr;

	if(--unit->refs == 0)
		delete unit;
}

void Game::DialogTalk(DialogContext& ctx, cstring msg)
{
	assert(msg);

	ctx.dialog_text = msg;
	ctx.dialog_wait = 1.f + float(strlen(ctx.dialog_text)) / 20;

	int ani;
	if(!ctx.talker->usable && ctx.talker->data->type == UNIT_TYPE::HUMAN && ctx.talker->action == A_NONE && Rand() % 3 != 0)
	{
		ani = Rand() % 2 + 1;
		ctx.talker->mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		ctx.talker->mesh_inst->groups[0].speed = 1.f;
		ctx.talker->animation = ANI_PLAY;
		ctx.talker->action = A_ANIMATION;
	}
	else
		ani = 0;

	game_gui->AddSpeechBubble(ctx.talker, ctx.dialog_text);

	ctx.pc->Train(TrainWhat::Talk, 0.f, 0);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
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

//=============================================================================
// Rozdziela z³oto pomiêdzy cz³onków dru¿yny
//=============================================================================
void Game::AddGold(int count, vector<Unit*>* units, bool show, cstring msg, float time, bool defmsg)
{
	if(!units)
		units = &Team.active_members;

	if(units->size() == 1)
	{
		Unit& u = *(*units)[0];
		u.gold += count;
		if(show && u.IsPlayer())
		{
			if(&u == pc->unit)
				AddGameMsg(Format(msg, count), time);
			else
			{
				NetChangePlayer& c = Add1(u.player->player_info->changes);
				c.type = NetChangePlayer::GOLD_MSG;
				c.id = (defmsg ? 1 : 0);
				c.ile = count;
				u.player->player_info->UpdateGold();
			}
		}
		else if(!u.IsPlayer() && u.busy == Unit::Busy_Trading)
		{
			Unit* trader = FindPlayerTradingWithUnit(u);
			if(trader != pc->unit)
			{
				NetChangePlayer& c = Add1(trader->player->player_info->changes);
				c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
				c.id = u.netid;
				c.ile = u.gold;
			}
		}
		return;
	}

	int pc_count = 0, npc_count = 0;
	bool credit_info = false;

	for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsPlayer())
		{
			++pc_count;
			u.player->on_credit = false;
			u.player->gold_get = 0;
		}
		else
		{
			++npc_count;
			u.hero->on_credit = false;
			u.hero->gained_gold = false;
		}
	}

	for(int i = 0; i < 2 && count > 0; ++i)
	{
		Vec2 share = Team.GetShare(pc_count, npc_count);
		int gold_left = 0;

		for(vector<Unit*>::iterator it = units->begin(), end = units->end(); it != end; ++it)
		{
			Unit& u = **it;
			HeroPlayerCommon& hpc = *(u.IsPlayer() ? (HeroPlayerCommon*)u.player : u.hero);
			if(hpc.on_credit)
				continue;

			float gain = (u.IsPlayer() ? share.x : share.y) * count + hpc.split_gold;
			float gained_f;
			hpc.split_gold = modf(gain, &gained_f);
			int gained = (int)gained_f;
			if(hpc.credit > gained)
			{
				credit_info = true;
				hpc.credit -= gained;
				gold_left += gained;
				hpc.on_credit = true;
				if(u.IsPlayer())
					--pc_count;
				else
					--npc_count;
			}
			else if(hpc.credit)
			{
				credit_info = true;
				gained -= hpc.credit;
				gold_left += hpc.credit;
				hpc.credit = 0;
				u.gold += gained;
				if(u.IsPlayer())
					u.player->gold_get += gained;
				else
					u.hero->gained_gold = true;
			}
			else
			{
				u.gold += gained;
				if(u.IsPlayer())
					u.player->gold_get += gained;
				else
					u.hero->gained_gold = true;
			}
		}

		count = gold_left;
	}

	if(Net::IsOnline())
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
						u.player->player_info->update_flags |= PlayerInfo::UF_GOLD;
						if(show)
						{
							NetChangePlayer& c = Add1(u.player->player_info->changes);
							c.type = NetChangePlayer::GOLD_MSG;
							c.id = (defmsg ? 1 : 0);
							c.ile = u.player->gold_get;
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
					NetChangePlayer& c = Add1(trader->player->player_info->changes);
					c.type = NetChangePlayer::UPDATE_TRADER_GOLD;
					c.id = u.netid;
					c.ile = u.gold;
				}
			}
		}

		if(credit_info)
			Net::PushChange(NetChange::UPDATE_CREDIT);
	}
	else if(show)
		AddGameMsg(Format(msg, pc->gold_get), time);
}

int Game::CalculateQuestReward(int gold)
{
	return gold * (90 + Team.GetActiveTeamSize() * 10) / 100;
}

void Game::AddGoldArena(int count)
{
	vector<Unit*> v;
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->in_arena == 0)
			v.push_back(*it);
	}

	AddGold(count * (85 + v.size() * 15) / 100, &v, true);
}

void Game::EventTakeItem(int id)
{
	if(id == BUTTON_YES)
	{
		ItemSlot& slot = pc->unit->items[take_item_id];
		pc->credit += slot.item->value / 2;
		slot.team_count = 0;

		if(Net::IsLocal())
			CheckCredit(true);
		else
		{
			NetChange& c = Add1(Net::changes);
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
	if(L.city_ctx)
		return;

	bool is_clear = true;
	for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->IsAlive() && IsEnemy(*pc->unit, **it, true))
		{
			is_clear = false;
			break;
		}
	}

	if(is_clear)
	{
		bool cleared = false;
		if(!L.location->outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
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
			L.location->state = LS_CLEARED;

		bool prevent = false;
		if(L.event_handler)
			prevent = L.event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);

		if(cleared && prevent && L.location->spawn != SG_NONE)
		{
			if(L.location->type == L_CAMP)
				W.AddNews(Format(txNewsCampCleared, W.GetLocation(W.GetNearestSettlement(L.location->pos))->name.c_str()));
			else
				W.AddNews(Format(txNewsLocCleared, L.location->name.c_str()));
		}
	}
}

void Game::SpawnArenaViewers(int count)
{
	assert(InRange(count, 1, 9));

	vector<Mesh::Point*> points;
	UnitData& ud = *UnitData::Get("viewer");
	InsideBuilding* arena = GetArena();
	Mesh* mesh = arena->type->inside_mesh;

	for(vector<Mesh::Point>::iterator it = mesh->attach_points.begin(), end = mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "o_s_viewer_", 11) == 0)
			points.push_back(&*it);
	}

	while(count > 0)
	{
		int id = Rand() % points.size();
		Mesh::Point* pt = points[id];
		points.erase(points.begin() + id);
		Vec3 pos(pt->mat._41 + arena->offset.x, pt->mat._42, pt->mat._43 + arena->offset.y);
		Vec3 look_at(arena->offset.x, 0, arena->offset.y);
		Unit* u = SpawnUnitNearLocation(arena->ctx, pos, ud, &look_at, -1, 2.f);
		if(u && Net::IsOnline())
			Net_SpawnUnit(u);
		--count;
		u->ai->loc_timer = Random(6.f, 12.f);
		arena_viewers.push_back(u);
	}
}

void Game::RemoveArenaViewers()
{
	UnitData* ud = UnitData::Get("viewer");
	LevelContext& ctx = GetArena()->ctx;

	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		if((*it)->data == ud)
			RemoveUnit(*it);
	}

	arena_viewers.clear();
}

bool Game::CanWander(Unit& u)
{
	if(L.city_ctx && u.ai->loc_timer <= 0.f && !dont_wander && IS_SET(u.data->flags, F_AI_WANDERS))
	{
		if(u.busy != Unit::Busy_No)
			return false;
		if(u.IsHero())
		{
			if(u.hero->team_member && u.hero->mode != HeroData::Wander)
				return false;
			else if(QM.quest_tournament->IsGenerated())
				return false;
			else
				return true;
		}
		else if(u.in_building == -1)
			return true;
		else
			return false;
	}
	else
		return false;
}

float Game::PlayerAngleY()
{
	const float pt0 = 4.6662526f;
	return cam.rot.y - pt0;
}

Vec3 Game::GetExitPos(Unit& u, bool force_border)
{
	const Vec3& pos = u.pos;

	if(L.location->outside)
	{
		if(u.in_building != -1)
			return L.city_ctx->inside_buildings[u.in_building]->exit_area.Midpoint().XZ();
		else if(L.city_ctx && !force_border)
		{
			float best_dist, dist;
			int best_index = -1, index = 0;

			for(vector<EntryPoint>::const_iterator it = L.city_ctx->entry_points.begin(), end = L.city_ctx->entry_points.end(); it != end; ++it, ++index)
			{
				if(it->exit_area.IsInside(u.pos))
				{
					// unit is already inside exit area, goto outside exit
					best_index = -1;
					break;
				}
				else
				{
					dist = Vec2::Distance(Vec2(u.pos.x, u.pos.z), it->exit_area.Midpoint());
					if(best_index == -1 || dist < best_dist)
					{
						best_dist = dist;
						best_index = index;
					}
				}
			}

			if(best_index != -1)
				return L.city_ctx->entry_points[best_index].exit_area.Midpoint().XZ();
		}

		int best = 0;
		float dist, best_dist;

		// lewa krawêdŸ
		best_dist = abs(pos.x - 32.f);

		// prawa krawêdŸ
		dist = abs(pos.x - (256.f - 32.f));
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
		dist = abs(pos.z - (256.f - 32.f));
		if(dist < best_dist)
			best = 3;

		switch(best)
		{
		default:
			assert(0);
		case 0:
			return Vec3(32.f, 0, pos.z);
		case 1:
			return Vec3(256.f - 32.f, 0, pos.z);
		case 2:
			return Vec3(pos.x, 0, 32.f);
		case 3:
			return Vec3(pos.x, 0, 256.f - 32.f);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)L.location;
		if(L.dungeon_level == 0 && inside->from_portal)
			return inside->portal->pos;
		Int2& pt = inside->GetLevelData().staircase_up;
		return Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1);
	}
}

void Game::AttackReaction(Unit& attacked, Unit& attacker)
{
	if(attacker.IsPlayer() && attacked.IsAI() && attacked.in_arena == -1 && !attacked.attack_team)
	{
		if(attacked.data->group == G_CITIZENS)
		{
			if(!Team.is_bandit)
			{
				Team.is_bandit = true;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
		}
		else if(attacked.data->group == G_CRAZIES)
		{
			if(!Team.crazies_attack)
			{
				Team.crazies_attack = true;
				if(Net::IsOnline())
					Net::PushChange(NetChange::CHANGE_FLAGS);
			}
		}
		if(attacked.dont_attack)
		{
			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
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

Game::CanLeaveLocationResult Game::CanLeaveLocation(Unit& unit)
{
	if(QM.quest_secret->state == Quest_Secret::SECRET_FIGHT)
		return CanLeaveLocationResult::InCombat;

	if(L.city_ctx)
	{
		for(Unit* p_unit : Team.members)
		{
			Unit& u = *p_unit;
			if(u.summoner != nullptr)
				continue;

			if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
				return CanLeaveLocationResult::TeamTooFar;

			if(u.IsPlayer())
			{
				if(u.in_building != -1 || Vec3::Distance2d(unit.pos, u.pos) > 8.f)
					return CanLeaveLocationResult::TeamTooFar;
			}

			for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE.x && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}
	else
	{
		for(Unit* p_unit : Team.members)
		{
			Unit& u = *p_unit;
			if(u.summoner != nullptr)
				continue;

			if(u.busy != Unit::Busy_No || Vec3::Distance2d(unit.pos, u.pos) > 8.f)
				return CanLeaveLocationResult::TeamTooFar;

			for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && IsEnemy(u, u2) && u2.IsAI() && u2.ai->in_combat && Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE.x && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}

	return CanLeaveLocationResult::Yes;
}

void Game::SpawnHeroesInsideDungeon()
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	Room* p = lvl.GetUpStairsRoom();
	int room_id = lvl.GetRoomId(p);
	int chance = 23;
	bool first = true;

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
			if(ok && (Rand() % 20 < chance || Rand() % 3 == 0 || first))
				ok_room.push_back(room_id);
		}

		first = false;

		if(ok_room.empty())
			break;
		else
		{
			room_id = ok_room[Rand() % ok_room.size()];
			ok_room.clear();
			sprawdzone.push_back(std::make_pair(&lvl.rooms[room_id], room_id));
			--chance;
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
	if(Rand() % 2 == 0)
		--end;
	for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && IsEnemy(*pc->unit, u))
		{
			for(vector<std::pair<Room*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
			{
				if(it->first->IsInside(u.pos))
				{
					gold += u.gold;
					for(int i = 0; i < SLOT_MAX; ++i)
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
					if(u.data->mesh->IsLoaded())
					{
						u.animation = u.current_animation = ANI_DIE;
						u.SetAnimationAtEnd(NAMES::ani_die);
						CreateBlood(L.local_ctx, u, true);
					}
					else
						blood_to_spawn.push_back(&u);
					u.hp = 0.f;
					++GameStats::Get().total_kills;

					// przenieœ fizyke
					btVector3 a_min, a_max;
					u.cobj->getWorldTransform().setOrigin(btVector3(1000, 1000, 1000));
					u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
					phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);

					if(u.event_handler)
						u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);
					break;
				}
			}
		}
	}
	for(vector<Chest*>::iterator it2 = L.local_ctx.chests->begin(), end2 = L.local_ctx.chests->end(); it2 != end2; ++it2)
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
		int x1 = max(a.pos.x, b.pos.x),
			x2 = min(a.pos.x + a.size.x, b.pos.x + b.size.x),
			y1 = max(a.pos.y, b.pos.y),
			y2 = min(a.pos.y + a.size.y, b.pos.y + b.size.y);

		// szukaj drzwi
		for(int y = y1; y < y2; ++y)
		{
			for(int x = x1; x < x2; ++x)
			{
				Pole& po = lvl.map[x + y * lvl.w];
				if(po.type == DRZWI)
				{
					Door* door = lvl.FindDoor(Int2(x, y));
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(&door->mesh_inst->mesh->anims[0]);
					}
				}
			}
		}
	}

	// stwórz bohaterów
	int count = Random(2, 4);
	LocalVector<Unit*> heroes;
	p = sprawdzone.back().first;
	int team_level = Random(4, 13);
	for(int i = 0; i < count; ++i)
	{
		int level = team_level + Random(-2, 2);
		Unit* u = SpawnUnitInsideRoom(*p, ClassInfo::GetRandomData(), level);
		if(u)
			heroes->push_back(u);
		else
			break;
	}

	// sortuj przedmioty wed³ug wartoœci
	std::sort(items.begin(), items.end(), [](const ItemSlot& a, const ItemSlot& b)
	{
		return a.item->GetWeightValue() < b.item->GetWeightValue();
	});

	// rozdziel z³oto
	int gold_per_hero = gold / heroes->size();
	for(vector<Unit*>::iterator it = heroes->begin(), end = heroes->end(); it != end; ++it)
		(*it)->gold += gold_per_hero;
	gold -= gold_per_hero * heroes->size();
	if(gold)
		heroes->back()->gold += gold;

	// rozdziel przedmioty
	vector<Unit*>::iterator heroes_it = heroes->begin(), heroes_end = heroes->end();
	while(!items.empty())
	{
		ItemSlot& item = items.back();
		for(int i = 0, ile = item.count; i < ile; ++i)
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

	for(int i = 0; i < 50; ++i)
	{
		Vec3 pos = room.GetRandomPos(0.5f);
		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pos, 0.25f);
		if(!Collide(global_col, pos, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pos;
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			L.local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool try_exact)
{
	assert(item);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!try_exact)
		{
			float a = Random(), b = Random();
			if(b < a)
				std::swap(a, b);
			pt = Vec3(pos.x + b * radius*cos(2 * PI*a / b), 0, pos.y + b * radius*sin(2 * PI*a / b));
		}
		else
		{
			try_exact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(L.local_ctx.type == LevelContext::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			L.local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& region_size, bool try_exact)
{
	assert(item);
	assert(region_size.x > 0 && region_size.y > 0);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!try_exact)
			pt = Vec3(pos.x + Random(region_size.x), 0, pos.y + Random(region_size.y));
		else
		{
			try_exact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(L.local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(L.local_ctx.type == LevelContext::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			L.local_ctx.items->push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

void Game::GenerateQuestUnits()
{
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::None && L.location_index == QM.quest_sawmill->start_loc)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("artur_drwal"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txArthur;
			QM.quest_sawmill->sawmill_state = Quest_Sawmill::State::GeneratedUnit;
			QM.quest_sawmill->hd_lumberjack.Get(*u->human_data);
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_mine->start_loc && QM.quest_mine->mine_state == Quest_Mine::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("inwestor"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txQuest[272];
			QM.quest_mine->mine_state = Quest_Mine::State::SpawnedInvestor;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_bandits->start_loc && QM.quest_bandits->bandits_state == Quest_Bandits::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("mistrz_agentow"), -2);
		assert(u);
		if(u)
		{
			u->hero->name = txQuest[273];
			QM.quest_bandits->bandits_state = Quest_Bandits::State::GeneratedMaster;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_mages->start_loc && QM.quest_mages2->mages_state == Quest_Mages2::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_magowie_uczony"), -2);
		assert(u);
		if(u)
		{
			QM.quest_mages2->mages_state = Quest_Mages2::State::GeneratedScholar;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_mages2->mage_loc)
	{
		if(QM.quest_mages2->mages_state == Quest_Mages2::State::TalkedWithCaptain)
		{
			Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				QM.quest_mages2->mages_state = Quest_Mages2::State::GeneratedOldMage;
				QM.quest_mages2->good_mage_name = u->hero->name;
				if(devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}
		else if(QM.quest_mages2->mages_state == Quest_Mages2::State::MageLeft)
		{
			Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
			assert(u);
			if(u)
			{
				QM.quest_mages2->scholar = u;
				u->hero->know_name = true;
				u->hero->name = QM.quest_mages2->good_mage_name;
				u->ApplyHumanData(QM.quest_mages2->hd_mage);
				QM.quest_mages2->mages_state = Quest_Mages2::State::MageGeneratedInCity;
				if(devmode)
					Info("Generated quest unit '%s'.", u->GetRealName());
			}
		}
	}

	if(L.location_index == QM.quest_orcs->start_loc && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_orkowie_straznik"));
		assert(u);
		if(u)
		{
			u->StartAutoTalk();
			QM.quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedGuard;
			QM.quest_orcs2->guard = u;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_goblins->start_loc && QM.quest_goblins->goblins_state == Quest_Goblins::State::None)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_gobliny_szlachcic"));
		assert(u);
		if(u)
		{
			QM.quest_goblins->nobleman = u;
			QM.quest_goblins->hd_nobleman.Get(*u->human_data);
			u->hero->name = txQuest[274];
			QM.quest_goblins->goblins_state = Quest_Goblins::State::GeneratedNobleman;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_evil->start_loc && QM.quest_evil->evil_state == Quest_Evil::State::None)
	{
		CityBuilding* b = L.city_ctx->FindBuilding(BuildingGroup::BG_INN);
		Unit* u = SpawnUnitNearLocation(L.local_ctx, b->walk_pt, *UnitData::Get("q_zlo_kaplan"), nullptr, 10);
		assert(u);
		if(u)
		{
			u->rot = Random(MAX_ANGLE);
			u->hero->name = txQuest[275];
			u->StartAutoTalk();
			QM.quest_evil->cleric = u;
			QM.quest_evil->evil_state = Quest_Evil::State::GeneratedCleric;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(Team.is_bandit)
		return;

	// sawmill quest
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
	{
		if(QM.quest_sawmill->days >= 30 && L.city_ctx)
		{
			QM.quest_sawmill->days = 29;
			Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_tartak"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				QM.quest_sawmill->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}
	else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		int ile = QM.quest_sawmill->days / 30;
		if(ile)
		{
			QM.quest_sawmill->days -= ile * 30;
			AddGold(ile * 400, nullptr, true);
		}
	}

	if(QM.quest_mine->days >= QM.quest_mine->days_required &&
		((QM.quest_mine->mine_state2 == Quest_Mine::State2::InBuild && QM.quest_mine->mine_state == Quest_Mine::State::Shares) || // inform player about building mine & give gold
			QM.quest_mine->mine_state2 == Quest_Mine::State2::Built || // inform player about possible investment
			QM.quest_mine->mine_state2 == Quest_Mine::State2::InExpand || // inform player about finished mine expanding
			QM.quest_mine->mine_state2 == Quest_Mine::State2::Expanded)) // inform player about finding portal
	{
		Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
		if(u)
		{
			QM.quest_mine->messenger = u;
			u->StartAutoTalk(true);
		}
	}

	GenerateQuestUnits2(true);

	if(QM.quest_evil->evil_state == Quest_Evil::State::GenerateMage && L.location_index == QM.quest_evil->mage_loc)
	{
		Unit* u = SpawnUnitInsideInn(*UnitData::Get("q_zlo_mag"), -2);
		assert(u);
		if(u)
		{
			QM.quest_evil->evil_state = Quest_Evil::State::GeneratedMage;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(W.GetDay() == 6 && W.GetMonth() == 2 && L.city_ctx && IS_SET(L.city_ctx->flags, City::HaveArena)
		&& L.location_index == QM.quest_tournament->GetCity() && !QM.quest_tournament->IsGenerated())
		QM.quest_tournament->GenerateUnits();
}

void Game::GenerateQuestUnits2(bool on_enter)
{
	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::Counting && QM.quest_goblins->days <= 0)
	{
		Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("q_gobliny_poslaniec"), &Team.leader->pos, -2, 2.f);
		if(u)
		{
			if(Net::IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			QM.quest_goblins->messenger = u;
			u->StartAutoTalk(true);
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft && QM.quest_goblins->days <= 0)
	{
		Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("q_gobliny_mag"), &Team.leader->pos, 5, 2.f);
		if(u)
		{
			if(Net::IsOnline() && !on_enter)
				Net_SpawnUnit(u);
			QM.quest_goblins->messenger = u;
			QM.quest_goblins->goblins_state = Quest_Goblins::State::GeneratedMage;
			u->StartAutoTalk(true);
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}
}

void Game::UpdateQuests(int days)
{
	if(Team.is_bandit)
		return;

	RemoveQuestUnits(false);

	int income = 0;

	// sawmill
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild)
	{
		QM.quest_sawmill->days += days;
		if(QM.quest_sawmill->days >= 30 && L.city_ctx && game_state == GS_LEVEL)
		{
			QM.quest_sawmill->days = 29;
			Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_tartak"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				if(Net::IsOnline())
					Net_SpawnUnit(u);
				QM.quest_sawmill->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}
	else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		QM.quest_sawmill->days += days;
		int count = QM.quest_sawmill->days / 30;
		if(count)
		{
			QM.quest_sawmill->days -= count * 30;
			income += count * 400;
		}
	}

	// mine
	if(QM.quest_mine->mine_state2 == Quest_Mine::State2::InBuild)
	{
		QM.quest_mine->days += days;
		if(QM.quest_mine->days >= QM.quest_mine->days_required)
		{
			if(QM.quest_mine->mine_state == Quest_Mine::State::Shares)
			{
				// player invesetd in mine, inform him about finishing
				if(L.city_ctx && game_state == GS_LEVEL)
				{
					Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
					if(u)
					{
						if(Net::IsOnline())
							Net_SpawnUnit(u);
						W.AddNews(Format(txMineBuilt, W.GetLocation(QM.quest_mine->target_loc)->name.c_str()));
						QM.quest_mine->messenger = u;
						u->StartAutoTalk(true);
					}
				}
			}
			else
			{
				// player got gold, don't inform him
				W.AddNews(Format(txMineBuilt, W.GetLocation(QM.quest_mine->target_loc)->name.c_str()));
				QM.quest_mine->mine_state2 = Quest_Mine::State2::Built;
				QM.quest_mine->days -= QM.quest_mine->days_required;
				QM.quest_mine->days_required = Random(60, 90);
				if(QM.quest_mine->days >= QM.quest_mine->days_required)
					QM.quest_mine->days = QM.quest_mine->days_required - 1;
			}
		}
	}
	else if(QM.quest_mine->mine_state2 == Quest_Mine::State2::Built
		|| QM.quest_mine->mine_state2 == Quest_Mine::State2::InExpand
		|| QM.quest_mine->mine_state2 == Quest_Mine::State2::Expanded)
	{
		// mine is built/in expand/expanded
		// count time to news about expanding/finished expanding/found portal
		QM.quest_mine->days += days;
		if(QM.quest_mine->days >= QM.quest_mine->days_required && L.city_ctx && game_state == GS_LEVEL)
		{
			Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				if(Net::IsOnline())
					Net_SpawnUnit(u);
				QM.quest_mine->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}

	// give gold from mine
	income += QM.quest_mine->GetIncome(days);

	if(income != 0)
		AddGold(income, nullptr, true);

	// update contest
	int stan; // 0 - before contest, 1 - time for contest, 2 - after contest
	int month = W.GetMonth();
	int day = W.GetDay();

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

	Quest_Contest* contest = QM.quest_contest;
	switch(stan)
	{
	case 0:
		if(contest->state != Quest_Contest::CONTEST_NOT_DONE)
		{
			contest->state = Quest_Contest::CONTEST_NOT_DONE;
			contest->where = W.GetRandomSettlementIndex(contest->where);
		}
		contest->generated = false;
		contest->units.clear();
		break;
	case 1:
		contest->state = Quest_Contest::CONTEST_TODAY;
		if(!contest->generated && game_state == GS_LEVEL && L.location_index == contest->where)
			SpawnDrunkmans();
		break;
	case 2:
		contest->state = Quest_Contest::CONTEST_DONE;
		contest->generated = false;
		contest->units.clear();
		break;
	}

	//----------------------------
	// mages
	if(QM.quest_mages2->mages_state == Quest_Mages2::State::Counting)
	{
		QM.quest_mages2->days -= days;
		if(QM.quest_mages2->days <= 0)
		{
			// from now golem can be encountered on road
			QM.quest_mages2->mages_state = Quest_Mages2::State::Encounter;
		}
	}

	// orcs
	if(Any(QM.quest_orcs2->orcs_state, Quest_Orcs2::State::CompletedJoined, Quest_Orcs2::State::CampCleared, Quest_Orcs2::State::PickedClass))
	{
		QM.quest_orcs2->days -= days;
		if(QM.quest_orcs2->days <= 0)
			QM.quest_orcs2->orc->StartAutoTalk();
	}

	// goblins
	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::Counting || QM.quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft)
		QM.quest_goblins->days -= days;

	// crazies
	if(QM.quest_crazies->crazies_state == Quest_Crazies::State::PickedStone)
		QM.quest_crazies->days -= days;

	QM.quest_tournament->Progress();

	if(L.city_ctx)
		GenerateQuestUnits2(false);
}

void Game::RemoveQuestUnit(UnitData* ud, bool on_leave)
{
	assert(ud);

	Unit* unit = L.FindUnit([=](Unit* unit)
	{
		return unit->data == ud && unit->IsAlive();
	});

	if(unit)
		RemoveUnit(unit, !on_leave);
}

void Game::RemoveQuestUnits(bool on_leave)
{
	if(L.city_ctx)
	{
		if(QM.quest_sawmill->messenger)
		{
			RemoveQuestUnit(UnitData::Get("poslaniec_tartak"), on_leave);
			QM.quest_sawmill->messenger = nullptr;
		}

		if(QM.quest_mine->messenger)
		{
			RemoveQuestUnit(UnitData::Get("poslaniec_kopalnia"), on_leave);
			QM.quest_mine->messenger = nullptr;
		}

		if(L.is_open && L.location_index == QM.quest_sawmill->start_loc && QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
			&& QM.quest_sawmill->build_state == Quest_Sawmill::BuildState::None)
		{
			Unit* u = L.city_ctx->FindInn()->FindUnit(UnitData::Get("artur_drwal"));
			if(u && u->IsAlive())
			{
				QM.quest_sawmill->build_state = Quest_Sawmill::BuildState::LumberjackLeft;
				RemoveUnit(u, !on_leave);
			}
		}

		if(QM.quest_mages2->scholar && QM.quest_mages2->mages_state == Quest_Mages2::State::ScholarWaits)
		{
			RemoveQuestUnit(UnitData::Get("q_magowie_uczony"), on_leave);
			QM.quest_mages2->scholar = nullptr;
			QM.quest_mages2->mages_state = Quest_Mages2::State::Counting;
			QM.quest_mages2->days = Random(15, 30);
		}

		if(QM.quest_orcs2->guard && QM.quest_orcs2->orcs_state >= Quest_Orcs2::State::GuardTalked)
		{
			RemoveQuestUnit(UnitData::Get("q_orkowie_straznik"), on_leave);
			QM.quest_orcs2->guard = nullptr;
		}
	}

	if(QM.quest_bandits->bandits_state == Quest_Bandits::State::AgentTalked)
	{
		QM.quest_bandits->bandits_state = Quest_Bandits::State::AgentLeft;
		for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
		{
			if(*it == QM.quest_bandits->agent && (*it)->IsAlive())
			{
				RemoveUnit(*it, !on_leave);
				break;
			}
		}
		QM.quest_bandits->agent = nullptr;
	}

	if(QM.quest_mages2->mages_state == Quest_Mages2::State::MageLeaving)
	{
		QM.quest_mages2->hd_mage.Set(*QM.quest_mages2->scholar->human_data);
		QM.quest_mages2->scholar = nullptr;
		RemoveQuestUnit(UnitData::Get("q_magowie_stary"), on_leave);
		QM.quest_mages2->mages_state = Quest_Mages2::State::MageLeft;
	}

	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::MessengerTalked && QM.quest_goblins->messenger)
	{
		RemoveQuestUnit(UnitData::Get("q_gobliny_poslaniec"), on_leave);
		QM.quest_goblins->messenger = nullptr;
	}

	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::GivenBow && QM.quest_goblins->nobleman)
	{
		RemoveQuestUnit(UnitData::Get("q_gobliny_szlachcic"), on_leave);
		QM.quest_goblins->nobleman = nullptr;
		QM.quest_goblins->goblins_state = Quest_Goblins::State::NoblemanLeft;
		QM.quest_goblins->days = Random(15, 30);
	}

	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::MageTalked && QM.quest_goblins->messenger)
	{
		RemoveQuestUnit(UnitData::Get("q_gobliny_mag"), on_leave);
		QM.quest_goblins->messenger = nullptr;
		QM.quest_goblins->goblins_state = Quest_Goblins::State::MageLeft;
	}

	if(QM.quest_evil->evil_state == Quest_Evil::State::ClericLeaving)
	{
		RemoveUnit(QM.quest_evil->cleric, !on_leave);
		QM.quest_evil->cleric = nullptr;
		QM.quest_evil->evil_state = Quest_Evil::State::ClericLeft;
	}
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
		RemoveElement(QM.quest_contest->units, unit);

		if(Net::IsOnline() && unit->IsPlayer() && unit->player != pc)
		{
			NetChangePlayer& c = Add1(unit->player->player_info->changes);
			c.type = NetChangePlayer::LOOK_AT;
			c.id = -1;
		}
	}
}

int Game::GetUnitEventHandlerQuestRefid()
{
	// specjalna wartoœæ u¿ywana dla wskaŸnika na Game
	return -2;
}

// czy ktokolwiek w dru¿ynie rozmawia
// bêdzie u¿ywane w multiplayer
bool Game::IsAnyoneTalking() const
{
	if(Net::IsLocal())
	{
		if(Net::IsOnline())
		{
			for(Unit* unit : Team.active_members)
			{
				if(unit->IsPlayer() && unit->player->dialog_ctx->dialog_mode)
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

bool Game::RemoveQuestItem(const Item* item, int refid)
{
	Unit* unit;
	int slot_id;

	if(Team.FindItemInTeam(item, refid, &unit, &slot_id))
	{
		RemoveItem(*unit, slot_id, 1);
		return true;
	}
	else
		return false;
}

void Game::UpdateGame2(float dt)
{
	// arena
	if(arena_tryb != Arena_Brak)
		UpdateArena(dt);

	// tournament
	if(QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
		QM.quest_tournament->Update(dt);

	// sharing of team items between team members
	UpdateTeamItemShares();

	// contest
	UpdateContest(dt);

	// quest bandits
	if(QM.quest_bandits->bandits_state == Quest_Bandits::State::Counting)
	{
		QM.quest_bandits->timer -= dt;
		if(QM.quest_bandits->timer <= 0.f)
		{
			// spawn agent
			Unit* u = SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("agent"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				if(Net::IsOnline())
					Net_SpawnUnit(u);
				QM.quest_bandits->bandits_state = Quest_Bandits::State::AgentCome;
				QM.quest_bandits->agent = u;
				u->StartAutoTalk(true);
			}
		}
	}

	// quest mages
	if(QM.quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined && L.location_index == QM.quest_mages2->target_loc)
	{
		QM.quest_mages2->timer += dt;
		if(QM.quest_mages2->timer >= 30.f && QM.quest_mages2->scholar->auto_talk == AutoTalkMode::No)
			QM.quest_mages2->scholar->StartAutoTalk();
	}

	// quest evil
	if(QM.quest_evil->evil_state == Quest_Evil::State::SpawnedAltar && L.location_index == QM.quest_evil->target_loc)
	{
		for(Unit* unit : Team.members)
		{
			Unit& u = *unit;
			if(u.IsStanding() && u.IsPlayer() && Vec3::Distance(u.pos, QM.quest_evil->pos) < 5.f && CanSee(u.pos, QM.quest_evil->pos))
			{
				QM.quest_evil->evil_state = Quest_Evil::State::Summoning;
				sound_mgr->PlaySound2d(sEvil);
				QM.quest_evil->SetProgress(Quest_Evil::Progress::AltarEvent);
				// spawn undead
				InsideLocation* inside = (InsideLocation*)L.location;
				inside->spawn = SG_UNDEAD;
				uint offset = L.local_ctx.units->size();
				DungeonGenerator* gen = (DungeonGenerator*)loc_gen_factory->Get(L.location);
				gen->GenerateUnits();
				if(Net::IsOnline())
				{
					Net::PushChange(NetChange::EVIL_SOUND);
					for(uint i = offset, ile = L.local_ctx.units->size(); i < ile; ++i)
						Net_SpawnUnit(L.local_ctx.units->at(i));
				}
				break;
			}
		}
	}
	else if(QM.quest_evil->evil_state == Quest_Evil::State::ClosingPortals && !L.location->outside && L.location->GetLastLevel() == L.dungeon_level)
	{
		int d = QM.quest_evil->GetLocId(L.location_index);
		if(d != -1)
		{
			Quest_Evil::Loc& loc = QM.quest_evil->loc[d];
			if(loc.state != 3)
			{
				Unit* u = QM.quest_evil->cleric;

				if(u->ai->state == AIController::Idle)
				{
					// sprawdŸ czy podszed³ do portalu
					float dist = Vec3::Distance2d(u->pos, loc.pos);
					if(dist < 5.f)
					{
						// podejdŸ
						u->ai->idle_action = AIController::Idle_Move;
						u->ai->idle_data.pos = loc.pos;
						u->ai->timer = 1.f;
						u->hero->mode = HeroData::Wait;

						// zamknij
						if(dist < 2.f)
						{
							QM.quest_evil->timer -= dt;
							if(QM.quest_evil->timer <= 0.f)
							{
								loc.state = Quest_Evil::Loc::State::PortalClosed;
								u->hero->mode = HeroData::Follow;
								u->ai->idle_action = AIController::Idle_None;
								QM.quest_evil->OnUpdate(Format(txPortalClosed, L.location->name.c_str()));
								u->StartAutoTalk();
								QM.quest_evil->changed = true;
								++QM.quest_evil->closed;
								delete L.location->portal;
								L.location->portal = nullptr;
								W.AddNews(Format(txPortalClosedNews, L.location->name.c_str()));
								if(Net::IsOnline())
									Net::PushChange(NetChange::CLOSE_PORTAL);
							}
						}
						else
							QM.quest_evil->timer = 1.5f;
					}
					else
						u->hero->mode = HeroData::Follow;
				}
			}
		}
	}

	// secret quest
	Quest_Secret* secret = QM.quest_secret;
	if(secret->state == Quest_Secret::SECRET_FIGHT)
	{
		int ile[2] = { 0 };

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
			for(Unit* unit : at_arena)
			{
				unit->in_arena = -1;
				if(unit->hp <= 0.f)
				{
					unit->HealPoison();
					unit->live_state = Unit::ALIVE;
					unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
					unit->mesh_inst->groups[0].speed = 1.f;
					unit->action = A_ANIMATION;
					unit->animation = ANI_PLAY;
					if(unit->IsAI())
						unit->ai->Reset();
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::STAND_UP;
						c.unit = unit;
					}
				}

				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::CHANGE_ARENA_STATE;
					c.unit = unit;
				}
			}

			at_arena[0]->hp = at_arena[0]->hpmax;
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::UPDATE_HP;
				c.unit = at_arena[0];
			}

			if(ile[0])
			{
				// gracz wygra³
				secret->state = Quest_Secret::SECRET_WIN;
				at_arena[0]->StartAutoTalk();
			}
			else
			{
				// gracz przegra³
				secret->state = Quest_Secret::SECRET_LOST;
			}

			at_arena.clear();
		}
	}
}

//=================================================================================================
void Game::UpdateArena(float dt)
{
	if(arena_etap == Arena_OdliczanieDoPrzeniesienia)
	{
		arena_t += dt * 2;
		if(arena_t >= 1.f)
		{
			if(arena_tryb == Arena_Walka)
			{
				for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
				{
					if((*it)->in_arena == 0)
						L.WarpUnit(*it, WARP_ARENA);
				}
			}
			else
			{
				for(auto unit : at_arena)
					L.WarpUnit(unit, WARP_ARENA);

				if(!at_arena.empty())
				{
					at_arena[0]->in_arena = 0;
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = at_arena[0];
					}
					if(at_arena.size() >= 2)
					{
						at_arena[1]->in_arena = 1;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHANGE_ARENA_STATE;
							c.unit = at_arena[1];
						}
					}
				}
			}

			// reset cooldowns
			for(auto unit : at_arena)
			{
				if(unit->IsPlayer())
					unit->player->RefreshCooldown();
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
			if(GetArena()->ctx.building_id == pc->unit->in_building)
				sound_mgr->PlaySound2d(sArenaFight);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::ARENA_SOUND;
				c.id = 0;
			}
			arena_etap = Arena_TrwaWalka;
			for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
			{
				(*it)->frozen = FROZEN::NO;
				if((*it)->IsPlayer() && (*it)->player != pc)
				{
					NetChangePlayer& c = Add1((*it)->player->player_info->changes);
					c.type = NetChangePlayer::START_ARENA_COMBAT;
				}
			}
		}
	}
	else if(arena_etap == Arena_TrwaWalka)
	{
		// talking by observers
		for(vector<Unit*>::iterator it = arena_viewers.begin(), end = arena_viewers.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.ai->loc_timer -= dt;
			if(u.ai->loc_timer <= 0.f)
			{
				u.ai->loc_timer = Random(6.f, 12.f);

				cstring text;
				if(Rand() % 2 == 0)
					text = RandomString(txArenaText);
				else
					text = Format(RandomString(txArenaTextU), GetRandomArenaHero()->GetRealName());

				UnitTalk(u, text);
			}
		}

		// count how many are still alive
		int count[2] = { 0 }, alive[2] = { 0 };
		for(Unit* unit : at_arena)
		{
			++count[unit->in_arena];
			if(unit->live_state != Unit::DEAD)
				++alive[unit->in_arena];
		}

		if(alive[0] == 0 || alive[1] == 0)
		{
			arena_etap = Arena_OdliczanieDoKonca;
			arena_t = 0.f;
			bool victory_sound;
			if(alive[0] == 0)
			{
				arena_wynik = 1;
				victory_sound = false;
			}
			else
			{
				arena_wynik = 0;
				victory_sound = true;
			}
			if(arena_tryb != Arena_Walka)
			{
				if(count[0] == 0 || count[1] == 0)
					victory_sound = false; // someone quit
				else
					victory_sound = true;
			}

			if(GetArena()->ctx.building_id == pc->unit->in_building)
				sound_mgr->PlaySound2d(victory_sound ? sArenaWin : sArenaLost);
			if(Net::IsOnline())
			{
				NetChange& c = Add1(Net::changes);
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
				(*it)->frozen = FROZEN::YES;
				if((*it)->IsPlayer())
				{
					if((*it)->player == pc)
					{
						fallback_type = FALLBACK::ARENA_EXIT;
						fallback_t = -1.f;
					}
					else
					{
						NetChangePlayer& c = Add1((*it)->player->player_info->changes);
						c.type = NetChangePlayer::EXIT_ARENA;
					}
				}
			}

			arena_etap = Arena_OdliczanieDoWyjscia;
			arena_t = 0.f;
		}
	}
	else
	{
		arena_t += dt * 2;
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

				for(Unit* unit : at_arena)
				{
					if(unit->in_arena != 0)
					{
						RemoveUnit(unit);
						continue;
					}

					unit->frozen = FROZEN::NO;
					unit->in_arena = -1;
					if(unit->hp <= 0.f)
					{
						unit->HealPoison();
						unit->live_state = Unit::ALIVE;
						unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->action = A_ANIMATION;
						unit->animation = ANI_PLAY;
						if(unit->IsAI())
							unit->ai->Reset();
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::STAND_UP;
							c.unit = unit;
						}
					}

					L.WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = unit;
					}
				}
			}
			else
			{
				for(Unit* unit : at_arena)
				{
					unit->frozen = FROZEN::NO;
					unit->in_arena = -1;
					if(unit->hp <= 0.f)
					{
						unit->HealPoison();
						unit->live_state = Unit::ALIVE;
						unit->mesh_inst->Play("wstaje2", PLAY_ONCE | PLAY_PRIO3, 0);
						unit->mesh_inst->groups[0].speed = 1.f;
						unit->action = A_ANIMATION;
						unit->animation = ANI_PLAY;
						if(unit->IsAI())
							unit->ai->Reset();
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::STAND_UP;
							c.unit = unit;
						}
					}

					L.WarpUnit(unit, WARP_OUTSIDE);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::CHANGE_ARENA_STATE;
						c.unit = unit;
					}
				}

				if(arena_tryb == Arena_Pvp && arena_fighter && arena_fighter->IsHero())
				{
					arena_fighter->hero->lost_pvp = (arena_wynik == 0);
					StartDialog2(pvp_player, arena_fighter, FindDialog(IS_SET(arena_fighter->data->flags, F_CRAZY) ? "crazy_pvp" : "hero_pvp"));
				}
			}

			if(arena_tryb != Arena_Zawody)
			{
				RemoveArenaViewers();
				at_arena.clear();
			}
			else
				QM.quest_tournament->FinishCombat();
			arena_tryb = Arena_Brak;
			arena_free = true;
		}
	}
}

//=================================================================================================
void Game::CleanArena()
{
	InsideBuilding* arena = L.city_ctx->FindInsideBuilding(BuildingGroup::BG_ARENA);

	// wyrzuæ ludzi z areny
	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		Unit& u = **it;
		u.frozen = FROZEN::NO;
		u.in_arena = -1;
		u.in_building = -1;
		u.busy = Unit::Busy_No;
		if(u.hp <= 0.f)
		{
			u.HealPoison();
			u.live_state = Unit::ALIVE;
		}
		if(u.IsAI())
			u.ai->Reset();
		WarpUnit(u, arena->outside_spawn);
		u.rot = arena->outside_rot;
	}
	RemoveArenaViewers();
	arena_free = true;
	arena_tryb = Arena_Brak;
}

void Game::UpdateContest(float dt)
{
	Quest_Contest* contest = QM.quest_contest;
	if(Any(contest->state, Quest_Contest::CONTEST_NOT_DONE, Quest_Contest::CONTEST_DONE, Quest_Contest::CONTEST_TODAY))
		return;

	int id;
	InsideBuilding* inn = L.city_ctx->FindInn(id);
	Unit& innkeeper = *inn->FindUnit(UnitData::Get("innkeeper"));

	if(!innkeeper.IsAlive())
	{
		for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.busy = Unit::Busy_No;
			u.look_target = nullptr;
			u.event_handler = nullptr;
			if(u.IsPlayer())
			{
				BreakUnitAction(u, BREAK_ACTION_MODE::NORMAL, true);
				if(u.player != pc)
				{
					NetChangePlayer& c = Add1(u.player->player_info->changes);
					c.type = NetChangePlayer::LOOK_AT;
					c.id = -1;
				}
			}
		}
		contest->state = Quest_Contest::CONTEST_DONE;
		innkeeper.busy = Unit::Busy_No;
		return;
	}

	if(contest->state == Quest_Contest::CONTEST_STARTING)
	{
		// info about getting out of range
		for(Unit* unit : contest->units)
		{
			if(!unit->IsPlayer())
				continue;
			float dist = Vec3::Distance2d(unit->pos, innkeeper.pos);
			bool leaving_event = (dist > 10.f || unit->in_building != id);
			if(leaving_event != unit->player->leaving_event)
			{
				unit->player->leaving_event = leaving_event;
				if(leaving_event)
					AddGameMsg3(unit->player, GMS_GETTING_OUT_OF_RANGE);
			}
		}

		// update timer
		if(innkeeper.busy == Unit::Busy_No && innkeeper.IsStanding() && innkeeper.ai->state == AIController::Idle)
		{
			float prev = contest->time;
			contest->time += dt;
			if(prev < 5.f && contest->time >= 5.f)
				UnitTalk(innkeeper, txContestStart);
		}

		if(contest->time >= 15.f && innkeeper.busy != Unit::Busy_Talking)
		{
			// start contest
			contest->state = Quest_Contest::CONTEST_IN_PROGRESS;

			// gather units
			for(vector<Unit*>::iterator it = inn->ctx.units->begin(), end = inn->ctx.units->end(); it != end; ++it)
			{
				Unit& u = **it;
				if(u.IsStanding() && u.IsAI() && !u.event_handler && u.frozen == FROZEN::NO && u.busy == Unit::Busy_No)
				{
					bool ok = false;
					if(IS_SET(u.data->flags2, F2_CONTEST))
						ok = true;
					else if(IS_SET(u.data->flags2, F2_CONTEST_50))
					{
						if(Rand() % 2 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flags3, F3_CONTEST_25))
					{
						if(Rand() % 4 == 0)
							ok = true;
					}
					else if(IS_SET(u.data->flags3, F3_DRUNK_MAGE))
					{
						if(QM.quest_mages2->mages_state < Quest_Mages2::State::MageCured)
							ok = true;
					}

					if(ok)
						contest->units.push_back(*it);
				}
			}
			contest->state2 = 0;

			// start looking at innkeeper, remove busy units/players out of range
			bool removed = false;
			for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
			{
				Unit& u = **it;
				bool kick = false;
				if(u.IsPlayer())
				{
					float dist = Vec3::Distance2d(u.pos, innkeeper.pos);
					kick = (dist > 10.f || u.in_building != id);
				}
				if(kick || u.in_building != id || u.frozen != FROZEN::NO || !u.IsStanding())
				{
					if(u.IsPlayer())
						AddGameMsg3(u.player, GMS_LEFT_EVENT);
					*it = nullptr;
					removed = true;
				}
				else
				{
					BreakUnitAction(u, BREAK_ACTION_MODE::NORMAL, true);
					if(u.IsPlayer() && u.player != pc)
					{
						NetChangePlayer& c = Add1(u.player->player_info->changes);
						c.type = NetChangePlayer::LOOK_AT;
						c.id = innkeeper.netid;
					}
					u.busy = Unit::Busy_Yes;
					u.look_target = &innkeeper;
					u.event_handler = this;
				}
			}
			if(removed)
				RemoveNullElements(contest->units);

			// jeœli jest za ma³o ludzi
			if(contest->units.size() <= 1u)
			{
				contest->state = Quest_Contest::CONTEST_FINISH;
				contest->state2 = 3;
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
	else if(contest->state == Quest_Contest::CONTEST_IN_PROGRESS)
	{
		bool talking = true;
		cstring next_text = nullptr, next_drink = nullptr;

		switch(contest->state2)
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
			if((contest->state2 - 20) % 2 == 0)
			{
				if(contest->state2 != 20)
					talking = false;
				next_text = txContestTalk[13];
			}
			else
				next_drink = "spirit";
			break;
		}

		if(talking)
		{
			if(innkeeper.CanAct())
			{
				if(next_text)
					UnitTalk(innkeeper, next_text);
				else
				{
					assert(next_drink);
					contest->time = 0.f;
					const Consumable& drink = Item::Get(next_drink)->ToConsumable();
					for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
						(*it)->ConsumeItem(drink, true);
				}

				++contest->state2;
			}
		}
		else
		{
			contest->time += dt;
			if(contest->time >= 5.f)
			{
				if(contest->units.size() >= 2)
				{
					assert(next_text);
					UnitTalk(innkeeper, next_text);
					++contest->state2;
				}
				else if(contest->units.size() == 1)
				{
					contest->state = Quest_Contest::CONTEST_FINISH;
					contest->state2 = 0;
					innkeeper.look_target = contest->units.back();
					W.AddNews(Format(txContestWinNews, contest->units.back()->GetName()));
					UnitTalk(innkeeper, txContestWin);
				}
				else
				{
					contest->state = Quest_Contest::CONTEST_FINISH;
					contest->state2 = 1;
					W.AddNews(txContestNoWinner);
					UnitTalk(innkeeper, txContestNoWinner);
				}
			}
		}
	}
	else if(contest->state == Quest_Contest::CONTEST_FINISH)
	{
		if(innkeeper.CanAct())
		{
			switch(contest->state2)
			{
			case 0: // wygrana
				contest->state2 = 2;
				UnitTalk(innkeeper, txContestPrize);
				break;
			case 1: // remis
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = nullptr;
				contest->state = Quest_Contest::CONTEST_DONE;
				contest->generated = false;
				contest->winner = nullptr;
				break;
			case 2: // wygrana2
				innkeeper.busy = Unit::Busy_No;
				innkeeper.look_target = nullptr;
				contest->winner = contest->units.back();
				contest->units.clear();
				contest->state = Quest_Contest::CONTEST_DONE;
				contest->generated = false;
				contest->winner->look_target = nullptr;
				contest->winner->busy = Unit::Busy_No;
				contest->winner->event_handler = nullptr;
				break;
			case 3: // brak ludzi
				for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
				{
					Unit& u = **it;
					u.busy = Unit::Busy_No;
					u.look_target = nullptr;
					u.event_handler = nullptr;
					if(u.IsPlayer())
					{
						BreakUnitAction(u, BREAK_ACTION_MODE::NORMAL, true);
						if(u.player != pc)
						{
							NetChangePlayer& c = Add1(u.player->player_info->changes);
							c.type = NetChangePlayer::LOOK_AT;
							c.id = -1;
						}
					}
				}
				contest->state = Quest_Contest::CONTEST_DONE;
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
			u.mesh_inst->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
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
					u.mesh_inst->Deactivate(1);
				}
				else
				{
					// schowa³ broñ, zacznij wyci¹gaæ
					u.weapon_taken = u.weapon_hiding;
					u.weapon_hiding = W_NONE;
					u.weapon_state = WS_TAKING;
					CLEAR_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
				}
			}
			else
			{
				// chowa broñ, zacznij wyci¹gaæ
				u.mesh_inst->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
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
				u.mesh_inst->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
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
				u.mesh_inst->Deactivate(1);
			}
			else
			{
				// wyj¹³ broñ z pasa, zacznij chowaæ
				u.weapon_hiding = u.weapon_taken;
				u.weapon_taken = W_NONE;
				u.weapon_state = WS_HIDING;
				u.animation_state = 0;
				SET_BIT(u.mesh_inst->groups[1].state, MeshInstance::FLAG_BACK);
			}
			break;
		case WS_TAKEN:
			// zacznij chowaæ
			u.mesh_inst->Play(u.GetTakeWeaponAnimation(co == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
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
	case GMS_GETTING_OUT_OF_RANGE:
		text = txGmsGettingOutOfRange;
		break;
	case GMS_LEFT_EVENT:
		text = txGmsLeftEvent;
		break;
	case GMS_GAME_SAVED:
		text = txGameSaved;
		time = 1.f;
		break;
	case GMS_NEED_WEAPON:
		text = txINeedWeapon;
		time = 2.f;
		break;
	case GMS_NO_POTION:
		text = txNoHpp;
		time = 2.f;
		break;
	case GMS_CANT_DO:
		text = txCantDo;
		break;
	case GMS_DONT_LOOT_FOLLOWER:
		text = txDontLootFollower;
		break;
	case GMS_DONT_LOOT_ARENA:
		text = txDontLootArena;
		break;
	case GMS_UNLOCK_DOOR:
		text = txUnlockedDoor;
		break;
	case GMS_NEED_KEY:
		text = txNeedKey;
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

void Game::AddGameMsg3(PlayerController* player, GMS id)
{
	assert(player);
	if(player->is_local)
		AddGameMsg3(id);
	else
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::GAME_MESSAGE;
		c.id = id;
	}
}

void Game::UpdatePlayerView()
{
	LevelContext& ctx = L.GetContext(*pc->unit);
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
			float dist = Vec3::Distance(u.visual_pos, u2.visual_pos);

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
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_SHARE || inventory_mode == I_GIVE)
	{
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(inventory_mode == I_LOOT_CHEST && Net::IsLocal())
	{
		pc->action_chest->looted = false;
		pc->action_chest->mesh_inst->Play(&pc->action_chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
		sound_mgr->PlaySound3d(sChestClose, pc->action_chest->GetCenter(), 2.f, 5.f);
	}
	else if(inventory_mode == I_LOOT_CONTAINER)
	{
		if(Net::IsLocal())
		{
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = pc->unit;
				c.id = pc->unit->usable->netid;
				c.ile = USE_USABLE_END;
			}
			pc->unit->UseUsable(nullptr);
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}

	if(Net::IsOnline() && (inventory_mode == I_LOOT_BODY || inventory_mode == I_LOOT_CHEST))
	{
		if(Net::IsClient())
			Net::PushChange(NetChange::STOP_TRADE);
		else if(inventory_mode == I_LOOT_BODY)
			pc->action_unit->busy = Unit::Busy_No;
		else
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHEST_CLOSE;
			c.id = pc->action_chest->netid;
		}
	}

	if(Any(pc->next_action, NA_PUT, NA_GIVE, NA_SELL))
		pc->next_action = NA_NONE;

	pc->action = PlayerController::Action_None;
	inventory_mode = I_NONE;
}

// zamyka ekwipunek i wszystkie okna które on móg³by utworzyæ
void Game::CloseInventory()
{
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
	if(Net::IsLocal())
		return !QM.unique_completed_show && QM.unique_quests_completed == UNIQUE_QUESTS && L.city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
	else
		return QM.unique_completed_show && L.city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
}

void Game::UpdateGameDialogClient()
{
	if(dialog_context.show_choices)
	{
		if(game_gui->UpdateChoice(dialog_context, dialog_choices.size()))
		{
			dialog_context.show_choices = false;
			dialog_context.dialog_text = "";

			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHOICE;
			c.id = dialog_context.choice_selected;
		}
	}
	else if(dialog_context.dialog_wait > 0.f && dialog_context.skip_id != -1)
	{
		if(GKey.KeyPressedReleaseAllowed(GK_SKIP_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG) || GKey.KeyPressedReleaseAllowed(GK_ATTACK_USE)
			|| (GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE)))
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SKIP_DIALOG;
			c.id = dialog_context.skip_id;
			dialog_context.skip_id = -1;
		}
	}
}

bool Game::FindQuestItem2(Unit* unit, cstring id, Quest** out_quest, int* i_index, bool not_active)
{
	assert(unit && id);

	if(id[1] == '$')
	{
		// szukaj w za³o¿onych przedmiotach
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(unit->slots[i] && unit->slots[i]->IsQuest())
			{
				Quest* quest = QM.FindQuest(unit->slots[i]->refid, !not_active);
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
				Quest* quest = QM.FindQuest(it2->item->refid, !not_active);
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
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(unit->slots[i] && unit->slots[i]->IsQuest() && unit->slots[i]->id == id)
			{
				Quest* quest = QM.FindQuest(unit->slots[i]->refid, !not_active);
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
				Quest* quest = QM.FindQuest(it2->item->refid, !not_active);
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
	if(!InRange(typ, 0, 1))
		return false;

	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHEAT_KILLALL;
		c.id = typ;
		c.unit = ignore;
		return true;
	}

	switch(typ)
	{
	case 0:
		for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && IsEnemy(**it, unit) && *it != ignore)
				GiveDmg(L.local_ctx, nullptr, (*it)->hp, **it, nullptr);
		}
		if(L.city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = L.city_ctx->inside_buildings.begin(), end2 = L.city_ctx->inside_buildings.end(); it2 != end2; ++it2)
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
		for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive() && !(*it)->IsPlayer() && *it != ignore)
				GiveDmg(L.local_ctx, nullptr, (*it)->hp, **it, nullptr);
		}
		if(L.city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = L.city_ctx->inside_buildings.begin(), end2 = L.city_ctx->inside_buildings.end(); it2 != end2; ++it2)
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

	if(Net::IsServer())
	{
		if(id == BUTTON_YES)
		{
			// zaakceptuj pvp
			StartPvp(pvp_response.from->player, pvp_response.to);
		}
		else
		{
			// nie akceptuj pvp
			NetChangePlayer& c = Add1(pvp_response.from->player->player_info->changes);
			c.type = NetChangePlayer::NO_PVP;
			c.id = pvp_response.to->player->id;
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::PVP;
		c.unit = pvp_unit;
		if(id == BUTTON_YES)
			c.id = 1;
		else
			c.id = 0;
	}

	pvp_response.ok = false;
}

void Game::Cheat_ShowMinimap()
{
	if(!L.location->outside)
	{
		InsideLocationLevel& lvl = ((InsideLocation*)L.location)->GetLevelData();

		for(int y = 0; y < lvl.h; ++y)
		{
			for(int x = 0; x < lvl.w; ++x)
				minimap_reveal.push_back(Int2(x, y));
		}

		UpdateDungeonMinimap(false);

		if(Net::IsServer())
			Net::PushChange(NetChange::CHEAT_SHOW_MINIMAP);
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
		fallback_type = FALLBACK::ARENA;
		fallback_t = -1.f;
	}
	else
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::ENTER_ARENA;
	}

	// fallback postaci
	if(unit->IsPlayer())
	{
		if(unit->player == pc)
		{
			fallback_type = FALLBACK::ARENA;
			fallback_t = -1.f;
		}
		else
		{
			NetChangePlayer& c = Add1(player->player_info->changes);
			c.type = NetChangePlayer::ENTER_ARENA;
		}
	}

	// dodaj do areny
	player->unit->frozen = FROZEN::YES;
	player->arena_fights++;
	if(Net::IsOnline())
		player->stat_flags |= STAT_ARENA_FIGHTS;
	at_arena.push_back(player->unit);
	unit->frozen = FROZEN::YES;
	at_arena.push_back(unit);
	if(unit->IsHero())
		unit->hero->following = player->unit;
	else if(unit->IsPlayer())
	{
		unit->player->arena_fights++;
		if(Net::IsOnline())
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
	if(info_box->visible)
		return;

	if(Net::IsServer())
		UpdateServer(dt);
	else
		UpdateClient(dt);
}

void Game::CheckCredit(bool require_update, bool ignore)
{
	if(Team.GetActiveTeamSize() > 1)
	{
		vector<Unit*>& active_team = Team.active_members;
		int ile = active_team.front()->GetCredit();

		for(vector<Unit*>::iterator it = active_team.begin() + 1, end = active_team.end(); it != end; ++it)
		{
			int kredyt = (*it)->GetCredit();
			if(kredyt < ile)
				ile = kredyt;
		}

		if(ile > 0)
		{
			require_update = true;
			for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
				(*it)->GetCredit() -= ile;
		}
	}
	else
		pc->credit = 0;

	if(!ignore && require_update && Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

void Game::StartDialog2(PlayerController* player, Unit* talker, GameDialog* dialog)
{
	assert(player && talker);

	DialogContext& ctx = *player->dialog_ctx;
	assert(!ctx.dialog_mode);

	if(player != pc)
	{
		NetChangePlayer& c = Add1(player->player_info->changes);
		c.type = NetChangePlayer::START_DIALOG;
		c.id = talker->netid;
	}
	StartDialog(ctx, talker, dialog);
}

DialogContext* Game::FindDialogContext(Unit* talker)
{
	assert(talker);
	if(dialog_context.talker == talker)
		return &dialog_context;
	if(Net::IsOnline())
	{
		for(PlayerInfo* player : game_players)
		{
			if(player->pc->dialog_ctx->talker == talker)
				return player->pc->dialog_ctx;
		}
	}
	return nullptr;
}

void Game::CreateUnitPhysics(Unit& unit, bool position)
{
	btCapsuleShape* caps = new btCapsuleShape(unit.GetUnitRadius(), max(MIN_H, unit.GetUnitHeight()));
	unit.cobj = new btCollisionObject;
	unit.cobj->setCollisionShape(caps);
	unit.cobj->setUserPointer(&unit);
	unit.cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_UNIT);

	if(position)
	{
		Vec3 pos = unit.pos;
		pos.y += unit.GetUnitHeight();
		btVector3 bpos(ToVector3(unit.IsAlive() ? pos : Vec3(1000, 1000, 1000)));
		bpos.setY(unit.pos.y + max(MIN_H, unit.GetUnitHeight()) / 2);
		unit.cobj->getWorldTransform().setOrigin(bpos);
	}

	phy_world->addCollisionObject(unit.cobj, CG_UNIT);
}

void Game::UpdateUnitPhysics(Unit& unit, const Vec3& pos)
{
	btVector3 a_min, a_max, bpos(ToVector3(unit.IsAlive() ? pos : Vec3(1000, 1000, 1000)));
	bpos.setY(pos.y + max(MIN_H, unit.GetUnitHeight()) / 2);
	unit.cobj->getWorldTransform().setOrigin(bpos);
	unit.cobj->getCollisionShape()->getAabb(unit.cobj->getWorldTransform(), a_min, a_max);
	phy_broadphase->setAabb(unit.cobj->getBroadphaseHandle(), a_min, a_max, phy_dispatcher);
}

void Game::WarpNearLocation(LevelContext& ctx, Unit& unit, const Vec3& pos, float extra_radius, bool allow_exact, int tries)
{
	const float radius = unit.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, extra_radius + radius, &ignore);

	Vec3 tmp_pos = pos;
	if(!allow_exact)
		tmp_pos += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;

	for(int i = 0; i < tries; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
			break;

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
	}

	unit.pos = tmp_pos;
	MoveUnit(unit, true);
	unit.visual_pos = unit.pos;

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->player_info->warping = true;
	}

	if(unit.cobj)
		UpdateUnitPhysics(unit, unit.pos);
}

/* mode: 0 - normal training
1 - gain 1 point (tutorial)
2 - more points (potion) */
void Game::Train(Unit& unit, bool is_skill, int co, int mode)
{
	int value, *train_points, *train_next;
	if(is_skill)
	{
		if(unit.unmod_stats.skill[co] == Skill::MAX)
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
		if(unit.unmod_stats.attrib[co] == Attribute::MAX)
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
		ile = 10 - value / 10;
	else if(mode == 1)
		ile = 1;
	else
		ile = 12 - value / 12;

	if(ile >= 1)
	{
		value += ile;
		*train_points /= 2;

		if(is_skill)
		{
			*train_next = GetRequiredSkillPoints(value);
			unit.Set((SkillId)co, value);
		}
		else
		{
			*train_next = GetRequiredAttributePoints(value);
			unit.Set((AttributeId)co, value);
		}

		if(unit.player->IsLocal())
			ShowStatGain(is_skill, co, ile);
		else
		{
			NetChangePlayer& c = Add1(unit.player->player_info->changes);
			c.type = NetChangePlayer::GAIN_STAT;
			c.id = (is_skill ? 1 : 0);
			c.a = co;
			c.ile = ile;

			NetChangePlayer& c2 = Add1(unit.player->player_info->changes);
			c2.type = NetChangePlayer::STAT_CHANGED;
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
			unit.player->TrainMod2((SkillId)co, pts);
		else
			unit.player->TrainMod((AttributeId)co, pts);
	}
}

void Game::ShowStatGain(bool is_skill, int what, int value)
{
	cstring text, name;
	if(is_skill)
	{
		text = txGainTextSkill;
		name = Skill::skills[what].name.c_str();
	}
	else
	{
		text = txGainTextAttrib;
		name = Attribute::attributes[what].name.c_str();
	}

	AddGameMsg(Format(text, name, value), 3.f);
}

void Game::ActivateChangeLeaderButton(bool activate)
{
	game_gui->team_panel->bt[2].state = (activate ? Button::NONE : Button::DISABLED);
}

// zmienia tylko pozycjê bo ta funkcja jest wywo³ywana przy opuszczaniu miasta
void Game::WarpToInn(Unit& unit)
{
	assert(L.city_ctx);

	int id;
	InsideBuilding* inn = L.city_ctx->FindInn(id);

	WarpToArea(inn->ctx, (Rand() % 5 == 0 ? inn->arena2 : inn->arena1), unit.GetUnitRadius(), unit.pos, 20);
	unit.visual_pos = unit.pos;
	unit.in_building = id;
}

void Game::PayCredit(PlayerController* player, int ile)
{
	LocalVector<Unit*> _units;
	vector<Unit*>& units = _units;

	for(Unit* unit : Team.active_members)
	{
		if(unit != player->unit)
			units.push_back(unit);
	}

	AddGold(ile, &units, false);

	player->credit -= ile;
	if(player->credit < 0)
	{
		Warn("Player '%s' paid %d credit and now have %d!", player->name.c_str(), ile, player->credit);
		player->credit = 0;
#ifdef _DEBUG
		AddGameMsg("Player has invalid credit!", 5.f);
#endif
	}

	if(Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_CREDIT);
}

InsideBuilding* Game::GetArena()
{
	assert(L.city_ctx);
	for(InsideBuilding* b : L.city_ctx->inside_buildings)
	{
		if(b->type->group == BuildingGroup::BG_ARENA)
			return b;
	}
	assert(0);
	return nullptr;
}

void Game::CreateSaveImage(cstring filename)
{
	assert(filename);

	SURFACE surf;
	V(tSave->GetSurfaceLevel(0, &surf));

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
		V(tSave->GetSurfaceLevel(0, &surf));
		V(device->StretchRect(sSave, nullptr, surf, nullptr, D3DTEXF_NONE));
		surf->Release();
	}

	// zapisz obrazek
	V(D3DXSaveSurfaceToFile(filename, D3DXIFF_JPG, surf, nullptr, nullptr));
	surf->Release();

	// przywróc render target
	V(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf));
	V(device->SetRenderTarget(0, surf));
	surf->Release();
}

void Game::PlayerUseUsable(Usable* usable, bool after_action)
{
	Unit& u = *pc->unit;
	Usable& use = *usable;
	BaseUsable& bu = *use.base;

	bool ok = true;
	if(bu.item)
	{
		if(!u.HaveItem(bu.item) && u.slots[SLOT_WEAPON] != bu.item)
		{
			AddGameMsg2(Format(txNeedItem, bu.item->name.c_str()), 2.f);
			ok = false;
		}
		else if(pc->unit->weapon_state != WS_HIDDEN && (bu.item != &pc->unit->GetWeapon() || pc->unit->HaveShield()))
		{
			if(after_action)
				return;
			u.HideWeapon();
			pc->next_action = NA_USE;
			pc->next_action_data.usable = &use;
			if(Net::IsClient())
				Net::PushChange(NetChange::SET_NEXT_ACTION);
			ok = false;
		}
		else
			u.used_item = bu.item;
	}

	if(!ok)
		return;

	if(Net::IsLocal())
	{
		u.UseUsable(&use);
		pc_data.before_player = BP_NONE;

		if(IS_SET(bu.use_flags, BaseUsable::CONTAINER))
		{
			// loot container
			pc->action = PlayerController::Action_LootContainer;
			pc->action_container = &use;
			pc->chest_trade = &pc->action_container->container->items;
			CloseGamePanels();
			inventory_mode = I_LOOT_CONTAINER;
			BuildTmpInventory(0);
			game_gui->inv_trade_mine->mode = Inventory::LOOT_MY;
			BuildTmpInventory(1);
			game_gui->inv_trade_other->unit = nullptr;
			game_gui->inv_trade_other->items = &pc->action_container->container->items;
			game_gui->inv_trade_other->slots = nullptr;
			game_gui->inv_trade_other->title = Format("%s - %s", Inventory::txLooting, use.base->name.c_str());
			game_gui->inv_trade_other->mode = Inventory::LOOT_OTHER;
			game_gui->gp_trade->Show();
		}
		else
		{
			u.action = A_ANIMATION2;
			u.animation = ANI_PLAY;
			u.mesh_inst->Play(bu.anim.c_str(), PLAY_PRIO1, 0);
			u.mesh_inst->groups[0].speed = 1.f;
			u.target_pos = u.pos;
			u.target_pos2 = use.pos;
			if(use.base->limit_rot == 4)
				u.target_pos2 -= Vec3(sin(use.rot)*1.5f, 0, cos(use.rot)*1.5f);
			u.timer = 0.f;
			u.animation_state = AS_ANIMATION2_MOVE_TO_OBJECT;
			u.use_rot = Vec3::LookAtAngle(u.pos, u.usable->pos);
		}

		if(Net::IsOnline())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::USE_USABLE;
			c.unit = &u;
			c.id = u.usable->netid;
			c.ile = USE_USABLE_START;
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.id = pc_data.before_player_ptr.usable->netid;
		c.ile = USE_USABLE_START;

		if(IS_SET(bu.use_flags, BaseUsable::CONTAINER))
		{
			pc->action = PlayerController::Action_LootContainer;
			pc->action_container = pc_data.before_player_ptr.usable;
			pc->chest_trade = &pc->action_container->container->items;
		}

		pc->unit->action = A_PREPARE;
	}
}

SOUND Game::GetTalkSound(Unit& u)
{
	if(IS_SET(u.data->flags2, F2_XAR))
		return sXarTalk;
	else if(u.data->type == UNIT_TYPE::HUMAN)
		return sTalk[Rand() % 4];
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
	assert(text && Net::IsLocal());

	game_gui->AddSpeechBubble(&u, text);

	int ani = 0;
	if(u.data->type == UNIT_TYPE::HUMAN && u.action == A_NONE && Rand() % 3 != 0)
	{
		ani = Rand() % 2 + 1;
		u.mesh_inst->Play(ani == 1 ? "i_co" : "pokazuje", PLAY_ONCE | PLAY_PRIO2, 0);
		u.mesh_inst->groups[0].speed = 1.f;
		u.animation = ANI_PLAY;
		u.action = A_ANIMATION;
	}

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
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
	if(QM.quest_orcs2->orcs_state == Quest_Orcs2::State::ToldAboutCamp && QM.quest_orcs2->target_loc == L.location_index
		&& QM.quest_orcs2->talked == Quest_Orcs2::Talked::No)
	{
		QM.quest_orcs2->talked = Quest_Orcs2::Talked::AboutCamp;
		talker = QM.quest_orcs2->orc;
		text = txOrcCamp;
	}

	if(!talker)
	{
		TeamInfo info;
		Team.GetTeamInfo(info);
		bool pewna_szansa = false;

		if(info.sane_heroes > 0)
		{
			switch(L.location->type)
			{
			case L_CITY:
				if(LocationHelper::IsCity(L.location))
					text = RandomString(txAiCity);
				else
					text = RandomString(txAiVillage);
				break;
			case L_MOONWELL:
				text = txAiMoonwell;
				break;
			case L_FOREST:
				text = txAiForest;
				break;
			case L_CAMP:
				if(L.location->state != LS_CLEARED)
				{
					pewna_szansa = true;
					cstring co;

					switch(L.location->spawn)
					{
					case SG_GOBLINS:
						co = txSGOGoblins;
						break;
					case SG_BANDITS:
						co = txSGOBandits;
						break;
					case SG_ORCS:
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

			if(text && (pewna_szansa || Rand() % 2 == 0))
				talker = Team.GetRandomSaneHero();
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
	Quest_Evil* quest_evil = QM.quest_evil;
	if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals || quest_evil->evil_state == Quest_Evil::State::KillBoss)
	{
		if(quest_evil->evil_state == Quest_Evil::State::ClosingPortals)
		{
			int d = quest_evil->GetLocId(L.location_index);
			if(d != -1)
			{
				Quest_Evil::Loc& loc = quest_evil->loc[d];

				if(L.dungeon_level == L.location->GetLastLevel())
				{
					if(loc.state < Quest_Evil::Loc::State::TalkedAfterEnterLevel)
					{
						talker = quest_evil->cleric;
						text = txPortalCloseLevel;
						loc.state = Quest_Evil::Loc::State::TalkedAfterEnterLevel;
					}
				}
				else if(L.dungeon_level == 0 && loc.state == Quest_Evil::Loc::State::None)
				{
					talker = quest_evil->cleric;
					text = txPortalClose;
					loc.state = Quest_Evil::Loc::State::TalkedAfterEnterLocation;
				}
			}
		}
		else if(L.location_index == quest_evil->target_loc && !quest_evil->told_about_boss)
		{
			quest_evil->told_about_boss = true;
			talker = quest_evil->cleric;
			text = txXarDanger;
		}
	}

	// orc talking after entering level
	Quest_Orcs2* quest_orcs2 = QM.quest_orcs2;
	if(!talker && (quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs || quest_orcs2->orcs_state == Quest_Orcs2::State::GeneratedOrcs) && L.location_index == quest_orcs2->target_loc)
	{
		if(L.dungeon_level == 0)
		{
			if(quest_orcs2->talked < Quest_Orcs2::Talked::AboutBase)
			{
				quest_orcs2->talked = Quest_Orcs2::Talked::AboutBase;
				talker = quest_orcs2->orc;
				text = txGorushDanger;
			}
		}
		else if(L.dungeon_level == L.location->GetLastLevel())
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
	Quest_Mages2* quest_mages2 = QM.quest_mages2;
	if(!talker && (quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined || quest_mages2->mages_state == Quest_Mages2::State::MageRecruited))
	{
		if(quest_mages2->target_loc == L.location_index)
		{
			if(quest_mages2->mages_state == Quest_Mages2::State::OldMageJoined)
			{
				if(L.dungeon_level == 0 && quest_mages2->talked == Quest_Mages2::Talked::No)
				{
					quest_mages2->talked = Quest_Mages2::Talked::AboutHisTower;
					text = txMageHere;
				}
			}
			else
			{
				if(L.dungeon_level == 0)
				{
					if(quest_mages2->talked < Quest_Mages2::Talked::AfterEnter)
					{
						quest_mages2->talked = Quest_Mages2::Talked::AfterEnter;
						text = Format(txMageEnter, quest_mages2->evil_mage_name.c_str());
					}
				}
				else if(L.dungeon_level == L.location->GetLastLevel() && quest_mages2->talked < Quest_Mages2::Talked::BeforeBoss)
				{
					quest_mages2->talked = Quest_Mages2::Talked::BeforeBoss;
					text = txMageFinal;
				}
			}
		}

		if(text)
			talker = Team.FindTeamMember("q_magowie_stary");
	}

	// default talking about location
	if(!talker && L.dungeon_level == 0 && (L.enter_from == ENTER_FROM_OUTSIDE || L.enter_from >= ENTER_FROM_PORTAL))
	{
		TeamInfo info;
		Team.GetTeamInfo(info);

		if(info.sane_heroes > 0)
		{
			LocalString s;

			switch(L.location->type)
			{
			case L_DUNGEON:
			case L_CRYPT:
				{
					InsideLocation* inside = (InsideLocation*)L.location;
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

					if(inside->spawn == SG_NONE)
						s += txAiNoEnemies;
					else
					{
						cstring co;

						switch(inside->spawn)
						{
						case SG_GOBLINS:
							co = txSGOGoblins;
							break;
						case SG_ORCS:
							co = txSGOOrcs;
							break;
						case SG_BANDITS:
							co = txSGOBandits;
							break;
						case SG_UNDEAD:
						case SG_NECROMANCERS:
						case SG_EVIL:
							co = txSGOUndead;
							break;
						case SG_MAGES:
							co = txSGOMages;
							break;
						case SG_GOLEMS:
							co = txSGOGolems;
							break;
						case SG_MAGES_AND_GOLEMS:
							co = txSGOMagesAndGolems;
							break;
						case SG_UNKNOWN:
							co = txSGOUnk;
							break;
						case SG_CHALLANGE:
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

			UnitTalk(*Team.GetRandomSaneHero(), s->c_str());
			return;
		}
	}

	if(talker)
		UnitTalk(*talker, text);
}

Unit* Game::GetRandomArenaHero()
{
	LocalVector<Unit*> v;

	for(vector<Unit*>::iterator it = at_arena.begin(), end = at_arena.end(); it != end; ++it)
	{
		if((*it)->IsPlayer() || (*it)->IsHero())
			v->push_back(*it);
	}

	return v->at(Rand() % v->size());
}

cstring Game::GetRandomIdleText(Unit& u)
{
	if(IS_SET(u.data->flags3, F3_DRUNK_MAGE) && QM.quest_mages2->mages_state < Quest_Mages2::State::MageCured)
		return RandomString(txAiDrunkMageText);

	int n = Rand() % 100;
	if(n == 0)
		return RandomString(txAiSecretText);

	int type = 1; // 0 - tekst hero, 1 - normalny tekst

	switch(u.data->group)
	{
	case G_CRAZIES:
		if(u.IsTeamMember())
		{
			if(n < 33)
				return RandomString(txAiInsaneText);
			else if(n < 66)
				type = 0;
			else
				type = 1;
		}
		else
		{
			if(n < 50)
				return RandomString(txAiInsaneText);
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
				if(L.city_ctx->FindInn(id) && id == u.in_building)
				{
					if(IS_SET(u.data->flags, F_AI_DRUNKMAN) || QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_STARTING)
					{
						if(Rand() % 3 == 0)
							return RandomString(txAiDrunkText);
					}
					else
						return RandomString(txAiDrunkmanText);
				}
			}
			if(n < 10)
				return RandomString(txAiHumanText);
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
			return RandomString(txAiBanditText);
		else
			type = 1;
		break;
	case G_MAGES:
		if(IS_SET(u.data->flags, F_MAGE) && n < 50)
			return RandomString(txAiMageText);
		else
			type = 1;
		break;
	case G_GOBLINS:
		if(n < 50 && !IS_SET(u.data->flags2, F2_NOT_GOBLIN))
			return RandomString(txAiGoblinText);
		else
			type = 1;
		break;
	case G_ORCS:
		if(n < 50)
			return RandomString(txAiOrcText);
		else
			type = 1;
		break;
	}

	if(type == 0)
	{
		if(L.location->type == L_CITY)
			return RandomString(txAiHeroCityText);
		else if(L.location->outside)
			return RandomString(txAiHeroOutsideText);
		else
			return RandomString(txAiHeroDungeonText);
	}
	else
	{
		n = Rand() % 100;
		if(n < 60)
			return RandomString(txAiDefaultText);
		else if(L.location->outside)
			return RandomString(txAiOutsideText);
		else
			return RandomString(txAiInsideText);
	}
}

// usuwa podany przedmiot ze œwiata
// u¿ywane w queœcie z kamieniem szaleñców
bool Game::RemoveItemFromWorld(const Item* item)
{
	assert(item);

	if(L.local_ctx.RemoveItemFromWorld(item))
		return true;

	if(L.city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = L.city_ctx->inside_buildings.begin(), end = L.city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if((*it)->ctx.RemoveItemFromWorld(item))
				return true;
		}
	}

	return false;
}

const Item* Game::GetRandomItem(int max_value)
{
	int type = Rand() % 6;

	LocalVector<const Item*> items;

	switch(type)
	{
	case 0:
		for(Weapon* w : Weapon::weapons)
		{
			if(w->value <= max_value && w->CanBeGenerated())
				items->push_back(w);
		}
		break;
	case 1:
		for(Bow* b : Bow::bows)
		{
			if(b->value <= max_value && b->CanBeGenerated())
				items->push_back(b);
		}
		break;
	case 2:
		for(Shield* s : Shield::shields)
		{
			if(s->value <= max_value && s->CanBeGenerated())
				items->push_back(s);
		}
		break;
	case 3:
		for(Armor* a : Armor::armors)
		{
			if(a->value <= max_value && a->CanBeGenerated())
				items->push_back(a);
		}
		break;
	case 4:
		for(Consumable* c : Consumable::consumables)
		{
			if(c->value <= max_value && c->CanBeGenerated())
				items->push_back(c);
		}
		break;
	case 5:
		for(OtherItem* o : OtherItem::others)
		{
			if(o->value <= max_value && o->CanBeGenerated())
				items->push_back(o);
		}
		break;
	}

	return items->at(Rand() % items->size());
}

bool Game::CheckMoonStone(GroundItem* item, Unit& unit)
{
	assert(item);

	Quest_Secret* secret = QM.quest_secret;
	if(secret->state == Quest_Secret::SECRET_NONE && L.location->type == L_MOONWELL && item->item->id == "krystal"
		&& Vec3::Distance2d(item->pos, Vec3(128.f, 0, 128.f)) < 1.2f)
	{
		AddGameMsg(txSecretAppear, 3.f);
		secret->state = Quest_Secret::SECRET_DROPPED_STONE;
		Location& l = *W.CreateLocation(L_DUNGEON, Vec2(0, 0), -128.f, DWARF_FORT, SG_CHALLANGE, false, 3);
		l.st = 18;
		l.active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
		l.state = LS_UNKNOWN;
		secret->where = l.index;
		Vec2& cpos = L.location->pos;
		Item* note = GetSecretNote();
		note->desc = Format("\"%c %d km, %c %d km\"", cpos.y > l.pos.y ? 'S' : 'N', (int)abs((cpos.y - l.pos.y) / 3), cpos.x > l.pos.x ? 'W' : 'E', (int)abs((cpos.x - l.pos.x) / 3));
		unit.AddItem(note);
		delete item;
		if(Net::IsOnline())
			Net::PushChange(NetChange::SECRET_TEXT);
		return true;
	}

	return false;
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
		if(Team.GetActiveTeamSize() == 1u)
			Team.active_members[0]->MakeItemsTeam(false);
		Team.active_members.push_back(unit);
	}
	Team.members.push_back(unit);

	// set items as not team
	unit->MakeItemsTeam(false);

	// update TeamPanel if open
	if(game_gui->team_panel->visible)
		game_gui->team_panel->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::RECRUIT_NPC;
		c.unit = unit;
	}

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
		RemoveElementOrder(Team.active_members, unit);
	RemoveElementOrder(Team.members, unit);

	// set items as team
	unit->MakeItemsTeam(true);

	// update TeamPanel if open
	if(game_gui->team_panel->visible)
		game_gui->team_panel->Changed();

	// send info to other players
	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->netid;
	}

	if(unit->event_handler)
		unit->event_handler->HandleUnitEvent(UnitEventHandler::KICK, unit);
}

void Game::DropGold(int ile)
{
	pc->unit->gold -= ile;
	sound_mgr->PlaySound2d(sCoins);

	// animacja wyrzucania
	pc->unit->action = A_ANIMATION;
	pc->unit->mesh_inst->Play("wyrzuca", PLAY_ONCE | PLAY_PRIO2, 0);
	pc->unit->mesh_inst->groups[0].speed = 1.f;
	pc->unit->mesh_inst->frame_end_info = false;

	if(Net::IsLocal())
	{
		// stwórz przedmiot
		GroundItem* item = new GroundItem;
		item->item = Item::gold;
		item->count = ile;
		item->team_count = 0;
		item->pos = pc->unit->pos;
		item->pos.x -= sin(pc->unit->rot)*0.25f;
		item->pos.z -= cos(pc->unit->rot)*0.25f;
		item->rot = Random(MAX_ANGLE);
		AddGroundItem(L.GetContext(*pc->unit), item);

		// wyœlij info o animacji
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::DROP_ITEM;
			c.unit = pc->unit;
		}
	}
	else
	{
		// wyœlij komunikat o wyrzucaniu z³ota
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::DROP_GOLD;
		c.id = ile;
	}
}

void Game::AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// dodaj przedmiot
	unit.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && Net::IsServer())
	{
		if(unit.IsPlayer())
		{
			if(unit.player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(unit.player->player_info->changes);
				c.type = NetChangePlayer::ADD_ITEMS;
				c.item = item;
				c.ile = count;
				c.id = team_count;
			}
		}
		else
		{
			// sprawdŸ czy ta jednostka nie wymienia siê przedmiotami z graczem
			Unit* u = nullptr;
			for(Unit* member : Team.active_members)
			{
				if(member->IsPlayer() && member->player->IsTradingWith(&unit))
				{
					u = member;
					break;
				}
			}

			if(u && u->player != pc)
			{
				// wyœlij komunikat do gracza z aktualizacj¹ ekwipunku
				NetChangePlayer& c = Add1(u->player->player_info->changes);
				c.type = NetChangePlayer::ADD_ITEMS_TRADER;
				c.item = item;
				c.id = unit.netid;
				c.ile = count;
				c.a = team_count;
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
}

void Game::AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg)
{
	assert(item && count && team_count <= count);

	// dodaj przedmiot
	chest.AddItem(item, count, team_count);

	// komunikat
	if(send_msg && chest.looted)
	{
		Unit* u = FindChestUserIfPlayer(&chest);
		if(u)
		{
			// dodaj komunikat o dodaniu przedmiotu do skrzyni
			NetChangePlayer& c = Add1(u->player->player_info->changes);
			c.type = NetChangePlayer::ADD_ITEMS_CHEST;
			c.item = item;
			c.id = chest.netid;
			c.ile = count;
			c.a = team_count;
		}
	}

	// czy trzeba przebudowaæ tymczasowy ekwipunek
	if(game_gui->gp_trade->visible && pc->action == PlayerController::Action_LootChest && pc->action_chest == &chest)
		BuildTmpInventory(1);
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
		if(pc->action == PlayerController::Action_LootChest
			|| pc->action == PlayerController::Action_Trade
			|| pc->action == PlayerController::Action_LootContainer)
			slots = nullptr;
		else
			slots = pc->action_unit->slots;
		items = pc->chest_trade;
	}

	ids.clear();

	// jeœli to postaæ to dodaj za³o¿one przedmioty
	if(slots)
	{
		for(int i = 0; i < SLOT_MAX; ++i)
		{
			if(slots[i])
			{
				ids.push_back(-i - 1);
				++shift;
			}
		}
	}

	// nie za³o¿one przedmioty
	for(int i = 0, ile = (int)items->size(); i < ile; ++i)
		ids.push_back(i);
}

Unit* Game::FindChestUserIfPlayer(Chest* chest)
{
	assert(chest && chest->looted);

	for(Unit* unit : Team.active_members)
	{
		if(unit->IsPlayer() && unit->player->action == PlayerController::Action_LootChest && unit->player->action_chest == chest)
			return unit;
	}

	return nullptr;
}

void Game::RemoveItem(Unit& unit, int i_index, uint count)
{
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
			unit.items.erase(unit.items.begin() + i_index);
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

		if(Net::IsServer() && players > 1)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::CHANGE_EQUIPMENT;
			c.unit = &unit;
			c.id = type;
		}
	}

	// komunikat
	if(Net::IsServer())
	{
		if(unit.IsPlayer())
		{
			if(!unit.player->is_local)
			{
				// dodaj komunikat o usuniêciu przedmiotu
				NetChangePlayer& c = Add1(unit.player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS;
				c.id = i_index;
				c.ile = count;
			}
		}
		else
		{
			Unit* t = nullptr;

			// szukaj gracza który handluje z t¹ postaci¹
			for(Unit* member : Team.active_members)
			{
				if(member->IsPlayer() && member->player->IsTradingWith(&unit))
				{
					t = member;
					break;
				}
			}

			if(t && t->player != pc)
			{
				// dodaj komunikat o dodaniu przedmiotu
				NetChangePlayer& c = Add1(t->player->player_info->changes);
				c.type = NetChangePlayer::REMOVE_ITEMS_TRADER;
				c.id = unit.netid;
				c.ile = count;
				c.a = i_index;
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
}

bool Game::RemoveItem(Unit& unit, const Item* item, uint count)
{
	int i_index = unit.FindItem(item);
	if(i_index == Unit::INVALID_IINDEX)
		return false;
	RemoveItem(unit, i_index, count);
	return true;
}

Int2 Game::GetSpawnPoint()
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	if(L.enter_from >= ENTER_FROM_PORTAL)
		return pos_to_pt(inside->GetPortal(L.enter_from)->GetSpawnPos());
	else if(L.enter_from == ENTER_FROM_DOWN_LEVEL)
		return lvl.GetDownStairsFrontTile();
	else
		return lvl.GetUpStairsFrontTile();
}

InsideLocationLevel* Game::TryGetLevelData()
{
	if(L.location->type == L_DUNGEON || L.location->type == L_CRYPT || L.location->type == L_CAVE)
		return &((InsideLocation*)L.location)->GetLevelData();
	else
		return nullptr;
}

GroundItem* Game::SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item)
{
	assert(item);
	while(true)
	{
		int id = Rand() % lvl.rooms.size();
		if(!lvl.rooms[id].IsCorridor())
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(lvl.rooms[id], item);
			if(item2)
				return item2;
		}
	}
}

Unit* Game::SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& ud, int level, const Int2& pt, const Int2& pt2)
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

Unit* Game::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn, int flags)
{
	if(!inn)
		inn = L.city_ctx->FindInn();

	Vec3 pos;
	bool ok = false;
	if(IS_SET(flags, SU_MAIN_ROOM) || Rand() % 5 != 0)
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
		float rot = Random(MAX_ANGLE);
		Unit* u = CreateUnitWithAI(inn->ctx, ud, level, nullptr, &pos, &rot);
		if(u && IS_SET(flags, SU_TEMPORARY))
			u->temporary = true;
		return u;
	}
	else
		return nullptr;
}

void Game::SpawnDrunkmans()
{
	InsideBuilding* inn = L.city_ctx->FindInn();
	QM.quest_contest->generated = true;
	UnitData& pijak = *UnitData::Get("pijak");
	int ile = Random(4, 6);
	for(int i = 0; i < ile; ++i)
	{
		Unit* u = SpawnUnitInsideInn(pijak, Random(2, 15), inn, SU_TEMPORARY | SU_MAIN_ROOM);
		if(u && Net::IsOnline())
			Net_SpawnUnit(u);
	}
}

void Game::SetOutsideParams()
{
	cam.draw_range = 80.f;
	clear_color2 = Color::White;
	fog_params = Vec4(40, 80, 40, 0);
	fog_color = Vec4(0.9f, 0.85f, 0.8f, 1);
	ambient_color = Vec4(0.5f, 0.5f, 0.5f, 1);
}

void Game::OnEnterLevelOrLocation()
{
	ClearGui(false);
	lights_dt = 1.f;
	pc_data.autowalk = false;
	fallback_t = -0.5f;
	fallback_type = FALLBACK::NONE;
	if(Net::IsLocal())
	{
		for(auto unit : Team.members)
			unit->frozen = FROZEN::NO;
	}
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
	case I_LOOT_CONTAINER:
		my.mode = Inventory::LOOT_MY;
		other.mode = Inventory::LOOT_OTHER;
		other.unit = nullptr;
		other.title = Format("%s - %s", Inventory::txLooting, pc->action_container->base->name.c_str());
		break;
	default:
		assert(0);
		break;
	}

	BuildTmpInventory(0);
	BuildTmpInventory(1);
	game_gui->gp_trade->Show();
}

void Game::ShowAcademyText()
{
	if(GUI.GetDialog("academy") == nullptr)
		GUI.SimpleDialog(txQuest[271], world_map, "academy");
	if(Net::IsServer())
		Net::PushChange(NetChange::ACADEMY_TEXT);
}

const float price_mod_buy[] = { 1.25f, 1.0f, 0.75f };
const float price_mod_sell[] = { 0.25f, 0.5f, 0.75f };
const float price_mod_buy_v[] = { 1.25f, 1.0f, 0.9f };
const float price_mod_sell_v[] = { 0.5f, 0.75f, 0.9f };

int Game::GetItemPrice(const Item* item, Unit& unit, bool buy)
{
	assert(item);

	int cha = unit.Get(AttributeId::CHA);
	const float* mod_table;

	if(item->type == IT_OTHER && item->ToOther().other_type == Valuable)
	{
		if(buy)
			mod_table = price_mod_buy_v;
		else
			mod_table = price_mod_sell_v;
	}
	else
	{
		if(buy)
			mod_table = price_mod_buy;
		else
			mod_table = price_mod_sell;
	}

	float mod;
	if(cha <= 1)
		mod = mod_table[0];
	else if(cha < 50)
		mod = Lerp(mod_table[0], mod_table[1], float(cha) / 50);
	else if(cha == 50)
		mod = mod_table[1];
	else if(cha < 100)
		mod = Lerp(mod_table[1], mod_table[2], float(cha - 50) / 50);
	else
		mod = mod_table[2];

	int price = int(mod * item->value);
	if(price == 0 && buy)
		price = 1;

	return price;
}

void Game::VerifyObjects()
{
	int errors = 0, e;

	for(Location* l : W.GetLocations())
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
				Error("%d errors in outside location '%s'.", e, outside->name.c_str());
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
						Error("%d errors in city '%s', building '%s'.", e, city->name.c_str(), ib->type->id.c_str());
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
						Error("%d errors in multi inside location '%s' at level %d.", e, m->name.c_str(), index);
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
					Error("%d errors in single inside location '%s'.", e, s->name.c_str());
					errors += e;
				}
			}
		}
	}

	if(errors > 0)
		throw Format("Veryify objects failed with %d errors. Check log for details.", errors);
}

void Game::VerifyObjects(vector<Object*>& objects, int& errors)
{
	for(Object* o : objects)
	{
		if(!o->mesh && !o->base)
		{
			Error("Broken object at (%g,%g,%g).", o->pos.x, o->pos.y, o->pos.z);
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
	if(L.local_ctx.type == LevelContext::Inside)
	{
		inside = (InsideLocation*)L.location;
		lvl = &inside->GetLevelData();
	}

	// spawn unit
	if(event->unit_to_spawn)
	{
		if(L.local_ctx.type == LevelContext::Outside)
		{
			if(L.location->type == L_CITY)
				spawned = SpawnUnitInsideInn(*event->unit_to_spawn, event->unit_spawn_level);
			else
			{
				Vec3 pos(0, 0, 0);
				int count = 0;
				for(Unit* unit : *L.local_ctx.units)
				{
					pos += unit->pos;
					++count;
				}
				pos /= (float)count;
				spawned = SpawnUnitNearLocation(L.local_ctx, pos, *event->unit_to_spawn, nullptr, event->unit_spawn_level);
			}
		}
		else
		{
			Room& room = lvl->GetRoom(event->spawn_unit_room, inside->HaveDownStairs());
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
			Info("Generated unit %s (%g,%g).", event->unit_to_spawn->id.c_str(), spawned->pos.x, spawned->pos.z);

		// mark near units as guards if guarded (only in dungeon)
		if(IS_SET(spawned->data->flags2, F2_GUARDED) && lvl)
		{
			Room& room = lvl->GetRoom(event->spawn_unit_room, inside->HaveDownStairs());
			for(Unit* unit : *L.local_ctx.units)
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
			room = &lvl->GetRoom(event->spawn_unit_room2, inside->HaveDownStairs());
		spawned2 = SpawnUnitInsideRoomOrNear(*lvl, *room, *event->unit_to_spawn2, event->unit_spawn_level2);
		if(!spawned2)
			throw "Failed to spawn quest unit 2!";
		if(devmode)
			Info("Generated unit %s (%g,%g).", event->unit_to_spawn2->id.c_str(), spawned2->pos.x, spawned2->pos.z);
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
			for(Unit* unit : *L.local_ctx.units)
			{
				if(unit->IsAlive() && IsEnemy(*unit, *pc->unit) && (!best || unit->level > best->level))
					best = unit;
			}
			assert(best);
			if(best)
			{
				best->AddItem(event->item_to_give[0], 1, true);
				if(devmode)
					Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), best->data->id.c_str(), best->pos.x, best->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned:
		assert(spawned);
		if(spawned)
		{
			spawned->AddItem(event->item_to_give[0], 1, true);
			if(devmode)
				Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned->data->id.c_str(), spawned->pos.x, spawned->pos.z);
		}
		break;
	case Quest_Dungeon::Item_GiveSpawned2:
		assert(spawned2);
		if(spawned2)
		{
			spawned2->AddItem(event->item_to_give[0], 1, true);
			if(devmode)
				Info("Given item %s unit %s (%g,%g).", event->item_to_give[0]->id.c_str(), spawned2->data->id.c_str(), spawned2->pos.x, spawned2->pos.z);
		}
		break;
	case Quest_Dungeon::Item_OnGround:
		{
			GroundItem* item;
			if(lvl)
				item = SpawnGroundItemInsideAnyRoom(*lvl, event->item_to_give[0]);
			else
				item = SpawnGroundItemInsideRadius(event->item_to_give[0], Vec2(128, 128), 10.f);
			if(devmode)
				Info("Generated item %s on ground (%g,%g).", event->item_to_give[0]->id.c_str(), item->pos.x, item->pos.z);
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
					Info("Generated item %s in treasure chest (%g,%g).", event->item_to_give[0]->id.c_str(), chest->pos.x, chest->pos.z);
			}
		}
		break;
	case Quest_Dungeon::Item_InChest:
		{
			Chest* chest = L.local_ctx.GetRandomFarChest(GetSpawnPoint());
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
				Info(str.get_ref().c_str());
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

const Item* Game::GetRandomBook()
{
	if(Rand() % 2 == 0)
		return nullptr;
	if(Rand() % 50 == 0)
		return ItemList::GetItem("rare_books");
	else
		return ItemList::GetItem("books");
}
