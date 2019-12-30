#pragma once

//-----------------------------------------------------------------------------
#include "City.h"
#include "InsideLocation.h"

//-----------------------------------------------------------------------------
namespace LocationHelper
{
	inline bool IsCity(Location& loc)
	{
		City& city = (City&)loc;
		return loc.type == L_CITY && !city.IsVillage();
	}

	inline bool IsCity(Location* loc)
	{
		assert(loc);
		return IsCity(*loc);
	}

	inline bool IsVillage(Location& loc)
	{
		City& city = (City&)loc;
		return loc.type == L_CITY && city.IsVillage();
	}

	inline bool IsVillage(Location* loc)
	{
		assert(loc);
		return IsVillage(*loc);
	}

	inline bool IsMultiLevel(Location* loc)
	{
		assert(loc);
		if(loc->outside)
			return false;
		InsideLocation* inside = static_cast<InsideLocation*>(loc);
		return inside->IsMultilevel();
	}

	Unit* GetMayor(Location* loc);
	Unit* GetCaptain(Location* loc);
}
