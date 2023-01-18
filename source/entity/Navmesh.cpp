#include "Pch.h"
#include "Navmesh.h"

#include "Building.h"

//=================================================================================================
Navmesh::~Navmesh()
{
	DeleteElements(regions);
}

//=================================================================================================
void Navmesh::Init(Building& building, const Vec2& offset)
{
	regions.reserve(building.navmesh.size());
	for(Building::Region& buildingRegion : building.navmesh)
	{
		Region* region = new Region;
		region->box = buildingRegion.box;
		region->box += offset;
		region->height = buildingRegion.height;
		regions.push_back(region);
	}

	for(uint i = 0, count = regions.size(); i < count; ++i)
	{
		Region* region = regions[i];
		region->index = i;
		Box2d box = region->box;
		box.AddMargin(-0.01f);
		region->flat = region->height[0] == region->height[1] && region->height[0] == region->height[2] && region->height[0] == region->height[3];
		for(uint j = 1; j < count; ++j)
		{
			Region* region2 = regions[j];
			Box2d box2 = region2->box;
			box2.AddMargin(-0.01f);
			Box2d intersection;
			if(Box2d::Intersect(box, box2, intersection) && (intersection.SizeX() >= 0.03f || intersection.SizeY() >= 0.03f))
			{
				region->connected.push_back({ j, intersection });
				region2->connected.push_back({ i, intersection });
			}
		}
	}
}

//=================================================================================================
Navmesh::Region* Navmesh::GetRegion(const Vec2& pos)
{
	for(Region* region : regions)
	{
		if(region->box.IsInside(pos))
			return region;
	}
	return nullptr;
}

//=================================================================================================
bool Navmesh::GetHeight(const Vec2& pos, float& height)
{
	Region* region = GetRegion(pos);
	if(!region)
		return false;
	else
	{
		height = region->GetHeight(pos);
		return true;
	}
}

//=================================================================================================
bool Navmesh::FindPath(const Vec2& from, const Vec2& to, vector<Vec3>& path)
{
	Region* regionFrom = GetRegion(from);
	Region* regionTo = GetRegion(to);
	if(!regionFrom || !regionTo)
		return false;

	path.clear();
	if(regionFrom == regionTo)
	{
		path.push_back(from.XZ(regionFrom->GetHeight(from)));
		path.push_back(to.XZ(regionFrom->GetHeight(to)));
		return true;
	}

	for(Region* region : regions)
		region->prev = UINT_MAX;

	struct ToCheck
	{
		Region* region;
		float cost, sum;

		bool operator < (const ToCheck& tc) const
		{
			return sum < tc.sum;
		}
	};
	vector<ToCheck> toCheck;
	float dist = Vec2::DistanceSquared(regionFrom->box.Midpoint(), regionTo->box.Midpoint());
	toCheck.push_back({ regionFrom, dist, dist });
	regionFrom->prev = regionFrom->index;

	while(!toCheck.empty())
	{
		std::sort(toCheck.begin(), toCheck.end());

		ToCheck tc = toCheck.back();
		toCheck.pop_back();

		for(pair<uint, Box2d>& c : tc.region->connected)
		{
			Region* region = regions[c.first];
			if(region->prev != UINT_MAX)
				continue;

			region->prev = tc.region->index;
			if(region == regionTo)
			{
				toCheck.clear();
				break;
			}

			const float dist = Vec2::DistanceSquared(region->box.Midpoint(), regionTo->box.Midpoint());
			const float cost = tc.cost + Vec2::DistanceSquared(tc.region->box.Midpoint(), region->box.Midpoint());
			toCheck.push_back({ region, cost, dist + cost });
		}
	}

	// can't find path, disconnected region
	if(regionTo->prev == UINT_MAX)
		return false;

	path.push_back(to.XZ(regionTo->GetHeight(to)));
	Region* prevRegion = regionTo;
	Region* region = regions[regionTo->prev];
	while(true)
	{
		for(pair<uint, Box2d>& connection : prevRegion->connected)
		{
			if(connection.first == region->index)
			{
				Vec2 pos = connection.second.Midpoint();
				path.push_back(pos.XZ(prevRegion->GetHeight(pos)));
				break;
			}
		}
		if(region == regionFrom)
			break;
		prevRegion = region;
		region = regions[region->prev];
	}
	path.push_back(from.XZ(regionFrom->GetHeight(from)));
	std::reverse(path.begin(), path.end());
	return true;
}

//=================================================================================================
float Navmesh::Region::GetHeight(const Vec2& pos) const
{
	if(flat)
		return height[0];

	const float offsetx = (pos.x - box.v1.x) / box.SizeX();
	const float offsetz = (pos.y - box.v1.y) / box.SizeY();

	// which triangle is it? (right front or left back)
	if((offsetx * offsetx + offsetz * offsetz) < ((1 - offsetx) * (1 - offsetx) + (1 - offsetz) * (1 - offsetz)))
	{
		// left back
		const float dX = height[P_RIGHT_BACK] - height[P_LEFT_BACK];
		const float dZ = height[P_LEFT_FRONT] - height[P_LEFT_BACK];
		return dX * offsetx + dZ * offsetz + height[P_LEFT_BACK];
	}
	else
	{
		// right front
		const float dX = height[P_LEFT_FRONT] - height[P_RIGHT_FRONT];
		const float dZ = height[P_RIGHT_BACK] - height[P_RIGHT_FRONT];
		return dX * (1 - offsetx) + dZ * (1 - offsetz) + height[P_RIGHT_FRONT];
	}
}
