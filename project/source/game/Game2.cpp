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
#include "MultiInsideLocation.h"
#include "SingleInsideLocation.h"
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
#include "Quest_Tutorial.h"
#include "Debug.h"
#include "LocationGeneratorFactory.h"
#include "DungeonGenerator.h"
#include "Texture.h"
#include "Pathfinding.h"
#include "Arena.h"
#include "ResourceManager.h"
#include "ItemHelper.h"
#include "GlobalGui.h"
#include "FOV.h"
#include "PlayerInfo.h"
#include "CombatHelper.h"
#include "Quest_Scripted.h"
#include "Render.h"
#include "RenderTarget.h"

const float ALERT_RANGE = 20.f;
const float ALERT_SPAWN_RANGE = 25.f;
const float PICKUP_RANGE = 2.f;
const float TRAP_ARROW_SPEED = 45.f;
const float ARROW_TIMER = 5.f;
const float MIN_H = 1.5f; // hardcoded in GetPhysicsPos
const float TRAIN_KILL_RATIO = 0.1f;
const int NN = 64;
const float SMALL_DISTANCE = 0.001f;

Matrix m1, m2, m3, m4;

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
	while(true)
	{
		DrawItemImage(item, rt_item, 0.f);
		TEX tex = GetRender()->CopyToTexture(rt_item);
		if(tex)
			return tex;
	}
}

//=================================================================================================
void Game::DrawItemImage(const Item& item, RenderTarget* target, float rot)
{
	Render* render = GetRender();
	IDirect3DDevice9* device = render->GetDevice();

	if(IS_SET(ITEM_ALPHA, item.flags))
	{
		render->SetAlphaBlend(true);
		render->SetNoZWrite(true);
	}
	else
	{
		render->SetAlphaBlend(false);
		render->SetNoZWrite(false);
	}
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetTarget(target);

	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0));
	V(device->BeginScene());

	Mesh& mesh = *item.mesh;
	const TexId* tex_override = nullptr;
	if(item.type == IT_ARMOR)
	{
		tex_override = item.ToArmor().GetTextureOverride();
		if(tex_override)
		{
			assert(item.ToArmor().tex_override.size() == mesh.head.n_subs);
		}
	}

	Matrix matWorld;
	Mesh::Point* point = mesh.FindPoint("cam_rot");
	if(point)
		matWorld = Matrix::CreateFromAxisAngle(point->rot, rot);
	else
		matWorld = Matrix::RotationY(rot);
	Matrix matView = Matrix::CreateLookAt(mesh.head.cam_pos, mesh.head.cam_target, mesh.head.cam_up),
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

	uint passes;
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
	V(device->EndScene());

	render->SetTarget(nullptr);
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
		tout = L.terrain->Raytest(to, to + dist);
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
				Tile& p = lvl.map[x + z * lvl.w];
				if(IsBlocking(p.type))
				{
					const Box box(float(x) * 2, 0, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				else if(IS_SET(p.flags, Tile::F_LOW_CEILING))
				{
					const Box box(float(x) * 2, 3.f, float(z) * 2, float(x + 1) * 2, 4.f, float(z + 1) * 2);
					if(RayToBox(to, dist, box, &tout) && tout < min_tout && tout > 0.f)
						min_tout = tout;
				}
				if(p.type == STAIRS_UP)
				{
					if(vdSchodyGora->RayToMesh(to, dist, PtToPos(lvl.staircase_up), DirToRot(lvl.staircase_up_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == STAIRS_DOWN)
				{
					if(!lvl.staircase_down_in_wall
						&& vdSchodyDol->RayToMesh(to, dist, PtToPos(lvl.staircase_down), DirToRot(lvl.staircase_down_dir), tout) && tout < min_tout)
						min_tout = tout;
				}
				else if(p.type == DOORS || p.type == HOLE_FOR_DOORS)
				{
					Vec3 pos(float(x * 2) + 1, 0, float(z * 2) + 1);
					float rot;

					if(IsBlocking(lvl.map[x - 1 + z * lvl.w].type))
					{
						rot = 0;
						int mov = 0;
						if(lvl.rooms[lvl.map[x + (z - 1)*lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + (z + 1)*lvl.w].room]->IsCorridor())
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
						if(lvl.rooms[lvl.map[x - 1 + z * lvl.w].room]->IsCorridor())
							++mov;
						if(lvl.rooms[lvl.map[x + 1 + z * lvl.w].room]->IsCorridor())
							--mov;
						if(mov == 1)
							pos.x += 0.8229f;
						else if(mov == -1)
							pos.x -= 0.8229f;
					}

					if(vdNaDrzwi->RayToMesh(to, dist, pos, rot, tout) && tout < min_tout)
						min_tout = tout;

					Door* door = L.FindDoor(ctx, Int2(x, z));
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
	for(vector<CameraCollider>::iterator it = L.cam_colliders.begin(), end = L.cam_colliders.end(); it != end; ++it)
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

	Render* render = GetRender();
	eMesh = render->CompileShader("mesh.fx");
	eParticle = render->CompileShader("particle.fx");
	eSkybox = render->CompileShader("skybox.fx");
	eTerrain = render->CompileShader("terrain.fx");
	eArea = render->CompileShader("area.fx");
	ePostFx = render->CompileShader("post.fx");
	eGlow = render->CompileShader("glow.fx");
	eGrass = render->CompileShader("grass.fx");

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

	// sanity checks
#ifdef _DEBUG
	if(Net::IsLocal())
	{
		assert(pc->is_local);
		if(Net::IsServer())
		{
			for(PlayerInfo* info : N.players)
			{
				if(info->left == PlayerInfo::LEFT_NO)
				{
					assert(info->pc == info->pc->player_info->pc);
					if(info->id != 0)
						assert(!info->pc->is_local);
				}
			}
		}
	}
	else
	{
		assert(pc->is_local && pc == pc->player_info->pc);
	}
#endif

	L.minimap_opened_doors = false;

	if(QM.quest_tutorial->in_tutorial && !Net::IsOnline())
		QM.quest_tutorial->Update();

	drunk_anim = Clip(drunk_anim + dt);
	UpdatePostEffects(dt);

	portal_anim += dt;
	if(portal_anim >= 1.f)
		portal_anim -= 1.f;
	L.light_angle = Clip(L.light_angle + dt / 100);

	LevelContext& player_ctx = (pc->unit->in_building == -1 ? L.local_ctx : L.city_ctx->inside_buildings[pc->unit->in_building]->ctx);

	UpdateFallback(dt);
	if(!L.location)
		return;

	if(Net::IsLocal() && !QM.quest_tutorial->in_tutorial)
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
						pc->unit->rot = DirToRot(lvl.staircase_up_dir);
						L.WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
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
						pc->unit->rot = DirToRot(lvl.staircase_down_dir);
						L.WarpUnit(*pc->unit, Vec3(2.f*tile.x + 1.f, 0.f, 2.f*tile.y + 1.f));
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
		if(dialog_context.dialog_mode && gui->inventory->mode <= I_INVENTORY)
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
				cam.real_rot.y += -float(Key.GetMouseDif().y) * settings.mouse_sensitivity_f / div;
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
						cam.real_rot.x = Clip(cam.real_rot.x + float(Key.GetMouseDif().x) * settings.mouse_sensitivity_f / 400);
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
		if(!dialog_context.dialog_mode || !dialog_context.show_choices || !gui->game_gui->IsMouseInsideDialog())
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
	if(dialog_context.dialog_mode || pc->unit->look_target || gui->inventory->mode > I_INVENTORY)
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
		else if(gui->inventory->mode == I_LOOT_CHEST)
		{
			assert(pc->action == PlayerController::Action_LootChest);
			pos = pc->action_chest->pos;
			pc->unit->animation = ANI_KNEELS;
		}
		else if(gui->inventory->mode == I_LOOT_BODY)
		{
			assert(pc->action == PlayerController::Action_LootUnit);
			pos = pc->action_unit->GetLootCenter();
			pc->unit->animation = ANI_KNEELS;
		}
		else if(gui->inventory->mode == I_LOOT_CONTAINER)
		{
			// TODO: animacja
			assert(pc->action == PlayerController::Action_LootContainer);
			pos = pc->action_usable->pos;
			pc->unit->animation = ANI_STAND;
		}
		else if(dialog_context.dialog_mode)
		{
			pos = dialog_context.talker->pos;
			pc->unit->animation = ANI_STAND;
		}
		else
		{
			assert(pc->action == InventoryModeToActionRequired(gui->inventory->mode));
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

		if(gui->inventory->mode > I_INVENTORY)
		{
			if(gui->inventory->mode == I_LOOT_BODY)
			{
				if(pc->action_unit->IsAlive())
				{
					// grabiony cel o¿y³
					CloseInventory();
				}
			}
			else if(gui->inventory->mode == I_TRADE || gui->inventory->mode == I_SHARE || gui->inventory->mode == I_GIVE)
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
				dialog_context.EndDialog();
			}
		}

		pc_data.before_player = BP_NONE;
		pc_data.rot_buf = 0.f;
		pc_data.autowalk = false;
		pc_data.action_ready = false;
	}
	else if(!IsBlocking(pc->unit->action) && !pc->unit->HaveEffect(EffectId::Stun))
		UpdatePlayer(player_ctx, dt);
	else
	{
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
	L.UpdateDungeonMinimap(true);

	// aktualizuj dialogi
	if(Net::IsSingleplayer())
	{
		if(dialog_context.dialog_mode)
			dialog_context.Update(dt);
	}
	else if(Net::IsServer())
	{
		for(PlayerInfo* info : N.players)
		{
			if(info->left != PlayerInfo::LEFT_NO)
				continue;
			DialogContext& ctx = *info->u->player->dialog_ctx;
			if(ctx.dialog_mode)
			{
				if(!ctx.talker->IsStanding() || !ctx.talker->IsIdle() || ctx.talker->to_remove || ctx.talker->frozen != FROZEN::NO)
					ctx.EndDialog();
				else
					ctx.Update(dt);
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
		for(PlayerInfo* info : N.players)
		{
			if(info->left == PlayerInfo::LEFT_NO && info->pc != pc)
				info->pc->Update(dt, false);
		}
	}

	// aktualizuj kamerê
	SetupCamera(dt);
}

//=================================================================================================
void Game::UpdateFallback(float dt)
{
	if(fallback_type == FALLBACK::NO)
		return;

	dt /= game_speed;

	if(fallback_t <= 0.f)
	{
		fallback_t += dt * 2;

		if(fallback_t > 0.f)
		{
			switch(fallback_type)
			{
			case FALLBACK::TRAIN:
				if(fallback_1 == 3)
				{
					// learning perk
					gui->messages->AddGameMsg3(GMS_LEARNED_PERK);
					if(Net::IsClient())
					{
						fallback_type = FALLBACK::CLIENT2;
						fallback_t = 0.f;
					}
				}
				else if(Net::IsLocal())
				{
					if(fallback_1 == 2)
						QM.quest_tournament->Train(*pc);
					else
						pc->Train(fallback_1 == 1, fallback_2);
					pc->Rest(10, false);
					if(Net::IsOnline())
						pc->UseDays(10);
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
					c.count = fallback_2;
				}
				break;
			case FALLBACK::REST:
				if(Net::IsLocal())
				{
					pc->Rest(fallback_1, true);
					if(Net::IsOnline())
						pc->UseDays(fallback_1);
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
		u.TryStandup(dt);
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
		pc_data.rot_buf = 0.f;
		pc_data.action_ready = false;
	}

	u.prev_pos = u.pos;
	u.changed = true;

	bool idle = true;
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
			pc_data.rot_buf += float(Key.GetMouseDif().x) * settings.mouse_sensitivity_f / div;
			if(pc_data.rot_buf > 0.1f)
				pc_data.rot_buf = 0.1f;
			else if(pc_data.rot_buf < -0.1f)
				pc_data.rot_buf = -0.1f;
		}
		else
			pc_data.rot_buf = 0.f;

		const bool rotating = (rotate != 0 || pc_data.rot_buf != 0.f);

		u.running = false;
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
					u.animation = ANI_WALK_BACK;
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
						u.player->TrainMove(speed);

					// revealing minimap
					if(!L.location->outside)
					{
						Int2 new_tile(int(u.pos.x / 2), int(u.pos.z / 2));
						if(new_tile != prev_tile)
							FOV::DungeonReveal(new_tile, L.minimap_reveal);
					}
				}

				if(run && abs(u.speed - u.prev_speed) < 0.25f)
					u.running = true;
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
				WeaponType weapon = pc->last_weapon;

				// ustal któr¹ broñ wyj¹œæ
				if(weapon == W_NONE)
				{
					if(u.HaveWeapon())
						weapon = W_ONE_HANDED;
					else if(u.HaveBow())
						weapon = W_BOW;
				}
				else if(weapon == W_ONE_HANDED)
				{
					if(!u.HaveWeapon())
					{
						if(u.HaveBow())
							weapon = W_BOW;
						else
							weapon = W_NONE;
					}
				}
				else
				{
					if(!u.HaveBow())
					{
						if(u.HaveWeapon())
							weapon = W_ONE_HANDED;
						else
							weapon = W_NONE;
					}
				}

				if(weapon != W_NONE)
				{
					pc->last_weapon = weapon;
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
						u.mesh_inst->Play(u.GetTakeWeaponAnimation(weapon == W_ONE_HANDED), PLAY_ONCE | PLAY_PRIO1, 1);
						u.weapon_taken = weapon;
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
							pc->last_weapon = u.weapon_taken;
							u.weapon_state = WS_TAKEN;
							u.mesh_inst->Deactivate(1);
						}
						else
						{
							// schowa³ broñ za pas, zacznij wyci¹gaæ
							u.weapon_taken = u.weapon_hiding;
							u.weapon_hiding = W_NONE;
							pc->last_weapon = u.weapon_taken;
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
						u.mesh_inst->Play(u.GetTakeWeaponAnimation(weapon == W_ONE_HANDED), PLAY_ONCE | PLAY_BACK | PLAY_PRIO1, 1);
						u.weapon_hiding = weapon;
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
					gui->messages->AddGameMsg3(GMS_NEED_WEAPON);
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
				u.weapon_taken = pc->last_weapon = W_ONE_HANDED;
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
						pc->last_weapon = u.weapon_taken;
						u.weapon_state = WS_TAKEN;
						u.mesh_inst->Deactivate(1);
					}
					else
					{
						// schowa³ broñ za pas, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->last_weapon = u.weapon_taken;
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
						pc->last_weapon = u.weapon_taken = W_ONE_HANDED;
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->last_weapon = u.weapon_taken = W_ONE_HANDED;
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
					pc->last_weapon = u.weapon_taken = W_ONE_HANDED;
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
				u.weapon_taken = pc->last_weapon = W_BOW;
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
						pc->last_weapon = u.weapon_taken;
						u.weapon_state = WS_TAKEN;
						u.mesh_inst->Deactivate(1);
					}
					else
					{
						// schowa³ ³uk, zacznij wyci¹gaæ
						u.weapon_taken = u.weapon_hiding;
						u.weapon_hiding = W_NONE;
						pc->last_weapon = u.weapon_taken;
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
						pc->last_weapon = u.weapon_taken = W_BOW;
						u.mesh_inst->Play(NAMES::ani_take_bow, PLAY_ONCE | PLAY_PRIO1, 1);
					}
					else
					{
						// ju¿ wyj¹³ wiêc trzeba schowaæ i dodaæ info
						pc->last_weapon = u.weapon_taken = W_BOW;
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
					pc->last_weapon = u.weapon_taken = W_BOW;
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
				gui->messages->AddGameMsg3(GMS_NO_POTION);
		}
	} // allow_input == ALLOW_INPUT || allow_input == ALLOW_KEYBOARD

	if(u.usable)
		return;

	// check what is in front of player
	pc_data.before_player = BP_NONE;
	float dist, best_dist = 3.0f;

	// units in front of player
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		dist = Vec3::Distance2d(u.visual_pos, u2.visual_pos);

		if(u2.IsStanding())
			PlayerCheckObjectDistance(u, u2.visual_pos, &u2, best_dist, BP_UNIT);
		else if(u2.live_state == Unit::FALL || u2.live_state == Unit::DEAD)
			PlayerCheckObjectDistance(u, u2.GetLootCenter(), &u2, best_dist, BP_UNIT);
	}

	// chests in front of player
	if(ctx.chests && !ctx.chests->empty())
	{
		for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_CHEST);
	}

	// doors in front of player
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if(OR2_EQ((*it)->state, Door::Open, Door::Closed))
				PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_DOOR);
		}
	}

	// ground items in front of player
	for(vector<GroundItem*>::iterator it = ctx.items->begin(), end = ctx.items->end(); it != end; ++it)
		PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_ITEM);

	// usable objects in front of player
	for(vector<Usable*>::iterator it = ctx.usables->begin(), end = ctx.usables->end(); it != end; ++it)
	{
		if(!(*it)->user)
			PlayerCheckObjectDistance(u, (*it)->pos, *it, best_dist, BP_USABLE);
	}

	// use something in front of player
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
					gui->messages->AddGameMsg3(GMS_CANT_DO);
				}
				else if(u2->IsFollower() || u2->IsPlayer())
				{
					// nie mo¿na okradaæ sojuszników
					gui->messages->AddGameMsg3(GMS_DONT_LOOT_FOLLOWER);
				}
				else if(u2->in_arena != -1)
					gui->messages->AddGameMsg3(GMS_DONT_LOOT_ARENA);
				else if(Net::IsLocal())
				{
					if(Net::IsOnline() && u2->busy == Unit::Busy_Looted)
					{
						// ktoœ ju¿ ograbia zw³oki
						gui->messages->AddGameMsg3(GMS_IS_LOOTED);
					}
					else
					{
						// rozpoczynij wymianê przedmiotów
						pc->action = PlayerController::Action_LootUnit;
						pc->action_unit = u2;
						u2->busy = Unit::Busy_Looted;
						pc->chest_trade = &u2->items;
						gui->inventory->StartTrade(I_LOOT_BODY, *u2);
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
			else if(u2->IsAI() && u2->IsIdle() && u2->in_arena == -1 && u2->data->dialog && !u.IsEnemy(*u2))
			{
				if(Net::IsLocal())
				{
					if(u2->busy != Unit::Busy_No || !u2->CanTalk())
					{
						// osoba jest czymœ zajêta
						gui->messages->AddGameMsg3(GMS_UNIT_BUSY);
					}
					else
					{
						// rozpocznij rozmowê
						dialog_context.StartDialog(u2);
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
				if(pc_data.before_player_ptr.chest->user)
				{
					// ktoœ ju¿ zajmuje siê t¹ skrzyni¹
					gui->messages->AddGameMsg3(GMS_IS_LOOTED);
				}
				else
				{
					// rozpocznij ograbianie skrzyni
					gui->inventory->StartTrade2(I_LOOT_CHEST, pc_data.before_player_ptr.chest);

					// animacja / dŸwiêk
					pc->action_chest->mesh_inst->Play(&pc->action_chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END, 0);
					sound_mgr->PlaySound3d(sChestOpen, pc->action_chest->GetCenter(), Chest::SOUND_DIST);

					// event handler
					if(pc_data.before_player_ptr.chest->handler)
						pc_data.before_player_ptr.chest->handler->HandleChestEvent(ChestEventHandler::Opened, pc->action_chest);

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
						L.minimap_opened_doors = true;
					door->state = Door::Opening;
					door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
					door->mesh_inst->frame_end_info = false;
					if(Rand() % 2 == 0)
						sound_mgr->PlaySound3d(sDoor[Rand() % 3], door->GetCenter(), Door::SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::USE_DOOR;
						c.id = door->netid;
						c.count = 0;
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
						sound_mgr->PlaySound3d(sUnlock, center, Door::UNLOCK_SOUND_DIST);
						gui->messages->AddGameMsg3(GMS_UNLOCK_DOOR);
						if(!L.location->outside)
							L.minimap_opened_doors = true;
						door->locked = LOCK_NONE;
						door->state = Door::Opening;
						door->mesh_inst->Play(&door->mesh_inst->mesh->anims[0], PLAY_ONCE | PLAY_STOP_AT_END | PLAY_NO_BLEND, 0);
						door->mesh_inst->frame_end_info = false;
						if(Rand() % 2 == 0)
							sound_mgr->PlaySound3d(sDoor[Rand() % 3], center, Door::SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::USE_DOOR;
							c.id = door->netid;
							c.count = 0;
						}
					}
					else
					{
						gui->messages->AddGameMsg3(GMS_NEED_KEY);
						sound_mgr->PlaySound3d(sDoorClosed[Rand() % 2], center, Door::SOUND_DIST);
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
					sound_mgr->PlaySound3d(snd, door->GetCenter(), Door::SOUND_DIST);
				}
				if(Net::IsOnline())
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::USE_DOOR;
					c.id = door->netid;
					c.count = 1;
				}
			}
		}
		else if(pc_data.before_player == BP_ITEM)
		{
			// podnieœ przedmiot
			GroundItem& item = *pc_data.before_player_ptr.item;
			if(u.action == A_NONE)
			{
				bool up_anim = (item.pos.y > u.pos.y + 0.5f);

				u.action = A_PICKUP;
				u.animation = ANI_PLAY;
				u.mesh_inst->Play(up_anim ? "podnosi_gora" : "podnosi", PLAY_ONCE | PLAY_PRIO2, 0);
				u.mesh_inst->groups[0].speed = 1.f;
				u.mesh_inst->frame_end_info = false;

				if(Net::IsLocal())
				{
					AddItem(u, item.item, item.count, item.team_count);

					if(item.item->type == IT_GOLD)
						sound_mgr->PlaySound2d(sCoins);

					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::PICKUP_ITEM;
						c.unit = pc->unit;
						c.count = (up_anim ? 1 : 0);

						NetChange& c2 = Add1(Net::changes);
						c2.type = NetChange::REMOVE_ITEM;
						c2.id = item.netid;
					}

					RemoveElement(ctx.items, &item);
					pc_data.before_player = BP_NONE;

					for(Event& event : L.location->events)
					{
						if(event.type == EVENT_PICKUP)
						{
							ScriptEvent e(EVENT_PICKUP);
							e.item = &item;
							event.quest->FireEvent(e);
						}
					}

					delete &item;
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
		pc_data.target_unit = pc_data.before_player_ptr.unit;
	else
		pc_data.target_unit = nullptr;

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
					if(u.running)
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
					if(u.attack_power > 1.5f && u.animation_state == 1)
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
		if(u.frozen == FROZEN::NO && Any(u.action, A_NONE, A_ATTACK, A_BLOCK, A_BASH) && GKey.KeyPressedReleaseAllowed(GK_ACTION) && pc->CanUseAction()
			&& !QM.quest_tutorial->in_tutorial)
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

	Action& action = p->GetAction();
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
		if(dash && Net::IsLocal())
			p->Train(TrainWhat::Dash, 0.f, 0);
		p->unit->action = A_DASH;
		p->unit->run_attack = false;
		if(Net::IsLocal() || !from_server)
		{
			p->unit->use_rot = Clip(pc_data.action_rot + p->unit->rot + PI);
			action_point = Vec3(pc_data.action_rot, 0, 0);
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
				Team.RemoveTeamMember(existing_unit);
				L.RemoveUnit(existing_unit);
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
			Unit* unit = L.SpawnUnitNearLocation(L.GetContext(*p->unit), spawn_pos, *UnitData::Get("white_wolf_sum"), nullptr, p->unit->level);
			if(unit)
			{
				unit->summoner = p->unit;
				unit->in_arena = p->unit->in_arena;
				if(unit->in_arena != -1)
					arena->units.push_back(unit);
				Team.AddTeamMember(unit, true);
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
	sound_mgr->PlaySound3d(sSummon, real_pos, SPAWN_SOUND_DIST);

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
			if(dist < best_dist && L.CanSee(L.GetContext(u), u.pos, pos, type == BP_DOOR))
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
	L.global_col.clear();

	Level::IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { _me, nullptr };
	ignore.ignored_units = (const Unit**)ignored;
	L.GatherCollisionObjects(L.GetContext(*_me), L.global_col, gather_pos, gather_radius, &ignore);

	if(L.global_col.empty())
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ prosto po x i z
	if(!L.Collide(L.global_col, new_pos, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos) < SMALL_DISTANCE);
		_pos = new_pos;
		return 3;
	}

	// idŸ po x
	Vec3 new_pos2 = _me->pos;
	new_pos2.x = new_pos.x;
	if(!L.Collide(L.global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 1;
	}

	// idŸ po z
	new_pos2.x = _me->pos.x;
	new_pos2.z = new_pos.z;
	if(!L.Collide(L.global_col, new_pos2, _radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(_pos, new_pos2) < SMALL_DISTANCE);
		_pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
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
			if(L.terrain->IsInside(unit.pos))
			{
				L.terrain->SetH(unit.pos);
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
								if(!Team.IsLeader())
									gui->messages->AddGameMsg3(GMS_NOT_LEADER);
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
											gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
									}
									else
										Net_LeaveLocation(ENTER_FROM_OUTSIDE);
								}
								break;
							}
						}
					}
					else if(L.location_index != QM.quest_secret->where2
						&& (unit.pos.x < 33.f || unit.pos.x > 256.f - 33.f || unit.pos.z < 33.f || unit.pos.z > 256.f - 33.f))
					{
						if(!Team.IsLeader())
							gui->messages->AddGameMsg3(GMS_NOT_LEADER);
						else if(!Net::IsLocal())
							Net_LeaveLocation(ENTER_FROM_OUTSIDE);
						else
						{
							auto result = CanLeaveLocation(unit);
							if(result == CanLeaveLocationResult::Yes)
							{
								allow_exit = true;
								W.SetTravelDir(unit.pos);
							}
							else
								gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
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
		Int2 pt = PosToPt(unit.pos);

		if(pt == lvl.staircase_up)
		{
			Box2d box;
			switch(lvl.staircase_up_dir)
			{
			case GDIR_DOWN:
				unit.pos.y = (unit.pos.z - 2.f*lvl.staircase_up.y) / 2;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y + 1.4f, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case GDIR_LEFT:
				unit.pos.y = (unit.pos.x - 2.f*lvl.staircase_up.x) / 2;
				box = Box2d(2.f*lvl.staircase_up.x + 1.4f, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*(lvl.staircase_up.y + 1));
				break;
			case GDIR_UP:
				unit.pos.y = (2.f*lvl.staircase_up.y - unit.pos.z) / 2 + 1.f;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*(lvl.staircase_up.x + 1), 2.f*lvl.staircase_up.y + 0.6f);
				break;
			case GDIR_RIGHT:
				unit.pos.y = (2.f*lvl.staircase_up.x - unit.pos.x) / 2 + 1.f;
				box = Box2d(2.f*lvl.staircase_up.x, 2.f*lvl.staircase_up.y, 2.f*lvl.staircase_up.x + 0.6f, 2.f*(lvl.staircase_up.y + 1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == FROZEN::NO && !dash)
			{
				if(Team.IsLeader())
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
							gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(ENTER_FROM_UP_LEVEL);
				}
				else
					gui->messages->AddGameMsg3(GMS_NOT_LEADER);
			}
		}
		else if(pt == lvl.staircase_down)
		{
			Box2d box;
			switch(lvl.staircase_down_dir)
			{
			case GDIR_DOWN:
				unit.pos.y = (unit.pos.z - 2.f*lvl.staircase_down.y)*-1.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y + 1.4f, 2.f*(lvl.staircase_down.x + 1), 2.f*(lvl.staircase_down.y + 1));
				break;
			case GDIR_LEFT:
				unit.pos.y = (unit.pos.x - 2.f*lvl.staircase_down.x)*-1.f;
				box = Box2d(2.f*lvl.staircase_down.x + 1.4f, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x + 1), 2.f*(lvl.staircase_down.y + 1));
				break;
			case GDIR_UP:
				unit.pos.y = (2.f*lvl.staircase_down.y - unit.pos.z)*-1.f - 2.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*(lvl.staircase_down.x + 1), 2.f*lvl.staircase_down.y + 0.6f);
				break;
			case GDIR_RIGHT:
				unit.pos.y = (2.f*lvl.staircase_down.x - unit.pos.x)*-1.f - 2.f;
				box = Box2d(2.f*lvl.staircase_down.x, 2.f*lvl.staircase_down.y, 2.f*lvl.staircase_down.x + 0.6f, 2.f*(lvl.staircase_down.y + 1));
				break;
			}

			if(warped)
				return;

			if(unit.IsPlayer() && WantExitLevel() && box.IsInside(unit.pos) && unit.frozen == FROZEN::NO && !dash)
			{
				if(Team.IsLeader())
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
							gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
					}
					else
						Net_LeaveLocation(ENTER_FROM_DOWN_LEVEL);
				}
				else
					gui->messages->AddGameMsg3(GMS_NOT_LEADER);
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
					if(Team.IsLeader())
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
								gui->messages->AddGameMsg3(result == CanLeaveLocationResult::TeamTooFar ? GMS_GATHER_TEAM : GMS_NOT_IN_COMBAT);
						}
						else
							Net_LeaveLocation(ENTER_FROM_PORTAL + index);
					}
					else
						gui->messages->AddGameMsg3(GMS_NOT_LEADER);

					break;
				}
			}
			portal = portal->next_portal;
			++index;
		}
	}

	if(Net::IsLocal() || &unit != pc->unit || interpolate_timer <= 0.f)
	{
		unit.visual_pos = unit.pos;
		unit.changed = true;
	}
	unit.UpdatePhysics(unit.pos);
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
			Spell* spell = Spell::TryGet(_spells.name[i]);
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
	u->running = false;

	u->fake_unit = true; // to prevent sending hp changed message set temporary as fake unit
	if(base.group == G_PLAYER)
	{
		u->stats = new UnitStats;
		u->stats->fixed = false;
		u->stats->subprofile.value = 0;
		u->stats->Set(base.GetStatProfile());
	}
	else
		u->stats = base.GetStats(u->level);
	u->CalculateStats();
	u->hp = u->hpmax = u->CalculateMaxHp();
	u->stamina = u->stamina_max = u->CalculateMaxStamina();
	u->fake_unit = false;

	// items
	u->weight = 0;
	u->CalculateLoad();
	if(!custom && base.item_script)
	{
		ItemScript* script = base.item_script;
		if(base.stat_profile && !base.stat_profile->subprofiles.empty() && base.stat_profile->subprofiles[u->stats->subprofile.index]->item_script)
			script = base.stat_profile->subprofiles[u->stats->subprofile.index]->item_script;
		ParseItemScript(*u, script);
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
	if(base.trader && !test_unit)
	{
		u->stock = new TraderStock;
		u->stock->date = W.GetWorldtime();
		base.trader->stock->Parse(u->stock->items);
		if(!L.entering)
		{
			for(ItemSlot& slot : u->stock->items)
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
				u->human_data = new Human;
				u->human_data->beard = Rand() % MAX_BEARD - 1;
				u->human_data->hair = Rand() % MAX_HAIR - 1;
				u->human_data->mustache = Rand() % MAX_MUSTACHE - 1;
				u->human_data->height = Random(0.9f, 1.1f);
				if(IS_SET(base.flags2, F2_OLD))
					u->human_data->hair_color = Color::Hex(0xDED5D0);
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
			u->CreatePhysics();
		else
			u->cobj = nullptr;
	}

	if(Net::IsServer())
	{
		u->netid = Unit::netid_counter++;
		if(!L.entering)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SPAWN_UNIT;
			c.unit = u;
		}
	}

	return u;
}

//=================================================================================================
void GiveItem(Unit& unit, const int*& ps, int count)
{
	int type = *ps;
	++ps;
	switch(type)
	{
	case PS_ITEM:
		unit.AddItemAndEquipIfNone((const Item*)(*ps), count);
		break;
	case  PS_LIST:
		{
			const ItemList& lis = *(const ItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get());
		}
		break;
	case PS_LEVELED_LIST:
		{
			const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1)));
		}
		break;
	case PS_LEVELED_LIST_MOD:
		{
			int mod = *ps++;
			const LeveledItemList& lis = *(const LeveledItemList*)(*ps);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1) + mod));
		}
		break;
	case PS_SPECIAL_ITEM:
		{
			int special = *ps;
			const LeveledItemList& lis = ItemScript::GetSpecial(special, unit.stats->subprofile.value);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1)));
		}
		break;
	case PS_SPECIAL_ITEM_MOD:
		{
			int mod = *ps++;
			int special = *ps;
			const LeveledItemList& lis = ItemScript::GetSpecial(special, unit.stats->subprofile.value);
			for(int i = 0; i < count; ++i)
				unit.AddItemAndEquipIfNone(lis.Get(unit.level + Random(-2, 1) + mod));
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
		if(type == PS_LEVELED_LIST_MOD || type == PS_SPECIAL_ITEM_MOD)
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
		if(*it == &_unit || !(*it)->IsAlive() || Vec3::Distance((*it)->pos, _unit.pos) > 5.f || _unit.IsFriend(**it))
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

			int count = min(Random(pe.spawn_min, pe.spawn_max), pe.max_particles - pe.alive);
			vector<Particle>::iterator it2 = pe.particles.begin();

			for(int i = 0; i < count; ++i)
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

			pe.alive += count;
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

Game::ATTACK_RESULT Game::DoAttack(LevelContext& ctx, Unit& unit)
{
	Vec3 hitpoint;
	Unit* hitted;

	if(!CheckForHit(ctx, unit, hitted, hitpoint))
		return ATTACK_NOT_HIT;

	float power;
	if(unit.data->frames->extra)
		power = 1.f;
	else
		power = unit.data->frames->attack_power[unit.attack_id];
	return DoGenericAttack(ctx, unit, *hitted, hitpoint, unit.CalculateAttack()*unit.attack_power*power, unit.GetDmgType(), false);
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
		taker.Die(&ctx, giver);
	}
	else
	{
		// unit hurt sound
		if(taker.hurt_timer <= 0.f && taker.data->sounds->Have(SOUND_PAIN))
		{
			PlayAttachedSound(taker, taker.data->sounds->Random(SOUND_PAIN)->sound, Unit::PAIN_SOUND_DIST);
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
			case ANI_WALK_BACK:
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
					L.CreateBlood(ctx, u);
					if(u.summoner != nullptr && Net::IsLocal())
					{
						Team.RemoveTeamMember(&u);
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
								gui->inventory->inv_mine->RemoveSlotItem(pc->next_action_data.slot);
							break;
							// equip item after unequiping old one
						case NA_EQUIP:
							{
								int index = pc->GetNextActionItemIndex();
								if(index != -1)
									gui->inventory->inv_mine->EquipSlotItem(index);
							}
							break;
							// drop item after hiding it
						case NA_DROP:
							if(u.slots[pc->next_action_data.slot])
								gui->inventory->inv_mine->DropSlotItem(pc->next_action_data.slot);
							break;
							// use consumable
						case NA_CONSUME:
							{
								int index = pc->GetNextActionItemIndex();
								if(index != -1)
									gui->inventory->inv_mine->ConsumeItem(index);
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
								gui->inventory->inv_trade_mine->SellSlotItem(pc->next_action_data.slot);
							break;
							// put equipped item in container
						case NA_PUT:
							if(u.slots[pc->next_action_data.slot])
								gui->inventory->inv_trade_mine->PutSlotItem(pc->next_action_data.slot);
							break;
							// give equipped item
						case NA_GIVE:
							if(u.slots[pc->next_action_data.slot])
								gui->inventory->inv_trade_mine->GiveSlotItem(pc->next_action_data.slot);
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
			if(!u.mesh_inst)
			{
				// fix na skutek, nie na przyczynê ;(
				ReportError(4, Format("Unit %s dont have shooting animation, LS:%d A:%D ANI:%d PANI:%d ETA:%d.", u.GetName(), u.live_state, u.action, u.animation,
					u.current_animation, u.animation_state));
				goto koniec_strzelania;
			}
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

					sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, SHOOT_SOUND_DIST);

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
					u.PlaySound(sGulp, Unit::DRINK_SOUND_DIST);
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
					u.PlaySound(sEat, Unit::EAT_SOUND_DIST);
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
							c.count = USE_USABLE_END;
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
									sound_mgr->PlaySound3d(bu.sound->sound, u.GetCenter(), Usable::SOUND_DIST);
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
								L.global_col.clear();
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
									u.UpdatePhysics(u.pos);
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
					c.count = USE_USABLE_END;
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
							if(!unit->IsFriend(u))
							{
								if(!u.player->IsHit(unit))
								{
									float attack = 100.f + 5.f * u.Get(AttributeId::STR);
									float def = unit->CalculateDefense();
									float dmg = CombatHelper::CalculateDamage(attack, def);
									PlayHitSound(MAT_IRON, unit->GetBodyMaterial(), unit->GetCenter(), HIT_SOUND_DIST, true);
									if(unit->IsPlayer())
										unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, u.level);
									GiveDmg(ctx, &u, dmg, *unit);
									if(u.IsPlayer())
										u.player->Train(TrainWhat::BullsCharge, 0.f, unit->level);
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
					if(Net::IsLocal() || u.IsLocal())
						u.mesh_inst->groups[0].speed = u.GetRunSpeed() / u.data->run_speed;
				}
			}
			break;
		case A_DESPAWN:
			u.timer -= dt;
			if(u.timer <= 0.f)
				L.RemoveUnit(&u);
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
		case A_USE_ITEM:
			if(u.mesh_inst->frame_end_info2)
			{
				if(Net::IsLocal() && u.IsPlayer())
				{
					// magic scroll effect
					gui->messages->AddGameMsg3(u.player, GMS_TOO_COMPLICATED);
					u.ApplyStun(2.5f);
					u.RemoveItem(u.used_item, 1u);
					u.used_item = nullptr;
				}
				sound_mgr->PlaySound3d(sZap, u.GetCenter(), MAGIC_SCROLL_SOUND_DIST);
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

		hitted->BreakAction();

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

	DoGenericAttack(ctx, attacker, *hitted, hitpoint, attacker.CalculateAttack(&attacker.GetShield()), DMG_BLUNT, true);

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
			shape = L.shape_arrow;
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
				if(it->owner && it->owner->IsFriend(*hitted) || it->attack < -50.f)
				{
					// friendly fire
					if(hitted->action == A_BLOCK && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.count = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, ARROW_HIT_SOUND_DIST, false);
					continue;
				}

				// hit enemy unit
				if(it->owner && it->owner->IsAlive() && hitted->IsAI())
					AI_HitReaction(*hitted, it->start_pos);

				// calculate modifiers
				int mod = CombatHelper::CalculateModifier(DMG_PIERCE, hitted->data->flags);
				float m = 1.f;
				if(mod == -1)
					m += 0.25f;
				else if(mod == 1)
					m -= 0.25f;
				if(hitted->IsNotFighting())
					m += 0.1f;
				if(hitted->action == A_PAIN)
					m += 0.1f;

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

				// apply modifiers
				float attack = it->attack * m;

				if(hitted->action == A_BLOCK && angle_dif < PI * 2 / 5)
				{
					// play sound
					MATERIAL_TYPE mat = hitted->GetShield().material;
					sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
					if(Net::IsOnline())
					{
						NetChange& c = Add1(Net::changes);
						c.type = NetChange::HIT_SOUND;
						c.id = MAT_IRON;
						c.count = mat;
						c.pos = hitpoint;
					}

					// train blocking
					if(hitted->IsPlayer())
						hitted->player->Train(TrainWhat::BlockBullet, attack / hitted->hpmax, it->level);

					// reduce damage
					float block = hitted->CalculateBlock() * hitted->mesh_inst->groups[1].GetBlendT() * (1.f - angle_dif / (PI * 2 / 5));
					float stamina = min(block, attack);
					attack -= block;
					hitted->RemoveStaminaBlock(stamina);

					// pain animation & break blocking
					if(hitted->stamina < 0)
					{
						hitted->BreakAction();

						if(!IS_SET(hitted->data->flags, F_DONT_SUFFER))
						{
							if(hitted->action != A_POSITION)
								hitted->action = A_PAIN;
							else
								hitted->animation_state = 1;

							if(hitted->mesh_inst->mesh->head.n_groups == 2)
							{
								hitted->mesh_inst->frame_end_info2 = false;
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO1 | PLAY_ONCE, 1);
							}
							else
							{
								hitted->mesh_inst->frame_end_info = false;
								hitted->mesh_inst->Play(NAMES::ani_hurt, PLAY_PRIO3 | PLAY_ONCE, 0);
								hitted->mesh_inst->groups[0].speed = 1.f;
								hitted->animation = ANI_PLAY;
							}
						}
					}

					if(attack < 0)
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

				// calculate defense
				float def = hitted->CalculateDefense();

				// calculate damage
				float dmg = CombatHelper::CalculateDamage(attack, def);

				// hit sound
				PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

				// train player armor skill
				if(hitted->IsPlayer())
					hitted->player->Train(TrainWhat::TakeDamageArmor, attack / hitted->hpmax, it->level);

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

				// train player in bow
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
				if(it->poison_attack > 0.f)
				{
					float poison_res = hitted->GetPoisonResistance();
					if(poison_res > 0.f)
					{
						Effect e;
						e.effect = EffectId::Poison;
						e.source = EffectSource::Temporary;
						e.source_id = -1;
						e.value = -1;
						e.power = it->poison_attack / 5 * poison_res;
						e.time = 5.f;
						hitted->AddEffect(e);
					}
				}
			}
			else
			{
				// trafienie w postaæ z czara
				if(it->owner && it->owner->IsFriend(*hitted))
				{
					// frendly fire
					SpellHitEffect(ctx, *it, hitpoint, hitted);

					// dŸwiêk trafienia w postaæ
					if(hitted->action == A_BLOCK && AngleDiff(Clip(it->rot.y + PI), hitted->rot) < PI * 2 / 5)
					{
						MATERIAL_TYPE mat = hitted->GetShield().material;
						sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, mat), hitpoint, ARROW_HIT_SOUND_DIST);
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::HIT_SOUND;
							c.id = MAT_IRON;
							c.count = mat;
							c.pos = hitpoint;
						}
					}
					else
						PlayHitSound(MAT_IRON, hitted->GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, false);
					continue;
				}

				if(hitted->IsAI() && it->owner && it->owner->IsAlive())
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
				if(IS_SET(it->spell->flags, Spell::Poison))
				{
					float poison_res = hitted->GetPoisonResistance();
					if(poison_res > 0.f)
					{
						Effect e;
						e.effect = EffectId::Poison;
						e.source = EffectSource::Temporary;
						e.source_id = -1;
						e.value = -1;
						e.power = dmg / 5 * poison_res;
						e.time = 5.f;
						hitted->AddEffect(e);
					}
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
				sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, MAT_ROCK), hitpoint, ARROW_HIT_SOUND_DIST);

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

				if(Net::IsLocal() && QM.quest_tutorial->in_tutorial && callback.target)
				{
					void* ptr = callback.target->getUserPointer();
					QM.quest_tutorial->HandleBulletCollision(ptr);
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

Unit* Game::CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level, Human* human_data, const Vec3* pos, const float* rot, AIController** ai)
{
	Unit* u = CreateUnit(unit, level, human_data);
	u->in_building = ctx.building_id;

	if(pos)
	{
		if(ctx.type == LevelContext::Outside)
		{
			Vec3 pt = *pos;
			L.terrain->SetH(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		u->UpdatePhysics(u->pos);
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

void Game::ChangeLevel(int where)
{
	assert(where == 1 || where == -1);

	Info(where == 1 ? "Changing level to lower." : "Changing level to upper.");

	L.entering = true;
	L.event_handler = nullptr;
	L.UpdateDungeonMinimap(false);

	if(!QM.quest_tutorial->in_tutorial && QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone
		&& QM.quest_crazies->crazies_state < Quest_Crazies::State::End)
		QM.quest_crazies->CheckStone();

	if(Net::IsOnline() && N.active_players > 1)
	{
		int level = L.dungeon_level;
		if(where == -1)
			--level;
		else
			++level;
		if(level >= 0)
			N.OnChangeLevel(level);

		N.FilterServerChanges();
	}

	if(where == -1)
	{
		// poziom w górê
		if(L.dungeon_level == 0)
		{
			if(QM.quest_tutorial->in_tutorial)
			{
				QM.quest_tutorial->OnEvent(Quest_Tutorial::Exit);
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
			LocationGenerator* loc_gen = loc_gen_factory->Get(inside);
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

		LocationGenerator* loc_gen = loc_gen_factory->Get(inside);

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
	L.CheckIfLocationCleared();
	bool loaded_resources = L.location->RequireLoadingResources(nullptr);
	LoadResources(txLoadingComplete, false);

	SetMusic();

	if(Net::IsOnline() && N.active_players > 1)
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		prepared_stream.Reset();
		N.WriteLevelData(prepared_stream, loaded_resources);
		Info("Generated location packet: %d.", prepared_stream.GetNumberOfBytesUsed());
		gui->info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		gui->load_screen->visible = false;
		gui->main_menu->visible = false;
		gui->game_gui->visible = true;
	}

	Info("Randomness integrity: %d", RandVal());
	L.entering = false;
}

void Game::OpenDoorsByTeam(const Int2& pt)
{
	static vector<Int2> tmp_path;
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	for(Unit* unit : Team.members)
	{
		Int2 unit_pt = PosToPt(unit->pos);
		if(pathfinding->FindPath(L.local_ctx, unit_pt, pt, tmp_path))
		{
			for(vector<Int2>::iterator it2 = tmp_path.begin(), end2 = tmp_path.end(); it2 != end2; ++it2)
			{
				Tile& p = lvl.map[(*it2)(lvl.w)];
				if(p.type == DOORS)
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

	gui->world_map->Show();
	gui->game_gui->visible = false;
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

void Game::PlayAttachedSound(Unit& unit, SOUND sound, float distance)
{
	assert(sound);

	if(!sound_mgr->CanPlaySound())
		return;

	Vec3 pos = unit.GetHeadSoundPos();
	FMOD::Channel* channel = sound_mgr->CreateChannel(sound, pos, distance);
	AttachedSound& s = Add1(attached_sounds);
	s.channel = channel;
	s.unit = &unit;
}

void Game::StopAllSounds()
{
	sound_mgr->StopSounds();
	attached_sounds.clear();
}

Game::ATTACK_RESULT Game::DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const Vec3& hitpoint, float attack, int dmg_type, bool bash)
{
	// calculate modifiers
	int mod = CombatHelper::CalculateModifier(dmg_type, hitted.data->flags);
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
	attack *= m;

	// blocking
	if(hitted.action == A_BLOCK && angle_dif < PI / 2)
	{
		// play sound
		MATERIAL_TYPE hitted_mat = hitted.GetShield().material;
		MATERIAL_TYPE weapon_mat = (!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material);
		if(Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::HIT_SOUND;
			c.pos = hitpoint;
			c.id = weapon_mat;
			c.count = hitted_mat;
		}
		sound_mgr->PlaySound3d(GetMaterialSound(weapon_mat, hitted_mat), hitpoint, HIT_SOUND_DIST);

		// train blocking
		if(hitted.IsPlayer())
			hitted.player->Train(TrainWhat::BlockAttack, attack / hitted.hpmax, attacker.level);

		// reduce damage
		float block = hitted.CalculateBlock() * hitted.mesh_inst->groups[1].GetBlendT();
		float stamina = min(attack, block);
		if(IS_SET(attacker.data->flags2, F2_IGNORE_BLOCK))
			block *= 2.f / 3;
		if(attacker.attack_power >= 1.9f)
			stamina *= 4.f / 3;
		attack -= block;
		hitted.RemoveStaminaBlock(stamina);

		// pain animation & break blocking
		if(hitted.stamina < 0)
		{
			hitted.BreakAction();

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
		if(attack < 0)
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

	// calculate defense
	float def = hitted.CalculateDefense();

	// calculate damage
	float dmg = CombatHelper::CalculateDamage(attack, def);

	// hit sound
	PlayHitSound(!bash ? attacker.GetWeaponMaterial() : attacker.GetShield().material, hitted.GetBodyMaterial(), hitpoint, HIT_SOUND_DIST, dmg > 0.f);

	// train player armor skill
	if(hitted.IsPlayer())
		hitted.player->Train(TrainWhat::TakeDamageArmor, attack / hitted.hpmax, attacker.level);

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
			ratio = Clamp(dmgf / hitted.hpmax, TRAIN_KILL_RATIO, 1.f);
		else
			ratio = min(1.f, dmgf / hitted.hpmax);
		attacker.player->Train(bash ? TrainWhat::BashHit : TrainWhat::AttackHit, ratio, hitted.level);
	}

	GiveDmg(ctx, &attacker, dmg, hitted, &hitpoint);

	// apply poison
	if(IS_SET(attacker.data->flags, F_POISON_ATTACK))
	{
		float poison_res = hitted.GetPoisonResistance();
		if(poison_res > 0.f)
		{
			Effect e;
			e.effect = EffectId::Poison;
			e.source = EffectSource::Temporary;
			e.source_id = -1;
			e.value = -1;
			e.power = dmg / 10 * poison_res;
			e.time = 5.f;
			hitted.AddEffect(e);
		}
	}

	return hitted.action == A_PAIN ? ATTACK_CLEAN_HIT : ATTACK_HIT;
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
		int count = 1;
		if(IS_SET(spell.flags, Spell::Triple))
			count = 3;

		float expected_rot = Clip(-Vec3::Angle2d(coord, u.target_pos) + PI / 2);
		float current_rot = Clip(u.rot + PI);
		AdjustAngle(current_rot, expected_rot, ToRadians(10.f));

		for(int i = 0; i < count; ++i)
		{
			Bullet& b = Add1(ctx.bullets);

			b.level = u.level + u.CalculateMagicPower();
			b.backstab = 0;
			b.pos = coord;
			b.attack = float(spell.dmg);
			b.rot = Vec3(0, current_rot + Random(-0.05f, 0.05f), 0);
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
				if(!IS_SET(hitted->data->flags2, F2_BLOODLESS) && !u.IsFriend(*hitted))
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
					&& !u.IsEnemy(**it)
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
					u2.ReequipItems();

					// przenieœ fizyke
					L.WarpUnit(u2, u2.pos);

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
				if(!u.IsEnemy(**it) && !IS_SET((*it)->data->flags, F_UNDEAD) && Vec3::Distance(u.target_pos, (*it)->pos) < 0.5f)
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
		sound_mgr->PlaySound3d(spell.sound_cast, coord, spell.sound_cast_dist);
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
		sound_mgr->PlaySound3d(spell.sound_hit, pos, spell.sound_hit_dist);

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
				if(!(*it2)->IsAlive() || ((*it)->owner && (*it)->owner->IsFriend(**it2)))
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
					sound_mgr->PlaySound3d(trap.base->sound->sound, trap.pos, trap.base->sound_dist);
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

					sound_mgr->PlaySound3d(trap.base->sound2->sound, trap.pos, trap.base->sound_dist2);

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
				// move spears
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
								int mod = CombatHelper::CalculateModifier(DMG_PIERCE, unit->data->flags);
								float m = 1.f;
								if(mod == -1)
									m += 0.25f;
								else if(mod == 1)
									m -= 0.25f;
								if(unit->action == A_PAIN)
									m += 0.1f;

								// calculate attack & defense
								float attack = float(trap.base->attack) * m;
								float def = unit->CalculateDefense();
								float dmg = CombatHelper::CalculateDamage(attack, def);

								// dŸwiêk trafienia
								sound_mgr->PlaySound3d(GetMaterialSound(MAT_IRON, unit->GetBodyMaterial()), unit->pos + Vec3(0, 1.f, 0), HIT_SOUND_DIST);

								// train player armor skill
								if(unit->IsPlayer())
									unit->player->Train(TrainWhat::TakeDamageArmor, attack / unit->hpmax, 4);

								// damage
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
					sound_mgr->PlaySound3d(trap.base->sound3->sound, trap.pos, trap.base->sound_dist3);
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
					sound_mgr->PlaySound3d(trap.base->sound->sound, trap.pos, trap.base->sound_dist);

					if(is_local)
					{
						Bullet& b = Add1(ctx.bullets);
						b.level = 4;
						b.backstab = 0;
						b.attack = float(trap.base->attack);
						b.mesh = aArrow;
						b.pos = Vec3(2.f*trap.tile.x + trap.pos.x - float(int(trap.pos.x / 2) * 2) + Random(-trap.base->rw, trap.base->rw) - 1.2f*DirToPos(trap.dir).x,
							Random(0.5f, 1.5f),
							2.f*trap.tile.y + trap.pos.z - float(int(trap.pos.z / 2) * 2) + Random(-trap.base->h, trap.base->h) - 1.2f*DirToPos(trap.dir).y);
						b.start_pos = b.pos;
						b.rot = Vec3(0, DirToRot(trap.dir), 0);
						b.owner = nullptr;
						b.pe = nullptr;
						b.remove = false;
						b.speed = TRAP_ARROW_SPEED;
						b.spell = nullptr;
						b.tex = nullptr;
						b.tex_size = 0.f;
						b.timer = ARROW_TIMER;
						b.yspeed = 0.f;
						b.poison_attack = (trap.base->type == TRAP_POISON ? float(trap.base->attack) : 0.f);

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

						sound_mgr->PlaySound3d(sBow[Rand() % 2], b.pos, SHOOT_SOUND_DIST);

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

					Spell* fireball = Spell::TryGet("fireball");

					Explo* explo = new Explo;
					explo->pos = trap.pos;
					explo->pos.y += 0.2f;
					explo->size = 0.f;
					explo->sizemax = 2.f;
					explo->dmg = float(trap.base->attack);
					explo->tex = fireball->tex_explode;
					explo->owner = nullptr;

					sound_mgr->PlaySound3d(fireball->sound_hit, explo->pos, fireball->sound_hit_dist);

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
				Unit* hitted = e.hitted.back();
				if(!e.owner->IsFriend(*hitted))
				{
					if(hitted->IsAI() && e.owner->IsAlive())
						AI_HitReaction(*hitted, e.start_pos);
					GiveDmg(ctx, e.owner, e.dmg, *hitted, nullptr, DMG_NO_BLOOD | DMG_MAGICAL);
				}

				if(e.spell->sound_hit)
					sound_mgr->PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist);

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
					static vector<pair<Unit*, float>> targets;

					// traf w kolejny cel
					for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
					{
						if(!(*it2)->IsAlive() || IsInside(e.hitted, *it2))
							continue;

						float dist = Vec3::Distance((*it2)->pos, e.hitted.back()->pos);
						if(dist <= 5.f)
							targets.push_back(pair<Unit*, float>(*it2, dist));
					}

					if(!targets.empty())
					{
						if(targets.size() > 1)
						{
							std::sort(targets.begin(), targets.end(), [](const pair<Unit*, float>& target1, const pair<Unit*, float>& target2)
							{
								return target1.second < target2.second;
							});
						}

						Unit* target = nullptr;
						float dist;

						for(vector<pair<Unit*, float>>::iterator it2 = targets.begin(), end2 = targets.end(); it2 != end2; ++it2)
						{
							Vec3 hitpoint;
							Unit* new_hitted;
							if(RayTest(e.lines.back().pts.back(), it2->first->GetCenter(), hitted, hitpoint, new_hitted))
							{
								if(new_hitted == it2->first)
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
					sound_mgr->PlaySound3d(e.spell->sound_hit, e.lines.back().pts.back(), e.spell->sound_hit_dist);

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

void Game::BuildRefidTables()
{
	// units and unsable objects
	Unit::refid_table.clear();
	Usable::refid_table.clear();
	GroundItem::refid_table.clear();
	for(Location* loc : W.GetLocations())
	{
		if(loc)
			loc->BuildRefidTables();
	}

	// particles (buildings are build first because they are loaded first)
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();
	if(L.is_open)
	{
		if(L.city_ctx)
		{
			for(InsideBuilding* building : L.city_ctx->inside_buildings)
				building->ctx.BuildRefidTables();
		}
		L.local_ctx.BuildRefidTables();
	}
}

// clear game vars on new game or load
void Game::ClearGameVars(bool new_game)
{
	gui->inventory->mode = I_NONE;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	death_screen = 0;
	end_of_game = false;
	gui->minimap->city = nullptr;
	Team.ClearOnNewGameOrLoad();
	draw_flags = 0xFFFFFFFF;
	gui->game_gui->Reset();
	gui->journal->Reset();
	arena->Reset();
	debug_info = false;
	debug_info2 = false;
	gui->world_map->dialog_enc = nullptr;
	gui->game_gui->visible = false;
	gui->inventory->lock = nullptr;
	gui->world_map->picked_location = -1;
	post_effects.clear();
	grayout = 0.f;
	cam.Reset();
	lights_dt = 1.f;
	pc_data.Reset();
	SM.Reset();
	L.Reset();
	pathfinding->SetTarget(nullptr);
	gui->world_map->Clear();

	// odciemnianie na pocz¹tku
	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;

	if(new_game)
	{
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
		pc_data.picking_item_state = 0;
		L.is_open = false;
		gui->game_gui->PositionPanels();
		gui->Clear(true, false);
		if(!N.mp_quickload)
			gui->mp_box->visible = Net::IsOnline();
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
}

// called before starting new game/loading/at exit
void Game::ClearGame()
{
	Info("Clearing game.");

	draw_batch.Clear();

	LeaveLocation(true, false);

	// delete units on world map
	if((game_state == GS_WORLDMAP || prev_game_state == GS_WORLDMAP || (N.mp_load && prev_game_state == GS_LOAD)) && !L.is_open && Net::IsLocal() && !N.was_client)
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
		Team.members.clear();
		prev_game_state = GS_LOAD;
	}

	if(!N.net_strs.empty())
		StringPool.Free(N.net_strs);

	L.is_open = false;
	L.city_ctx = nullptr;
	QM.Clear();
	W.Reset();
	gui->Clear(true, false);
}

SOUND Game::GetItemSound(const Item* item)
{
	assert(item);

	switch(item->type)
	{
	case IT_WEAPON:
		return sItem[6];
	case IT_BOW:
		return sItem[4];
	case IT_SHIELD:
		return sItem[5];
	case IT_ARMOR:
		if(item->ToArmor().armor_type != AT_LIGHT)
			return sItem[2];
		else
			return sItem[1];
	case IT_AMULET:
		return sItem[8];
	case IT_RING:
		return sItem[9];
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

void Game::Unit_StopUsingUsable(LevelContext& ctx, Unit& u, bool send)
{
	u.animation = ANI_STAND;
	u.animation_state = AS_ANIMATION2_MOVE_TO_ENDPOINT;
	u.timer = 0.f;
	u.used_item = nullptr;

	const float unit_radius = u.GetUnitRadius();

	L.global_col.clear();
	Level::IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &u, nullptr };
	ignore.ignored_units = ignore_units;
	L.GatherCollisionObjects(ctx, L.global_col, u.pos, 2.f + unit_radius, &ignore);

	Vec3 tmp_pos = u.target_pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!L.Collide(L.global_col, tmp_pos, unit_radius))
		{
			if(i != 0 && ctx.have_terrain)
				tmp_pos.y = L.terrain->GetH(tmp_pos);
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
		u.UpdatePhysics(u.target_pos);

	if(send && Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.unit = &u;
		c.id = u.usable->netid;
		c.count = USE_USABLE_STOP;
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

	gui->inventory->lock = nullptr;

	loc_gen->OnEnter();

	OnEnterLevelOrLocation();
	OnEnterLevel();

	if(!loc_gen->first)
		Info("Entered level.");
}

void Game::LeaveLevel(bool clear)
{
	Info("Leaving level.");

	if(gui->game_gui)
		gui->game_gui->Reset();

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
		if(L.city_ctx && (!Net::IsLocal() || N.was_client))
			DeleteElements(L.city_ctx->inside_buildings);
		if(Net::IsClient() && !L.location->outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			Room::Free(lvl.rooms);
		}
	}

	ais.clear();
	L.RemoveColliders();
	StopAllSounds();

	ClearQuadtree();

	CloseAllPanels();

	cam.Reset();
	pc_data.rot_buf = 0.f;
	pc_data.target_unit = nullptr;
	dialog_context.dialog_mode = false;
	gui->inventory->mode = I_NONE;
	pc_data.before_player = BP_NONE;
}

void Game::LeaveLevel(LevelContext& ctx, bool clear)
{
	// cleanup units
	if(Net::IsLocal() && !clear && !N.was_client)
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

			unit.used_item = nullptr;
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
					unit.SetOrder(ORDER_FOLLOW);
					unit.talking = false;
					unit.mesh_inst->need_update = true;
					unit.ai->Reset();
					*it = nullptr;
				}
				else
				{
					if(unit.order == ORDER_LEAVE && unit.IsAlive())
					{
						delete unit.ai;
						delete &unit;
						*it = nullptr;
					}
					else
					{
						if(unit.live_state == Unit::DYING)
						{
							unit.live_state = Unit::DEAD;
							unit.mesh_inst->SetToEnd();
							L.CreateBlood(ctx, unit, true);
						}
						if(unit.ai->goto_inn && L.city_ctx)
						{
							// warp to inn if unit wanted to go there
							L.WarpToInn(**it);
						}
						delete unit.mesh_inst;
						unit.mesh_inst = nullptr;
						delete unit.ai;
						unit.ai = nullptr;
						unit.EndEffects();
					}
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

		// remove units that left this level
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

	if(Net::IsLocal() && !N.was_client)
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

void Game::UpdateContext(LevelContext& ctx, float dt)
{
	ProfilerBlock profiler_block([&] { return Format("UpdateContext %s", ctx.GetName()); });

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
						sound_mgr->PlaySound3d(sDoorBudge, door.pos, Door::BLOCKED_SOUND_DIST);
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

void Game::PlayHitSound(MATERIAL_TYPE mat2, MATERIAL_TYPE mat, const Vec3& hitpoint, float range, bool dmg)
{
	// sounds
	sound_mgr->PlaySound3d(GetMaterialSound(mat2, mat), hitpoint, range);
	if(mat != MAT_BODY && dmg)
		sound_mgr->PlaySound3d(GetMaterialSound(mat2, MAT_BODY), hitpoint, range);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::HIT_SOUND;
		c.pos = hitpoint;
		c.id = mat2;
		c.count = mat;

		if(mat != MAT_BODY && dmg)
		{
			NetChange& c2 = Add1(Net::changes);
			c2.type = NetChange::HIT_SOUND;
			c2.pos = hitpoint;
			c2.id = mat2;
			c2.count = MAT_BODY;
		}
	}
}

void Game::LoadingStart(int steps)
{
	gui->load_screen->Reset();
	loading_t.Reset();
	loading_dt = 0.f;
	loading_cap = 0.66f;
	loading_steps = steps;
	loading_index = 0;
	loading_first_step = true;
	clear_color = Color::Black;
	game_state = GS_LOAD;
	gui->load_screen->visible = true;
	gui->main_menu->visible = false;
	gui->game_gui->visible = false;
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
	gui->load_screen->SetProgressOptional(progress, text);

	if(end != 2)
	{
		++loading_index;
		if(end == 1)
			assert(loading_index == loading_steps);
	}
	if(end != 1)
	{
		loading_dt += loading_t.Tick();
		if(loading_dt >= 1.f / 30 || end == 2 || loading_first_step)
		{
			loading_dt = 0.f;
			DoPseudotick();
			loading_t.Tick();
			loading_first_step = false;
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
			else if(N.mp_load && Net::IsClient())
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

	if(L.location)
	{
		// spawn blood for units that are dead and their mesh just loaded
		L.SpawnBlood();

		// finished
		if((Net::IsLocal() || !N.mp_load_worldmap) && !L.location->outside)
			SetDungeonParamsToMeshes();
	}

	LoadingStep(text, 2);
}

// When there is something new to load, add task to load it when entering location etc
void Game::PreloadResources(bool worldmap)
{
	auto& mesh_mgr = ResourceManager::Get<Mesh>();

	if(Net::IsLocal())
		items_load.clear();

	if(!worldmap)
	{
		for(LevelContext& ctx : L.ForEachContext())
		{
			// load units - units respawn so need to check everytime...
			PreloadUnits(*ctx.units);
			// some traps respawn
			if(ctx.traps)
				PreloadTraps(*ctx.traps);

			// preload items, this info is sent by server so no need to redo this by clients (and it will be less complete)
			if(Net::IsLocal())
			{
				for(GroundItem* ground_item : *ctx.items)
					items_load.insert(ground_item->item);
				if(ctx.chests)
				{
					for(Chest* chest : *ctx.chests)
						PreloadItems(chest->items);
				}
				for(Usable* usable : *ctx.usables)
				{
					if(usable->container)
						PreloadItems(usable->container->items);
				}
			}
		}

		bool new_value = true;
		if(L.location->RequireLoadingResources(&new_value) == false)
		{
			// load music
			if(!sound_mgr->IsMusicDisabled())
				LoadMusic(L.GetLocationMusic(), false, true);

			// load objects
			for(Object* obj : *L.local_ctx.objects)
				mesh_mgr.AddLoadTask(obj->mesh);

			// load usables
			PreloadUsables(*L.local_ctx.usables);

			if(L.city_ctx)
			{
				// load buildings
				for(CityBuilding& b : L.city_ctx->buildings)
				{
					Building& type = *b.type;
					if(type.state == ResourceState::NotLoaded)
					{
						if(type.mesh)
							mesh_mgr.AddLoadTask(type.mesh);
						if(type.inside_mesh)
							mesh_mgr.AddLoadTask(type.inside_mesh);
						type.state = ResourceState::Loaded;
					}
				}

				for(InsideBuilding* ib : L.city_ctx->inside_buildings)
				{
					// load building objects
					for(Object* obj : ib->objects)
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

	UnitData& data = *unit->data;

	if(Net::IsLocal())
	{
		for(uint i = 0; i < SLOT_MAX; ++i)
		{
			if(unit->slots[i])
				items_load.insert(unit->slots[i]);
		}
		PreloadItems(unit->items);
		if(unit->stock)
			PreloadItems(unit->stock->items);
	}

	if(data.state == ResourceState::Loaded)
		return;

	if(data.mesh)
		mesh_mgr.AddLoadTask(data.mesh);

	for(int i = 0; i < SOUND_MAX; ++i)
	{
		for(SoundPtr sound : data.sounds->sounds[i])
			sound_mgr.AddLoadTask(sound);
	}

	if(data.tex)
	{
		for(TexId& ti : data.tex->textures)
		{
			if(ti.tex)
				tex_mgr.AddLoadTask(ti.tex);
		}
	}

	data.state = ResourceState::Loaded;
}

void Game::PreloadItems(vector<ItemSlot>& items)
{
	for(auto& slot : items)
		items_load.insert(slot.item);
}

void Game::PreloadItem(const Item* p_item)
{
	Item& item = *(Item*)p_item;
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
			for(SoundPtr sound : unit->data->sounds->sounds[i])
				assert(sound->IsLoaded());
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

void Game::DeleteUnit(Unit* unit)
{
	assert(unit);

	if(L.is_open)
	{
		RemoveElement(L.GetContext(*unit).units, unit);
		gui->game_gui->RemoveUnit(unit);
		if(pc_data.before_player == BP_UNIT && pc_data.before_player_ptr.unit == unit)
			pc_data.before_player = BP_NONE;
		if(unit == pc_data.target_unit)
			pc_data.target_unit = pc->unit;
		if(unit == pc_data.selected_unit)
			pc_data.selected_unit = nullptr;
		if(Net::IsClient())
		{
			if(pc->action == PlayerController::Action_LootUnit && pc->action_unit == unit)
				pc->unit->BreakAction();
		}
		else
		{
			for(PlayerInfo* player : N.players)
			{
				PlayerController* pc = player->pc;
				if(pc->action == PlayerController::Action_LootUnit && pc->action_unit == unit)
					pc->action_unit = nullptr;
			}
		}

		if(unit->player && Net::IsLocal())
		{
			switch(unit->player->action)
			{
			case PlayerController::Action_LootChest:
				{
					// close chest
					unit->player->action_chest->user = nullptr;
					unit->player->action_chest->mesh_inst->Play(&unit->player->action_chest->mesh_inst->mesh->anims[0],
						PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
					sound_mgr->PlaySound3d(sChestClose, unit->player->action_chest->GetCenter(), Chest::SOUND_DIST);
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
		if(!arena->free)
		{
			RemoveElementTry(arena->units, unit);
			if(arena->fighter == unit)
				arena->fighter = nullptr;
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

float Game::PlayerAngleY()
{
	const float pt0 = 4.6662526f;
	return cam.rot.y - pt0;
}

void Game::AttackReaction(Unit& attacked, Unit& attacker)
{
	if(attacker.IsPlayer() && attacked.IsAI() && attacked.in_arena == -1 && !attacked.attack_team)
	{
		if(attacked.data->group == G_CITIZENS)
		{
			if(!Team.is_bandit)
			{
				if(attacked.dont_attack && IS_SET(attacked.data->flags, F_PEACEFUL))
				{
					if(attacked.ai->state == AIController::Idle)
					{
						attacked.ai->state = AIController::Escape;
						attacked.ai->timer = Random(2.f, 4.f);
						attacked.ai->target = &attacker;
						attacked.ai->target_last_pos = attacker.pos;
						attacked.ai->escape_room = nullptr;
						attacked.ai->ignore = 0.f;
						attacked.ai->in_combat = false;
					}
				}
				else
					Team.SetBandit(true);
				{
					Team.is_bandit = true;
				}
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
		if(attacked.dont_attack && !IS_SET(attacked.data->flags, F_PEACEFUL))
		{
			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->dont_attack && !IS_SET((*it)->data->flags, F_PEACEFUL))
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
				if(&u != &u2 && u2.IsStanding() && u.IsEnemy(u2) && u2.IsAI() && u2.ai->in_combat
					&& Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE && L.CanSee(u, u2))
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
				if(&u != &u2 && u2.IsStanding() && u.IsEnemy(u2) && u2.IsAI() && u2.ai->in_combat
					&& Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE && L.CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}

	return CanLeaveLocationResult::Yes;
}

void Game::GenerateQuestUnits()
{
	if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::None && L.location_index == QM.quest_sawmill->start_loc)
	{
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("artur_drwal"), -2);
		assert(u);
		if(u)
		{
			QM.quest_sawmill->sawmill_state = Quest_Sawmill::State::GeneratedUnit;
			QM.quest_sawmill->hd_lumberjack.Get(*u->human_data);
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_mine->start_loc && QM.quest_mine->mine_state == Quest_Mine::State::None)
	{
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("inwestor"), -2);
		assert(u);
		if(u)
		{
			QM.quest_mine->mine_state = Quest_Mine::State::SpawnedInvestor;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_bandits->start_loc && QM.quest_bandits->bandits_state == Quest_Bandits::State::None)
	{
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("mistrz_agentow"), -2);
		assert(u);
		if(u)
		{
			QM.quest_bandits->bandits_state = Quest_Bandits::State::GeneratedMaster;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_mages->start_loc && QM.quest_mages2->mages_state == Quest_Mages2::State::None)
	{
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_magowie_uczony"), -2);
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
			Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
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
			Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_magowie_stary"), 15);
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
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_orkowie_straznik"));
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
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_gobliny_szlachcic"));
		assert(u);
		if(u)
		{
			QM.quest_goblins->nobleman = u;
			QM.quest_goblins->hd_nobleman.Get(*u->human_data);
			QM.quest_goblins->goblins_state = Quest_Goblins::State::GeneratedNobleman;
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(L.location_index == QM.quest_evil->start_loc && QM.quest_evil->evil_state == Quest_Evil::State::None)
	{
		CityBuilding* b = L.city_ctx->FindBuilding(BuildingGroup::BG_INN);
		Unit* u = L.SpawnUnitNearLocation(L.local_ctx, b->walk_pt, *UnitData::Get("q_zlo_kaplan"), nullptr, 10);
		assert(u);
		if(u)
		{
			u->rot = Random(MAX_ANGLE);
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
			Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_tartak"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				QM.quest_sawmill->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}
	else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working)
	{
		int count = QM.quest_sawmill->days / 30;
		if(count)
		{
			QM.quest_sawmill->days -= count * 30;
			Team.AddGold(count * Quest_Sawmill::PAYMENT, nullptr, true);
		}
	}

	if(QM.quest_mine->days >= QM.quest_mine->days_required &&
		((QM.quest_mine->mine_state2 == Quest_Mine::State2::InBuild && QM.quest_mine->mine_state == Quest_Mine::State::Shares) || // inform player about building mine & give gold
			QM.quest_mine->mine_state2 == Quest_Mine::State2::Built || // inform player about possible investment
			QM.quest_mine->mine_state2 == Quest_Mine::State2::InExpand || // inform player about finished mine expanding
			QM.quest_mine->mine_state2 == Quest_Mine::State2::Expanded)) // inform player about finding portal
	{
		Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
		if(u)
		{
			QM.quest_mine->messenger = u;
			u->StartAutoTalk(true);
		}
	}

	GenerateQuestUnits2();

	if(QM.quest_evil->evil_state == Quest_Evil::State::GenerateMage && L.location_index == QM.quest_evil->mage_loc)
	{
		Unit* u = L.SpawnUnitInsideInn(*UnitData::Get("q_zlo_mag"), -2);
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

void Game::GenerateQuestUnits2()
{
	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::Counting && QM.quest_goblins->days <= 0)
	{
		Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("q_gobliny_poslaniec"), &Team.leader->pos, -2, 2.f);
		if(u)
		{
			QM.quest_goblins->messenger = u;
			u->StartAutoTalk(true);
			if(devmode)
				Info("Generated quest unit '%s'.", u->GetRealName());
		}
	}

	if(QM.quest_goblins->goblins_state == Quest_Goblins::State::NoblemanLeft && QM.quest_goblins->days <= 0)
	{
		Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("q_gobliny_mag"), &Team.leader->pos, 5, 2.f);
		if(u)
		{
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
			Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_tartak"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
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
			income += count * Quest_Sawmill::PAYMENT;
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
					Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
					if(u)
					{
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
			Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("poslaniec_kopalnia"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
				QM.quest_mine->messenger = u;
				u->StartAutoTalk(true);
			}
		}
	}

	// give gold from mine
	income += QM.quest_mine->GetIncome(days);

	if(income != 0)
		Team.AddGold(income, nullptr, true);

	QM.quest_contest->Progress();

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
		GenerateQuestUnits2();
}

void Game::RemoveQuestUnit(UnitData* ud, bool on_leave)
{
	assert(ud);

	Unit* unit = L.FindUnit([=](Unit* unit)
	{
		return unit->data == ud && unit->IsAlive();
	});

	if(unit)
		L.RemoveUnit(unit, !on_leave);
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
				L.RemoveUnit(u, !on_leave);
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
				L.RemoveUnit(*it, !on_leave);
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
		L.RemoveUnit(QM.quest_evil->cleric, !on_leave);
		QM.quest_evil->cleric = nullptr;
		QM.quest_evil->evil_state = Quest_Evil::State::ClericLeft;
	}
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

void Game::UpdateGame2(float dt)
{
	// arena
	if(arena->mode != Arena::NONE)
		arena->Update(dt);

	// tournament
	if(QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
		QM.quest_tournament->Update(dt);

	// sharing of team items between team members
	Team.UpdateTeamItemShares();

	// contest
	QM.quest_contest->Update(dt);

	// quest bandits
	if(QM.quest_bandits->bandits_state == Quest_Bandits::State::Counting)
	{
		QM.quest_bandits->timer -= dt;
		if(QM.quest_bandits->timer <= 0.f)
		{
			// spawn agent
			Unit* u = L.SpawnUnitNearLocation(L.GetContext(*Team.leader), Team.leader->pos, *UnitData::Get("agent"), &Team.leader->pos, -2, 2.f);
			if(u)
			{
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
			if(u.IsStanding() && u.IsPlayer() && Vec3::Distance(u.pos, QM.quest_evil->pos) < 5.f && L.CanSee(L.local_ctx, u.pos, QM.quest_evil->pos))
			{
				QM.quest_evil->evil_state = Quest_Evil::State::Summoning;
				sound_mgr->PlaySound2d(sEvil);
				if(Net::IsOnline())
					Net::PushChange(NetChange::EVIL_SOUND);
				QM.quest_evil->SetProgress(Quest_Evil::Progress::AltarEvent);
				// spawn undead
				InsideLocation* inside = (InsideLocation*)L.location;
				inside->spawn = SG_UNDEAD;
				DungeonGenerator* gen = (DungeonGenerator*)loc_gen_factory->Get(L.location);
				gen->GenerateUnits();
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
						if(u->order != ORDER_WAIT)
							u->SetOrder(ORDER_WAIT);

						// zamknij
						if(dist < 2.f)
						{
							QM.quest_evil->timer -= dt;
							if(QM.quest_evil->timer <= 0.f)
							{
								loc.state = Quest_Evil::Loc::State::PortalClosed;
								u->SetOrder(ORDER_FOLLOW);
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
					else if(u->order != ORDER_FOLLOW)
						u->SetOrder(ORDER_FOLLOW);
				}
			}
		}
	}

	// secret quest
	Quest_Secret* secret = QM.quest_secret;
	if(secret->state == Quest_Secret::SECRET_FIGHT)
		secret->UpdateFight();
}

void Game::OnCloseInventory()
{
	if(gui->inventory->mode == I_TRADE)
	{
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(gui->inventory->mode == I_SHARE || gui->inventory->mode == I_GIVE)
	{
		if(Net::IsLocal())
		{
			pc->action_unit->busy = Unit::Busy_No;
			pc->action_unit->look_target = nullptr;
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}
	else if(gui->inventory->mode == I_LOOT_CHEST && Net::IsLocal())
	{
		pc->action_chest->user = nullptr;
		pc->action_chest->mesh_inst->Play(&pc->action_chest->mesh_inst->mesh->anims[0], PLAY_PRIO1 | PLAY_ONCE | PLAY_STOP_AT_END | PLAY_BACK, 0);
		sound_mgr->PlaySound3d(sChestClose, pc->action_chest->GetCenter(), Chest::SOUND_DIST);
	}
	else if(gui->inventory->mode == I_LOOT_CONTAINER)
	{
		if(Net::IsLocal())
		{
			if(Net::IsServer())
			{
				NetChange& c = Add1(Net::changes);
				c.type = NetChange::USE_USABLE;
				c.unit = pc->unit;
				c.id = pc->unit->usable->netid;
				c.count = USE_USABLE_END;
			}
			pc->unit->UseUsable(nullptr);
		}
		else
			Net::PushChange(NetChange::STOP_TRADE);
	}

	if(Net::IsOnline() && (gui->inventory->mode == I_LOOT_BODY || gui->inventory->mode == I_LOOT_CHEST))
	{
		if(Net::IsClient())
			Net::PushChange(NetChange::STOP_TRADE);
		else if(gui->inventory->mode == I_LOOT_BODY)
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
	gui->inventory->mode = I_NONE;
}

// zamyka ekwipunek i wszystkie okna które on móg³by utworzyæ
void Game::CloseInventory()
{
	OnCloseInventory();
	gui->inventory->mode = I_NONE;
	if(gui->game_gui)
	{
		gui->inventory->inv_mine->Hide();
		gui->inventory->gp_trade->Hide();
	}
}

void Game::CloseAllPanels(bool close_mp_box)
{
	if(gui->game_gui)
		gui->game_gui->ClosePanels(close_mp_box);
}

bool Game::CanShowEndScreen()
{
	if(Net::IsLocal())
		return !QM.unique_completed_show && QM.unique_quests_completed == QM.unique_quests && L.city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
	else
		return QM.unique_completed_show && L.city_ctx && !dialog_context.dialog_mode && pc->unit->IsStanding();
}

void Game::UpdateGameDialogClient()
{
	if(dialog_context.show_choices)
	{
		if(gui->game_gui->UpdateChoice(dialog_context, dialog_choices.size()))
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

void Game::UpdateGameNet(float dt)
{
	if(gui->info_box->visible)
		return;

	if(Net::IsServer())
		UpdateServer(dt);
	else
		UpdateClient(dt);
}

DialogContext* Game::FindDialogContext(Unit* talker)
{
	assert(talker);
	if(dialog_context.talker == talker)
		return &dialog_context;
	if(Net::IsOnline())
	{
		for(PlayerInfo* info : N.players)
		{
			if(info->pc->dialog_ctx->talker == talker)
				return info->pc->dialog_ctx;
		}
	}
	return nullptr;
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
			gui->messages->AddGameMsg2(Format(txNeedItem, bu.item->name.c_str()), 2.f);
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
			gui->inventory->StartTrade2(I_LOOT_CONTAINER, &use);
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
			c.count = USE_USABLE_START;
		}
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::USE_USABLE;
		c.id = pc_data.before_player_ptr.usable->netid;
		c.count = USE_USABLE_START;

		if(IS_SET(bu.use_flags, BaseUsable::CONTAINER))
		{
			pc->action = PlayerController::Action_LootContainer;
			pc->action_usable = pc_data.before_player_ptr.usable;
			pc->chest_trade = &pc->action_usable->container->items;
		}

		pc->unit->action = A_PREPARE;
	}
}

void Game::UnitTalk(Unit& u, cstring text)
{
	assert(text && Net::IsLocal());

	gui->game_gui->AddSpeechBubble(&u, text);

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
		c.count = -1;
		N.net_strs.push_back(c.str);
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
		bool always_use = false;

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
					always_use = true;
					cstring enemies;

					switch(L.location->spawn)
					{
					case SG_GOBLINS:
						enemies = txSGOGoblins;
						break;
					case SG_BANDITS:
						enemies = txSGOBandits;
						break;
					case SG_ORCS:
						enemies = txSGOOrcs;
						break;
					default:
						always_use = false;
						enemies = nullptr;
						break;
					}

					if(always_use)
						text = Format(txAiCampFull, enemies);
					else
						text = nullptr;
				}
				else
					text = txAiCampEmpty;
				break;
			}

			if(text && (always_use || Rand() % 2 == 0))
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
					if(loc.state < Quest_Evil::Loc::TalkedAfterEnterLevel)
					{
						talker = quest_evil->cleric;
						text = txPortalCloseLevel;
						loc.state = Quest_Evil::Loc::TalkedAfterEnterLevel;
					}
				}
				else if(L.dungeon_level == 0 && loc.state == Quest_Evil::Loc::None)
				{
					talker = quest_evil->cleric;
					text = txPortalClose;
					loc.state = Quest_Evil::Loc::TalkedAfterEnterLocation;
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
					case LABYRINTH:
						s = txAiLabyrinth;
						break;
					}

					if(inside->spawn == SG_NONE)
						s += txAiNoEnemies;
					else
					{
						cstring enemies;

						switch(inside->spawn)
						{
						case SG_GOBLINS:
							enemies = txSGOGoblins;
							break;
						case SG_ORCS:
							enemies = txSGOOrcs;
							break;
						case SG_BANDITS:
							enemies = txSGOBandits;
							break;
						case SG_UNDEAD:
						case SG_NECROMANCERS:
						case SG_EVIL:
							enemies = txSGOUndead;
							break;
						case SG_MAGES:
							enemies = txSGOMages;
							break;
						case SG_GOLEMS:
							enemies = txSGOGolems;
							break;
						case SG_MAGES_AND_GOLEMS:
							enemies = txSGOMagesAndGolems;
							break;
						case SG_UNKNOWN:
							enemies = txSGOUnk;
							break;
						case SG_CHALLANGE:
							enemies = txSGOPowerfull;
							break;
						default:
							enemies = txSGOEnemies;
							break;
						}

						s += Format(txAiNearEnemies, enemies);
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

cstring Game::GetRandomIdleText(Unit& u)
{
	if(IS_SET(u.data->flags3, F3_DRUNK_MAGE) && QM.quest_mages2->mages_state < Quest_Mages2::State::MageCured)
		return RandomString(txAiDrunkMageText);

	int n = Rand() % 100;
	if(n == 0)
		return RandomString(txAiSecretText);

	bool hero_text;

	switch(u.data->group)
	{
	case G_CRAZIES:
		if(u.IsTeamMember())
		{
			if(n < 33)
				return RandomString(txAiInsaneText);
			else if(n < 66)
				hero_text = true;
			else
				hero_text = false;
		}
		else
		{
			if(n < 50)
				return RandomString(txAiInsaneText);
			else
				hero_text = false;
		}
		break;
	case G_CITIZENS:
		if(!u.IsTeamMember())
		{
			if(u.in_building >= 0 && (IS_SET(u.data->flags, F_AI_DRUNKMAN) || IS_SET(u.data->flags3, F3_DRUNKMAN_AFTER_CONTEST)))
			{
				int id;
				if(L.city_ctx->FindInn(id) && id == u.in_building)
				{
					if(IS_SET(u.data->flags, F_AI_DRUNKMAN) || QM.quest_contest->state != Quest_Contest::CONTEST_TODAY)
					{
						if(Rand() % 3 == 0)
							return RandomString(txAiDrunkText);
					}
					else
						return RandomString(txAiDrunkContestText);
				}
			}
			if(n < 50)
				return RandomString(txAiHumanText);
			else
				hero_text = false;
		}
		else
		{
			if(n < 10)
				return RandomString(txAiHumanText);
			else if(n < 55)
				hero_text = true;
			else
				hero_text = false;
		}
		break;
	case G_BANDITS:
		if(n < 50)
			return RandomString(txAiBanditText);
		else
			hero_text = false;
		break;
	case G_MAGES:
	case G_UNDEAD:
		if(IS_SET(u.data->flags, F_MAGE) && n < 50)
			return RandomString(txAiMageText);
		else
			hero_text = false;
		break;
	case G_GOBLINS:
		if(!IS_SET(u.data->flags2, F2_NOT_GOBLIN))
			return RandomString(txAiGoblinText);
		hero_text = false;
		break;
	case G_ORCS:
		return RandomString(txAiOrcText);
	case G_ANIMALS:
		return RandomString(txAiWildHunterText);
	default:
		assert(0);
		hero_text = false;
		break;
	}

	if(hero_text)
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
				c.count = count;
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
				c.count = count;
				c.a = team_count;
			}
		}
	}

	// czy trzeba budowaæ tymczasowy ekwipunek
	int rebuild_id = -1;
	if(&unit == pc->unit)
	{
		if(gui->inventory->inv_mine->visible || gui->inventory->gp_trade->visible)
			rebuild_id = 0;
	}
	else if(gui->inventory->gp_trade->visible && gui->inventory->inv_trade_other->unit == &unit)
		rebuild_id = 1;
	if(rebuild_id != -1)
		gui->inventory->BuildTmpInventory(rebuild_id);
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
	gui->Clear(false, true);
	lights_dt = 1.f;
	pc_data.autowalk = false;
	pc_data.selected_unit = pc->unit;
	fallback_t = -0.5f;
	fallback_type = FALLBACK::NONE;
	if(Net::IsLocal())
	{
		for(auto unit : Team.members)
			unit->frozen = FROZEN::NO;
	}

	// events v2
	for(Event& e : L.location->events)
	{
		if(e.type == EVENT_ENTER)
		{
			ScriptEvent event(EVENT_ENTER);
			event.location = L.location;
			e.quest->FireEvent(event);
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
				spawned = L.SpawnUnitInsideInn(*event->unit_to_spawn, event->unit_spawn_level);
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
				spawned = L.SpawnUnitNearLocation(L.local_ctx, pos, *event->unit_to_spawn, nullptr, event->unit_spawn_level);
			}
		}
		else
		{
			Room& room = lvl->GetRoom(event->spawn_unit_room, inside->HaveDownStairs());
			spawned = L.SpawnUnitInsideRoomOrNear(*lvl, room, *event->unit_to_spawn, event->unit_spawn_level);
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
				if(unit != spawned && unit->IsFriend(*spawned) && lvl->GetRoom(PosToPt(unit->pos)) == &room)
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
			room = lvl->GetRoom(PosToPt(spawned->pos));
		else
			room = &lvl->GetRoom(event->spawn_unit_room2, inside->HaveDownStairs());
		spawned2 = L.SpawnUnitInsideRoomOrNear(*lvl, *room, *event->unit_to_spawn2, event->unit_spawn_level2);
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
	case Quest_Dungeon::Item_DontSpawn:
		break;
	case Quest_Dungeon::Item_GiveStrongest:
		{
			Unit* best = nullptr;
			for(Unit* unit : *L.local_ctx.units)
			{
				if(unit->IsAlive() && unit->IsEnemy(*pc->unit) && (!best || unit->level > best->level))
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
				item = L.SpawnGroundItemInsideAnyRoom(*lvl, event->item_to_give[0]);
			else
				item = L.SpawnGroundItemInsideRadius(event->item_to_give[0], Vec2(128, 128), 10.f);
			if(devmode)
				Info("Generated item %s on ground (%g,%g).", event->item_to_give[0]->id.c_str(), item->pos.x, item->pos.z);
		}
		break;
	case Quest_Dungeon::Item_InTreasure:
		if(inside && (inside->type == L_CRYPT || inside->target == LABYRINTH))
		{
			Chest* chest = nullptr;

			if(inside->type == L_CRYPT)
			{
				Room& room = *lvl->rooms[inside->special_room];
				LocalVector2<Chest*> chests;
				for(Chest* chest2 : lvl->chests)
				{
					if(room.IsInside(chest2->pos))
						chests.push_back(chest2);
				}

				if(!chests.empty())
					chest = chests.RandomItem();
			}
			else
				chest = RandomItem(lvl->chests);

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
			Chest* chest = L.local_ctx.GetRandomFarChest(L.GetSpawnPoint());
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
