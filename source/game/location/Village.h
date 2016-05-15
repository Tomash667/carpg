// wioska
#pragma once

//-----------------------------------------------------------------------------
#include "City.h"

//-----------------------------------------------------------------------------
struct Village : public City
{
	Building* v_buildings[2];

	Village()
	{

	}

	virtual void Save(HANDLE file, bool local);
	virtual void Load(HANDLE file, bool local);
	virtual LOCATION_TOKEN GetToken() const
	{
		return LT_VILLAGE;
	}

	inline bool IsGenerated() const
	{
		return last_visit != -1;
	}
};
