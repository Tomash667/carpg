#include "Pch.h"
#include "InsideBuilding.h"

#include "BitStreamFunc.h"
#include "BuildingGroup.h"
#include "Content.h"
#include "Door.h"
#include "GameCommon.h"
#include "GroundItem.h"
#include "Level.h"
#include "Navmesh.h"
#include "Object.h"

#include <ParticleSystem.h>

//=================================================================================================
InsideBuilding::~InsideBuilding()
{
	delete navmesh;
}

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
	f << underground;
	f << canEnter;
	f << (navmesh != nullptr);

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
	if(LOAD_VERSION >= V_DEV)
		f >> underground;
	if(LOAD_VERSION >= V_0_20)
		f >> canEnter;
	if(LOAD_VERSION >= V_DEV)
	{
		if(f.Read<bool>())
		{
			navmesh = new Navmesh;
			navmesh->Init(*building, offset);
		}
	}

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
	f << underground;
	f << canEnter;
	f << (navmesh != nullptr);
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
	f >> underground;
	f >> canEnter;
	bool haveNavmesh = f.Read<bool>();
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
	int flags = Level::PBOF_RECREATE;
	if(haveNavmesh)
		flags |= Level::PBOF_CUSTOM_PHYSICS;
	gameLevel->ProcessBuildingObjects(*this, gameLevel->cityCtx, this, building->insideMesh, nullptr, 0.f, GDIR_DOWN, offset.XZ(), building, nullptr, flags);
	if(haveNavmesh)
	{
		navmesh = new Navmesh;
		navmesh->Init(*building, offset);
	}

	return true;
}
