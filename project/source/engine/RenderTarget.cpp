#include "Pch.h"
#include "EngineCore.h"
#include "RenderTarget.h"
#include "DirectX.h"

SURFACE RenderTarget::GetSurface()
{
	if(!surf)
	{
		tmp_surf = true;
		V(tex->GetSurfaceLevel(0, &surf));
	}
	return surf;
}

void RenderTarget::SaveToFile(cstring filename)
{
	SURFACE s;
	if(surf)
		s = surf;
	else
		V(tex->GetSurfaceLevel(0, &s));
	V(D3DXSaveSurfaceToFile(filename, D3DXIFF_JPG, s, nullptr, nullptr));
	if(!surf)
		surf->Release();
}
