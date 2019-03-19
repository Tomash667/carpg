#include "Pch.h"
#include "EngineCore.h"
#include "SuperShader.h"
#include "File.h"
#include "Engine.h"
#include "DirectX.h"

extern string g_system_dir;

//=================================================================================================
SuperShader::SuperShader() : pool(nullptr)
{
}

//=================================================================================================
void SuperShader::InitOnce()
{
	V(D3DXCreateEffectPool(&pool));
	Engine::Get().RegisterShader(this);
}

//=================================================================================================
void SuperShader::Cleanup()
{
	SafeRelease(pool);
}

//=================================================================================================
void SuperShader::OnInit()
{
	FileReader f(Format("%s/shaders/super.fx", g_system_dir.c_str()));
	FileTime file_time = f.GetTime();
	if(file_time != edit_time)
	{
		f.ReadToString(code);
		edit_time = file_time;
	}

	Info("Setting up super shader parameters.");
	GetShader(0);
	ID3DXEffect* e = shaders[0].e;
	hMatCombined = e->GetParameterByName(nullptr, "matCombined");
	hMatWorld = e->GetParameterByName(nullptr, "matWorld");
	hMatBones = e->GetParameterByName(nullptr, "matBones");
	hTint = e->GetParameterByName(nullptr, "tint");
	hAmbientColor = e->GetParameterByName(nullptr, "ambientColor");
	hFogColor = e->GetParameterByName(nullptr, "fogColor");
	hFogParams = e->GetParameterByName(nullptr, "fogParams");
	hLightDir = e->GetParameterByName(nullptr, "lightDir");
	hLightColor = e->GetParameterByName(nullptr, "lightColor");
	hLights = e->GetParameterByName(nullptr, "lights");
	hSpecularColor = e->GetParameterByName(nullptr, "specularColor");
	hSpecularIntensity = e->GetParameterByName(nullptr, "specularIntensity");
	hSpecularHardness = e->GetParameterByName(nullptr, "specularHardness");
	hCameraPos = e->GetParameterByName(nullptr, "cameraPos");
	hTexDiffuse = e->GetParameterByName(nullptr, "texDiffuse");
	hTexNormal = e->GetParameterByName(nullptr, "texNormal");
	hTexSpecular = e->GetParameterByName(nullptr, "texSpecular");
	assert(hMatCombined && hMatWorld && hMatBones && hTint && hAmbientColor && hFogColor && hFogParams && hLightDir && hLightColor && hLights && hSpecularColor
		&& hSpecularIntensity && hSpecularHardness && hCameraPos && hTexDiffuse && hTexNormal && hTexSpecular);
}

//=================================================================================================
void SuperShader::OnReload()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		V(it->e->OnResetDevice());
}

//=================================================================================================
void SuperShader::OnReset()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		V(it->e->OnLostDevice());
}

//=================================================================================================
void SuperShader::OnRelease()
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
		SafeRelease(it->e);
	shaders.clear();
}

//=================================================================================================
uint SuperShader::GetShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const
{
	uint id = 0;
	if(animated)
		id |= (1 << ANIMATED);
	if(have_binormals)
		id |= (1 << HAVE_BINORMALS);
	if(fog)
		id |= (1 << FOG);
	if(specular)
		id |= (1 << SPECULAR);
	if(normal)
		id |= (1 << NORMAL);
	if(point_light)
		id |= (1 << POINT_LIGHT);
	if(dir_light)
		id |= (1 << DIR_LIGHT);
	return id;
}

//=================================================================================================
ID3DXEffect* SuperShader::GetShader(uint id)
{
	for(vector<Shader>::iterator it = shaders.begin(), end = shaders.end(); it != end; ++it)
	{
		if(it->id == id)
			return it->e;
	}

	return CompileShader(id);
}

//=================================================================================================
ID3DXEffect* SuperShader::CompileShader(uint id)
{
	Engine& engine = Engine::Get();
	D3DXMACRO macros[10] = { 0 };
	uint i = 0;

	if(IS_SET(id, 1 << ANIMATED))
	{
		macros[i].Name = "ANIMATED";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << HAVE_BINORMALS))
	{
		macros[i].Name = "HAVE_BINORMALS";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << FOG))
	{
		macros[i].Name = "FOG";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << SPECULAR))
	{
		macros[i].Name = "SPECULAR_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << NORMAL))
	{
		macros[i].Name = "NORMAL_MAP";
		macros[i].Definition = "1";
		++i;
	}
	if(IS_SET(id, 1 << POINT_LIGHT))
	{
		macros[i].Name = "POINT_LIGHT";
		macros[i].Definition = "1";
		++i;

		macros[i].Name = "LIGHTS";
		macros[i].Definition = (engine.shader_version == 2 ? "2" : "3");
		++i;
	}
	else if(IS_SET(id, 1 << DIR_LIGHT))
	{
		macros[i].Name = "DIR_LIGHT";
		macros[i].Definition = "1";
		++i;
	}

	macros[i].Name = "VS_VERSION";
	macros[i].Definition = (engine.shader_version == 3 ? "vs_3_0" : "vs_2_0");
	++i;

	macros[i].Name = "PS_VERSION";
	macros[i].Definition = (engine.shader_version == 3 ? "ps_3_0" : "ps_2_0");
	++i;

	Info("Compiling super shader: %u", id);

	CompileShaderParams params = { "super.fx" };
	params.cache_name = Format("%d_super%u.fcx", engine.shader_version, id);
	params.file_time = edit_time;
	params.input = &code;
	params.macros = macros;
	params.pool = pool;

	Shader& s = Add1(shaders);
	s.e = engine.CompileShader(params);
	s.id = id;

	return s.e;
}
