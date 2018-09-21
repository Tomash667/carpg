#include "Pch.h"
#include "EngineCore.h"
#include "DebugDrawer.h"
#include "Engine.h"
#include "DirectX.h"

DebugDrawer::DebugDrawer() : effect(nullptr), vb(nullptr)
{
}

void DebugDrawer::InitOnce()
{
	effect = Engine::Get().CompileShader("debug.fx");
}

void DebugDrawer::OnInit()
{
	hTechSimple = effect->GetTechniqueByName("simple");
	assert(hTechSimple);

	hMatCombined = effect->GetParameterByName(nullptr, "matCombined");
	assert(hMatCombined);
}

void DebugDrawer::OnReset()
{
	if(effect)
		V(effect->OnLostDevice());
	SafeRelease(vb);
}

void DebugDrawer::OnReload()
{
	if(effect)
		V(effect->OnResetDevice());
}

void DebugDrawer::OnRelease()
{
	SafeRelease(effect);
	SafeRelease(vb);
}

void DebugDrawer::Draw()
{
	if(handler)
	{
		Engine& engine = Engine::Get();
		engine.SetAlphaBlend(true);
		engine.SetAlphaTest(false);
		engine.SetNoZWrite(false);

		handler(this);
	}
}

void DebugDrawer::BeginBatch()
{
	assert(!batch);
	batch = true;
}

void DebugDrawer::AddQuad(const Vec3(&pts)[4], const Vec4& color)
{
	verts.push_back(VColor(pts[0], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[2], color));
	verts.push_back(VColor(pts[1], color));
	verts.push_back(VColor(pts[3], color));
}

void DebugDrawer::EndBatch()
{
	assert(batch);
	batch = false;

	if(verts.empty())
		return;

	if(!vb || verts.size() > vb_size)
	{
		SafeRelease(vb);
		V(Engine::Get().device->CreateVertexBuffer(verts.size() * sizeof(VColor), D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &vb, nullptr));
		vb_size = verts.size();
	}

	void* ptr;
	V(vb->Lock(0, 0, &ptr, D3DLOCK_DISCARD));
	memcpy(ptr, verts.data(), verts.size() * sizeof(VColor));
	V(vb->Unlock());

	uint passes;

	V(effect->SetTechnique(hTechSimple));
	V(effect->Begin(&passes, 0));
	V(effect->BeginPass(0));

	V(effect->SetMatrix(hMatCombined, (const D3DXMATRIX*)&game.cam.matViewProj));
	V(effect->CommitChanges());

	V(Engine::Get().device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, verts.size() / 3));

	V(effect->EndPass());
	V(effect->End());
}
