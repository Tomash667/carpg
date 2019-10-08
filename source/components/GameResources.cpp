#include "Pch.h"
#include "GameCore.h"
#include "GameResources.h"
#include <Render.h>
#include <SceneManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Camera.h>
#include <ResourceManager.h>
#include "Item.h"

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
	CreateMissingTexture();
	CreateItemScene();
	GetResources();
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
void GameResources::CreateItemScene()
{
	rt_item = render->CreateRenderTarget(Int2(ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE));

	scene = new Scene;
	scene->clear_color = Color::None;
	scene->use_point_light = true;
	scene->ambient_color = Color(128, 128, 128);
	scene->use_fog = true;
	scene->fog_color = Color::White;
	scene->fog_range = Vec2(25.f, 50.f);
	scene_mgr->AddScene(scene);

	camera = new Camera;
	camera->fov = PI / 4;
	camera->aspect = 1.f;
	camera->zmin = 0.1f;
	camera->zmax = 25.f;
	scene_mgr->AddCamera(camera);

	node = SceneNode::Get();
	scene->Add(node);

	light = SceneNode::Get();
	light->SetLight(10.f);
	scene->Add(light);
}

//=================================================================================================
void GameResources::GetResources()
{
	mesh_human = res_mgr->Get<Mesh>("human.qmsh");
}

//=================================================================================================
void GameResources::LoadData()
{
	res_mgr->Load(mesh_human);
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

	// generate image
	Texture* tex;
	do
	{
		DrawItemIcon(item, rt_item, 0.f);
		tex = render->CopyToTexture(rt_item);
	}
	while(tex == nullptr);

	// set
	item.icon = tex;
	if(it != item_texture_map.end())
		item_texture_map.insert(it, ItemTextureMap::value_type(item.mesh, tex));
	else
		over_item_textures.push_back(tex);
}

//=================================================================================================
void GameResources::DrawItemIcon(const Item& item, RenderTarget* target, float rot)
{
	Mesh& mesh = *item.mesh;

	camera->from = mesh.head.cam_pos;
	camera->to = mesh.head.cam_target;
	camera->up = mesh.head.cam_up;
	camera->changed = true;

	node->SetMesh(item.mesh);
	Mesh::Point* point = mesh.FindPoint("cam_rot");
	if(point)
		node->rot = Vec3::FromAxisAngle(point->rot, rot);
	else
		node->rot.y = rot;
	if(item.type == IT_ARMOR && !item.ToArmor().tex_override.empty())
	{
		if(const Armor& armor = item.ToArmor(); !armor.tex_override.empty())
		{
			assert(armor.tex_override.size() == mesh.head.n_subs);
			node->tex = armor.tex_override.data();
		}
	}

	scene_mgr->Draw(target, scene, camera);

	FIXME;
	/*if(IsSet(ITEM_ALPHA, item.flags))
	{
		render->SetAlphaBlend(true);
		render->SetNoZWrite(true);
	}
	else
	{
		render->SetAlphaBlend(false);
		render->SetNoZWrite(false);
	}*/
}
