#include "Pch.h"
#include "GameCore.h"
#include "LoadingHandler.h"
#include "Game.h"

void LoadingHandler::Step()
{
	game->LoadingStep();
}
