#include "Pch.h"
#include <AppEntry.h>
#include "Game.h"

int AppEntry(char*)
{
	Game game;
	return game.Start() ? 0 : 1;
}
