#pragma once

//-----------------------------------------------------------------------------
// Room target
enum class RoomTarget
{
	None,
	Corridor,
	EntryPrev,
	EntryNext,
	Treasury,
	Portal,
	Prison,
	Throne,
	PortalCreate
};

//-----------------------------------------------------------------------------
// Room inside dungeon
struct Room : ObjectPoolProxy<Room>
{
	Int2 pos, size;
	vector<Room*> connected;
	RoomTarget target;
	RoomType* type; // not saved - used only during generation
	int index, group;

	static const int MIN_SIZE = 19;
	static const float HEIGHT;
	static const float HEIGHT_LOW;

	Vec3 Center() const
	{
		return Vec3(float(pos.x * 2 + size.x), 0, float(pos.y * 2 + size.y));
	}
	Int2 CenterTile() const
	{
		return pos + size / 2;
	}
	bool IsInside(const Int2& pt) const
	{
		return pt.x >= pos.x && pt.y >= pos.y && pt.x < pos.x + size.x && pt.y < pos.y + size.y;
	}
	bool IsInside(float x, float z) const
	{
		return (x >= 2.f * pos.x && z >= 2.f * pos.y && x <= 2.f * (pos.x + size.x) && z <= 2.f * (pos.y + size.y));
	}
	bool IsInside(const Vec3& pos) const
	{
		return IsInside(pos.x, pos.z);
	}
	float Distance(const Vec3& pos) const
	{
		if(IsInside(pos))
			return 0.f;
		else
			return Vec3::Distance2d(pos, Center());
	}
	float Distance(const Room& room) const
	{
		return Vec3::Distance2d(Center(), room.Center());
	}
	Vec3 GetRandomPos() const
	{
		return Vec3(Random(2.f * (pos.x + 1), 2.f * (pos.x + size.x - 1)), 0, Random(2.f * (pos.y + 1), 2.f * (pos.y + size.y - 1)));
	}
	bool GetRandomPos(float margin, Vec3& outPos) const
	{
		float min_size = (min(size.x, size.y) - 1) * 2.f;
		if(margin * 2 >= min_size)
			return false;
		outPos = Vec3(
			Random(2.f * (pos.x + 1) + margin, 2.f * (pos.x + size.x - 1) - margin),
			0,
			Random(2.f * (pos.y + 1) + margin, 2.f * (pos.y + size.y - 1) - margin));
		return true;
	}
	Vec3 GetRandomPos(float margin) const
	{
		assert(margin * 2 < (min(size.x, size.y) - 1) * 2.f);
		return Vec3(
			Random(2.f * (pos.x + 1) + margin, 2.f * (pos.x + size.x - 1) - margin),
			0,
			Random(2.f * (pos.y + 1) + margin, 2.f * (pos.y + size.y - 1) - margin));
	}

	bool IsCorridor() const { return target == RoomTarget::Corridor; }
	bool IsConnected(Room* room);
	bool CanJoinRoom() const { return Any(target, RoomTarget::None, RoomTarget::EntryPrev, RoomTarget::EntryNext); }
	void AddTile(const Int2& pt);

	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};

//-----------------------------------------------------------------------------
struct RoomGroup
{
	struct Connection
	{
		int groupIndex, myRoom, otherRoom;
	};

	int index;
	vector<Connection> connections;
	vector<int> rooms;
	RoomTarget target;

	bool IsConnected(int groupIndex) const;
	bool HaveRoom(int roomIndex) const;
	void Save(GameWriter& f);
	void Load(GameReader& f);
	static void SetRoomGroupConnections(vector<RoomGroup>& groups, vector<Room*>& rooms);
};
