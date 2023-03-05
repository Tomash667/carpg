#pragma once

//-----------------------------------------------------------------------------
#include "LocationPart.h"
#include "Building.h"

//-----------------------------------------------------------------------------
struct InsideBuilding final : public LocationPart
{
	InsideBuilding(int partId) : LocationPart(LocationPart::Type::Building, partId, false), canEnter(true), navmesh(nullptr), underground{ 999.f, 999.f } {}
	~InsideBuilding();
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);

	Vec2 offset;
	Vec3 insideSpawn, outsideSpawn, xspherePos;
	Box2d enterRegion, exitRegion, region1, region2;
	float outsideRot, top, xsphereRadius, enterY, underground[2];
	Building* building;
	Navmesh* navmesh;
	Int2 levelShift;
	bool canEnter;
};
