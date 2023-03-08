#pragma once

//-----------------------------------------------------------------------------
enum BaseLocationId
{
	HUMAN_FORT, // 2-3 levels, no traps
	DWARF_FORT, // 2-4 levels, have traps
	MAGE_TOWER, // 3-5 round small level; 50% chance for mages, 25% for mages & golems
	BANDITS_HIDEOUT, // 2-3 level, traps at entrance (75% chance for bandits)
	HERO_CRYPT, // 2-3 levels, someone important was burried here; 50% chance for undead, 25% for necromancers
	MONSTER_CRYPT, // 2-3 levels, some dangerous monsters were locked here; 75% chance for undead
	OLD_TEMPLE, // 1-3 levels, 25% undead, 25% necromancers, 25% evil
	VAULT, // 1 level with traps; 25% chance for empty, 25% for bandits
	NECROMANCER_BASE, // 50% chance for necromancers, 25% evil
	LABYRINTH, // maze
	CAVE,
	ANCIENT_ARMORY,
	TUTORIAL_FORT, // like HUMAN_FORT but with 100% door chance
	THRONE_FORT, // like HUMAN_FORT but with throne room
	THRONE_VAULT, // like VAULT but with throne room
	CRYPT_2_TEXTURE,
	BASE_LOCATION_MAX
};
static_assert(BASE_LOCATION_MAX <= 32, "Too many base locations, used as flags!");

//-----------------------------------------------------------------------------
// Location flags
enum BaseLocationOptions
{
	BLO_ROUND = 1 << 0,
	BLO_LABYRINTH = 1 << 1,
	BLO_LESS_FOOD = 1 << 2,
	BLO_DOOR_ENTRY = 1 << 3,
	BLO_GOES_UP = 1 << 4
};

//-----------------------------------------------------------------------------
struct RoomType;

//-----------------------------------------------------------------------------
template<typename T>
struct NameValue
{
	cstring id;
	T* value;

	NameValue(nullptr_t) : id(nullptr), value(nullptr) {}
	NameValue(cstring id) : id(id), value(nullptr) {}
};

//-----------------------------------------------------------------------------
typedef NameValue<RoomType> RoomStr;
typedef NameValue<UnitGroup> GroupStr;

//-----------------------------------------------------------------------------
struct RoomStrChance
{
	cstring id;
	RoomType* room;
	int chance;

	RoomStrChance(cstring id, int chance) : id(id), chance(chance), room(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
// Traps flags
enum TRAP_FLAGS
{
	TRAPS_NORMAL = 1 << 0,
	TRAPS_MAGIC = 1 << 1,
	TRAPS_NEAR_ENTRANCE = 1 << 2,
	TRAPS_NEAR_END = 1 << 3
};

//-----------------------------------------------------------------------------
struct LocationTexturePack
{
	struct Entry
	{
		cstring id, idNormal, idSpecular;
		TexturePtr tex, texNormal, texSpecular;

		Entry() : id(nullptr), idNormal(nullptr), idSpecular(nullptr), tex(nullptr), texNormal(nullptr), texSpecular(nullptr)
		{
		}
		Entry(cstring id, cstring idNormal, cstring idSpecular) : id(id), idNormal(idNormal), idSpecular(idSpecular), tex(nullptr), texNormal(nullptr),
			texSpecular(nullptr)
		{
		}
	};

	Entry floor, wall, ceil;

	LocationTexturePack()
	{
	}
	LocationTexturePack(cstring idFloor, cstring idWall, cstring idCeil) : floor(idFloor, nullptr, nullptr), wall(idWall, nullptr, nullptr), ceil(idCeil, nullptr, nullptr)
	{
	}
	LocationTexturePack(cstring idFloor, cstring idFloorNormal, cstring idFloorSpecular, cstring idWall, cstring idWallNormal, cstring idWallSpecular,
		cstring idCeil, cstring idCeilNormal, cstring idCeilSpecular) : floor(idFloor, idFloorNormal, idFloorSpecular), wall(idWall, idWallNormal,
		idWallSpecular), ceil(idCeil, idCeilNormal, idCeilSpecular)
	{
	}
};

//-----------------------------------------------------------------------------
// Base location types
struct BaseLocation
{
	cstring name;
	Int2 levels;
	int size, sizeLvl, joinRoom, joinCorridor, corridorChance;
	Int2 corridorSize, roomSize;
	int options;
	RoomStr stairs, required;
	Color fogColor, ambientColor;
	Vec2 fogRange;
	float drawRange;
	RoomStrChance* rooms;
	uint roomCount, roomTotal;
	int doorChance, doorOpen, barsChance;
	GroupStr group[3];
	int groupChance[3];
	int traps, tex2;
	LocationTexturePack tex;

	RoomType* GetRandomRoomType() const;
	UnitGroup* GetRandomGroup() const;
	static void PreloadTextures();
	static uint SetRoomPointers();
};
extern BaseLocation gBaseLocations[];
extern const uint nBaseLocations;
