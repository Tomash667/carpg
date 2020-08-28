#include "Pch.h"
#include "BaseLocation.h"

#include "RoomType.h"
#include "UnitGroup.h"

#include <ResourceManager.h>

//-----------------------------------------------------------------------------
RoomStrChance human_fort_rooms[] = {
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 5),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 2),
	RoomStrChance("beer_storeroom", 5),
	RoomStrChance("meeting_room", 10),
	RoomStrChance("training_room", 5),
	RoomStrChance("crafting_room", 5),
	RoomStrChance("kitchen", 5)
};

//-----------------------------------------------------------------------------
RoomStrChance mage_tower_rooms[] = {
	RoomStrChance("magic", 3),
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 6),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 1),
	RoomStrChance("beer_storeroom", 3),
	RoomStrChance("meeting_room", 8),
	RoomStrChance("training_room", 4),
	RoomStrChance("crafting_room", 4),
	RoomStrChance("kitchen", 4)
};

//-----------------------------------------------------------------------------
RoomStrChance necro_base_rooms[] = {
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 5),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 2),
	RoomStrChance("beer_storeroom", 5),
	RoomStrChance("shrine", 5),
	RoomStrChance("meeting_room", 10),
	RoomStrChance("training_room", 5),
	RoomStrChance("crafting_room", 5),
	RoomStrChance("kitchen", 5)
};

//-----------------------------------------------------------------------------
RoomStrChance vault_rooms[] = {
	RoomStrChance("bedroom", 5),
	RoomStrChance("library", 2),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 5),
	RoomStrChance("beer_storeroom", 5),
	RoomStrChance("meeting_room", 5),
	RoomStrChance("training_room", 2),
	RoomStrChance("crafting_room", 2),
	RoomStrChance("kitchen", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance crypt_rooms[] = {
	RoomStrChance("graves", 8),
	RoomStrChance("graves2", 8),
	RoomStrChance("shrine", 4),
	RoomStrChance("crypt_room", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance labyrinth_rooms[] = {
	RoomStrChance("labyrinth_treasure", 1)
};

//-----------------------------------------------------------------------------
RoomStrChance tutorial_rooms[] = {
	RoomStrChance("tutorial", 1)
};

//-----------------------------------------------------------------------------
// Name is only visible in devmode
BaseLocation g_base_locations[] = {
	//	  NAME				LEVELS			MAP			JOIN		CORRIDOR	CORRIDOR	ROOM		OTHER...
	//										SIZE		COR	ROOM				LENGTH		SIZE
	"Human fort",			Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		human_fort_rooms, countof(human_fort_rooms), 0, 50, 25, 2, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Dwarf fort",			Int2(2, 4),		40, 3,		50, 5,		25,			Int2(5,12),	Int2(4,8),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		human_fort_rooms, countof(human_fort_rooms), 0, 60, 20, 6, "random", nullptr, nullptr, 100, 0, 0, TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile.jpg", "mur078.jpg", "sufit3.jpg"),
	"Mage tower",			Int2(4, 5),		30, 0,		0,	33,		0,			Int2(0,0),	Int2(4,7),	BLO_MAGIC_LIGHT | BLO_ROUND | BLO_DOOR_ENTRY | BLO_GOES_UP, "stairs", nullptr,
		Color(100,0,0), Color(0.4f,0.3f,0.3f), Vec2(10,20), 20.f,
		mage_tower_rooms, countof(mage_tower_rooms), 0, 100, 0, 3, "mages", "mages_and_golems", "random", 50, 25, 25, TRAPS_MAGIC, -1,
		LocationTexturePack("floor_pavingStone_ceramic.jpg", "stone01d.jpg", "block02b.jpg"),
	"Bandits hideout",		Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		human_fort_rooms, countof(human_fort_rooms), 0, 80, 10, 3, "bandits", "random", nullptr, 75, 25, 0, TRAPS_NORMAL | TRAPS_NEAR_ENTRANCE, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "sup075.jpg"),
	"Hero crypt",			Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		crypt_rooms, countof(crypt_rooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Monster crypt",		Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		crypt_rooms, countof(crypt_rooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Old temple",			Int2(1, 3),		40, 2,		35, 15,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "crypt_stairs", "shrine",
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		necro_base_rooms, countof(necro_base_rooms), 0, 80, 5, 1, "undead", "necromancers", "evil", 25, 25, 25, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Safehouse",			Int2(1, 1),		30, 0,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		vault_rooms, countof(vault_rooms), 0, 100, 0, 3, "bandits", nullptr, "random", 25, 25, 50, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Necromancers base",	Int2(2, 3),		45, 3,		35,	15,		25,			Int2(5,10),	Int2(5,10), BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "crypt_stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		necro_base_rooms, countof(necro_base_rooms), 0, 80, 5, 1, "necromancers", "evil", "random", 50, 25, 25, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_paving_littleStones3.jpg", "256-03b.jpg", "sufit2.jpg"),
	"Labyrinth",			Int2(1, 1),		60, 0,		0, 0,		0,			Int2(0,0),	Int2(6,6),	BLO_LABYRINTH, nullptr, nullptr,
		Color::Black, Color(0.33f,0.33f,0.33f), Vec2(3.f,15.f), 15.f,
		labyrinth_rooms, countof(labyrinth_rooms), 0, 0, 0, 3, "undead", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack("block01b.jpg", "stone01b.jpg", "block01d.jpg"),
	"Cave",					Int2(0,0),		52, 0,		0, 0,		0,			Int2(0,0),	Int2(0,0),	BLO_LABYRINTH,	nullptr, nullptr,
		Color::Black, Color(0.4f,0.4f,0.4f), Vec2(16.f,25.f), 25.f,
		nullptr, 0, 0, 0, 0, 0, "random", nullptr, nullptr, 100, 0, 0, 0, ANCIENT_ARMORY,
		LocationTexturePack("rock2.jpg", "rock1.jpg", "rock3.jpg"),
	"Ancient armory",		Int2(1,1),		45, 0,		35, 0,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "crypt_stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		human_fort_rooms, countof(human_fort_rooms), 0, 80, 5, 1, "golems", nullptr, nullptr, 100, 0, 0, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Tutorial",				Int2(1,1),		22, 0,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		tutorial_rooms, countof(tutorial_rooms), 0, 100, 0, 0, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Fort with throne",		Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		human_fort_rooms, countof(human_fort_rooms), 0, 50, 25, 2, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Safehouse with throne",Int2(2, 2),		35, 2,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		vault_rooms, countof(vault_rooms), 0, 100, 0, 3, "bandits", nullptr, "random", 25, 25, 50, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Crypt 2nd texture",	Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		crypt_rooms, countof(crypt_rooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, -1,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01b.jpg", "sufit2.jpg")
};
const uint n_base_locations = countof(g_base_locations);

//=================================================================================================
RoomType* BaseLocation::GetRandomRoomType() const
{
	assert(rooms);

	int total = 0, choice = Rand() % room_total;

	for(uint i = 0; i < room_count; ++i)
	{
		total += rooms[i].chance;
		if(choice < total)
			return rooms[i].room;
	}

	assert(0);
	return rooms[0].room;
}

//=================================================================================================
UnitGroup* BaseLocation::GetRandomGroup() const
{
	UnitGroup* result;
	int n = Rand() % 100;
	if(n < group_chance[0])
		result = group[0].value;
	else if(n < group_chance[0] + group_chance[1])
		result = group[1].value;
	else if(n < group_chance[0] + group_chance[1] + group_chance[2])
		result = group[2].value;
	else
		result = UnitGroup::random;

	if(result->is_list)
		result = result->GetRandomGroup();

	return result;
}

//=================================================================================================
void BaseLocation::PreloadTextures()
{
	for(uint i = 0; i < n_base_locations; ++i)
	{
		auto& bl = g_base_locations[i];
		if(bl.tex.floor.id)
		{
			bl.tex.floor.tex = res_mgr->Load<Texture>(bl.tex.floor.id);
			if(bl.tex.floor.id_normal)
				bl.tex.floor.tex_normal = res_mgr->Load<Texture>(bl.tex.floor.id_normal);
			if(bl.tex.floor.id_specular)
				bl.tex.floor.tex_specular = res_mgr->Load<Texture>(bl.tex.floor.id_specular);
		}
		if(bl.tex.wall.id)
		{
			bl.tex.wall.tex = res_mgr->Load<Texture>(bl.tex.wall.id);
			if(bl.tex.wall.id_normal)
				bl.tex.wall.tex_normal = res_mgr->Load<Texture>(bl.tex.wall.id_normal);
			if(bl.tex.wall.id_specular)
				bl.tex.wall.tex_specular = res_mgr->Load<Texture>(bl.tex.wall.id_specular);
		}
		if(bl.tex.ceil.id)
		{
			bl.tex.ceil.tex = res_mgr->Load<Texture>(bl.tex.ceil.id);
			if(bl.tex.ceil.id_normal)
				bl.tex.ceil.tex_normal = res_mgr->Load<Texture>(bl.tex.ceil.id_normal);
			if(bl.tex.ceil.id_specular)
				bl.tex.ceil.tex_specular = res_mgr->Load<Texture>(bl.tex.ceil.id_specular);
		}
	}
}

//=================================================================================================
uint BaseLocation::SetRoomPointers()
{
	uint errors = 0;

	for(uint i = 0; i < n_base_locations; ++i)
	{
		BaseLocation& base = g_base_locations[i];

		if(base.stairs.id)
			base.stairs.value = RoomType::Get(base.stairs.id);

		if(base.required.id)
			base.required.value = RoomType::Get(base.required.id);

		for(int j = 0; j < 3; ++j)
		{
			if(base.group[j].id)
			{
				base.group[j].value = UnitGroup::TryGet(base.group[j].id);
				if(!base.group[j].value)
				{
					++errors;
					Error("Missing unit group '%s'.", base.group[j].id);
					base.group[j].value = UnitGroup::empty;
				}
			}
			else
				base.group[j].value = UnitGroup::empty;
		}

		if(base.rooms)
		{
			base.room_total = 0;
			for(uint j = 0; j < base.room_count; ++j)
			{
				base.rooms[j].room = RoomType::Get(base.rooms[j].id);
				base.room_total += base.rooms[j].chance;
			}
		}
	}

	return errors;
}
