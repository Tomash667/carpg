#pragma once

#include <CarpgLib.h>

using namespace app;

class Editor;
class EditorUi;
struct EditorCamera;
struct Level;
struct Room;

enum Dir
{
	DIR_NONE = -1,
	DIR_RIGHT,
	DIR_TOP_RIGHT,
	DIR_TOP,
	DIR_TOP_LEFT,
	DIR_LEFT,
	DIR_BOTTOM_LEFT,
	DIR_BOTTOM,
	DIR_BOTTOM_RIGHT
};
