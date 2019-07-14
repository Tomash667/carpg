#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class TerrainShader : public ShaderHandler
{
public:
	TerrainShader(Render* render);
	~TerrainShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	void SetCamera(const CameraBase& camera);
	void SetFog(const Vec4& fog_color, const Vec4& fog_params);
	void SetLight(const Vec4& light_dir, const Vec4& diffuse_color, const Vec4& ambient_color);
	void Draw(Terrain* terrain, const vector<uint>& parts);

private:
	Render* render;
	IDirect3DDevice9* device;
	IDirect3DVertexDeclaration9* vertex_decl;
	ID3DXEffect* effect;
	D3DXHANDLE tech;
	D3DXHANDLE h_mat, h_world, h_tex_blend, h_tex[5], h_color_ambient, h_color_diffuse, h_light_dir, h_fog_color, h_fog_params;
	Matrix mat_view_proj;
	Vec4 fog_color, fog_params, light_dir, diffuse_color, ambient_color;
};
