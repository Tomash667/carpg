#include "Pch.h"
#include "GameCore.h"
#include "Quest_Artifacts.h"
#include "World.h"
#include "BaseLocation.h"

//=================================================================================================
void Quest_Artifacts::Start()
{
	start_loc = -1;
	type = Q_ARTIFACTS;
	category = QuestCategory::Unique;

	// one of corners
	const Vec2& bounds = world->GetWorldBounds();
	Vec2 pos;
	switch(Rand() % 4)
	{
	case 0:
		pos = Vec2(bounds.x, bounds.x);
		break;
	case 1:
		pos = Vec2(bounds.y, bounds.x);
		break;
	case 2:
		pos = Vec2(bounds.x, bounds.y);
		break;
	case 3:
		pos = Vec2(bounds.y, bounds.y);
		break;
	}
	Location* target = world->CreateLocation(L_DUNGEON, pos, 64.f, HERO_CRYPT, UnitGroup::Get("golems"), true, 2);
	target->active_quest = this;
	target->st = 12;
	target_loc = target->index;

	SetupEvent();
}

//=================================================================================================
bool Quest_Artifacts::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(!done)
		SetupEvent();

	return true;
}

//=================================================================================================
void Quest_Artifacts::SetupEvent()
{
	spawn_item = Quest_Event::Item_InTreasure;
	at_level = 1;
	item_to_give[0] = Item::Get("am_gladiator");
	unit_to_spawn = UnitData::Get("golem_adamantine");
	unit_to_spawn2 = UnitData::Get("golem_iron");
	spawn_unit_room = RoomTarget::Treasury;
	spawn_unit_room2 = RoomTarget::Treasury;
}
