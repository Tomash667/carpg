#pragma once

#include "App.h"

class Game : public App
{
public:
	Game();
	void OnUpdate(float dt) override;
};
