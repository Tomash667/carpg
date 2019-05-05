#include "Pch.h"
#include "EngineCore.h"
#include "GrassShader.h"
#include "Render.h"
#include "Mesh.h"
#include "CameraBase.h"
#include "DirectX.h"

//=================================================================================================
GrassShader::GrassShader(Render* render) : render(render), device(render->GetDevice()), vertex_decl(nullptr), effect(nullptr), vb(nullptr), vb_size(0)
{
	const D3DVERTEXELEMENT9 decl[] = {
		{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,	0},
		{0, 12,	D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_NORMAL,	0},
		{0, 24, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	0},
		{1, 0,	D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	1},
		{1, 16,	D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	2},
		{1, 32,	D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	3},
		{1, 48,	D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_TEXCOORD,	4},
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(decl, &vertex_decl));

	render->RegisterShader(this);
}

//=================================================================================================
GrassShader::~GrassShader()
{
	SafeRelease(vertex_decl);
}

//=================================================================================================
void GrassShader::OnInit()
{
	effect = render->CompileShader("grass.fx");

	tech = effect->GetTechniqueByName("grass");
	assert(tech);

	h_view_proj = effect->GetParameterByName(nullptr, "matViewProj");
	h_tex = effect->GetParameterByName(nullptr, "texDiffuse");
	h_fog_color = effect->GetParameterByName(nullptr, "fogColor");
	h_fog_params = effect->GetParameterByName(nullptr, "fogParam");
	h_ambient = effect->GetParameterByName(nullptr, "ambientColor");
	assert(h_view_proj && h_tex && h_fog_color && h_fog_params && h_ambient);
}

//=================================================================================================
void GrassShader::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
	vb_size = 0;
}

//=================================================================================================
void GrassShader::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void GrassShader::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void GrassShader::SetFog(const Vec4& color, const Vec4& params)
{
	fog_color = color;
	fog_params = params;
}

//=================================================================================================
void GrassShader::SetCamera(const CameraBase& camera)
{
	mat_view_proj = camera.matViewProj;
}

//=================================================================================================
void GrassShader::Begin(uint max_size)
{
	render->SetAlphaBlend(false);
	render->SetAlphaTest(true);
	render->SetNoCulling(true);
	render->SetNoZWrite(false);

	// create vertex buffer if existing is too small
	if(!vb || vb_size < max_size)
	{
		SafeRelease(vb);
		if(vb_size < max_size)
			vb_size = max_size;
		V(device->CreateVertexBuffer(sizeof(Matrix)*vb_size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb, nullptr));
	}

	// setup stream source for instancing
	V(device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1));
	V(device->SetStreamSource(1, vb, 0, sizeof(Matrix)));
	V(device->SetVertexDeclaration(vertex_decl));

	// set effect
	Vec4 ambient_color(0.8f, 0.8f, 0.8f, 1.f);
	uint passes;
	V(effect->SetTechnique(tech));
	V(effect->SetVector(h_fog_color, (D3DXVECTOR4*)&fog_color));
	V(effect->SetVector(h_fog_params, (D3DXVECTOR4*)&fog_params));
	V(effect->SetVector(h_ambient, (D3DXVECTOR4*)&ambient_color));
	V(effect->SetMatrix(h_view_proj, (D3DXMATRIX*)&mat_view_proj));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));
}

//=================================================================================================
void GrassShader::Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count)
{
	// setup instancing data
	Matrix* m;
	V(vb->Lock(0, 0, (void**)&m, D3DLOCK_DISCARD));
	int index = 0;
	for(vector< const vector<Matrix>* >::const_iterator it = patches.begin(), end = patches.end(); it != end; ++it)
	{
		const vector<Matrix>& vm = **it;
		memcpy(&m[index], &vm[0], sizeof(Matrix)*vm.size());
		index += vm.size();
	}
	V(vb->Unlock());

	// setup stream source for mesh
	V(device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | count));
	V(device->SetStreamSource(0, mesh->vb, 0, mesh->vertex_size));
	V(device->SetIndices(mesh->ib));

	// draw
	for(int i = 0; i < mesh->head.n_subs; ++i)
	{
		V(effect->SetTexture(h_tex, mesh->subs[i].tex->tex));
		V(effect->CommitChanges());
		V(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, mesh->subs[i].min_ind, mesh->subs[i].n_ind, mesh->subs[i].first * 3, mesh->subs[i].tris));
	}
}

//=================================================================================================
void GrassShader::End()
{
	V(effect->EndPass());
	V(effect->End());

	// restore vertex stream frequency
	V(device->SetStreamSourceFreq(0, 1));
	V(device->SetStreamSourceFreq(1, 1));
}
