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
	void Prepare() override;
	void InitOnce() override;
	void LoadLanguage() override;
	void Draw(ControlDrawData*) override;
	void LoadOldGui(FileReader& f);
	void Clear(bool reset_mpbox);
	void Setup(PlayerController* pc);

	LoadScreen* load_screen;
	GameGui* game_gui;
	Inventory* inventory;
};
