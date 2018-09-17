#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"
#include "Container.h"

//-----------------------------------------------------------------------------
class GlobalGui : public GameComponent, public Container
{
public:
	GlobalGui();
	~GlobalGui();
	void InitOnce() override;
	void Draw(ControlDrawData*) override;

	LoadScreen* load_screen;
	GameGui* game_gui;
};
