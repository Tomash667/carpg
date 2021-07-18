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
	DIR_FORWARD_RIGHT,
	DIR_FORWARD,
	DIR_FORWARD_LEFT,
	DIR_LEFT,
	DIR_BACKWARD_LEFT,
	DIR_BACKWARD,
	DIR_BACKWARD_RIGHT,
	DIR_UP,
	DIR_DOWN
};

Dir Reversed(Dir dir);
