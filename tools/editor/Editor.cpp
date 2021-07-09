#include "Pch.h"
#include "Editor.h"

#include "EditorCamera.h"
#include "EditorUi.h"
#include "Level.h"
#include "NativeDialogs.h"

#include <Engine.h>
#include <File.h>
#include <Gui.h>
#include <Input.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>
#include <SceneNode.h>

Editor::Editor() : ui(nullptr), level(nullptr), scene(nullptr), camera(nullptr)
{
	engine->SetTitle("Editor");
	render->SetShadersDir("../../../carpglib/shaders");
}

Editor::~Editor()
{
	delete ui;
	delete level;
	delete scene;
	delete camera;
}

bool Editor::OnInit()
{
	res_mgr->AddDir("../../../bin/data");
	ui = new EditorUi(this);
	gui->Add(ui);
	NewLevel();
	scene = new Scene;
	scene->clear_color = Color(0, 128, 255);
	camera = new EditorCamera;
	camera->from = Vec3(-1, 4, -4);
	camera->to = Vec3(0, 0, 0);
	Matrix matView = Matrix::CreateLookAt(camera->from, camera->to);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), 0.1f, 50.f);
	camera->mat_view_proj = matView * matProj;
	camera->mat_view_inv = matView.Inverse();
	SceneNode* node = SceneNode::Get();
	node->center = Vec3::Zero;
	node->SetMesh(res_mgr->Load<Mesh>("beczka.qmsh"));
	node->mat = Matrix::IdentityMatrix;
	scene->Add(node);
	scene_mgr->SetScene(scene, camera);
	return true;
}

void Editor::OnDraw()
{
	scene_mgr->Draw(nullptr);
	gui->Draw(true, true);
	render->Present();
}

void Editor::OnUpdate(float dt)
{
	if(input->Shortcut(KEY_ALT, Key::F4))
	{
		engine->Shutdown();
		return;
	}
	if(input->Shortcut(KEY_ALT, Key::Enter))
		engine->SetFullscreen(!engine->IsFullscreen());
	if(input->Shortcut(KEY_CONTROL, Key::U))
		engine->UnlockCursor();

	if(input->Shortcut(KEY_CONTROL, Key::N))
		NewLevel();
	if(input->Shortcut(KEY_CONTROL, Key::O))
		OpenLevel();
	if(input->Shortcut(KEY_CONTROL, Key::S))
		SaveLevel();
	if(input->Shortcut(KEY_CONTROL | KEY_SHIFT, Key::S))
		SaveLevelAs();
}

void Editor::NewLevel()
{
	delete level;
	level = new Level;
}

void Editor::OpenLevel()
{
	cstring path = PickFile("Level (.lvl)\0*.lvl\0\0", false, "lvl");
	if(!path)
		return;
	delete level;
	level = new Level;
	level->path = path;
	level->filename = io::FilenameFromPath(path);
	if(!DoLoadLevel())
	{
		engine->ShowError("Failed to load level!");
		return;
	}
}

bool Editor::DoLoadLevel()
{
	FileReader f(level->path);
	if(!level->Load(f))
		return false;
	byte marker;
	f >> marker;
	if(marker == 0xFE)
	{
		// TODO
		f >> marker;
	}
	return marker == 0xFF;
}

void Editor::SaveLevel()
{
	if(level->path.empty())
	{
		cstring path = PickFile("Level (.lvl)\0*.lvl\0\0", true, "lvl");
		if(!path)
			return;
		level->path = path;
		level->filename = io::FilenameFromPath(path);
	}
	DoSaveLevel();
}

void Editor::SaveLevelAs()
{
	cstring path = PickFile("Level (.lvl)\0*.lvl\0\0", true, "lvl");
	if(!path)
		return;
	level->path = path;
	level->filename = io::FilenameFromPath(path);
	DoSaveLevel();
}

void Editor::DoSaveLevel()
{
	FileWriter f(level->path);
	level->Save(f);
	f.Write<byte>(0xFE);
	f.Write<byte>(0xFF);
}
