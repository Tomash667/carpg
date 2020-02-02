#pragma once

//-----------------------------------------------------------------------------
struct RoomType
{
	enum Flags
	{
		REPEAT = 1 << 0,
		TREASURE = 1 << 1
	};

	struct Obj
	{
		union
		{
			BaseObject* obj2;
			ObjectGroup* group;
		};
		Int2 count;
		bool is_group;
	};

	string id;
	vector<Obj> objs;
	int flags;

	RoomType() : flags(0) {}

	static RoomType* Get(Cstring id);
	static vector<RoomType*> types;
};
