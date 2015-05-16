#pragma once

#include "Engine.h"

enum ANIM
{
	A3_BRAK,
	A3_STOI,
	A3_IDZIE,
	A3_BOJOWA,
	A3_BOJOWA2
};

struct Unit3
{
	AnimeshInstance* ani;
	VEC3 pos, look_pos;
	float rot, look_rot, look_rotX;
	ANIM anim, prev_anim;

	VEC3 GetEyePos();
};

struct Game3 : public Engine
{
	void InitGame();
	void OnChar(char c);
	void OnCleanup();
	void OnDraw();
	void OnReload();
	void OnReset();
	void OnResize();
	void OnTick(float dt);

	void SetupShaders();
	void DrawUnit(Unit3& u, MATRIX& matViewProj);

	Animesh* podloga, *czlowiek, *rurka, *bron;

	ID3DXEffect* eAni, *eMesh, *eParticle, *eSkybox, *eTerrain, *eArea;
	D3DXHANDLE techAnim, techHair, techAnimDir, techHairDir, techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox, techTerrain, techArea, techTrail;
	D3DXHANDLE hAniCombined, hAniWorld, hAniBones, hAniTex, hAniFogColor, hAniFogParam, hAniTint, hAniHairColor, hAniAmbientColor, hAniLightDir, hAniLightColor, hAniLights,
		hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir, hMeshLightColor, hMeshLights,
		hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor,
		hTerrainCombined, hTerrainWorld, hTerrainTexBlend, hTerrainTex[5], hTerrainColorAmbient, hTerrainColorDiffuse, hTerrainLightDir, hTerrainFogColor, hTerrainFogParam;

	Unit3 u1, u2;
	float rotY;
};
