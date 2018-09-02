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
	GUI.DrawText(GUI.default_font, Format("state:%s\nlocation: %p %s\nindex: %d\ntravel index:%d", state, W.current_location,
		W.current_location ? W.current_location->name.c_str() : "", W.current_location_index, W.travel_location_index),
		0, Color::Black, Rect(0, 0, 500, 200));
	assert(W.current_location == L.location);
	assert(W.current_location_index == L.location_index);
}
