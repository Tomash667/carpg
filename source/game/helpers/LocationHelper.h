#pragma once

#include "City.h"

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
}
