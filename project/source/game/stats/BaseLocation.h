// bazowe typy podziemi i krypt
#pragma once

//-----------------------------------------------------------------------------
#include "SpawnGroup.h"
#include "Resource.h"

//-----------------------------------------------------------------------------
enum BaseLocationId
{
	// fort ludzi, 2-3 poziomów, bez pu³apek czy ukrytych przejœæ
	HUMAN_FORT,
	// fort krasnoludów 3-5 poziomów, pu³apki, tajne przejœcia, na dole skarby
	DWARF_FORT,
	// wie¿a magów, 4-6 okr¹g³ych ma³ych poziomów
	// 50% szansy na magów, 25% na magów i golemy
	MAGE_TOWER,
	// kryjówka bandytów, 1 poziom, pu³apki przy wejœciu
	// 75% szansy na bandytów
	BANDITS_HIDEOUT,
	// krypta w której pochowano jak¹œ znan¹ osobê, 2-3 poziomów, na pocz¹tku ostrze¿enia, na ostatnim poziomie du¿o pu³apek i centralna sala ze zw³okami i skarbami
	// 50% szansy na nieumar³ych, 25% nekro
	HERO_CRYPT,
	// uwiêziono tu jakiegoœ z³ego potwora, 2-3 poziomów, jak wy¿ej
	// 75% szansy na nieumar³ych
	MONSTER_CRYPT,
	// stara œwi¹tynia, 1-3 poziomów, mo¿e byæ oczyszczona lub nie ze z³ych kap³anów/nekromantów
	// 25% nieumarli, 25% nekro, 25% Ÿli
	OLD_TEMPLE,
	// ukryta skrytka, 1 poziom, wygl¹da jak zwyk³y poziom ale s¹ ukryte przejœcia i pu³apki, na koñcu skarb
	// 25% nic, 25% bandyci
	VAULT,
	// baza nekromantów
	// 50% nekro, 25% z³o
	NECROMANCER_BASE,
	// labirynt
	LABIRYNTH,
	// jaskina
	CAVE,
	// koñcowy poziom questu kopalnia
	KOPALNIA_POZIOM,
	// jak HUMAN_FORT ale 100% szansy na drzwi
	TUTORIAL_FORT,
	// jak HUMAN_FORT ale z sal¹ tronow¹
	THRONE_FORT,
	// jak VAULT ale z sal¹ tronow¹
	THRONE_VAULT,
	// druga tekstura krypty
	CRYPT_2_TEXTURE
};

//-----------------------------------------------------------------------------
// Location flags
enum BaseLocationOptions
{
	BLO_ROUND = 1 << 0,
	BLO_LABIRYNTH = 1 << 1,
	BLO_MAGIC_LIGHT = 1 << 2,
	BLO_LESS_FOOD = 1 << 3,
	BLO_NO_TEX_WRAP = 1 << 4,
};

//-----------------------------------------------------------------------------
struct RoomType;

//-----------------------------------------------------------------------------
struct RoomStr
{
	cstring id;
	RoomType* room;

	explicit RoomStr(cstring _id) : id(_id), room(nullptr)
	{
	}
};

//-----------------------------------------------------------------------------
struct RoomStrChance
{
	cstring id;
	RoomType* room;
	int chance;

	RoomStrChance(cstring _id, int _chance) : id(_id), chance(_chance), room(nullptr)
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
// Bazowa lokalizacja, rodzaj podziemi/krypty
struct BaseLocation
{
	cstring name;
	Int2 levels;
	int size, size_lvl, join_room, join_corridor, corridor_chance;
	Int2 corridor_size, room_size;
	int options;
	RoomStr schody, wymagany;
	Vec3 fog_color, fog_color_lvl, ambient_color, ambient_color_lvl;
	Vec2 fog_range, fog_range_lvl;
	float draw_range, draw_range_lvl;
	RoomStrChance* rooms;
	uint room_count, room_total;
	int door_chance, door_open, bars_chance;
	SPAWN_GROUP sg1, sg2, sg3;
	int schance1, schance2, schance3;
	int traps, tex2;
	LocationTexturePack tex;

	RoomType* GetRandomRoomType() const;
	SPAWN_GROUP GetRandomSpawnGroup() const;
	static void PreloadTextures();
};
extern BaseLocation g_base_locations[];
extern const uint n_base_locations;
