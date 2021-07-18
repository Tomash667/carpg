#pragma once

class Editor;
class EditorUi;
class MeshBuilder;
struct EditorCamera;
struct Level;
struct Room;
struct RoomLink;

enum Dir
{
	DIR_NONE = -1,
	DIR_RIGHT,
	DIR_FRONT_RIGHT,
	DIR_FRONT,
	DIR_FRONT_LEFT,
	DIR_LEFT,
	DIR_BACK_LEFT,
	DIR_BACK,
	DIR_BACK_RIGHT,
	DIR_UP,
	DIR_DOWN
};

Dir Reversed(Dir dir);
