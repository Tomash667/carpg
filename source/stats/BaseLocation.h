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
	BLO_MAGIC_LIGHT = 1 << 2,
	BLO_LESS_FOOD = 1 << 3,
	BLO_DOOR_ENTRY = 1 << 4,
	BLO_GOES_UP = 1 << 5
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
		cstring id, id_normal, id_specular;
		TexturePtr tex, tex_normal, tex_specular;

		Entry() : id(nullptr), id_normal(nullptr), id_specular(nullptr), tex(nullptr), tex_normal(nullptr), tex_specular(nullptr)
		{
		}
		Entry(cstring id, cstring id_normal, cstring id_specular) : id(id), id_normal(id_normal), id_specular(id_specular), tex(nullptr), tex_normal(nullptr),
			tex_specular(nullptr)
		{
		}
	};

	Entry floor, wall, ceil;

	LocationTexturePack()
	{
	}
	LocationTexturePack(cstring id_floor, cstring id_wall, cstring id_ceil) : floor(id_floor, nullptr, nullptr), wall(id_wall, nullptr, nullptr), ceil(id_ceil, nullptr, nullptr)
	{
	}
	LocationTexturePack(cstring id_floor, cstring id_floor_nrm, cstring id_floor_spec, cstring id_wall, cstring id_wall_nrm, cstring id_wall_spec, cstring id_ceil,
		cstring id_ceil_nrm, cstring id_ceil_spec) : floor(id_floor, id_floor_nrm, id_floor_spec), wall(id_wall, id_wall_nrm, id_wall_spec),
		ceil(id_ceil, id_ceil_nrm, id_ceil_spec)
	{
	}
};

//-----------------------------------------------------------------------------
// Base location types
struct BaseLocation
{
	cstring name;
	Int2 levels;
	int size, size_lvl, join_room, join_corridor, corridor_chance;
	Int2 corridor_size, room_size;
	int options;
	RoomStr stairs, required;
	Color fog_color, ambient_color;
	Vec2 fog_range;
	float draw_range;
	RoomStrChance* rooms;
	uint room_count, room_total;
	int door_chance, door_open, bars_chance;
	GroupStr group[3];
	int group_chance[3];
	int traps, tex2;
	LocationTexturePack tex;

	RoomType* GetRandomRoomType() const;
	UnitGroup* GetRandomGroup() const;
	static void PreloadTextures();
	static uint SetRoomPointers();
};
extern BaseLocation g_base_locations[];
extern const uint n_base_locations;
