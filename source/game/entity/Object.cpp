#include "Pch.h"
#include "Base.h"
#include "Object.h"
#include "SaveState.h"
#include "ResourceManager.h"
#include "BitStreamFunc.h"

#define L_ANG(a,b) b

//-----------------------------------------------------------------------------
// Variants
VariantObj::Entry bench_variants[] = {
	"lawa.qmsh", nullptr,
	"lawa2.qmsh", nullptr,
	"lawa3.qmsh", nullptr,
	"lawa4.qmsh", nullptr
};
VariantObj bench_variant = {bench_variants, countof(bench_variants), false};

//-----------------------------------------------------------------------------
// Nazwa nie jest wyœwietlana
Obj g_objs[] = {
	Obj("barrel", OBJ_NEAR_WALL, 0, L_ANG("Beczka", "Barrel"), "beczka.qmsh", 0.38f, 1.083f),
	Obj("barrels", OBJ_NEAR_WALL, 0, L_ANG("Beczki", "Barrels"), "beczki.qmsh"),
	Obj("big_barrel", OBJ_NEAR_WALL, 0, L_ANG("Du¿a beczka", "Big barrel"), "beczka2.qmsh"),
	Obj("box", OBJ_NEAR_WALL, 0, L_ANG("Skrzynka", "Box"), "skrzynka.qmsh"),
	Obj("boxes", OBJ_NEAR_WALL, 0, L_ANG("Skrzynki", "Boxes"), "skrzynki.qmsh"),
	Obj("table", 0, 0, L_ANG("Stó³", "Table"), "stol.qmsh"),
	Obj("table2", 0, 0, "Stó³ z obiektami", "stol2.qmsh"),
	Obj("wagon", 0, OBJ2_MULTI_PHYSICS, L_ANG("Wóz", "Wagon"), "woz.qmsh"),
	Obj("bow_target", OBJ_NEAR_WALL|OBJ_PHYSICS_PTR, 0, L_ANG("Tarcza strzelnicza", "Archery target"), "tarcza_strzelnicza.qmsh"),
	Obj("torch", OBJ_NEAR_WALL|OBJ_LIGHT|OBJ_IMPORTANT, 0, L_ANG("Pochodnia", "Torch"), "pochodnia.qmsh", 0.27f/4, 2.18f, -1, 2.2f, nullptr, 0.5f),
	Obj("torch_off", OBJ_NEAR_WALL, 0, L_ANG("Pochodnia (zgaszona)", "Torch (not lit)"), "pochodnia.qmsh", 0.27f/4, 2.18f),
	Obj("tanning_rack", OBJ_NEAR_WALL, 0, L_ANG("Stojak do garbowania", "Tanning rack"), "tanning_rack.qmsh", 0),
	Obj("altar", OBJ_NEAR_WALL|OBJ_IMPORTANT|OBJ_REQUIRED, 0, L_ANG("O³tarz", "Altar"), "oltarz.qmsh", 0),
	Obj("bloody_altar", OBJ_NEAR_WALL|OBJ_IMPORTANT|OBJ_BLOOD_EFFECT, 0, "Krwawy o³tarz", "krwawy_oltarz.qmsh", -1, 0.782f),
	Obj("chair", OBJ_USEABLE|OBJ_CHAIR, 0, L_ANG("Krzes³o", "Chair"), "krzeslo.qmsh"),
	Obj("bookshelf", OBJ_NEAR_WALL, 0, L_ANG("Biblioteczka", "Bookshelf"), "biblioteczka.qmsh"),
	Obj("tent", 0, 0, L_ANG("Namiot", "Tent"), "namiot.qmsh"),
	Obj("hay", OBJ_NEAR_WALL, 0, L_ANG("Snopek", "Hay"), "snopek.qmsh"),
	Obj("firewood", OBJ_NEAR_WALL, 0, L_ANG("Drewno na opa³", "Firewood"), "drewno_na_opal.qmsh"),
	Obj("wardrobe", OBJ_NEAR_WALL, 0, L_ANG("Szafa", "Wardrobe"), "szafa.qmsh"),
	Obj("bed", OBJ_NEAR_WALL, 0, L_ANG("£ó¿ko", "Bed"), "lozko.qmsh"),
	Obj("bedding", OBJ_NO_PHYSICS, 0, L_ANG("Pos³anie", "Bedding"), "poslanie.qmsh"),
	Obj("painting1", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz1.qmsh"),
	Obj("painting2", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz2.qmsh"),
	Obj("painting3", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz3.qmsh"),
	Obj("painting4", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz4.qmsh"),
	Obj("painting5", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz5.qmsh"),
	Obj("painting6", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz6.qmsh"),
	Obj("painting7", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz7.qmsh"),
	Obj("painting8", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz8.qmsh"),
	Obj("bench", OBJ_NEAR_WALL|OBJ_USEABLE|OBJ_BENCH, OBJ2_VARIANT, L_ANG("£awa", "Bench"), nullptr, -1, 0.f, &bench_variant),
	Obj("bench_dir", OBJ_NEAR_WALL|OBJ_USEABLE, OBJ2_BENCH_ROT|OBJ2_VARIANT, L_ANG("£awa", "Bench"), nullptr, -1, 0.f, &bench_variant),
	Obj("campfire", OBJ_NO_PHYSICS|OBJ_LIGHT|OBJ_CAMPFIRE, 0, L_ANG("Ognisko", "Campfire"), "ognisko.qmsh", 2.147f/2, 0.43f, -1, 0.4f),
	Obj("campfire_off", OBJ_NO_PHYSICS, 0, L_ANG("Ognisko (zgaszone)", "Burnt campfire"), "ognisko2.qmsh", 2.147f/2, 0.43f),
	Obj("chest", OBJ_CHEST|OBJ_NEAR_WALL, 0, L_ANG("Skrzynia", "Chest"), "skrzynia.qmsh"),
	Obj("chest_r", OBJ_CHEST|OBJ_REQUIRED|OBJ_IMPORTANT|OBJ_IN_MIDDLE, 0, L_ANG("Skrzynia", "Chest"), "skrzynia.qmsh"),
	Obj("melee_target", OBJ_PHYSICS_PTR, 0, L_ANG("Manekin treningowy", "Melee target"), "manekin.qmsh", 0.27f, 1.85f),
	Obj("emblem", OBJ_NO_PHYSICS|OBJ_NEAR_WALL|OBJ_ON_WALL, 0, L_ANG("Emblemat", "Emblem"), "emblemat.qmsh"),
	Obj("emblem_t", OBJ_NO_PHYSICS|OBJ_NEAR_WALL|OBJ_ON_WALL, 0, L_ANG("Emblemat", "Emblem"), "emblemat_t.qmsh"),
	Obj("gobelin", OBJ_NO_PHYSICS|OBJ_NEAR_WALL|OBJ_ON_WALL, 0, "Gobelin", "gobelin.qmsh"),
	Obj("table_and_chairs", OBJ_TABLE, 0, L_ANG("Stó³ z krzes³ami", "Table and chairs"), nullptr, 3.f, 0.6f),
	Obj("tablechairs", OBJ_TABLE, 0, L_ANG("Stó³ z krzes³ami", "Table and chairs"), nullptr, 3.f, 0.6f), // za d³uga nazwa dla blendera :|
	Obj("anvil", OBJ_USEABLE|OBJ_ANVIL, 0, L_ANG("Kowad³o", "Anvil"), "kowadlo.qmsh"),
	Obj("cauldron", OBJ_USEABLE|OBJ_CAULDRON, 0, L_ANG("Kocio³", "Cauldron"), "kociol.qmsh"),
	Obj("grave", OBJ_NEAR_WALL, 0, L_ANG("Grób", "Grave"), "grob.qmsh"),
	Obj("tombstone_1", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek.qmsh"),
	Obj("mushrooms", OBJ_NO_PHYSICS, 0, L_ANG("Grzyby", "Mushrooms"), "grzyby.qmsh"),
	Obj("stalactite", OBJ_NO_PHYSICS, 0, L_ANG("Stalaktyt", "Stalactite"), "stalaktyt.qmsh"),
	Obj("plant2", OBJ_NO_PHYSICS|OBJ_BILLBOARD, 0, L_ANG("Krzak", "Plant"), "krzak2.qmsh", 1),
	Obj("rock", 0, 0, L_ANG("Kamieñ", "Rock"), "kamien.qmsh", 0.456f, 0.67f),
	Obj("tree", OBJ_SCALEABLE, 0, L_ANG("Drzewo", "Tree"), "drzewo.qmsh", 0.043f, 5.f, 1),
	Obj("tree2", OBJ_SCALEABLE, 0, L_ANG("Drzewo", "Tree"), "drzewo2.qmsh", 0.024f, 5.f, 1),
	Obj("tree3", OBJ_SCALEABLE, 0, L_ANG("Drzewo", "Tree"), "drzewo3.qmsh", 0.067f, 5.f, 1),
	Obj("grass", OBJ_NO_PHYSICS, 0, L_ANG("Trawa", "Grass"), "trawa.qmsh", 1),
	Obj("plant", OBJ_NO_PHYSICS, 0, L_ANG("Krzak", "Plant"), "krzak.qmsh", 1),
	Obj("plant_pot", 0, 0, L_ANG("Roœlina w doniczce", "Plant pot"), "doniczka.qmsh", 0.514f/2, 0.1f/2, 1),
	Obj("desk", 0, 0, L_ANG("Biurko", "Desk"), "biurko.qmsh", 1),
	Obj("withered_tree", OBJ_SCALEABLE, 0, L_ANG("Uschniête drzewo", "Withered tree"), "drzewo_uschniete.qmsh", 0.58f, 6.f),
	Obj("tartak", OBJ_BUILDING, 0, "Tartak", "tartak.qmsh", 0.f, 0.f),
	Obj("iron_ore", OBJ_USEABLE|OBJ_IRON_VAIN|OBJ_V0_CONVERSION, 0, "Ruda ¿elaza", "iron_ore.qmsh"),
	Obj("gold_ore", OBJ_USEABLE|OBJ_GOLD_VAIN|OBJ_V0_CONVERSION, 0, "Ruda z³ota", "gold_ore.qmsh"),
	Obj("portal", OBJ_DOUBLE_PHYSICS|OBJ_IMPORTANT|OBJ_REQUIRED|OBJ_IN_MIDDLE, 0, "Portal", "portal.qmsh"),
	Obj("magic_thing", OBJ_IN_MIDDLE|OBJ_LIGHT, 0, "Magiczne coœ", "magiczne_cos.qmsh", 1.122f/2, 0.844f, -1, 0.844f),
	Obj("throne", OBJ_IMPORTANT|OBJ_REQUIRED|OBJ_USEABLE|OBJ_THRONE|OBJ_NEAR_WALL, 0, L_ANG("Tron", "Throne"), "tron.qmsh"),
	Obj("moonwell", OBJ_WATER_EFFECT, 0, "Ksiê¿ycowa studnia", "moonwell.qmsh", 1.031f, 0.755f),
	Obj("moonwell_phy", OBJ_DOUBLE_PHYSICS|OBJ_PHY_ROT, 0, "Fizyka studni", "moonwell_phy.qmsh"),
	Obj("tomashu_dom", OBJ_BUILDING, 0, "Mój domek :3", "tomashu_dom.qmsh"),
	Obj("shelves", OBJ_NEAR_WALL, 0, "Pó³ki", "polki.qmsh"),
	Obj("stool", OBJ_USEABLE|OBJ_STOOL, 0, "Sto³ek", "stolek.qmsh", 0.27f, 0.44f),
	Obj("bania", 0, 0, "Bania", "bania.qmsh", 1.f, 1.f),
	Obj("painting_x1", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz_x1.qmsh"),
	Obj("painting_x2", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz_x2.qmsh"),
	Obj("painting_x3", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz_x3.qmsh"),
	Obj("painting_x4", OBJ_NEAR_WALL|OBJ_NO_PHYSICS|OBJ_HIGH|OBJ_ON_WALL, 0, L_ANG("Obraz", "Painting"), "obraz_x4.qmsh"),
	Obj("coveredwell", 0, 0, "Studnia", "coveredwell.qmsh", 1.237f, 2.94f),
	Obj("obelisk", 0, 0, "Obelisk", "obelisk.qmsh"),
	Obj("tombstone_x1", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x1.qmsh"),
	Obj("tombstone_x2", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x2.qmsh"),
	Obj("tombstone_x3", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x3.qmsh"),
	Obj("tombstone_x4", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x4.qmsh"),
	Obj("tombstone_x5", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x5.qmsh"),
	Obj("tombstone_x6", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x6.qmsh"),
	Obj("tombstone_x7", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x7.qmsh"),
	Obj("tombstone_x8", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x8.qmsh"),
	Obj("tombstone_x9", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek_x9.qmsh"),
	Obj("wall", OBJ_PHY_BLOCKS_CAM, 0, "Mur", "mur.qmsh", 0),
	Obj("tower", OBJ_PHY_BLOCKS_CAM, 0, "Wie¿a", "wieza.qmsh"),
	Obj("gate", OBJ_DOUBLE_PHYSICS|OBJ_PHY_BLOCKS_CAM, 0, "Brama", "brama.qmsh", 0),
	Obj("grate", 0, 0, "Kraty", "kraty.qmsh"),
	Obj("to_remove", 0, 0, "Do usuniêcia", "snopek.qmsh"),
	Obj("boxes2", 0, 0, "Pud³a", "pudla.qmsh"),
	Obj("barrel_broken", 0, 0, "Beczka rozbita", "beczka_rozbita.qmsh", 0.38f, 1.083f),
	Obj("fern", OBJ_NO_PHYSICS, 0, "Paproæ", "paproc.qmsh", 0.f, 0.f, 1),
	Obj("stocks", 0, OBJ2_MULTI_PHYSICS, "Dyby", "dyby.qmsh"),
	Obj("winplant", OBJ_NO_PHYSICS, 0, "Krzaki przed oknem", "krzaki_okno.qmsh", 0.f, 0.f, 1),
	Obj("smallroof", OBJ_NO_PHYSICS, OBJ2_CAM_COLLIDERS, "Daszek", "daszek.qmsh", 0.f, 0.f),
	Obj("rope", OBJ_NO_PHYSICS, 0, "Lina", "lina.qmsh", 0.f, 0.f),
	Obj("wheel", OBJ_NO_PHYSICS, 0, "Ko³o", "kolo.qmsh", 0.f, 0.f),
	Obj("woodpile", 0, 0, "Drewno", "woodpile.qmsh"),
	Obj("grass2", OBJ_NO_PHYSICS, 0, "Trawa", "trawa2.qmsh", 0.f, 0.f, 1),
	Obj("corn", OBJ_NO_PHYSICS, 0, "Zbo¿e", "zboze.qmsh", 0.f, 0.f, 1),
};
const uint n_objs = countof(g_objs);

//=================================================================================================
void Object::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, nullptr);
	WriteFile(file, &rot, sizeof(rot), &tmp, nullptr);
	WriteFile(file, &scale, sizeof(scale), &tmp, nullptr);

	if(base)
	{
		byte len = (byte)strlen(base->id);
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		WriteFile(file, base->id, len, &tmp, nullptr);
	}
	else
	{
		byte len = 0;
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		len = (byte)strlen(mesh->res->filename);
		WriteFile(file, &len, sizeof(len), &tmp, nullptr);
		WriteFile(file, mesh->res->filename, len, &tmp, nullptr);
	}
}

//=================================================================================================
bool Object::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, nullptr);
	ReadFile(file, &rot, sizeof(rot), &tmp, nullptr);
	ReadFile(file, &scale, sizeof(scale), &tmp, nullptr);

	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, nullptr);

	if(len)
	{
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
		if(LOAD_VERSION >= V_0_2_20)
			base = FindObject(BUF);
		else
		{
			if(strcmp(BUF, "tombstone") == 0)
				base = FindObject("tombstone_1");
			else
				base = FindObject(BUF);
		}
		mesh = base->mesh;
		if(LOAD_VERSION == V_0_2 && IS_SET(base->flags, OBJ_V0_CONVERSION))
			return false;
	}
	else
	{
		base = nullptr;
		ReadFile(file, &len, sizeof(len), &tmp, nullptr);
		ReadFile(file, BUF, len, &tmp, nullptr);
		BUF[len] = 0;
		if(LOAD_VERSION >= V_0_3)
			mesh = ResourceManager::Get().GetLoadedMesh(BUF)->data;
		else
		{
			if(strcmp(BUF, "mur.qmsh") == 0 || strcmp(BUF, "mur2.qmsh") == 0 || strcmp(BUF, "brama.qmsh") == 0)
			{
				base = FindObject("to_remove");
				mesh = base->mesh;
			}
			else
				mesh = ResourceManager::Get().GetLoadedMesh(BUF)->data;
		}
	}

	return true;
}

//=================================================================================================
void Object::Write(BitStream& stream) const
{
	stream.Write(pos);
	stream.Write(rot);
	stream.Write(scale);
	if(base)
		WriteString1(stream, base->id);
	else
	{
		stream.Write<byte>(0);
		WriteString1(stream, mesh->res->filename);
	}
}

//=================================================================================================
bool Object::Read(BitStream& stream)
{
	if(!stream.Read(pos)
		|| !stream.Read(rot)
		|| !stream.Read(scale)
		|| !ReadString1(stream))
		return false;
	if(BUF[0])
	{
		// use base obj
		base = FindObject(BUF);
		if(!base)
		{
			ERROR(Format("Missing base object '%stream'!", BUF));
			return false;
		}
		mesh = base->mesh;
	}
	else
	{
		// use mesh
		if(!ReadString1(stream))
			return false;
		mesh = ResourceManager::Get().GetLoadedMesh(BUF)->data;
		base = nullptr;
	}
	return true;
}
