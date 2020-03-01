#include "Pch.h"
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
	Location* target = nullptr;
	for(int i = 0; i < 10; ++i)
	{
		Vec2 pos;
		switch(Rand() % 4)
		{
		case 0:
			pos = Vec2(bounds.x + 16.f, bounds.x + 16.f);
			break;
		case 1:
			pos = Vec2(bounds.y - 16.f, bounds.x + 16.f);
			break;
		case 2:
			pos = Vec2(bounds.x + 16.f, bounds.y - 16.f);
			break;
		case 3:
			pos = Vec2(bounds.y - 16.f, bounds.y - 16.f);
			break;
		}
		if(!world->TryFindPlace(pos, 64.f))
			continue;
		target = world->CreateLocation(L_DUNGEON, pos, HERO_CRYPT, 2);
		if(target)
		{
			target->group = UnitGroup::Get("golems");
			break;
		}
	}
	if(!target)
		throw "Failed to create artifacts quest location.";
	target->active_quest = this;
	target->st = 12;
	target_loc = target->index;

	SetupEvent();
}

//=================================================================================================
Quest::LoadResult Quest_Artifacts::Load(GameReader& f)
{
	Quest_Dungeon::Load(f);

	if(!done)
		SetupEvent();

	return LoadResult::Ok;
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
