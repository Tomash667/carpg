#include "Pch.h"
#include "EngineCore.h"
#include "DebugDrawer.h"
#include "Render.h"
#include "CameraBase.h"
#include "DirectX.h"

//=================================================================================================
DebugDrawer::DebugDrawer(Render* render) : render(render), device(render->GetDevice()), effect(nullptr), vertex_decl(nullptr), vb(nullptr), batch(false)
{
	const D3DVERTEXELEMENT9 decl[] = {
		{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_POSITION,	0 },
		{ 0, 12, D3DDECLTYPE_FLOAT4,	D3DDECLMETHOD_DEFAULT,	D3DDECLUSAGE_COLOR,		0 },
		D3DDECL_END()
	};
	V(device->CreateVertexDeclaration(decl, &vertex_decl));

	render->RegisterShader(this);
}

//=================================================================================================
DebugDrawer::~DebugDrawer()
{
	SafeRelease(vertex_decl);
}

//=================================================================================================
void DebugDrawer::OnInit()
{
	effect = render->CompileShader("debug.fx");

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
void DebugDrawer::SetCamera(const CameraBase& camera)
{
	mat_view_proj = camera.matViewProj;
}

//=================================================================================================
void DebugDrawer::Draw()
{
	if(!handler)
		return;

	render->SetAlphaBlend(true);
	render->SetAlphaTest(false);
	render->SetNoZWrite(false);
	render->SetNoCulling(true);
	V(device->SetVertexDeclaration(vertex_decl));
	if(vb)
		V(device->SetStreamSource(0, vb, 0, sizeof(VColor)));

	handler(this);
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

	V(effect->SetMatrix(hMatCombined, (const D3DXMATRIX*)&mat_view_proj));
	V(effect->CommitChanges());

	V(device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());

	verts.clear();
}
