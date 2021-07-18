#include "Pch.h"

Dir Reversed(Dir dir)
{
	switch(dir)
	{
	default: return dir;
	case DIR_RIGHT: return DIR_LEFT;
	case DIR_FORWARD_RIGHT: return DIR_BACKWARD_LEFT;
	case DIR_FORWARD: return DIR_BACKWARD;
	case DIR_FORWARD_LEFT: return DIR_BACKWARD_RIGHT;
	case DIR_LEFT: return DIR_RIGHT;
	case DIR_BACKWARD_LEFT: return DIR_FORWARD_RIGHT;
	case DIR_BACKWARD: return DIR_FORWARD;
	case DIR_BACKWARD_RIGHT: return DIR_FORWARD_LEFT;
	case DIR_UP: return DIR_DOWN;
	case DIR_DOWN: return DIR_UP;
	}
}