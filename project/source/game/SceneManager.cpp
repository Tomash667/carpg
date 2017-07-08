#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "SceneNode.h"

SceneManager Singleton<SceneManager>::instance;

Scene2::Scene2(SceneManager& mgr) : mgr(mgr), render_surface(nullptr)
{

}

void Scene2::Add(SceneNode2* node)
{
	assert(node);
	nodes.push_back(node);
}

TEX Scene2::RenderToTexture()
{
	HRESULT hr = mgr.device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return nullptr;

	assert(render_surface);
	TEX tex = *render_surface;

	// ustaw render target
	SURFACE surf = nullptr;
	//if(sItemRegion)
	//	V(device->SetRenderTarget(0, sItemRegion));
	//else
	//{
		V(tex->GetSurfaceLevel(0, &surf));
		V(mgr.device->SetRenderTarget(0, surf));
	//}

	// pocz¹tek renderowania
	V(mgr.device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, clear_color, 1.f, 0));
	V(mgr.device->BeginScene());

	mgr.DrawScene(*this);

	// koniec renderowania
	V(mgr.device->EndScene());


	// przywróæ stary render target
	V(mgr.device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf));
	V(mgr.device->SetRenderTarget(0, surf));
	surf->Release();

	return tex;
}

void Scene2::SetForRenderTarget(const INT2& size)
{
	auto& res_mgr = ResourceManager::Get();
	INT2 upsize(next_pow2(size.x), next_pow2(size.y));
	if(render_surface && surface_real_size != upsize)
	{
		res_mgr.DestroyRenderSurface(render_surface);
		render_surface = nullptr;
	}
	if(!render_surface)
		render_surface = res_mgr.CreateRenderSurface(upsize);
	surface_size = size;
	surface_real_size = upsize;
}

Scene2* SceneManager::CreateScene()
{
	auto scene = new Scene2(*this);
	scenes.push_back(scene);
	return scene;
}

void SceneManager::DrawScene(Scene2& scene)
{
	MATRIX mat_world, mat_view, mat_proj, mat_viewproj;
	auto& camera = scene.GetCamera();
	D3DXMatrixLookAtLH(&mat_view, &camera.from, &camera.to, &camera.up);
	D3DXMatrixPerspectiveFovLH(&mat_proj, camera.fov, camera.aspect, camera.zmin, camera.zmax);
	mat_viewproj = mat_view * mat_proj;

	auto e = e_mesh;
	UINT passes;
	V(e->Begin(&passes, 0));
	V(e->BeginPass(0));

	LightData ld;
	ld.pos = VEC3(0, 0, 0);
	ld.color = VEC3(1, 1, 1);
	ld.range = 10.f;

	V(e->SetTechnique(h_tech));
	V(e->SetVector(h_fog_color, &VEC4(1, 1, 1, 1)));
	V(e->SetVector(h_fog_param, &VEC4(25.f, 50.f, 25.f, 0)));
	V(e->SetVector(h_ambient_color, &VEC4(1, 1, 1, 1)));
	V(e->SetRawValue(h_lights, &ld, 0, sizeof(LightData)));
	V(e->SetVector(h_tint, &VEC4(1, 1, 1, 1)));

	for(auto node : scene.nodes)
	{
		D3DXMatrixTranslation(&mat_world, node->pos);

		auto& mesh = *node->mesh;

		V(e->SetMatrix(h_mat_combined, &(mat_world * mat_viewproj)));
		V(e->SetMatrix(h_mat_world, &mat_world));

		V(device->SetVertexDeclaration(Game::Get().vertex_decl[mesh.vertex_decl]));
		V(device->SetStreamSource(0, mesh.vb, 0, mesh.vertex_size));
		V(device->SetIndices(mesh.ib));

		for(int i = 0; i<mesh.head.n_subs; ++i)
		{
			const Animesh::Submesh& sub = mesh.subs[i];
			V(e->SetTexture(h_tex_diffuse, mesh.GetTexture(i)));
			V(e->CommitChanges());
			V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, sub.min_ind, sub.n_ind, sub.first * 3, sub.tris));
		}
	}

	V(e->EndPass());
	V(e->End());
}

void SceneManager::Init(IDirect3DDevice9* _device)
{
	assert(_device);
	device = _device;
}

void SceneManager::SetupShaders(ID3DXEffect* e)
{
	e_mesh = e;

	h_tech = e->GetTechniqueByName("mesh");
	assert(h_tech);

	h_mat_combined = e->GetParameterByName(nullptr, "matCombined");
	h_mat_world = e->GetParameterByName(nullptr, "matWorld");
	h_tex_diffuse = e->GetParameterByName(nullptr, "texDiffuse");
	h_fog_color = e->GetParameterByName(nullptr, "fogColor");
	h_fog_param = e->GetParameterByName(nullptr, "fogParam");
	h_tint = e->GetParameterByName(nullptr, "tint");
	h_ambient_color = e->GetParameterByName(nullptr, "ambientColor");
	h_light_dir = e->GetParameterByName(nullptr, "lightDir");
	h_light_color = e->GetParameterByName(nullptr, "lightColor");
	h_lights = e->GetParameterByName(nullptr, "lights");
	assert(h_mat_combined && h_mat_world && h_tex_diffuse && h_fog_color && h_fog_param && h_tint && h_ambient_color && h_light_dir && h_light_color && h_lights);
}
