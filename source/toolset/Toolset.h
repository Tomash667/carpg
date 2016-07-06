#pragma once

#include "Gui2.h"

class Engine;

namespace gui
{
	class Overlay;
}

class Toolset
{
public:
	void Init(Engine* engine);
	void Start();
	void End();
	void OnDraw();
	bool OnUpdate(float dt);

private:
	void HandleMenuEvent(int id);

	Engine* engine;
	gui::Overlay* overlay;
};
