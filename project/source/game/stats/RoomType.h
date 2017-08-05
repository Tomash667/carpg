// rodzaje pomieszcze� jakie mo�na znale�� w podziemiach
#pragma once

//-----------------------------------------------------------------------------
// Obiekt w pomieszczeniu
struct ObjEntry
{
	cstring id;
	Int2 count;
};

//-----------------------------------------------------------------------------
// Room flags
enum ROOMTYPE_FLAGS
{
	RT_REPEAT = 1 << 0,
	RT_TREASURE = 1 << 1
};
//-----------------------------------------------------------------------------
// Typ pomieszczenia
struct RoomType
{
	cstring id;
	ObjEntry* objs;
	int count, flags;
};
extern RoomType g_room_types[];
extern int n_room_types;

//-----------------------------------------------------------------------------
RoomType* FindRoomType(cstring id);
