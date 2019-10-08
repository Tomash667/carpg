#include "Pch.h"
#include "GameCore.h"
#include "GameResources.h"
#include "Item.h"
#include "SceneNode.h"
#include "Game.h"
#include <ResourceManager.h>
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
	mesh_human = res_mgr->Load<Mesh>("human.qmsh");
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
		if(const Armor & armor = item.ToArmor(); !armor.tex_override.empty())
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
