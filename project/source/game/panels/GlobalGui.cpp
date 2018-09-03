#include "Pch.h"
#include "GameCore.h"
#include "GlobalGui.h"
#include "World.h"
#include "Level.h"
#include "Game.h"

void GlobalGui::Draw(ControlDrawData*)
{
	FIXME;
	if(!Any(Game::Get().game_state, GS_LEVEL, GS_WORLDMAP))
		return;

	cstring state;
	switch(W.GetState())
	{
	default:
	case World::State::ON_MAP:
		state = "ON_MAP";
		break;
	case World::State::ENCOUNTER:
		state = "ENCOUNTER";
		break;
	case World::State::INSIDE_LOCATION:
		state = "INSIDE_LOCATION";
		break;
	case World::State::INSIDE_ENCOUNTER:
		state = "INSIDE_ENCOUNTER";
		break;
	case World::State::TRAVEL:
		state = "TRAVEL";
		break;
	}
	GUI.DrawText(GUI.default_font, Format("state:%s\nlocation: %p %s\nindex: %d\ntravel index:%d", state, W.GetCurrentLocation(),
		W.GetCurrentLocation() ? W.GetCurrentLocation()->name.c_str() : "", W.GetCurrentLocationIndex(), W.GetTravelLocationIndex()),
		0, Color::Black, Rect(0, 0, 500, 200));
	assert(W.GetCurrentLocation() == L.location);
	assert(W.GetCurrentLocationIndex() == L.location_index);
}
