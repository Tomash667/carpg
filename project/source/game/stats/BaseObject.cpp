#include "Pch.h"
#include "Core.h"
#include "BaseObject.h"

//-----------------------------------------------------------------------------
// Variants
VariantObject::Entry bench_variants[] = {
	"lawa.qmsh", nullptr,
	"lawa2.qmsh", nullptr,
	"lawa3.qmsh", nullptr,
	"lawa4.qmsh", nullptr
};
VariantObject bench_variant = { bench_variants, countof(bench_variants), false };

//-----------------------------------------------------------------------------
// Nazwa nie jest wyœwietlana
BaseObject g_objs[] = {
	BaseObject("barrel", OBJ_NEAR_WALL, 0, "Barrel", "beczka.qmsh", 0.38f, 1.083f),
	BaseObject("barrels", OBJ_NEAR_WALL, 0, "Barrels", "beczki.qmsh"),
	BaseObject("big_barrel", OBJ_NEAR_WALL, 0, "Big barrel", "beczka2.qmsh"),
	BaseObject("box", OBJ_NEAR_WALL, 0, "Box", "skrzynka.qmsh"),
	BaseObject("boxes", OBJ_NEAR_WALL, 0, "Boxes", "skrzynki.qmsh"),
	BaseObject("table", 0, 0, "Table", "stol.qmsh"),
	BaseObject("table2", 0, 0, "Table with objects", "stol2.qmsh"),
	BaseObject("wagon", 0, OBJ2_MULTI_PHYSICS, "Wagon", "woz.qmsh"),
	BaseObject("bow_target", OBJ_NEAR_WALL | OBJ_PHYSICS_PTR, 0, "Archery target", "tarcza_strzelnicza.qmsh"),
	BaseObject("torch", OBJ_NEAR_WALL | OBJ_LIGHT | OBJ_IMPORTANT, 0, "Torch", "pochodnia.qmsh", 0.27f / 4, 2.18f, -1, 2.2f, nullptr, 0.5f),
	BaseObject("torch_off", OBJ_NEAR_WALL, 0, "Torch (not lit)", "pochodnia.qmsh", 0.27f / 4, 2.18f),
	BaseObject("tanning_rack", OBJ_NEAR_WALL, 0, "Tanning rack", "tanning_rack.qmsh", 0),
	BaseObject("altar", OBJ_NEAR_WALL | OBJ_IMPORTANT | OBJ_REQUIRED, 0, "Altar", "oltarz.qmsh", 0),
	BaseObject("bloody_altar", OBJ_NEAR_WALL | OBJ_IMPORTANT | OBJ_BLOOD_EFFECT, 0, "Bloody altar", "krwawy_oltarz.qmsh", -1, 0.782f),
	BaseObject("chair", OBJ_USEABLE | OBJ_CHAIR, 0, "Chair", "krzeslo.qmsh"),
	BaseObject("bookshelf", OBJ_NEAR_WALL | OBJ_USEABLE | OBJ_BOOKSHELF, 0, "Bookshelf", "biblioteczka.qmsh"),
	BaseObject("tent", 0, 0, "Tent", "namiot.qmsh"),
	BaseObject("hay", OBJ_NEAR_WALL, 0, "Hay", "snopek.qmsh"),
	BaseObject("firewood", OBJ_NEAR_WALL, 0, "Firewood", "drewno_na_opal.qmsh"),
	BaseObject("wardrobe", OBJ_NEAR_WALL, 0, "Wardrobe", "szafa.qmsh"),
	BaseObject("bed", OBJ_NEAR_WALL, 0, "Bed", "lozko.qmsh"),
	BaseObject("bedding", OBJ_NO_PHYSICS, 0, "Bedding", "poslanie.qmsh"),
	BaseObject("painting1", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz1.qmsh"),
	BaseObject("painting2", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz2.qmsh"),
	BaseObject("painting3", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz3.qmsh"),
	BaseObject("painting4", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz4.qmsh"),
	BaseObject("painting5", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz5.qmsh"),
	BaseObject("painting6", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz6.qmsh"),
	BaseObject("painting7", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz7.qmsh"),
	BaseObject("painting8", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz8.qmsh"),
	BaseObject("bench", OBJ_NEAR_WALL | OBJ_USEABLE | OBJ_BENCH, OBJ2_VARIANT, "Bench", nullptr, -1, 0.f, &bench_variant),
	BaseObject("bench_dir", OBJ_NEAR_WALL | OBJ_USEABLE, OBJ2_BENCH_ROT | OBJ2_VARIANT, "Rotated bench", nullptr, -1, 0.f, &bench_variant),
	BaseObject("campfire", OBJ_NO_PHYSICS | OBJ_LIGHT | OBJ_CAMPFIRE, 0, "Campfire", "ognisko.qmsh", 2.147f / 2, 0.43f, -1, 0.4f),
	BaseObject("campfire_off", OBJ_NO_PHYSICS, 0, "Burnt campfire", "ognisko2.qmsh", 2.147f / 2, 0.43f),
	BaseObject("chest", OBJ_CHEST | OBJ_NEAR_WALL, 0, "Chest", "skrzynia.qmsh"),
	BaseObject("chest_r", OBJ_CHEST | OBJ_REQUIRED | OBJ_IMPORTANT | OBJ_IN_MIDDLE, 0, "Chest", "skrzynia.qmsh"),
	BaseObject("melee_target", OBJ_PHYSICS_PTR, 0, "Melee target", "manekin.qmsh", 0.27f, 1.85f),
	BaseObject("emblem", OBJ_NO_PHYSICS | OBJ_NEAR_WALL | OBJ_ON_WALL, 0, "Emblem", "emblemat.qmsh"),
	BaseObject("emblem_t", OBJ_NO_PHYSICS | OBJ_NEAR_WALL | OBJ_ON_WALL, 0, "Emblem", "emblemat_t.qmsh"),
	BaseObject("gobelin", OBJ_NO_PHYSICS | OBJ_NEAR_WALL | OBJ_ON_WALL, 0, "Gobelin", "gobelin.qmsh"),
	BaseObject("table_and_chairs", OBJ_TABLE, 0, "Table and chairs", nullptr, 3.f, 0.6f),
	BaseObject("tablechairs", OBJ_TABLE, 0, "Table and chairs", nullptr, 3.f, 0.6f), // za d³uga nazwa dla blendera :|
	BaseObject("anvil", OBJ_USEABLE | OBJ_ANVIL, 0, "Anvil", "kowadlo.qmsh"),
	BaseObject("cauldron", OBJ_USEABLE | OBJ_CAULDRON, 0, "Cauldron", "kociol.qmsh"),
	BaseObject("grave", OBJ_NEAR_WALL, 0, "Grave", "grob.qmsh"),
	BaseObject("tombstone_1", 0, 0, "Tombstone", "nagrobek.qmsh"),
	BaseObject("mushrooms", OBJ_NO_PHYSICS, 0, "Mushrooms", "grzyby.qmsh"),
	BaseObject("stalactite", OBJ_NO_PHYSICS, 0, "Stalactite", "stalaktyt.qmsh"),
	BaseObject("plant2", OBJ_NO_PHYSICS | OBJ_BILLBOARD, 0, "Plant", "krzak2.qmsh", 1),
	BaseObject("rock", 0, 0, "Rock", "kamien.qmsh", 0.456f, 0.67f),
	BaseObject("tree", OBJ_SCALEABLE, 0, "Tree", "drzewo.qmsh", 0.043f, 5.f, 1),
	BaseObject("tree2", OBJ_SCALEABLE, 0, "Tree", "drzewo2.qmsh", 0.024f, 5.f, 1),
	BaseObject("tree3", OBJ_SCALEABLE, 0, "Tree", "drzewo3.qmsh", 0.067f, 5.f, 1),
	BaseObject("grass", OBJ_NO_PHYSICS | OBJ_PRELOAD, 0, "Grass", "trawa.qmsh", 1),
	BaseObject("plant", OBJ_NO_PHYSICS, 0, "Plant", "krzak.qmsh", 1),
	BaseObject("plant_pot", 0, 0, "Plant pot", "doniczka.qmsh", 0.514f / 2, 0.1f / 2, 1),
	BaseObject("desk", 0, 0, "Desk", "biurko.qmsh", 1),
	BaseObject("withered_tree", OBJ_SCALEABLE, 0, "Withered tree", "drzewo_uschniete.qmsh", 0.58f, 6.f),
	BaseObject("tartak", OBJ_BUILDING, 0, "Sawmill", "tartak.qmsh", 0.f, 0.f),
	BaseObject("iron_ore", OBJ_USEABLE | OBJ_IRON_VEIN, 0, "Iron ore", "iron_ore.qmsh"),
	BaseObject("gold_ore", OBJ_USEABLE | OBJ_GOLD_VEIN, 0, "Gold ore", "gold_ore.qmsh"),
	BaseObject("portal", OBJ_DOUBLE_PHYSICS | OBJ_IMPORTANT | OBJ_REQUIRED | OBJ_IN_MIDDLE, 0, "Portal", "portal.qmsh"),
	BaseObject("magic_thing", OBJ_IN_MIDDLE | OBJ_LIGHT, 0, "Magic device", "magiczne_cos.qmsh", 1.122f / 2, 0.844f, -1, 0.844f),
	BaseObject("throne", OBJ_IMPORTANT | OBJ_REQUIRED | OBJ_USEABLE | OBJ_THRONE | OBJ_NEAR_WALL, 0, "Throne", "tron.qmsh"),
	BaseObject("moonwell", OBJ_WATER_EFFECT, 0, "Moonwell", "moonwell.qmsh", 1.031f, 0.755f),
	BaseObject("moonwell_phy", OBJ_DOUBLE_PHYSICS | OBJ_PHY_ROT, 0, "Moonwell physics", "moonwell_phy.qmsh"),
	BaseObject("tomashu_dom", OBJ_BUILDING, 0, "Tomashu house", "tomashu_dom.qmsh"),
	BaseObject("shelves", OBJ_NEAR_WALL, 0, "Shelves", "polki.qmsh"),
	BaseObject("stool", OBJ_USEABLE | OBJ_STOOL, 0, "Stool", "stolek.qmsh", 0.27f, 0.44f),
	BaseObject("bania", 0, 0, "Bania", "bania.qmsh", 1.f, 1.f),
	BaseObject("painting_x1", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz_x1.qmsh"),
	BaseObject("painting_x2", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz_x2.qmsh"),
	BaseObject("painting_x3", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz_x3.qmsh"),
	BaseObject("painting_x4", OBJ_NEAR_WALL | OBJ_NO_PHYSICS | OBJ_HIGH | OBJ_ON_WALL, 0, "Painting", "obraz_x4.qmsh"),
	BaseObject("coveredwell", 0, 0, "Covered well", "coveredwell.qmsh", 1.237f, 2.94f),
	BaseObject("obelisk", 0, 0, "Obelisk", "obelisk.qmsh"),
	BaseObject("tombstone_x1", 0, 0, "Tombstone", "nagrobek_x1.qmsh"),
	BaseObject("tombstone_x2", 0, 0, "Tombstone", "nagrobek_x2.qmsh"),
	BaseObject("tombstone_x3", 0, 0, "Tombstone", "nagrobek_x3.qmsh"),
	BaseObject("tombstone_x4", 0, 0, "Tombstone", "nagrobek_x4.qmsh"),
	BaseObject("tombstone_x5", 0, 0, "Tombstone", "nagrobek_x5.qmsh"),
	BaseObject("tombstone_x6", 0, 0, "Tombstone", "nagrobek_x6.qmsh"),
	BaseObject("tombstone_x7", 0, 0, "Tombstone", "nagrobek_x7.qmsh"),
	BaseObject("tombstone_x8", 0, 0, "Tombstone", "nagrobek_x8.qmsh"),
	BaseObject("tombstone_x9", 0, 0, "Tombstone", "nagrobek_x9.qmsh"),
	BaseObject("wall", OBJ_PHY_BLOCKS_CAM, 0, "Wall", "mur.qmsh", 0),
	BaseObject("tower", OBJ_PHY_BLOCKS_CAM, 0, "Tower", "wieza.qmsh"),
	BaseObject("gate", OBJ_DOUBLE_PHYSICS | OBJ_PHY_BLOCKS_CAM, 0, "Gate", "brama.qmsh", 0),
	BaseObject("grate", 0, 0, "Grate", "kraty.qmsh"),
	BaseObject("to_remove", 0, 0, "To remove", "snopek.qmsh"),
	BaseObject("boxes2", 0, 0, "Boxes", "pudla.qmsh"),
	BaseObject("barrel_broken", 0, 0, "Broken barrel", "beczka_rozbita.qmsh", 0.38f, 1.083f),
	BaseObject("fern", OBJ_NO_PHYSICS, 0, "Fern", "paproc.qmsh", 0.f, 0.f, 1),
	BaseObject("stocks", 0, OBJ2_MULTI_PHYSICS, "Stocks", "dyby.qmsh"),
	BaseObject("winplant", OBJ_NO_PHYSICS, 0, "Window plants", "krzaki_okno.qmsh", 0.f, 0.f, 1),
	BaseObject("smallroof", OBJ_NO_PHYSICS, OBJ2_CAM_COLLIDERS, "Small roof", "daszek.qmsh", 0.f, 0.f),
	BaseObject("rope", OBJ_NO_PHYSICS, 0, "Rope", "lina.qmsh", 0.f, 0.f),
	BaseObject("wheel", OBJ_NO_PHYSICS, 0, "Wheel", "kolo.qmsh", 0.f, 0.f),
	BaseObject("woodpile", 0, 0, "Wood pile", "woodpile.qmsh"),
	BaseObject("grass2", OBJ_NO_PHYSICS, 0, "Grass", "trawa2.qmsh", 0.f, 0.f, 1),
	BaseObject("corn", OBJ_NO_PHYSICS | OBJ_PRELOAD, 0, "Corn", "zboze.qmsh", 0.f, 0.f, 1),
};
const uint n_objs = countof(g_objs);

//=================================================================================================
BaseObject* BaseObject::TryGet(cstring id, bool* is_variant)
{
	assert(id);

	if(strcmp(id, "painting") == 0)
	{
		if(is_variant)
			*is_variant = true;
		return TryGet(GetRandomPainting());
	}

	if(strcmp(id, "tombstone") == 0)
	{
		if(is_variant)
			*is_variant = true;
		int id = Random(0, 9);
		if(id != 0)
			return TryGet(Format("tombstone_x%d", id));
		else
			return TryGet("tombstone_1");
	}

	if(strcmp(id, "random") == 0)
	{
		switch(Rand() % 3)
		{
		case 0: return TryGet("wheel");
		case 1: return TryGet("rope");
		case 2: return TryGet("woodpile");
		}
	}

	for(uint i = 0; i < n_objs; ++i)
	{
		if(strcmp(g_objs[i].id, id) == 0)
			return &g_objs[i];
	}

	return nullptr;
}

//=================================================================================================
cstring BaseObject::GetRandomPainting()
{
	if(Rand() % 100 == 0)
		return "painting3";
	switch(Rand() % 23)
	{
	case 0:
		return "painting1";
	case 1:
	case 2:
		return "painting2";
	case 3:
	case 4:
		return "painting4";
	case 5:
	case 6:
		return "painting5";
	case 7:
	case 8:
		return "painting6";
	case 9:
		return "painting7";
	case 10:
		return "painting8";
	case 11:
	case 12:
	case 13:
		return "painting_x1";
	case 14:
	case 15:
	case 16:
		return "painting_x2";
	case 17:
	case 18:
	case 19:
		return "painting_x3";
	case 20:
	case 21:
	case 22:
	default:
		return "painting_x4";
	}
}
