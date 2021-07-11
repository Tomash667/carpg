#include "Pch.h"
#include "Editor.h"

#include "EditorCamera.h"
#include "EditorUi.h"
#include "Level.h"
#include "MeshBuilder.h"
#include "NativeDialogs.h"
#include "Room.h"

#include <BasicShader.h>
#include <Engine.h>
#include <File.h>
#include <Gui.h>
#include <Input.h>
#include <Physics.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <SceneManager.h>
#include <SceneNode.h>

const Color GRID_COLOR(0, 255, 128);

Editor::Editor() : ui(nullptr), level(nullptr), scene(nullptr), camera(nullptr), shapeGrid(nullptr), world(nullptr), builder(nullptr)
{
	engine->SetTitle("Editor");
	engine->SetWindowSize(Int2(1280, 720));
	render->SetShadersDir("../../../carpglib/shaders");
}

Editor::~Editor()
{
	delete ui;
	delete builder;
	delete level;
	delete scene;
	delete camera;

	delete shapeGrid;
}

bool Editor::OnInit()
{
	res_mgr->AddDir("../../../bin/data");
	res_mgr->AddDir("data");

	ui = new EditorUi(this);
	gui->Add(ui);

	builder = new MeshBuilder;

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

	world = engine->GetPhysicsWorld();
	shapeGrid = new btStaticPlaneShape(btVector3(0, 1, 0), 0.f);

	NewLevel();

	return true;
}

void Editor::OnCleanup()
{
	if(world)
		world->Reset();
}

void Editor::OnDraw()
{
	scene_mgr->Prepare();
	builder->Draw(*camera);
	scene_mgr->DrawSceneNodes();

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

	// rooms
	for(Room* room : level->rooms)
	{
		if(room != roomHover)
			DrawRect(room->box, Color::White);
	}

	if(roomHover && roomHover != roomSelect)
		DrawRect(roomHover->box, Color(255, 64, 140));

	if(roomSelect)
		DrawRect(roomSelect->box, Color::Red);

	if(action == A_ADD_ROOM)
		DrawRect(roomBox, Color::Red);

	shader->Draw();

	shader->PrepareForShapes(*camera);
	if(markerValid)
	{
		shader->DrawShape(MeshShape::Sphere, Matrix::Scale(0.1f) * Matrix::Translation(marker), Color::White);
	}

	if(roomSelect && action == A_NONE)
	{
		for(int i = 0; i < 8; ++i)
		{
			bool selected = (roomSelect == roomHover && hoverDir == (Dir)i);
			shader->DrawShape(MeshShape::Sphere, Matrix::Scale(selected ? 0.3f : 0.1f) * Matrix::Translation(roomSelect->GetPoint((Dir)i)),
				selected ? Color::Red : Color(255, 64, 140));
		}
	}

	gui->Draw(true, true);

	render->Present();
}

void Editor::DrawRect(const Box& box, Color color)
{
	shader->DrawLine(Vec3(box.v1.x, box.v1.y, box.v1.z), Vec3(box.v2.x, box.v1.y, box.v1.z), 0.04f, color);
	shader->DrawLine(Vec3(box.v1.x, box.v1.y, box.v2.z), Vec3(box.v2.x, box.v1.y, box.v2.z), 0.04f, color);
	shader->DrawLine(Vec3(box.v1.x, box.v1.y, box.v1.z), Vec3(box.v1.x, box.v1.y, box.v2.z), 0.04f, color);
	shader->DrawLine(Vec3(box.v2.x, box.v1.y, box.v1.z), Vec3(box.v2.x, box.v1.y, box.v2.z), 0.04f, color);
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

	const Vec3 from = camera->from;
	const Vec3 to = camera->from + camera->GetDir() * 40.f;
	btCollisionWorld::ClosestRayResultCallback callback(ToVector3(from), ToVector3(to));
	//callback.m_flags = 1; // backface culling
	world->rayTest(ToVector3(from), ToVector3(to), callback);
	if(callback.hasHit())
	{
		markerValid = true;
		marker = ToVec3(callback.m_hitPointWorld);
		marker = Vec3(round(marker.x * 10) / 10, round(marker.y * 10) / 10, round(marker.z * 10) / 10);
	}
	else
		markerValid = false;

	roomHover = nullptr;
	switch(action)
	{
	case A_NONE:
		if(markerValid)
		{
			for(Room* room : level->rooms)
			{
				if(marker.x >= room->box.v1.x && marker.x <= room->box.v2.x && marker.z >= room->box.v1.z && marker.z <= room->box.v2.z)
				{
					roomHover = room;
					hoverDir = DIR_NONE;
					for(int i = 0; i < 8; ++i)
					{
						if(Vec3::Distance(marker, room->GetPoint((Dir)i)) <= 0.1f)
						{
							hoverDir = (Dir)i;
							break;
						}
					}
					break;
				}
			}
		}

		if(input->PressedRelease(Key::LeftButton))
		{
			action = A_ADD_ROOM;
			actionPos = marker;
			roomBox = Box(actionPos);
			roomHover = nullptr;
			roomSelect = nullptr;
		}
		else if(input->PressedRelease(Key::RightButton))
			roomSelect = roomHover;
		else if(input->PressedRelease(Key::Delete))
		{
			if(roomSelect)
			{
				if(roomHover == roomSelect)
					roomHover = nullptr;
				RemoveElement(level->rooms, roomSelect);
				delete roomSelect;
				roomSelect = nullptr;
			}
		}
		else if(input->PressedRelease(Key::M))
		{
			if(roomSelect)
			{
				action = A_MOVE;
				actionPos = marker;
				roomHover = nullptr;
				roomBox = roomSelect->box;
			}
		}
		else if(input->PressedRelease(Key::R))
		{
			if(roomSelect && roomSelect == roomHover && hoverDir != DIR_NONE)
			{
				action = A_RESIZE;
				actionPos = marker;
				roomHover = nullptr;
				roomBox = roomSelect->box;
				resizeLock.clear();
				switch(hoverDir)
				{
				case DIR_RIGHT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_LEFT));
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_LEFT));
					break;
				case DIR_TOP_RIGHT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_LEFT));
					break;
				case DIR_TOP:
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_LEFT));
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_RIGHT));
					break;
				case DIR_TOP_LEFT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_RIGHT));
					break;
				case DIR_LEFT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_BOTTOM_RIGHT));
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_RIGHT));
					break;
				case DIR_BOTTOM_LEFT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_RIGHT));
					break;
				case DIR_BOTTOM:
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_LEFT));
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_RIGHT));
					break;
				case DIR_BOTTOM_RIGHT:
					resizeLock.push_back(roomSelect->GetPoint(DIR_TOP_LEFT));
					break;
				}
			}
		}
		break;

	case A_ADD_ROOM:
		if(markerValid)
			roomBox = Box::CreateBoundingBox(actionPos, marker);
		if(input->Pressed(Key::LeftButton) || input->Pressed(Key::Spacebar))
		{
			if(roomBox.SizeX() > 0 && roomBox.SizeZ() > 0)
			{
				Room* room = new Room;
				room->box = roomBox;
				room->box.v2.y = room->box.v1.y + 4;
				level->rooms.push_back(room);
				roomSelect = room;
				builder->Build(level);
			}
			action = A_NONE;
		}
		if(input->Pressed(Key::RightButton) || input->Pressed(Key::Escape))
			action = A_NONE;
		break;

	case A_MOVE:
		if(markerValid)
		{
			Vec3 dif = marker - actionPos;
			if(dif != Vec3::Zero)
			{
				actionPos = marker;
				roomSelect->box += dif;
				roomSelect->box.v2.y = roomSelect->box.v1.y + 4;
				builder->Build(level);
			}
		}
		if(input->Pressed(Key::LeftButton) || input->Pressed(Key::Spacebar))
			action = A_NONE;
		if(input->Pressed(Key::RightButton) || input->Pressed(Key::Escape))
		{
			action = A_NONE;
			roomSelect->box = roomBox;
			builder->Build(level);
		}
		break;

	case A_RESIZE:
		if(markerValid && marker != actionPos)
		{
			roomSelect->box = Box(marker);
			for(const Vec3& pt : resizeLock)
				roomSelect->box.AddPoint(pt);
			roomSelect->box.v2.y = roomSelect->box.v1.y + 4;
			builder->Build(level);
		}
		if(input->Pressed(Key::LeftButton) || input->Pressed(Key::Spacebar))
			action = A_NONE;
		if(input->Pressed(Key::RightButton) || input->Pressed(Key::Escape))
		{
			action = A_NONE;
			roomSelect->box = roomBox;
			builder->Build(level);
		}
		break;
	}
}

void Editor::NewLevel()
{
	// cleanup
	delete level;
	world->Reset();

	level = new Level;
	camera->from = Vec3(0, 10, -5);
	camera->LookAt(Vec3::Zero);

	cobjGrid = new btCollisionObject;
	cobjGrid->setCollisionShape(shapeGrid);
	world->addCollisionObject(cobjGrid);

	markerValid = false;
	action = A_NONE;
	roomHover = nullptr;
	roomSelect = nullptr;
	builder->Clear();
}

void Editor::OpenLevel()
{
	cstring path = PickFile("Level (.lvl)\0*.lvl\0\0", false, "lvl");
	if(!path)
		return;
	NewLevel();
	level->path = path;
	level->filename = io::FilenameFromPath(path);
	if(!DoLoadLevel())
	{
		engine->ShowError("Failed to load level!");
		NewLevel();
		return;
	}

	builder->Build(level);
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
