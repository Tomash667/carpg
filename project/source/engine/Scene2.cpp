#include "Pch.h"
#include "EngineCore.h"
#include "Scene2.h"
#include "Render.h"
#include "Engine.h"
#include "DirectX.h"

bool Scene2::Draw(RenderTarget* target)
{
	Render* render = Engine::Get().GetRender();
	IDirect3DDevice9* device = render->GetDevice();
	if(!render->IsReady())
		return false;

	render->SetAlphaBlend(false);
	render->SetAlphaTest(false);
	render->SetNoCulling(false);
	render->SetNoZWrite(false);

	if(target)
		render->SetTarget(target);
	V(device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, clear_color, 1.f, 0));
	V(device->BeginScene());

	// TODO

	V(device->EndScene());
	if(target)
		render->SetTarget(nullptr);

	return true;
}
