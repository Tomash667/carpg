#pragma once

//-----------------------------------------------------------------------------
struct CityBuilding
{
	Building* building;
	Int2 pt, unitPt;
	GameDirection dir;
	Vec3 walkPt;

	CityBuilding() {}
	explicit CityBuilding(Building* building) : building(building) {}
	bool GetCanEnter();
	Box GetEntryArea();
	InsideBuilding* GetInsideBuilding();
	float GetRot();
	Vec3 GetUnitPos();
	void SetCanEnter(bool value);
};
