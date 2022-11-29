#include "Pch.h"
#include "GameResources.h"

#include "Ability.h"
#include "BaseTrap.h"
#include "Building.h"
#include "Game.h"
#include "GameGui.h"
#include "Item.h"
#include "Language.h"
#include "Level.h"
#include "Tile.h"

#include <Mesh.h>
#include <Render.h>
#include <RenderTarget.h>
#include <ResourceManager.h>
#include <SoundManager.h>
#include <Scene.h>
#include <SceneManager.h>

GameResources* game_res;

//=================================================================================================
GameResources::GameResources() : scene(nullptr), node(nullptr), camera(nullptr)
{
}

//=================================================================================================
GameResources::~GameResources()
{
	delete scene;
	delete camera;
	for(auto& item : item_texture_map)
		delete item.second;
	for(Texture* tex : over_item_textures)
		delete tex;
}

//=================================================================================================
void GameResources::Init()
{
	scene = new Scene;
	scene->clear_color = Color::None;

	Light light;
	light.range = 10.f;
	light.color = Vec4::One;
	scene->lights.push_back(light);

	node = SceneNode::Get();
	scene->Add(node);

	camera = new Camera;

	aHuman = resMgr->Load<Mesh>("human.qmsh");
	rt_item = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE), RenderTarget::F_NO_DRAW);
	missing_item_texture = CreatePlaceholderTexture(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));
}

//=================================================================================================
Texture* GameResources::CreatePlaceholderTexture(const Int2& size)
{
	cstring name = Format("placeholder;%d;%d", size.x, size.y);
	Texture* tex = resMgr->TryGet<Texture>(name);
	if(tex)
		return tex;

	Buf buf;
	Color* texData = buf.Get<Color>(sizeof(Color) * size.x * size.y);
	const uint col[2] = { Color(255, 0, 255), Color(0, 255, 0) };
	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
			texData[x + y * size.x] = col[(x >= size.x / 2 ? 1 : 0) + (y >= size.y / 2 ? 1 : 0) % 2];
	}
	TEX t = render->CreateImmutableTexture(size, texData);

	tex = new Texture;
	tex->path = name;
	tex->filename = tex->path.c_str();
	tex->tex = t;
	tex->state = ResourceState::Loaded;
	resMgr->AddResource(tex);

	return tex;
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
	resMgr->AddTaskCategory(txLoadGuiTextures);
	tBlack = resMgr->Load<Texture>("czern.bmp");
	tWarning = resMgr->Load<Texture>("warning.png");
	tError = resMgr->Load<Texture>("error.png");
	game_gui->LoadData();
	net->LoadData();

	// particles
	resMgr->AddTaskCategory(txLoadParticles);
	tBlood[BLOOD_RED] = resMgr->Load<Texture>("krew.png");
	tBlood[BLOOD_GREEN] = resMgr->Load<Texture>("krew2.png");
	tBlood[BLOOD_BLACK] = resMgr->Load<Texture>("krew3.png");
	tBlood[BLOOD_BONE] = resMgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_ROCK] = resMgr->Load<Texture>("kamien.png");
	tBlood[BLOOD_IRON] = resMgr->Load<Texture>("iskra.png");
	tBlood[BLOOD_SLIME] = resMgr->Load<Texture>("slime_part.png");
	tBloodSplat[BLOOD_RED] = resMgr->Load<Texture>("krew_slad.png");
	tBloodSplat[BLOOD_GREEN] = resMgr->Load<Texture>("krew_slad2.png");
	tBloodSplat[BLOOD_BLACK] = resMgr->Load<Texture>("krew_slad3.png");
	tBloodSplat[BLOOD_BONE] = nullptr;
	tBloodSplat[BLOOD_ROCK] = nullptr;
	tBloodSplat[BLOOD_IRON] = nullptr;
	tBloodSplat[BLOOD_SLIME] = nullptr;
	tSpark = resMgr->Load<Texture>("iskra.png");
	tSpawn = resMgr->Load<Texture>("spawn_fog.png");
	tLightingLine = resMgr->Load<Texture>("lighting_line.png");
	tFlare = resMgr->Load<Texture>("flare.png");
	tFlare2 = resMgr->Load<Texture>("flare2.png");
	tWater = resMgr->Load<Texture>("water.png");
	tVignette = resMgr->Load<Texture>("vignette.jpg");

	// preload terrain textures
	resMgr->AddTaskCategory(txLoadTerrainTextures);
	tGrass = resMgr->Load<Texture>("trawa.jpg");
	tGrass2 = resMgr->Load<Texture>("Grass0157_5_S.jpg");
	tGrass3 = resMgr->Load<Texture>("LeavesDead0045_1_S.jpg");
	tRoad = resMgr->Load<Texture>("droga.jpg");
	tFootpath = resMgr->Load<Texture>("ziemia.jpg");
	tField = resMgr->Load<Texture>("pole.jpg");
	tFloorBase.diffuse = resMgr->Load<Texture>("droga.jpg");
	tFloorBase.normal = nullptr;
	tFloorBase.specular = nullptr;
	tWallBase.diffuse = resMgr->Load<Texture>("sciana.jpg");
	tWallBase.normal = resMgr->Load<Texture>("sciana_nrm.jpg");
	tWallBase.specular = resMgr->Load<Texture>("sciana_spec.jpg");
	tCeilBase.diffuse = resMgr->Load<Texture>("sufit.jpg");
	tCeilBase.normal = nullptr;
	tCeilBase.specular = nullptr;
	BaseLocation::PreloadTextures();
	tFloor[1] = tFloorBase;
	tCeil[1] = tCeilBase;
	tWall[1] = tWallBase;

	// meshes
	resMgr->AddTaskCategory(txLoadModels);
	resMgr->Load(aHuman);
	aHair[0] = resMgr->Load<Mesh>("hair1.qmsh");
	aHair[1] = resMgr->Load<Mesh>("hair2.qmsh");
	aHair[2] = resMgr->Load<Mesh>("hair3.qmsh");
	aHair[3] = resMgr->Load<Mesh>("hair4.qmsh");
	aHair[4] = resMgr->Load<Mesh>("hair5.qmsh");
	aEyebrows = resMgr->Load<Mesh>("eyebrows.qmsh");
	aMustache[0] = resMgr->Load<Mesh>("mustache1.qmsh");
	aMustache[1] = resMgr->Load<Mesh>("mustache2.qmsh");
	aBeard[0] = resMgr->Load<Mesh>("beard1.qmsh");
	aBeard[1] = resMgr->Load<Mesh>("beard2.qmsh");
	aBeard[2] = resMgr->Load<Mesh>("beard3.qmsh");
	aBeard[3] = resMgr->Load<Mesh>("beard4.qmsh");
	aBeard[4] = resMgr->Load<Mesh>("beardm1.qmsh");
	aArrow = resMgr->Load<Mesh>("arrow.qmsh");
	aSkybox = resMgr->Load<Mesh>("skybox.qmsh");
	aBag = resMgr->Load<Mesh>("worek.qmsh");
	aGrating = resMgr->Load<Mesh>("kratka.qmsh");
	aDoorWall = resMgr->Load<Mesh>("nadrzwi.qmsh");
	aDoorWall2 = resMgr->Load<Mesh>("nadrzwi2.qmsh");
	aStairsDown = resMgr->Load<Mesh>("schody_dol.qmsh");
	aStairsDown2 = resMgr->Load<Mesh>("schody_dol2.qmsh");
	aStairsUp = resMgr->Load<Mesh>("schody_gora.qmsh");
	aSpellball = resMgr->Load<Mesh>("spellball.qmsh");
	aDoor = resMgr->Load<Mesh>("drzwi.qmsh");
	aDoor2 = resMgr->Load<Mesh>("drzwi2.qmsh");
	aStun = resMgr->Load<Mesh>("stunned.qmsh");
	aPortal = resMgr->Load<Mesh>("dark_portal.qmsh");
	aDungeonDoor = resMgr->Load<Mesh>("dungeon_door.qmsh");
	mVine = resMgr->Load<Mesh>("vine.qmsh");

	PreloadBuildings();
	PreloadTraps();
	PreloadAbilities();
	PreloadObjects();
	PreloadItem(Item::Get("gold"));

	// physic meshes
	vdStairsUp = resMgr->Load<VertexData>("schody_gora.phy");
	vdStairsDown = resMgr->Load<VertexData>("schody_dol.phy");
	vdDoorHole = resMgr->Load<VertexData>("nadrzwi.phy");

	// sounds
	resMgr->AddTaskCategory(txLoadSounds);
	sGulp = resMgr->Load<Sound>("gulp.mp3");
	sCoins = resMgr->Load<Sound>("moneta2.mp3");
	sBow[0] = resMgr->Load<Sound>("bow1.mp3");
	sBow[1] = resMgr->Load<Sound>("bow2.mp3");
	sDoor[0] = resMgr->Load<Sound>("drzwi-02.mp3");
	sDoor[1] = resMgr->Load<Sound>("drzwi-03.mp3");
	sDoor[2] = resMgr->Load<Sound>("drzwi-04.mp3");
	sDoorClose = resMgr->Load<Sound>("104528__skyumori__door-close-sqeuak-02.mp3");
	sDoorClosed[0] = resMgr->Load<Sound>("wont_budge.mp3");
	sDoorClosed[1] = resMgr->Load<Sound>("wont_budge2.mp3");
	sItem[0] = resMgr->Load<Sound>("bottle.wav"); // potion
	sItem[1] = resMgr->Load<Sound>("armor-light.wav"); // light armor
	sItem[2] = resMgr->Load<Sound>("chainmail1.wav"); // heavy armor
	sItem[3] = resMgr->Load<Sound>("metal-ringing.wav"); // crystal
	sItem[4] = resMgr->Load<Sound>("wood-small.wav"); // bow
	sItem[5] = resMgr->Load<Sound>("cloth-heavy.wav"); // shield
	sItem[6] = resMgr->Load<Sound>("sword-unsheathe.wav"); // weapon
	sItem[7] = resMgr->Load<Sound>("interface3.wav"); // other
	sItem[8] = resMgr->Load<Sound>("amulet.mp3"); // amulet
	sItem[9] = resMgr->Load<Sound>("ring.mp3"); // ring
	sChestOpen = resMgr->Load<Sound>("chest_open.mp3");
	sChestClose = resMgr->Load<Sound>("chest_close.mp3");
	sDoorBudge = resMgr->Load<Sound>("door_budge.mp3");
	sRock = resMgr->Load<Sound>("atak_kamien.mp3");
	sWood = resMgr->Load<Sound>("atak_drewno.mp3");
	sCrystal = resMgr->Load<Sound>("atak_krysztal.mp3");
	sMetal = resMgr->Load<Sound>("atak_metal.mp3");
	sBody[0] = resMgr->Load<Sound>("atak_cialo.mp3");
	sBody[1] = resMgr->Load<Sound>("atak_cialo2.mp3");
	sBody[2] = resMgr->Load<Sound>("atak_cialo3.mp3");
	sBody[3] = resMgr->Load<Sound>("atak_cialo4.mp3");
	sBody[4] = resMgr->Load<Sound>("atak_cialo5.mp3");
	sBone = resMgr->Load<Sound>("atak_kosci.mp3");
	sSkin = resMgr->Load<Sound>("atak_skora.mp3");
	sSlime = resMgr->Load<Sound>("slime_hit.wav");
	sArenaFight = resMgr->Load<Sound>("arena_fight.mp3");
	sArenaWin = resMgr->Load<Sound>("arena_wygrana.mp3");
	sArenaLost = resMgr->Load<Sound>("arena_porazka.mp3");
	sUnlock = resMgr->Load<Sound>("unlock.mp3");
	sEvil = resMgr->Load<Sound>("TouchofDeath.mp3");
	sEat = resMgr->Load<Sound>("eat.mp3");
	sSummon = resMgr->Load<Sound>("whooshy-puff.wav");
	sZap = resMgr->Load<Sound>("zap.mp3");
	sCancel = resMgr->Load<Sound>("cancel.mp3");
	sCoughs = resMgr->Load<Sound>("coughs.mp3");

	// music
	LoadMusic(MusicType::Title);
}

//=================================================================================================
void GameResources::PreloadBuildings()
{
	for(Building* b : Building::buildings)
	{
		if(b->mesh)
		{
			resMgr->LoadMeshMetadata(b->mesh);
			if(!b->mesh->FindPoint("o_s_point"))
				Warn("Missing building '%s' point.", b->id.c_str());
		}
		if(b->inside_mesh)
			resMgr->LoadMeshMetadata(b->inside_mesh);
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
			t.mesh = resMgr->Get<Mesh>(t.mesh_id);
			resMgr->LoadMeshMetadata(t.mesh);

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
		if(t.sound_id)
			t.sound = resMgr->Get<Sound>(t.sound_id);
		if(t.sound_id2)
			t.sound2 = resMgr->Get<Sound>(t.sound_id2);
		if(t.sound_id3)
			t.sound3 = resMgr->Get<Sound>(t.sound_id3);
	}
}

//=================================================================================================
void GameResources::PreloadAbilities()
{
	for(Ability* ptr_ability : Ability::abilities)
	{
		Ability& ability = *ptr_ability;

		if(ability.sound_cast)
			resMgr->Load(ability.sound_cast);
		if(ability.sound_hit)
			resMgr->Load(ability.sound_hit);
		if(ability.tex)
			resMgr->Load(ability.tex);
		if(ability.tex_particle)
			resMgr->Load(ability.tex_particle);
		if(ability.tex_explode.diffuse)
			resMgr->Load(ability.tex_explode.diffuse);
		if(ability.tex_icon)
			resMgr->Load(ability.tex_icon);
		if(ability.mesh)
			resMgr->Load(ability.mesh);

		if(ability.type == Ability::Ball || ability.type == Ability::Point)
			ability.shape = new btSphereShape(ability.size);
	}
}

//=================================================================================================
void GameResources::PreloadObjects()
{
	for(pair<const int, BaseObject*>& p : BaseObject::items)
	{
		BaseObject& obj = *p.second;
		if(obj.mesh || obj.variants)
		{
			if(obj.mesh && !IsSet(obj.flags, OBJ_SCALEABLE | OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS) && obj.type == OBJ_CYLINDER)
				obj.shape = new btCylinderShape(btVector3(obj.r, obj.h, obj.r));

			Mesh::Point* point;

			if(IsSet(obj.flags, OBJ_PRELOAD))
			{
				if(obj.variants)
				{
					for(Mesh* mesh : obj.variants->meshes)
						resMgr->Load(mesh);
				}
				else if(obj.mesh)
					resMgr->Load(obj.mesh);
			}

			if(obj.variants)
			{
				assert(!IsSet(obj.flags, OBJ_DOUBLE_PHYSICS | OBJ_MULTI_PHYSICS)); // not supported for variant mesh yet
				Mesh* mesh = obj.variants->meshes[0];
				resMgr->LoadMeshMetadata(mesh);
				point = mesh->FindPoint("hit");
			}
			else
			{
				resMgr->LoadMeshMetadata(obj.mesh);
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

				if(!IsSet(obj.flags, OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS))
					obj.shape = new btBoxShape(ToVector3(point->size));

				if(IsSet(obj.flags, OBJ_PHY_ROT))
					obj.type = OBJ_HITBOX_ROT;

				if(IsSet(obj.flags, OBJ_MULTI_PHYSICS))
				{
					LocalVector<Mesh::Point*> points;
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
						if(!IsSet(obj.flags, OBJ_NO_PHYSICS | OBJ_TMP_PHYSICS))
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
void GameResources::PreloadItem(const Item* p_item)
{
	Item& item = *(Item*)p_item;
	if(item.state == ResourceState::Loaded)
		return;

	if(resMgr->IsLoadScreen())
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
							resMgr->Load(tex_o.diffuse);
					}
				}
			}
			else if(item.type == IT_BOOK)
			{
				Book& book = item.ToBook();
				resMgr->Load(book.scheme->tex);
			}

			if(item.mesh)
				resMgr->Load(item.mesh);
			else if(item.tex)
				resMgr->Load(item.tex);
			resMgr->AddTask(&item, TaskCallback(this, &GameResources::GenerateItemIconTask));

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
						resMgr->Load(tex_o.diffuse);
				}
			}
		}
		else if(item.type == IT_BOOK)
		{
			Book& book = item.ToBook();
			resMgr->Load(book.scheme->tex);
		}

		if(item.mesh)
			resMgr->Load(item.mesh);
		else if(item.tex)
			resMgr->Load(item.tex);
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
		item.icon = missing_item_texture;
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

	DrawItemIcon(item, rt_item, 0.f);
	Texture* tex = render->CopyToTexture(rt_item);

	item.icon = tex;
	if(it != item_texture_map.end())
		item_texture_map.insert(it, ItemTextureMap::value_type(item.mesh, tex));
	else
		over_item_textures.push_back(tex);
}

//=================================================================================================
void GameResources::DrawItemIcon(const Item& item, RenderTarget* target, float rot)
{
	// setup node
	Mesh& mesh = *item.mesh;
	mesh.EnsureIsLoaded();
	Mesh::Point* point = mesh.FindPoint("cam_rot");
	if(point)
		node->mat = Matrix::CreateFromAxisAngle(point->rot, rot);
	else
		node->mat = Matrix::RotationY(rot);
	node->SetMesh(&mesh);
	node->center = Vec3::Zero;
	node->tex_override = nullptr;
	if(item.type == IT_ARMOR)
	{
		if(const Armor& armor = item.ToArmor(); !armor.tex_override.empty())
		{
			node->tex_override = armor.GetTextureOverride();
			assert(armor.tex_override.size() == mesh.head.n_subs);
		}
	}

	// light
	Vec3& light_pos = scene->lights.back().pos;
	point = mesh.FindPoint("light");
	if(point)
		light_pos = Vec3::TransformZero(point->mat);
	else
		light_pos = mesh.head.cam_pos;

	// setup camera
	Matrix mat_view = Matrix::CreateLookAt(mesh.head.cam_pos, mesh.head.cam_target, mesh.head.cam_up),
		mat_proj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 1.f, 0.1f, 25.f);
	camera->mat_view_proj = mat_view * mat_proj;
	camera->from = mesh.head.cam_pos;
	camera->to = mesh.head.cam_target;

	// draw
	sceneMgr->SetScene(scene, camera);
	sceneMgr->Draw(target);
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
	case MAT_SLIME:
		return sSlime;
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
		if(Any(item->ToConsumable().subtype, Consumable::Subtype::Food, Consumable::Subtype::Herb))
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
Mesh* GameResources::GetEntryMesh(EntryType type)
{
	switch(type)
	{
	default:
		assert(0);
	case ENTRY_STAIRS_UP:
		return aStairsUp;
	case ENTRY_STAIRS_DOWN:
		return aStairsDown;
	case ENTRY_STAIRS_DOWN_IN_WALL:
		return aStairsDown2;
	case ENTRY_DOOR:
		return aDungeonDoor;
	}
}

//=================================================================================================
void GameResources::LoadMusic(MusicType type, bool new_load_screen, bool instant)
{
	if(soundMgr->IsDisabled())
		return;

	MusicList* list = musicLists[(int)type];
	if(list->IsLoaded())
		return;

	if(new_load_screen)
		resMgr->AddTaskCategory(txLoadMusic);

	for(Music* music : list->musics)
	{
		if(instant)
			resMgr->LoadInstant(music);
		else
			resMgr->Load(music);
	}
}

//=================================================================================================
void GameResources::LoadCommonMusic()
{
	LoadMusic(MusicType::Boss, false);
	LoadMusic(MusicType::Death, false);
	LoadMusic(MusicType::Travel, false);
}

//=================================================================================================
void GameResources::LoadTrap(BaseTrap* trap)
{
	assert(trap);
	if(trap->state != ResourceState::NotLoaded)
		return;

	if(trap->mesh)
		resMgr->Load(trap->mesh);
	if(trap->sound)
		resMgr->Load(trap->sound);
	if(trap->sound2)
		resMgr->Load(trap->sound2);
	if(trap->sound3)
		resMgr->Load(trap->sound3);

	trap->state = ResourceState::Loaded;
}
