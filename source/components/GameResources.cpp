#include "Pch.h"
#include "GameCore.h"
#include "GameResources.h"
#include "Item.h"
#include "Action.h"
#include "Building.h"
#include "BaseTrap.h"
#include "Spell.h"
#include "SceneNode.h"
#include "Language.h"
#include "Game.h"
#include "GameGui.h"
#include "Level.h"
#include <ResourceManager.h>
#include <SoundManager.h>
#include <Mesh.h>
#include <Render.h>
#include <DirectX.h>

GameResources* global::game_res;

//=================================================================================================
GameResources::~GameResources()
{
	for(auto& item : item_texture_map)
		delete item.second;
	for(Texture* tex : over_item_textures)
		delete tex;
}

//=================================================================================================
void GameResources::Init()
{
	aHuman = res_mgr->Load<Mesh>("human.qmsh");
	rt_item = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));
	CreateMissingTexture();
}

//=================================================================================================
void GameResources::CreateMissingTexture()
{
	TEX tex = render->CreateTexture(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));
	TextureLock lock(tex);
	const uint col[2] = { Color(255, 0, 255), Color(0, 255, 0) };
	for(int y = 0; y < ITEM_IMAGE_SIZE; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < ITEM_IMAGE_SIZE; ++x)
		{
			*pix = col[(x >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) + (y >= ITEM_IMAGE_SIZE / 2 ? 1 : 0) % 2];
			++pix;
		}
	}

	missing_item_texture.tex = tex;
	missing_item_texture.state = ResourceState::Loaded;
}

//=================================================================================================
void GameResources::LoadLanguage()
{
	txLoadGuiTextures = Str("loadGuiTextures");
	txLoadTerrainTextures = Str("loadTerrainTextures");
	txLoadParticles = Str("loadParticles");
	txLoadModels = Str("loadModels");
	txLoadSounds = Str("loadSounds");
	txLoadMusic = Str("loadMusic");
}

//=================================================================================================
void GameResources::LoadData()
{
	// gui textures
	res_mgr->AddTaskCategory(txLoadGuiTextures);
	tBlack = res_mgr->Load<Texture>("czern.bmp");
	tPortal = res_mgr->Load<Texture>("dark_portal.png");
	tWarning = res_mgr->Load<Texture>("warning.png");
	tError = res_mgr->Load<Texture>("error.png");
	Action::LoadData();
	game_gui->LoadData();
	net->LoadData();

	// particles
	res_mgr->AddTaskCategory(txLoadParticles);
	tBlood[BLOOD_RED] = res_mgr->Load<Texture>("krew.png");
	tBlood[BLOOD_GREEN] = res_mgr->Load<Texture>("krew2.png");
	tBlood[BLOOD_BLACK] = res_mgr->Load<Texture>("krew3.png");
	tBlood[BLOOD_BONE] = res_mgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_ROCK] = res_mgr->Load<Texture>("kamien.png");
	tBlood[BLOOD_IRON] = res_mgr->Load<Texture>("iskra.png");
	tBloodSplat[BLOOD_RED] = res_mgr->Load<Texture>("krew_slad.png");
	tBloodSplat[BLOOD_GREEN] = res_mgr->Load<Texture>("krew_slad2.png");
	tBloodSplat[BLOOD_BLACK] = res_mgr->Load<Texture>("krew_slad3.png");
	tBloodSplat[BLOOD_BONE] = nullptr;
	tBloodSplat[BLOOD_ROCK] = nullptr;
	tBloodSplat[BLOOD_IRON] = nullptr;
	tSpark = res_mgr->Load<Texture>("iskra.png");
	tSpawn = res_mgr->Load<Texture>("spawn_fog.png");
	tLightingLine = res_mgr->Load<Texture>("lighting_line.png");
	game_level->LoadData();

	// preload terrain textures
	res_mgr->AddTaskCategory(txLoadTerrainTextures);
	tGrass = res_mgr->Load<Texture>("trawa.jpg");
	tGrass2 = res_mgr->Load<Texture>("Grass0157_5_S.jpg");
	tGrass3 = res_mgr->Load<Texture>("LeavesDead0045_1_S.jpg");
	tRoad = res_mgr->Load<Texture>("droga.jpg");
	tFootpath = res_mgr->Load<Texture>("ziemia.jpg");
	tField = res_mgr->Load<Texture>("pole.jpg");
	tFloorBase.diffuse = res_mgr->Load<Texture>("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = res_mgr->Load<Texture>("sciana.jpg");
	tWallBase.normal = res_mgr->Load<Texture>("sciana_nrm.jpg");
	tWallBase.specular = res_mgr->Load<Texture>("sciana_spec.jpg");
	tCeilBase.diffuse = res_mgr->Load<Texture>("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;
	BaseLocation::PreloadTextures();
	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	// meshes
	res_mgr->AddTaskCategory(txLoadModels);
	res_mgr->Load(aHuman);
	aBox = res_mgr->Load<Mesh>("box.qmsh");
	aCylinder = res_mgr->Load<Mesh>("cylinder.qmsh");
	aSphere = res_mgr->Load<Mesh>("sphere.qmsh");
	aCapsule = res_mgr->Load<Mesh>("capsule.qmsh");
	aHair[0] = res_mgr->Load<Mesh>("hair1.qmsh");
	aHair[1] = res_mgr->Load<Mesh>("hair2.qmsh");
	aHair[2] = res_mgr->Load<Mesh>("hair3.qmsh");
	aHair[3] = res_mgr->Load<Mesh>("hair4.qmsh");
	aHair[4] = res_mgr->Load<Mesh>("hair5.qmsh");
	aEyebrows = res_mgr->Load<Mesh>("eyebrows.qmsh");
	aMustache[0] = res_mgr->Load<Mesh>("mustache1.qmsh");
	aMustache[1] = res_mgr->Load<Mesh>("mustache2.qmsh");
	aBeard[0] = res_mgr->Load<Mesh>("beard1.qmsh");
	aBeard[1] = res_mgr->Load<Mesh>("beard2.qmsh");
	aBeard[2] = res_mgr->Load<Mesh>("beard3.qmsh");
	aBeard[3] = res_mgr->Load<Mesh>("beard4.qmsh");
	aBeard[4] = res_mgr->Load<Mesh>("beardm1.qmsh");
	aArrow = res_mgr->Load<Mesh>("strzala.qmsh");
	aSkybox = res_mgr->Load<Mesh>("skybox.qmsh");
	aBag = res_mgr->Load<Mesh>("worek.qmsh");
	aChest = res_mgr->Load<Mesh>("skrzynia.qmsh");
	aGrating = res_mgr->Load<Mesh>("kratka.qmsh");
	aDoorWall = res_mgr->Load<Mesh>("nadrzwi.qmsh");
	aDoorWall2 = res_mgr->Load<Mesh>("nadrzwi2.qmsh");
	aStairsDown = res_mgr->Load<Mesh>("schody_dol.qmsh");
	aStairsDown2 = res_mgr->Load<Mesh>("schody_dol2.qmsh");
	aStairsUp = res_mgr->Load<Mesh>("schody_gora.qmsh");
	aSpellball = res_mgr->Load<Mesh>("spellball.qmsh");
	aDoor = res_mgr->Load<Mesh>("drzwi.qmsh");
	aDoor2 = res_mgr->Load<Mesh>("drzwi2.qmsh");
	aStun = res_mgr->Load<Mesh>("stunned.qmsh");

	PreloadBuildings();
	PreloadTraps();
	PreloadSpells();
	PreloadObjects();
	PreloadItems();

	// physic meshes
	vdStairsUp = res_mgr->Load<VertexData>("schody_gora.phy");
	vdStairsDown = res_mgr->Load<VertexData>("schody_dol.phy");
	vdDoorHole = res_mgr->Load<VertexData>("nadrzwi.phy");

	// sounds
	res_mgr->AddTaskCategory(txLoadSounds);
	sGulp = res_mgr->Load<Sound>("gulp.mp3");
	sCoins = res_mgr->Load<Sound>("moneta2.mp3");
	sBow[0] = res_mgr->Load<Sound>("bow1.mp3");
	sBow[1] = res_mgr->Load<Sound>("bow2.mp3");
	sDoor[0] = res_mgr->Load<Sound>("drzwi-02.mp3");
	sDoor[1] = res_mgr->Load<Sound>("drzwi-03.mp3");
	sDoor[2] = res_mgr->Load<Sound>("drzwi-04.mp3");
	sDoorClose = res_mgr->Load<Sound>("104528__skyumori__door-close-sqeuak-02.mp3");
	sDoorClosed[0] = res_mgr->Load<Sound>("wont_budge.mp3");
	sDoorClosed[1] = res_mgr->Load<Sound>("wont_budge2.mp3");
	sItem[0] = res_mgr->Load<Sound>("bottle.wav"); // potion
	sItem[1] = res_mgr->Load<Sound>("armor-light.wav"); // light armor
	sItem[2] = res_mgr->Load<Sound>("chainmail1.wav"); // heavy armor
	sItem[3] = res_mgr->Load<Sound>("metal-ringing.wav"); // crystal
	sItem[4] = res_mgr->Load<Sound>("wood-small.wav"); // bow
	sItem[5] = res_mgr->Load<Sound>("cloth-heavy.wav"); // shield
	sItem[6] = res_mgr->Load<Sound>("sword-unsheathe.wav"); // weapon
	sItem[7] = res_mgr->Load<Sound>("interface3.wav"); // other
	sItem[8] = res_mgr->Load<Sound>("amulet.mp3"); // amulet
	sItem[9] = res_mgr->Load<Sound>("ring.mp3"); // ring
	sChestOpen = res_mgr->Load<Sound>("chest_open.mp3");
	sChestClose = res_mgr->Load<Sound>("chest_close.mp3");
	sDoorBudge = res_mgr->Load<Sound>("door_budge.mp3");
	sRock = res_mgr->Load<Sound>("atak_kamien.mp3");
	sWood = res_mgr->Load<Sound>("atak_drewno.mp3");
	sCrystal = res_mgr->Load<Sound>("atak_krysztal.mp3");
	sMetal = res_mgr->Load<Sound>("atak_metal.mp3");
	sBody[0] = res_mgr->Load<Sound>("atak_cialo.mp3");
	sBody[1] = res_mgr->Load<Sound>("atak_cialo2.mp3");
	sBody[2] = res_mgr->Load<Sound>("atak_cialo3.mp3");
	sBody[3] = res_mgr->Load<Sound>("atak_cialo4.mp3");
	sBody[4] = res_mgr->Load<Sound>("atak_cialo5.mp3");
	sBone = res_mgr->Load<Sound>("atak_kosci.mp3");
	sSkin = res_mgr->Load<Sound>("atak_skora.mp3");
	sArenaFight = res_mgr->Load<Sound>("arena_fight.mp3");
	sArenaWin = res_mgr->Load<Sound>("arena_wygrana.mp3");
	sArenaLost = res_mgr->Load<Sound>("arena_porazka.mp3");
	sUnlock = res_mgr->Load<Sound>("unlock.mp3");
	sEvil = res_mgr->Load<Sound>("TouchofDeath.mp3");
	sEat = res_mgr->Load<Sound>("eat.mp3");
	sSummon = res_mgr->Load<Sound>("whooshy-puff.wav");
	sZap = res_mgr->Load<Sound>("zap.mp3");
	sCancel = res_mgr->Load<Sound>("cancel.mp3");

	// music
	LoadMusic(MusicType::Title);
}

//=================================================================================================
void GameResources::PreloadBuildings()
{
	for(Building* b : Building::buildings)
	{
		if(b->mesh)
			res_mgr->LoadMeshMetadata(b->mesh);
		if(b->inside_mesh)
			res_mgr->LoadMeshMetadata(b->inside_mesh);
	}
}

//=================================================================================================
void GameResources::PreloadTraps()
{
	for(uint i = 0; i < BaseTrap::n_traps; ++i)
	{
		BaseTrap& t = BaseTrap::traps[i];
		if(t.mesh_id)
		{
			t.mesh = res_mgr->Get<Mesh>(t.mesh_id);
			res_mgr->LoadMeshMetadata(t.mesh);

			Mesh::Point* pt = t.mesh->FindPoint("hitbox");
			assert(pt);
			if(pt->type == Mesh::Point::BOX)
			{
				t.rw = pt->size.x;
				t.h = pt->size.z;
			}
			else
				t.h = t.rw = pt->size.x;
		}
		if(t.mesh_id2)
			t.mesh2 = res_mgr->Get<Mesh>(t.mesh_id2);
		if(t.sound_id)
			t.sound = res_mgr->Get<Sound>(t.sound_id);
		if(t.sound_id2)
			t.sound2 = res_mgr->Get<Sound>(t.sound_id2);
		if(t.sound_id3)
			t.sound3 = res_mgr->Get<Sound>(t.sound_id3);
	}
}

//=================================================================================================
void GameResources::PreloadSpells()
{
	for(Spell* spell_ptr : Spell::spells)
	{
		Spell& spell = *spell_ptr;

		if(spell.sound_cast)
			res_mgr->Load(spell.sound_cast);
		if(spell.sound_hit)
			res_mgr->Load(spell.sound_hit);
		if(spell.tex)
			res_mgr->Load(spell.tex);
		if(spell.tex_particle)
			res_mgr->Load(spell.tex_particle);
		if(spell.tex_explode)
			res_mgr->Load(spell.tex_explode);
		if(spell.mesh)
			res_mgr->Load(spell.mesh);

		if(spell.type == Spell::Ball || spell.type == Spell::Point)
			spell.shape = new btSphereShape(spell.size);
	}
}

//=================================================================================================
void GameResources::PreloadObjects()
{
	for(BaseObject* p_obj : BaseObject::objs)
	{
		BaseObject& obj = *p_obj;
		if(obj.mesh || obj.variants)
		{
			if(obj.mesh && !IsSet(obj.flags, OBJ_SCALEABLE | OBJ_NO_PHYSICS) && obj.type == OBJ_CYLINDER)
				obj.shape = new btCylinderShape(btVector3(obj.r, obj.h, obj.r));

			Mesh::Point* point;

			if(IsSet(obj.flags, OBJ_PRELOAD))
			{
				if(obj.variants)
				{
					for(Mesh* mesh : obj.variants->meshes)
						res_mgr->Load(mesh);
				}
				else if(obj.mesh)
					res_mgr->Load(obj.mesh);
			}

			if(obj.variants)
			{
				assert(!IsSet(obj.flags, OBJ_DOUBLE_PHYSICS | OBJ_MULTI_PHYSICS)); // not supported for variant mesh yet
				Mesh* mesh = obj.variants->meshes[0];
				res_mgr->LoadMeshMetadata(mesh);
				point = mesh->FindPoint("hit");
			}
			else
			{
				res_mgr->LoadMeshMetadata(obj.mesh);
				point = obj.mesh->FindPoint("hit");
			}

			if(!point || !point->IsBox() || IsSet(obj.flags, OBJ_BUILDING | OBJ_SCALEABLE) || obj.type == OBJ_CYLINDER)
			{
				obj.size = Vec2::Zero;
				obj.matrix = nullptr;
			}
			else
			{
				assert(point->size.x >= 0 && point->size.y >= 0 && point->size.z >= 0);
				obj.matrix = &point->mat;
				obj.size = point->size.XZ();

				if(!IsSet(obj.flags, OBJ_NO_PHYSICS))
					obj.shape = new btBoxShape(ToVector3(point->size));

				if(IsSet(obj.flags, OBJ_PHY_ROT))
					obj.type = OBJ_HITBOX_ROT;

				if(IsSet(obj.flags, OBJ_MULTI_PHYSICS))
				{
					LocalVector2<Mesh::Point*> points;
					Mesh::Point* prev_point = point;

					while(true)
					{
						Mesh::Point* new_point = obj.mesh->FindNextPoint("hit", prev_point);
						if(new_point)
						{
							assert(new_point->IsBox() && new_point->size.x >= 0 && new_point->size.y >= 0 && new_point->size.z >= 0);
							points.push_back(new_point);
							prev_point = new_point;
						}
						else
							break;
					}

					assert(points.size() > 1u);
					obj.next_obj = new BaseObject[points.size() + 1];
					for(uint i = 0, size = points.size(); i < size; ++i)
					{
						BaseObject& o2 = obj.next_obj[i];
						o2.shape = new btBoxShape(ToVector3(points[i]->size));
						if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
							o2.flags = OBJ_PHY_BLOCKS_CAM;
						o2.matrix = &points[i]->mat;
						o2.size = points[i]->size.XZ();
						o2.type = obj.type;
					}
					obj.next_obj[points.size()].shape = nullptr;
				}
				else if(IsSet(obj.flags, OBJ_DOUBLE_PHYSICS))
				{
					Mesh::Point* point2 = obj.mesh->FindNextPoint("hit", point);
					if(point2 && point2->IsBox())
					{
						assert(point2->size.x >= 0 && point2->size.y >= 0 && point2->size.z >= 0);
						obj.next_obj = new BaseObject;
						if(!IsSet(obj.flags, OBJ_NO_PHYSICS))
						{
							btBoxShape* shape = new btBoxShape(ToVector3(point2->size));
							obj.next_obj->shape = shape;
							if(IsSet(obj.flags, OBJ_PHY_BLOCKS_CAM))
								obj.next_obj->flags = OBJ_PHY_BLOCKS_CAM;
						}
						else
							obj.next_obj->shape = nullptr;
						obj.next_obj->matrix = &point2->mat;
						obj.next_obj->size = point2->size.XZ();
						obj.next_obj->type = obj.type;
					}
				}
			}
		}
	}
}

//=================================================================================================
void GameResources::PreloadItems()
{
	PreloadItem(Item::Get("beer"));
	PreloadItem(Item::Get("vodka"));
	PreloadItem(Item::Get("spirit"));
	PreloadItem(Item::Get("p_hp"));
	PreloadItem(Item::Get("p_hp2"));
	PreloadItem(Item::Get("p_hp3"));
	PreloadItem(Item::Get("gold"));
	ItemListResult list = ItemList::Get("normal_food");
	for(const Item* item : list.lis->items)
		PreloadItem(item);
	list = ItemList::Get("orc_food");
	for(const Item* item : list.lis->items)
		PreloadItem(item);
}

//=================================================================================================
void GameResources::PreloadItem(const Item* p_item)
{
	Item& item = *(Item*)p_item;
	if(item.state == ResourceState::Loaded)
		return;

	if(res_mgr->IsLoadScreen())
	{
		if(item.state != ResourceState::Loading)
		{
			if(item.type == IT_ARMOR)
			{
				Armor& armor = item.ToArmor();
				if(!armor.tex_override.empty())
				{
					for(TexOverride& tex_o : armor.tex_override)
					{
						if(tex_o.diffuse)
							res_mgr->Load(tex_o.diffuse);
					}
				}
			}
			else if(item.type == IT_BOOK)
			{
				Book& book = item.ToBook();
				res_mgr->Load(book.scheme->tex);
			}

			if(item.mesh)
				res_mgr->Load(item.mesh);
			else if(item.tex)
				res_mgr->Load(item.tex);
			res_mgr->AddTask(&item, TaskCallback(this, &GameResources::GenerateItemIconTask));

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
				for(TexOverride& tex_o : armor.tex_override)
				{
					if(tex_o.diffuse)
						res_mgr->Load(tex_o.diffuse);
				}
			}
		}
		else if(item.type == IT_BOOK)
		{
			Book& book = item.ToBook();
			res_mgr->Load(book.scheme->tex);
		}

		if(item.mesh)
			res_mgr->Load(item.mesh);
		else if(item.tex)
			res_mgr->Load(item.tex);
		GenerateItemIcon(item);

		item.state = ResourceState::Loaded;
	}
}

//=================================================================================================
void GameResources::GenerateItemIconTask(TaskData& task_data)
{
	Item& item = *(Item*)task_data.ptr;
	GenerateItemIcon(item);
}

//=================================================================================================
void GameResources::GenerateItemIcon(Item& item)
{
	item.state = ResourceState::Loaded;

	// use missing texture if no mesh/texture
	if(!item.mesh && !item.tex)
	{
		item.icon = &missing_item_texture;
		item.flags &= ~ITEM_GROUND_MESH;
		return;
	}

	// if item use image, set it as icon
	if(item.tex)
	{
		item.icon = item.tex;
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

	Texture* tex;
	do
	{
		DrawItemIcon(item, rt_item, 0.f);
		tex = render->CopyToTexture(rt_item);
	}
	while(tex == nullptr);

	item.icon = tex;
	if(it != item_texture_map.end())
		item_texture_map.insert(it, ItemTextureMap::value_type(item.mesh, tex));
	else
		over_item_textures.push_back(tex);
}

//=================================================================================================
void GameResources::DrawItemIcon(const Item& item, RenderTarget* target, float rot)
{
	IDirect3DDevice9* device = render->GetDevice();

	if(IsSet(ITEM_ALPHA, item.flags))
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
	const TexOverride* tex_override = nullptr;
	if(item.type == IT_ARMOR)
	{
		if(const Armor& armor = item.ToArmor(); !armor.tex_override.empty())
		{
			tex_override = armor.GetTextureOverride();
			assert(armor.tex_override.size() == mesh.head.n_subs);
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

	ID3DXEffect* effect = game->eMesh;
	V(effect->SetTechnique(game->techMesh));
	V(effect->SetMatrix(game->hMeshCombined, (D3DXMATRIX*)&(matWorld * matView * matProj)));
	V(effect->SetMatrix(game->hMeshWorld, (D3DXMATRIX*)&matWorld));
	V(effect->SetVector(game->hMeshFogColor, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(effect->SetVector(game->hMeshFogParam, (D3DXVECTOR4*)&Vec4(25.f, 50.f, 25.f, 0)));
	V(effect->SetVector(game->hMeshAmbientColor, (D3DXVECTOR4*)&Vec4(0.5f, 0.5f, 0.5f, 1)));
	V(effect->SetVector(game->hMeshTint, (D3DXVECTOR4*)&Vec4(1, 1, 1, 1)));
	V(effect->SetRawValue(game->hMeshLights, &ld, 0, sizeof(LightData)));

	V(device->SetVertexDeclaration(render->GetVertexDeclaration(mesh.vertex_decl)));
	V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
	V(device->SetIndices(mesh.ib));

	uint passes;
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	for(int i = 0; i < mesh.head.n_subs; ++i)
	{
		const Mesh::Submesh& sub = mesh.subs[i];
		V(effect->SetTexture(game->hMeshTex, mesh.GetTexture(i, tex_override)));
		V(effect->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
	}

	V(effect->EndPass());
	V(effect->End());
	V(device->EndScene());

	render->SetTarget(nullptr);
}

//=================================================================================================
Sound* GameResources::GetMaterialSound(MATERIAL_TYPE attack_mat, MATERIAL_TYPE hit_mat)
{
	switch(hit_mat)
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

//=================================================================================================
Sound* GameResources::GetItemSound(const Item* item)
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
		if(Any(item->ToConsumable().cons_type, ConsumableType::Food, ConsumableType::Herb))
			return sItem[7];
		else
			return sItem[0];
	case IT_OTHER:
		if(IsSet(item->flags, ITEM_CRYSTAL_SOUND))
			return sItem[3];
		else
			return sItem[7];
	case IT_GOLD:
		return sCoins;
	default:
		return sItem[7];
	}
}

//=================================================================================================
void GameResources::LoadMusic(MusicType type, bool new_load_screen, bool instant)
{
	if(sound_mgr->IsMusicDisabled())
		return;

	bool first = true;

	for(MusicTrack* track : MusicTrack::tracks)
	{
		if(track->type == type)
		{
			if(first)
			{
				if(track->music->IsLoaded())
				{
					// music for this type is loaded
					return;
				}
				if(new_load_screen)
					res_mgr->AddTaskCategory(txLoadMusic);
				first = false;
			}
			if(instant)
				res_mgr->LoadInstant(track->music);
			else
				res_mgr->Load(track->music);
		}
	}
}

//=================================================================================================
void GameResources::LoadCommonMusic()
{
	LoadMusic(MusicType::Boss, false);
	LoadMusic(MusicType::Death, false);
	LoadMusic(MusicType::Travel, false);
}
