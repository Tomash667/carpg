#include "Pch.h"
#include "EngineCore.h"
#include "DebugDrawer.h"
#include "Engine.h"
#include "Render.h"
#include "CameraBase.h"
#include "DirectX.h"

//=================================================================================================
DebugDrawer::DebugDrawer() : effect(nullptr), vertex_decl(nullptr), vb(nullptr), batch(false)
{
}

//=================================================================================================
DebugDrawer::~DebugDrawer()
{
}

//=================================================================================================
void DebugDrawer::InitOnce()
{
	Render* render = Engine::Get().GetRender();

	render->RegisterShader(this);

	const D3DVERTEXELEMENT9 v[] = {
		{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,		0 },
		{ 0, 12, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,			0 },
		D3DDECL_END()
	};
	V(render->GetDevice()->CreateVertexDeclaration(v, &vertex_decl));
}

//=================================================================================================
void DebugDrawer::Cleanup()
{
	SafeRelease(vertex_decl);
}

//=================================================================================================
void DebugDrawer::OnInit()
{
	effect = Engine::Get().GetRender()->CompileShader("debug.fx");

	hTechSimple = effect->GetTechniqueByName("simple");
	assert(hTechSimple);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	assert(hMatCombined);
}

//=================================================================================================
void DebugDrawer::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
}

//=================================================================================================
void DebugDrawer::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

//=================================================================================================
void DebugDrawer::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

//=================================================================================================
void DebugDrawer::Draw()
{
	if(handler)
	{
		Render* render = Engine::Get().GetRender();
		render->SetAlphaBlend(true);
		render->SetAlphaTest(false);
		render->SetNoZWrite(false);
		render->SetNoCulling(true);
		V(render->GetDevice()->SetVertexDeclaration(vertex_decl));
		if(vb)
			V(render->GetDevice()->SetStreamSource(0, vb, 0, sizeof(VColor)));

		handler(this);
	}
}

//=================================================================================================
void DebugDrawer::BeginBatch()
{
	assert(!batch);
	batch = true;
}

//=================================================================================================
void DebugDrawer::AddQuad(const Vec3(&pts)[4], const Vec4& color)
{
	verts.push_back(VColor(pts[0], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[3], color));
}

//=================================================================================================
void DebugDrawer::EndBatch()
{
	assert(batch);
	batch = false;

	if(verts.empty())
		return;

	IDirect3DDevice9* device = Engine::Get().GetRender()->GetDevice();

	if(!vb || verts.size() > vb_size)
	{
		SafeRelease(vb);
		V(device->CreateVertexBuffer(verts.size() * sizeof(VColor), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr));
		vb_size = verts.size();
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));
	}

	void* ptr;
	V(vb->Lock(0, 0, &ptr, D3DLOCK_DISCARD));
	memcpy(ptr, verts.data(), verts.size() * sizeof(VColor));
	V(vb->Unlock());

	uint passes;

	V(effect->SetTechnique(hTechSimple));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(effect->SetMatrix(hMatCombined, (const D3DXMATRIX*)&Engine::Get().cam_base->matViewProj));
	V(effect->CommitChanges());

	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());

	verts.clear();
}
