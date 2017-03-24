// rodzaje pomieszczeñ jakie mo¿na znaleŸæ w podziemiach
#pragma once

//-----------------------------------------------------------------------------
// Obiekt w pomieszczeniu
struct ObjEntry
{
	cstring id;
	INT2 count;
};

//-----------------------------------------------------------------------------
// Room flags
#define RT_POWTARZAJ (1<<0)

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
