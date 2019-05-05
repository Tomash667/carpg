#pragma once

//-----------------------------------------------------------------------------
// Room target
enum class RoomTarget
{
	None,
	Corridor,
	StairsUp,
	StairsDown,
	Treasury,
	Portal,
	Prison,
	Throne,
	PortalCreate
};

//-----------------------------------------------------------------------------
// Struktura opisuj¹ca pomieszczenie w podziemiach
struct Room : ObjectPoolProxy<Room>
{
	Int2 pos, size;
	vector<Room*> connected;
	RoomTarget target;
	int index;

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
		return (x >= 2.f*pos.x && z >= 2.f*pos.y && x <= 2.f*(pos.x + size.x) && z <= 2.f*(pos.y + size.y));
	}
	bool IsInside(const Vec3& _pos) const
	{
		return IsInside(_pos.x, _pos.z);
	}
	float Distance(const Vec3& _pos) const
	{
		if(IsInside(_pos))
			return 0.f;
		else
			return Vec3::Distance2d(_pos, Center());
	}
	float Distance(const Room& room) const
	{
		return Vec3::Distance2d(Center(), room.Center());
	}
	Vec3 GetRandomPos() const
	{
		return Vec3(Random(2.f*(pos.x + 1), 2.f*(pos.x + size.x - 1)), 0, Random(2.f*(pos.y + 1), 2.f*(pos.y + size.y - 1)));
	}
	bool GetRandomPos(float margin, Vec3& out_pos) const
	{
		float min_size = (min(size.x, size.y) - 1) * 2.f;
		if(margin * 2 >= min_size)
			return false;
		out_pos = Vec3(
			Random(2.f*(pos.x + 1) + margin, 2.f*(pos.x + size.x - 1) - margin),
			0,
			Random(2.f*(pos.y + 1) + margin, 2.f*(pos.y + size.y - 1) - margin));
		return true;
	}
	Vec3 GetRandomPos(float margin) const
	{
		assert(margin * 2 < (min(size.x, size.y) - 1) * 2.f);
		return Vec3(
			Random(2.f*(pos.x + 1) + margin, 2.f*(pos.x + size.x - 1) - margin),
			0,
			Random(2.f*(pos.y + 1) + margin, 2.f*(pos.y + size.y - 1) - margin));
	}

	bool IsCorridor() const { return target == RoomTarget::Corridor; }
	bool IsConnected(Room* room);
	bool CanJoinRoom() const { return target == RoomTarget::None || target == RoomTarget::StairsUp || target == RoomTarget::StairsDown; }
	void AddTile(const Int2& pt);

	void Save(FileWriter& f);
	void Load(FileReader& f);
	void Write(BitStreamWriter& f) const;
	void Read(BitStreamReader& f);
};

//-----------------------------------------------------------------------------
struct RoomGroup
{
	vector<int> rooms;
	RoomTarget target;
};
