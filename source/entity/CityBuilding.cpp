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
void CityBuilding::SetCanEnter(bool value)
{
	InsideBuilding* inside = GetInsideBuilding();
	if(inside)
	{
		inside->canEnter = value;
		if(gameLevel->ready && Net::IsServer())
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::SET_CAN_ENTER;
			c.id = inside->partId;
			c.count = value ? 1 : 0;
		}
	}
}
