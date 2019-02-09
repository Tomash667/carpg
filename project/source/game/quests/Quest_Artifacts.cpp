#include "Pch.h"
#include "GameCore.h"
#include "Quest_Artifacts.h"
#include "World.h"
#include "BaseLocation.h"

//=================================================================================================
void Quest_Artifacts::Start()
{
	start_loc = -1;
	quest_id = Q_ARTIFACTS;
	type = QuestType::Unique;

	// one of corners
	const Vec2& bounds = W.GetWorldBounds();
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
	Location* target = W.CreateLocation(L_CRYPT, pos, 64.f, HERO_CRYPT, SG_GOLEMS, true, 2);
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
	spawn_unit_room = RoomTarget::Treasury;
}
