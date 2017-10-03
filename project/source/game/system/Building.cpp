#include "Pch.h"
#include "Core.h"
#include "Building.h"
#include "ContentLoader.h"
#include "Crc.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
vector<Building*> Building::buildings;

//=================================================================================================
Building* Building::TryGet(const AnyString& id)
{
	for(auto building : buildings)
	{
		if(building->id == id)
			return building;
	}

	return nullptr;
}

/*
//-----------------------------------------------------------------------------
// Hardcoded building groups
BuildingGroup* BuildingGroup::BG_INN;
BuildingGroup* BuildingGroup::BG_HALL;
BuildingGroup* BuildingGroup::BG_TRAINING_GROUNDS;
BuildingGroup* BuildingGroup::BG_ARENA;
BuildingGroup* BuildingGroup::BG_FOOD_SELLER;
BuildingGroup* BuildingGroup::BG_ALCHEMIST;
BuildingGroup* BuildingGroup::BG_BLACKSMITH;
BuildingGroup* BuildingGroup::BG_MERCHANT;*/

//=================================================================================================
// Find old building using hardcoded identifier
// Required for pre 0.5 compability
Building* Building::GetOld(OLD_BUILDING building_id)
{
	cstring name;
	switch(building_id)
	{
	case OLD_BUILDING::B_MERCHANT:
		name = "merchant";
		break;
	case OLD_BUILDING::B_BLACKSMITH:
		name = "blacksmith";
		break;
	case OLD_BUILDING::B_ALCHEMIST:
		name = "alchemist";
		break;
	case OLD_BUILDING::B_TRAINING_GROUNDS:
		name = "training_grounds";
		break;
	case OLD_BUILDING::B_INN:
		name = "inn";
		break;
	case OLD_BUILDING::B_CITY_HALL:
		name = "city_hall";
		break;
	case OLD_BUILDING::B_VILLAGE_HALL:
		name = "village_hall";
		break;
	case OLD_BUILDING::B_BARRACKS:
		name = "barracks";
		break;
	case OLD_BUILDING::B_HOUSE:
		name = "house";
		break;
	case OLD_BUILDING::B_HOUSE2:
		name = "house2";
		break;
	case OLD_BUILDING::B_HOUSE3:
		name = "house3";
		break;
	case OLD_BUILDING::B_ARENA:
		name = "arena";
		break;
	case OLD_BUILDING::B_FOOD_SELLER:
		name = "food_seller";
		break;
	case OLD_BUILDING::B_COTTAGE:
		name = "cottage";
		break;
	case OLD_BUILDING::B_COTTAGE2:
		name = "cottage2";
		break;
	case OLD_BUILDING::B_COTTAGE3:
		name = "cottage3";
		break;
	case OLD_BUILDING::B_VILLAGE_INN:
		name = "village_inn";
		break;
	case OLD_BUILDING::B_VILLAGE_HALL_OLD:
		name = "village_hall_old";
		break;
	case OLD_BUILDING::B_NONE:
	case OLD_BUILDING::B_MAX:
	default:
		assert(0);
		return nullptr;
	}

	return Get(name);
}
