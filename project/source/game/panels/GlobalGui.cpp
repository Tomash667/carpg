#include "Pch.h"
#include "GameCore.h"
#include "GlobalGui.h"
#include "World.h"
#include "Level.h"

void GlobalGui::Draw(ControlDrawData*)
{
	cstring state;
	switch(W.state)
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
	case World::State::TRAVEL:
		state = "TRAVEL";
		break;
	}
	GUI.DrawText(GUI.default_font, Format("state:%s\nlocation: %p %s\nindex: %d", state, W.current_location,
		W.current_location ? W.current_location->name.c_str() : "", W.current_location_index),
		0, Color::Black, Rect(0, 0, 500, 200));
	assert(W.current_location == L.location);
	assert(W.current_location_index == L.location_index);
}
