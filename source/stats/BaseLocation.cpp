#include "Pch.h"
#include "BaseLocation.h"

#include "RoomType.h"
#include "UnitGroup.h"

#include <ResourceManager.h>

//-----------------------------------------------------------------------------
RoomStrChance humanFortRooms[] = {
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 5),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 2),
	RoomStrChance("beerStoreroom", 5),
	RoomStrChance("meetingRoom", 10),
	RoomStrChance("trainingRoom", 5),
	RoomStrChance("craftingRoom", 5),
	RoomStrChance("kitchen", 5)
};

//-----------------------------------------------------------------------------
RoomStrChance mageTowerRooms[] = {
	RoomStrChance("magic", 3),
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 6),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 1),
	RoomStrChance("beerStoreroom", 3),
	RoomStrChance("meetingRoom", 8),
	RoomStrChance("trainingRoom", 4),
	RoomStrChance("craftingRoom", 4),
	RoomStrChance("kitchen", 4)
};

//-----------------------------------------------------------------------------
RoomStrChance necroBaseRooms[] = {
	RoomStrChance("bedroom", 10),
	RoomStrChance("commander", 2),
	RoomStrChance("library", 5),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 2),
	RoomStrChance("beerStoreroom", 5),
	RoomStrChance("shrine", 5),
	RoomStrChance("meetingRoom", 10),
	RoomStrChance("trainingRoom", 5),
	RoomStrChance("craftingRoom", 5),
	RoomStrChance("kitchen", 5)
};

//-----------------------------------------------------------------------------
RoomStrChance vaultRooms[] = {
	RoomStrChance("bedroom", 5),
	RoomStrChance("library", 2),
	RoomStrChance("storeroom", 10),
	RoomStrChance("saferoom", 5),
	RoomStrChance("beerStoreroom", 5),
	RoomStrChance("meetingRoom", 5),
	RoomStrChance("trainingRoom", 2),
	RoomStrChance("craftingRoom", 2),
	RoomStrChance("kitchen", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance cryptRooms[] = {
	RoomStrChance("graves", 8),
	RoomStrChance("graves2", 8),
	RoomStrChance("shrine", 4),
	RoomStrChance("cryptRoom", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance labyrinthRooms[] = {
	RoomStrChance("labyrinthTreasure", 1)
};

//-----------------------------------------------------------------------------
RoomStrChance tutorialRooms[] = {
	RoomStrChance("tutorial", 1)
};

//-----------------------------------------------------------------------------
// Name is only visible in devmode
BaseLocation gBaseLocations[] = {
	//	  NAME				LEVELS			MAP			JOIN		CORRIDOR	CORRIDOR	ROOM		OTHER...
	//										SIZE		COR	ROOM				LENGTH		SIZE
	"Human fort",			Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		humanFortRooms, countof(humanFortRooms), 0, 50, 25, 2, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Dwarf fort",			Int2(2, 4),		40, 3,		50, 5,		25,			Int2(5,12),	Int2(4,8),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		humanFortRooms, countof(humanFortRooms), 0, 60, 20, 6, "random", nullptr, nullptr, 100, 0, 0, TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile.jpg", "mur078.jpg", "sufit3.jpg"),
	"Mage tower",			Int2(4, 5),		30, 0,		0,	33,		0,			Int2(0,0),	Int2(4,7),	BLO_MAGIC_LIGHT | BLO_ROUND | BLO_DOOR_ENTRY | BLO_GOES_UP, "stairs", nullptr,
		Color(100,0,0), Color(0.4f,0.3f,0.3f), Vec2(10,20), 20.f,
		mageTowerRooms, countof(mageTowerRooms), 0, 100, 0, 3, "mages", "mages_and_golems", "random", 50, 25, 25, TRAPS_MAGIC, -1,
		LocationTexturePack("floor_pavingStone_ceramic.jpg", "stone01d.jpg", "block02b.jpg"),
	"Bandits hideout",		Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		humanFortRooms, countof(humanFortRooms), 0, 80, 10, 3, "bandits", "random", nullptr, 75, 25, 0, TRAPS_NORMAL | TRAPS_NEAR_ENTRANCE, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "sup075.jpg"),
	"Hero crypt",			Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		cryptRooms, countof(cryptRooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Monster crypt",		Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		cryptRooms, countof(cryptRooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Old temple",			Int2(1, 3),		40, 2,		35, 15,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "cryptStairs", "shrine",
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		necroBaseRooms, countof(necroBaseRooms), 0, 80, 5, 1, "undead", "necromancers", "evil", 25, 25, 25, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Safehouse",			Int2(1, 1),		30, 0,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		vaultRooms, countof(vaultRooms), 0, 100, 0, 3, "bandits", nullptr, "random", 25, 25, 50, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Necromancers base",	Int2(2, 3),		45, 3,		35,	15,		25,			Int2(5,10),	Int2(5,10), BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "cryptStairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		necroBaseRooms, countof(necroBaseRooms), 0, 80, 5, 1, "necromancers", "evil", "random", 50, 25, 25, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_paving_littleStones3.jpg", "256-03b.jpg", "sufit2.jpg"),
	"Labyrinth",			Int2(1, 1),		60, 0,		0, 0,		0,			Int2(0,0),	Int2(6,6),	BLO_LABYRINTH, nullptr, nullptr,
		Color::Black, Color(0.33f,0.33f,0.33f), Vec2(3.f,15.f), 15.f,
		labyrinthRooms, countof(labyrinthRooms), 0, 0, 0, 3, "undead", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack("block01b.jpg", "stone01b.jpg", "block01d.jpg"),
	"Cave",					Int2(0,0),		52, 0,		0, 0,		0,			Int2(0,0),	Int2(0,0),	BLO_LABYRINTH,	nullptr, nullptr,
		Color::Black, Color(0.4f,0.4f,0.4f), Vec2(16.f,25.f), 25.f,
		nullptr, 0, 0, 0, 0, 0, "random", nullptr, nullptr, 100, 0, 0, 0, ANCIENT_ARMORY,
		LocationTexturePack("rock2.jpg", "rock1.jpg", "rock3.jpg"),
	"Ancient armory",		Int2(1,1),		45, 0,		35, 0,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "cryptStairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		humanFortRooms, countof(humanFortRooms), 0, 80, 5, 1, "golems", nullptr, nullptr, 100, 0, 0, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Tutorial",				Int2(1,1),		22, 0,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		tutorialRooms, countof(tutorialRooms), 0, 100, 0, 0, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Fort with throne",		Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	BLO_DOOR_ENTRY, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		humanFortRooms, countof(humanFortRooms), 0, 50, 25, 2, "random", nullptr, nullptr, 100, 0, 0, 0, -1,
		LocationTexturePack(),
	"Safehouse with throne",Int2(2, 2),		35, 2,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, "stairs", nullptr,
		Color::Black, Color(0.3f,0.3f,0.3f), Vec2(10,20), 20.f,
		vaultRooms, countof(vaultRooms), 0, 100, 0, 3, "bandits", nullptr, "random", 25, 25, 50, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Crypt 2nd texture",	Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, "stairs", nullptr,
		Color(40,40,40), Color(0.25f,0.25f,0.25f), Vec2(5.f,18.f), 18.f,
		cryptRooms, countof(cryptRooms), 0, 80, 5, 1, "undead", "necromancers", "random", 50, 25, 25, TRAPS_NORMAL | TRAPS_NEAR_END, -1,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01b.jpg", "sufit2.jpg")
};
const uint nBaseLocations = countof(gBaseLocations);

//=================================================================================================
RoomType* BaseLocation::GetRandomRoomType() const
{
	assert(rooms);

	int total = 0, choice = Rand() % roomTotal;

	for(uint i = 0; i < roomCount; ++i)
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
	if(n < groupChance[0])
		result = group[0].value;
	else if(n < groupChance[0] + groupChance[1])
		result = group[1].value;
	else if(n < groupChance[0] + groupChance[1] + groupChance[2])
		result = group[2].value;
	else
		result = UnitGroup::random;

	if(result->isList)
		result = result->GetRandomGroup();

	return result;
}

//=================================================================================================
void BaseLocation::PreloadTextures()
{
	for(uint i = 0; i < nBaseLocations; ++i)
	{
		auto& bl = gBaseLocations[i];
		if(bl.tex.floor.id)
		{
			bl.tex.floor.tex = resMgr->Load<Texture>(bl.tex.floor.id);
			if(bl.tex.floor.idNormal)
				bl.tex.floor.texNormal = resMgr->Load<Texture>(bl.tex.floor.idNormal);
			if(bl.tex.floor.idSpecular)
				bl.tex.floor.texSpecular = resMgr->Load<Texture>(bl.tex.floor.idSpecular);
		}
		if(bl.tex.wall.id)
		{
			bl.tex.wall.tex = resMgr->Load<Texture>(bl.tex.wall.id);
			if(bl.tex.wall.idNormal)
				bl.tex.wall.texNormal = resMgr->Load<Texture>(bl.tex.wall.idNormal);
			if(bl.tex.wall.idSpecular)
				bl.tex.wall.texSpecular = resMgr->Load<Texture>(bl.tex.wall.idSpecular);
		}
		if(bl.tex.ceil.id)
		{
			bl.tex.ceil.tex = resMgr->Load<Texture>(bl.tex.ceil.id);
			if(bl.tex.ceil.idNormal)
				bl.tex.ceil.texNormal = resMgr->Load<Texture>(bl.tex.ceil.idNormal);
			if(bl.tex.ceil.idSpecular)
				bl.tex.ceil.texSpecular = resMgr->Load<Texture>(bl.tex.ceil.idSpecular);
		}
	}
}

//=================================================================================================
uint BaseLocation::SetRoomPointers()
{
	uint errors = 0;

	for(uint i = 0; i < nBaseLocations; ++i)
	{
		BaseLocation& base = gBaseLocations[i];

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
			base.roomTotal = 0;
			for(uint j = 0; j < base.roomCount; ++j)
			{
				base.rooms[j].room = RoomType::Get(base.rooms[j].id);
				base.roomTotal += base.rooms[j].chance;
			}
		}
	}

	return errors;
}
