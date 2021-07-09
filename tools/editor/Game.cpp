#include "Pch.h"
#include "Game.h"
#include <Engine.h>
#include <Render.h>
#include <Input.h>

Game::Game()
{
	app::engine->SetTitle("carpglib template");
	app::render->SetShadersDir("../carpglib/shaders");
}

void Game::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Pressed(Key::Escape))
	{
		app::engine->Shutdown();
		return;
	}
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		app::engine->SetFullscreen(!app::engine->IsFullscreen());
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		app::engine->UnlockCursor();
}
