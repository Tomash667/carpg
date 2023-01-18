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
	bool HaveInside();
	float GetRot();
	Vec3 GetUnitPos();
	Vec3 GetShift();
	void SetCanEnter(bool value);
	void CreateInside();
};
