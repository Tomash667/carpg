#include "Pch.h"

Dir Reversed(Dir dir)
{
	switch(dir)
	{
	default: return dir;
	case DIR_RIGHT: return DIR_LEFT;
	case DIR_FRONT_RIGHT: return DIR_BACK_LEFT;
	case DIR_FRONT: return DIR_BACK;
	case DIR_FRONT_LEFT: return DIR_BACK_RIGHT;
	case DIR_LEFT: return DIR_RIGHT;
	case DIR_BACK_LEFT: return DIR_FRONT_RIGHT;
	case DIR_BACK: return DIR_FRONT;
	case DIR_BACK_RIGHT: return DIR_FRONT_LEFT;
	case DIR_UP: return DIR_DOWN;
	case DIR_DOWN: return DIR_UP;
	}
}