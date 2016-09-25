#pragma once

#include "Overlay.h"

class Engine;

namespace gui
{
	class Window;
}

class Toolset : public gui::Overlay
{
public:
	Toolset();
	~Toolset();

	void Update(float dt) override;

	void Init(Engine* engine);
	void Start();
	void End();
	void OnDraw();
	bool OnUpdate(float dt);

private:
	void HandleMenuEvent(int id);

	Engine* engine;
	bool closing;
};
