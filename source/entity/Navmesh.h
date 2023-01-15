#pragma once

//-----------------------------------------------------------------------------
struct Navmesh
{
	enum Point
	{
		P_LEFT_BACK,
		P_RIGHT_BACK,
		P_LEFT_FRONT,
		P_RIGHT_FRONT
	};

	struct Region
	{
		Box2d box;
		array<float, 4> height;
		vector<pair<uint, Box2d>> connected;
		uint index;
		int prev;
		bool flat;

		float GetHeight(const Vec2& pos) const;
	};

	~Navmesh();
	void Init(Building* building, const Vec2& offset);
	Region* GetRegion(const Vec2& pos);
	bool GetHeight(const Vec2& pos, float& height);
	bool FindPath(const Vec2& from, const Vec2& to, vector<Vec3>& path);

private:
	vector<Region*> regions;
};
