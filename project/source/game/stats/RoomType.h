// rodzaje pomieszczeñ jakie mo¿na znaleŸæ w podziemiach
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
	uint count;
	int flags;

	static RoomType* Find(cstring id);
	static void Validate(uint& err);
	static RoomType types[];
	static int types_count;
};
