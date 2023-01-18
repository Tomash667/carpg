#include "Pch.h"
#include "CityBuilding.h"

#include "City.h"
#include "GameCommon.h"
#include "Level.h"
#include "Net.h"

//=================================================================================================
bool CityBuilding::GetCanEnter()
{
	InsideBuilding* inside = GetInsideBuilding();
	if(inside)
		return inside->canEnter;
	return false;
}

//=================================================================================================
Box CityBuilding::GetEntryArea()
{
	InsideBuilding* inside = GetInsideBuilding();
	if(inside)
		return inside->enterRegion.ToBoxXZ(inside->enterY, inside->enterY);
	return Box();
}

//=================================================================================================
InsideBuilding* CityBuilding::GetInsideBuilding()
{
	assert(building->insideMesh);

	for(InsideBuilding* inside : gameLevel->cityCtx->insideBuildings)
	{
		if(inside->building == building)
			return inside;
	}

	return nullptr;
}

//=================================================================================================
bool CityBuilding::HaveInside()
{
	if(!building->insideMesh)
		return false;
	else if(IsSet(building->flags, Building::OPTIONAL_INSIDE))
		return GetInsideBuilding() != nullptr;
	else
		return true;
}

//=================================================================================================
float CityBuilding::GetRot()
{
	return DirToRot(dir);
}

//=================================================================================================
Vec3 CityBuilding::GetUnitPos()
{
	return Vec3(float(unitPt.x) * 2 + 1, 0, float(unitPt.y) * 2 + 1);
}

//=================================================================================================
Vec3 CityBuilding::GetShift()
{
	return Vec3(float(pt.x + building->shift[dir].x) * 2, 0.f, float(pt.y + building->shift[dir].y) * 2);
}

//=================================================================================================
void CityBuilding::SetCanEnter(bool value)
{
	InsideBuilding* inside = GetInsideBuilding();
	if(inside)
	{
		inside->canEnter = value;
		if(gameLevel->ready && Net::IsServer())
		{
			NetChange& c = Net::PushChange(NetChange::SET_CAN_ENTER);
			c.id = inside->partId;
			c.count = value ? 1 : 0;
		}
	}
}

//=================================================================================================
void CityBuilding::CreateInside()
{
	if(HaveInside() || !building->insideMesh)
		return;

	gameLevel->CreateInsideBuilding(*this);
}
