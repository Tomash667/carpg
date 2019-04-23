#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class GrassShader : public ShaderHandler
{
public:
	GrassShader(Render* render);
	~GrassShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void SetFog(const Vec4& color, const Vec4& params);
	void SetCamera(const CameraBase& camera);
	void Begin(uint max_size);
	void Draw(Mesh* mesh, const vector<const vector<Matrix>*>& patches, uint count);
	void End();

private:
	Render* render;
	IDirect3DDevice9* device;
	IDirect3DVertexDeclaration9* vertex_decl;
	ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE h_view_proj, h_tex, h_fog_color, h_fog_params, h_ambient;
	VB vb;
	uint vb_size;
	Matrix mat_view_proj;
	Vec4 fog_color, fog_params;
};
