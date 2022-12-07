#pragma once

//-----------------------------------------------------------------------------
#include "LocationPart.h"
#include "Building.h"

//-----------------------------------------------------------------------------
struct InsideBuilding final : public LocationPart
{
	Vec2 offset;
	Vec3 insideSpawn, outsideSpawn, xspherePos;
	Box2d enterRegion, exitRegion, region1, region2;
	float outsideRot, top, xsphereRadius, enterY;
	Building* building;
	Int2 levelShift;

	InsideBuilding(int partId) : LocationPart(LocationPart::Type::Building, partId, false) {}
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
