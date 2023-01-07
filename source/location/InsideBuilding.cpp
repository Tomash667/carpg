#include "Pch.h"
#include "InsideBuilding.h"

#include "BitStreamFunc.h"
#include "BuildingGroup.h"
#include "Content.h"
#include "Door.h"
#include "GameCommon.h"
#include "GroundItem.h"
#include "Level.h"
#include "Object.h"

#include <ParticleSystem.h>

//=================================================================================================
void InsideBuilding::Save(GameWriter& f)
{
	f << offset;
	f << insideSpawn;
	f << outsideSpawn;
	f << xspherePos;
	f << enterRegion;
	f << exitRegion;
	f << outsideRot;
	f << top;
	f << xsphereRadius;
	f << building->id;
	f << levelShift;
	f << region1;
	f << region2;
	f << enterY;
	f << canEnter;

	LocationPart::Save(f);
}

//=================================================================================================
void InsideBuilding::Load(GameReader& f)
{
	f >> offset;
	f >> insideSpawn;
	f >> outsideSpawn;
	f >> xspherePos;
	f >> enterRegion;
	f >> exitRegion;
	f >> outsideRot;
	f >> top;
	f >> xsphereRadius;
	building = Building::Get(f.ReadString1());
	f >> levelShift;
	f >> region1;
	f >> region2;
	f >> enterY;
	if(LOAD_VERSION >= V_0_20)
		f >> canEnter;

	LocationPart::Load(f);
}

//=================================================================================================
void InsideBuilding::Write(BitStreamWriter& f)
{
	LocationPart::Write(f);

	f << levelShift;
	f << building->id;
	f << xspherePos;
	f << enterRegion;
	f << exitRegion;
	f << top;
	f << xsphereRadius;
	f << enterY;
	f << canEnter;
}

//=================================================================================================
bool InsideBuilding::Read(BitStreamReader& f)
{
	if(!LocationPart::Read(f))
		return false;

	f >> levelShift;
	const string& buildingId = f.ReadString1();
	f >> xspherePos;
	f >> enterRegion;
	f >> exitRegion;
	f >> top;
	f >> xsphereRadius;
	f >> enterY;
	f >> canEnter;
	if(!f)
	{
		Error("Broken packet for inside building.");
		return false;
	}
	building = Building::Get(buildingId);
	if(!building || !building->insideMesh)
	{
		Error("Invalid building id '%s'.", buildingId.c_str());
		return false;
	}
	offset = Vec2(512.f * levelShift.x + 256.f, 512.f * levelShift.y + 256.f);
	gameLevel->ProcessBuildingObjects(*this, gameLevel->cityCtx, this, building->insideMesh, nullptr, 0.f, GDIR_DOWN,
		Vec3(offset.x, 0, offset.y), building, nullptr, true);

	return true;
}
