#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "TextBox.h"
#include "PickItemDialog.h"
#include "GameGui.h"
#include "Inventory.h"
#include "Journal.h"
#include "TeamPanel.h"
#include "StatsPanel.h"
#include "Minimap.h"
#include "Console.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "WorldMapGui.h"
#include "GameMenu.h"
#include "Options.h"
#include "SaveLoadPanel.h"
#include "CreateCharacterPanel.h"
#include "MultiplayerPanel.h"
#include "CreateServerPanel.h"
#include "PickServerPanel.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "Controls.h"
#include "GetTextDialog.h"
#include "GetNumberDialog.h"
#include "GameMessages.h"
#include "MpBox.h"
#include "PickFileDialog.h"
#include "GlobalGui.h"
#include "ResourceManager.h"

//=================================================================================================
void Game::OnResize()
{
	GUI.OnResize();
	if(gui->game_gui)
		gui->game_gui->PositionPanels();
	gui->console->Event(GuiEvent_WindowResize);
}

//=================================================================================================
void Game::OnFocus(bool focus, const Int2& activation_point)
{
	if(!focus && gui->game_gui)
		gui->game_gui->use_cursor = false;
	if(focus && activation_point.x != -1)
		GUI.cursor_pos = activation_point;
}

//=================================================================================================
void Game::UpdateGui(float dt)
{
	GUI.Update(dt, cursor_allow_move ? mouse_sensitivity_f : -1.f);
}
