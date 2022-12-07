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
		enum Flags
		{
			F_REQUIRED = 1 << 0,
			F_IN_MIDDLE = 1 << 1
		};

		union
		{
			BaseObject* obj;
			ObjectGroup* group;
		};
		Int2 count;
		Vec3 pos;
		float rot;
		int flags;
		bool isGroup, forcePos, forceRot;
	};

	string id;
	vector<Obj> objs;
	int flags;

	RoomType() : flags(0) {}

	static RoomType* Get(Cstring id);
	static RoomType* GetS(const string& id) { return Get(id); }
	static vector<RoomType*> types;
};
