#pragma once

//-----------------------------------------------------------------------------
#include "ShaderHandler.h"

//-----------------------------------------------------------------------------
class SuperShader : public ShaderHandler
{
	enum Switches
	{
		ANIMATED,
		HAVE_BINORMALS,
		FOG,
		SPECULAR,
		NORMAL,
		POINT_LIGHT,
		DIR_LIGHT
	};

	struct Shader
	{
		ID3DXEffect* e;
		uint id;
	};

public:
	SuperShader(Render* render);
	~SuperShader();
	void OnInit() override;
	void OnReset() override;
	void OnReload() override;
	void OnRelease() override;
	uint GetShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const;
	ID3DXEffect* GetShader(uint id);
	ID3DXEffect* CompileShader(uint id);
	ID3DXEffect* GetEffect() const { return shaders.front().e; }

	D3DXHANDLE hMatCombined, hMatWorld, hMatBones, hTint, hAmbientColor, hFogColor, hFogParams, hLightDir, hLightColor, hLights, hSpecularColor,
		hSpecularIntensity, hSpecularHardness, hCameraPos, hTexDiffuse, hTexNormal, hTexSpecular;

private:
	Render* render;
	string code;
	FileTime edit_time;
	ID3DXEffectPool* pool;
	vector<Shader> shaders;
};
