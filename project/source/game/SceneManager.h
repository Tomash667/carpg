#pragma once

#include "Resource.h"

struct Camera2
{
	VEC3 from, to, up;
	float fov, aspect, zmin, zmax;
};

class SceneNode2
{
public:
	Mesh* mesh;
	VEC3 pos;
};

class Scene2
{
	friend class SceneManager;

	Scene2(SceneManager& mgr);

public:

	void Add(SceneNode2* node);
	Camera2& GetCamera() { return camera; }
	TEX RenderToTexture();
	void SetForRenderTarget(const INT2& size);

private:
	SceneManager& mgr;
	vector<SceneNode2*> nodes;
	TEX* render_surface;
	Camera2 camera;
	INT2 surface_size, surface_real_size;
	DWORD clear_color;
};

class SceneManager : public Singleton<SceneManager>
{
	friend class Scene2;
public:
	Scene2* CreateScene();
	void DrawScene(Scene2& scene);
	void Init(IDirect3DDevice9* device);
	void SetupShaders(ID3DXEffect* e_mesh);

private:
	vector<Scene2*> scenes;
	IDirect3DDevice9* device;
	ID3DXEffect* e_mesh;
	D3DXHANDLE h_tech;
	D3DXHANDLE h_mat_combined, h_mat_world, h_tex_diffuse, h_fog_color, h_fog_param, h_tint, h_ambient_color, h_light_dir, h_light_color, h_lights;
};
