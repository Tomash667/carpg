#include "Pch.h"
#include "GameCore.h"
#include "Building.h"
#include "ContentLoader.h"
#include "Crc.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
vector<Building*> Building::buildings;

//=================================================================================================
Building* Building::TryGet(Cstring id)
{
	for(auto building : buildings)
	{
		if(building->id == id)
			return building;
	}

	return nullptr;
}

//=================================================================================================
// Find old building using hardcoded identifier
// Required for pre 0.5 compability
Building* old::Convert(BUILDING building_id)
{
	cstring name;
	switch(building_id)
	{
	case BUILDING::B_MERCHANT:
		name = "merchant";
		break;
	case BUILDING::B_BLACKSMITH:
		name = "blacksmith";
		break;
	case BUILDING::B_ALCHEMIST:
		name = "alchemist";
		break;
	case BUILDING::B_TRAINING_GROUNDS:
		name = "training_grounds";
		break;
	case BUILDING::B_INN:
		name = "inn";
		break;
	case BUILDING::B_CITY_HALL:
		name = "city_hall";
		break;
	case BUILDING::B_VILLAGE_HALL:
		name = "village_hall";
		break;
	case BUILDING::B_BARRACKS:
		name = "barracks";
		break;
	case BUILDING::B_HOUSE:
		name = "house";
		break;
	case BUILDING::B_HOUSE2:
		name = "house2";
		break;
	case BUILDING::B_HOUSE3:
		name = "house3";
		break;
	case BUILDING::B_ARENA:
		name = "arena";
		break;
	case BUILDING::B_FOOD_SELLER:
		name = "food_seller";
		break;
	case BUILDING::B_COTTAGE:
		name = "cottage";
		break;
	case BUILDING::B_COTTAGE2:
		name = "cottage2";
		break;
	case BUILDING::B_COTTAGE3:
		name = "cottage3";
		break;
	case BUILDING::B_VILLAGE_INN:
		name = "village_inn";
		break;
	case BUILDING::B_VILLAGE_HALL_OLD:
		name = "village_hall_old";
		break;
	case BUILDING::B_NONE:
	case BUILDING::B_MAX:
	default:
		assert(0);
		return nullptr;
	}

	return Building::Get(name);
}
