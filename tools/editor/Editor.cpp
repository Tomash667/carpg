#include "Pch.h"
#include "Editor.h"

#include "EditorCamera.h"
#include "EditorUi.h"
#include "Level.h"
#include "NativeDialogs.h"

#include <BasicShader.h>
#include <Engine.h>
#include <File.h>
#include <Gui.h>
#include <Input.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>
#include <SceneNode.h>

const Color GRID_COLOR(0, 255, 128);

Editor::Editor() : ui(nullptr), level(nullptr), scene(nullptr), camera(nullptr)
{
	engine->SetTitle("Editor");
	engine->SetWindowSize(Int2(1280, 720));
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

	scene = new Scene;
	scene->clear_color = Color(0, 128, 255);

	camera = new EditorCamera;

	SceneNode* node = SceneNode::Get();
	node->center = Vec3::Zero;
	node->SetMesh(res_mgr->Load<Mesh>("beczka.qmsh"));
	node->mat = Matrix::IdentityMatrix;
	scene->Add(node);

	scene_mgr->SetScene(scene, camera);

	shader = render->GetShader<BasicShader>();

	NewLevel();

	return true;
}

void Editor::OnDraw()
{
	scene_mgr->Draw(nullptr);

	// grid
	const int range = 40;
	shader->Prepare(*camera);
	const Vec3 start(round(camera->from.x), 0, round(camera->from.z));
	const float y = 0.f;// editor->yLevel;
	for(int i = -range; i <= range; ++i)
	{
		shader->DrawLine(Vec3(start.x - range, y, start.z + i), Vec3(start.x + range, y, start.z + i), 0.02f, GRID_COLOR);
		shader->DrawLine(Vec3(start.x + i, y, start.z - range), Vec3(start.x + i, y, start.z + range), 0.02f, GRID_COLOR);
	}
	shader->Draw();

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

	camera->Update(dt);
}

void Editor::NewLevel()
{
	delete level;
	level = new Level;
	camera->from = Vec3(0, 10, -5);
	camera->LookAt(Vec3::Zero);
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
		NewLevel();
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
		camera->Load(f);
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
	camera->Save(f);
	f.Write<byte>(0xFF);
}
