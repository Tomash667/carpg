#include "Pch.h"
#include "Game.h"
#include "SaveState.h"

#define L_ANG(a,b) b

//-----------------------------------------------------------------------------
// Variants
VariantObj::Entry bench_variants[] = {
	"lawa.qmsh", NULL,
	"lawa2.qmsh", NULL,
	"lawa3.qmsh", NULL,
	"lawa4.qmsh", NULL
};
VariantObj bench_variant = {bench_variants, countof(bench_variants), false};

//-----------------------------------------------------------------------------
// Nazwa nie jest wyœwietlana
Obj g_objs[] = {
	Obj("barrel", OBJ_PRZY_SCIANIE, 0, L_ANG("Beczka", "Barrel"), "beczka.qmsh", 0.38f, 1.083f),
	Obj("barrels", OBJ_PRZY_SCIANIE, 0, L_ANG("Beczki", "Barrels"), "beczki.qmsh"),
	Obj("big_barrel", OBJ_PRZY_SCIANIE, 0, L_ANG("Du¿a beczka", "Big barrel"), "beczka2.qmsh"),
	Obj("box", OBJ_PRZY_SCIANIE, 0, L_ANG("Skrzynka", "Box"), "skrzynka.qmsh"),
	Obj("boxes", OBJ_PRZY_SCIANIE, 0, L_ANG("Skrzynki", "Boxes"), "skrzynki.qmsh"),
	Obj("table", 0, 0, L_ANG("Stó³", "Table"), "stol.qmsh"),
	Obj("table2", 0, 0, "Stó³ z obiektami", "stol2.qmsh"),
	Obj("wagon", 0, OBJ2_WIELOFIZYKA, L_ANG("Wóz", "Wagon"), "woz.qmsh"),
	Obj("bow_target", OBJ_PRZY_SCIANIE|OBJ_WSKAZNIK_NA_FIZYKE, 0, L_ANG("Tarcza strzelnicza", "Archery target"), "tarcza_strzelnicza.qmsh"),
	Obj("torch", OBJ_PRZY_SCIANIE|OBJ_SWIATLO|OBJ_WAZNE, 0, L_ANG("Pochodnia", "Torch"), "pochodnia.qmsh", 0.27f/4, 2.18f, -1, 2.2f, NULL, 0.5f),
	Obj("torch_off", OBJ_PRZY_SCIANIE, 0, L_ANG("Pochodnia (zgaszona)", "Torch (not lit)"), "pochodnia.qmsh", 0.27f/4, 2.18f),
	Obj("tanning_rack", OBJ_PRZY_SCIANIE, 0, L_ANG("Stojak do garbowania", "Tanning rack"), "tanning_rack.qmsh", 0),
	Obj("altar", OBJ_PRZY_SCIANIE|OBJ_WAZNE|OBJ_WYMAGANE, 0, L_ANG("O³tarz", "Altar"), "oltarz.qmsh", 0),
	Obj("bloody_altar", OBJ_PRZY_SCIANIE|OBJ_WAZNE|OBJ_KREW, 0, "Krwawy o³tarz", "krwawy_oltarz.qmsh", -1, 0.782f),
	Obj("chair", OBJ_UZYWALNY|OBJ_KRZESLO, 0, L_ANG("Krzes³o", "Chair"), "krzeslo.qmsh"),
	Obj("bookshelf", OBJ_PRZY_SCIANIE, 0, L_ANG("Biblioteczka", "Bookshelf"), "biblioteczka.qmsh"),
	Obj("tent", 0, 0, L_ANG("Namiot", "Tent"), "namiot.qmsh"),
	Obj("hay", OBJ_PRZY_SCIANIE, 0, L_ANG("Snopek", "Hay"), "snopek.qmsh"),
	Obj("firewood", OBJ_PRZY_SCIANIE, 0, L_ANG("Drewno na opa³", "Firewood"), "drewno_na_opal.qmsh"),
	Obj("wardrobe", OBJ_PRZY_SCIANIE, 0, L_ANG("Szafa", "Wardrobe"), "szafa.qmsh"),
	Obj("bed", OBJ_PRZY_SCIANIE, 0, L_ANG("£ó¿ko", "Bed"), "lozko.qmsh"),
	Obj("bedding", OBJ_BRAK_FIZYKI, 0, L_ANG("Pos³anie", "Bedding"), "poslanie.qmsh"),
	Obj("painting1", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz1.qmsh"),
	Obj("painting2", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz2.qmsh"),
	Obj("painting3", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz3.qmsh"),
	Obj("painting4", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz4.qmsh"),
	Obj("painting5", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz5.qmsh"),
	Obj("painting6", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz6.qmsh"),
	Obj("painting7", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz7.qmsh"),
	Obj("painting8", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz8.qmsh"),
	Obj("bench", OBJ_PRZY_SCIANIE|OBJ_UZYWALNY|OBJ_LAWA, OBJ2_VARIANT, L_ANG("£awa", "Bench"), NULL, -1, 0.f, &bench_variant),
	Obj("bench_dir", OBJ_PRZY_SCIANIE|OBJ_UZYWALNY, OBJ2_LAWA_DIR|OBJ2_VARIANT, L_ANG("£awa", "Bench"), NULL, -1, 0.f, &bench_variant),
	Obj("campfire", OBJ_BRAK_FIZYKI|OBJ_SWIATLO|OBJ_OGNISKO, 0, L_ANG("Ognisko", "Campfire"), "ognisko.qmsh", 2.147f/2, 0.43f, -1, 0.4f),
	Obj("campfire_off", OBJ_BRAK_FIZYKI, 0, L_ANG("Ognisko (zgaszone)", "Burnt campfire"), "ognisko2.qmsh", 2.147f/2, 0.43f),
	Obj("chest", OBJ_SKRZYNIA|OBJ_PRZY_SCIANIE, 0, L_ANG("Skrzynia", "Chest"), "skrzynia.qmsh"),
	Obj("chest_r", OBJ_SKRZYNIA|OBJ_WYMAGANE|OBJ_WAZNE|OBJ_NA_SRODKU, 0, L_ANG("Skrzynia", "Chest"), "skrzynia.qmsh"),
	Obj("melee_target", OBJ_WSKAZNIK_NA_FIZYKE, 0, L_ANG("Manekin treningowy", "Melee target"), "manekin.qmsh", 0.27f, 1.85f),
	Obj("emblem", OBJ_BRAK_FIZYKI|OBJ_PRZY_SCIANIE|OBJ_NA_SCIANIE, 0, L_ANG("Emblemat", "Emblem"), "emblemat.qmsh"),
	Obj("emblem_t", OBJ_BRAK_FIZYKI|OBJ_PRZY_SCIANIE|OBJ_NA_SCIANIE|OBJ_OPCJONALNY, 0, L_ANG("Emblemat", "Emblem"), "emblemat_t.qmsh"),
	Obj("gobelin", OBJ_BRAK_FIZYKI|OBJ_PRZY_SCIANIE|OBJ_NA_SCIANIE|OBJ_BRAK_PRZESUNIECIA, 0, "Gobelin", "gobelin.qmsh"),
	Obj("table_and_chairs", OBJ_STOL, 0, L_ANG("Stó³ z krzes³ami", "Table and chairs"), NULL, 3.f, 0.6f),
	Obj("tablechairs", OBJ_STOL, 0, L_ANG("Stó³ z krzes³ami", "Table and chairs"), NULL, 3.f, 0.6f), // za d³uga nazwa dla blendera :|
	Obj("anvil", OBJ_UZYWALNY|OBJ_KOWADLO, 0, L_ANG("Kowad³o", "Anvil"), "kowadlo.qmsh"),
	Obj("cauldron", OBJ_UZYWALNY|OBJ_KOCIOLEK, 0, L_ANG("Kocio³", "Cauldron"), "kociol.qmsh"),
	Obj("grave", OBJ_PRZY_SCIANIE, 0, L_ANG("Grób", "Grave"), "grob.qmsh"),
	Obj("tombstone_1", 0, 0, L_ANG("Nagrobek", "Tombstone"), "nagrobek.qmsh"),
	Obj("mushrooms", OBJ_BRAK_FIZYKI, 0, L_ANG("Grzyby", "Mushrooms"), "grzyby.qmsh"),
	Obj("stalactite", OBJ_BRAK_FIZYKI, 0, L_ANG("Stalaktyt", "Stalactite"), "stalaktyt.qmsh"),
	Obj("plant2", OBJ_BRAK_FIZYKI|OBJ_BILLBOARD, 0, L_ANG("Krzak", "Plant"), "krzak2.qmsh", 1),
	Obj("rock", 0, 0, L_ANG("Kamieñ", "Rock"), "kamien.qmsh", 0.456f, 0.67f),
	Obj("tree", OBJ_SKALOWALNY, 0, L_ANG("Drzewo", "Tree"), "drzewo.qmsh", 0.043f, 5.f, 1),
	Obj("tree2", OBJ_SKALOWALNY, 0, L_ANG("Drzewo", "Tree"), "drzewo2.qmsh", 0.024f, 5.f, 1),
	Obj("tree3", OBJ_SKALOWALNY, 0, L_ANG("Drzewo", "Tree"), "drzewo3.qmsh", 0.067f, 5.f, 1),
	Obj("grass", OBJ_BRAK_FIZYKI, 0, L_ANG("Trawa", "Grass"), "trawa.qmsh", 1),
	Obj("plant", OBJ_BRAK_FIZYKI, 0, L_ANG("Krzak", "Plant"), "krzak.qmsh", 1),
	Obj("plant_pot", 0, 0, L_ANG("Roœlina w doniczce", "Plant pot"), "doniczka.qmsh", 0.514f/2, 0.1f/2, 1),
	Obj("desk", 0, 0, L_ANG("Biurko", "Desk"), "biurko.qmsh", 1),
	Obj("withered_tree", OBJ_SKALOWALNY, 0, L_ANG("Uschniête drzewo", "Withered tree"), "drzewo_uschniete.qmsh", 0.58f, 6.f),
	Obj("tartak", OBJ_BUDYNEK, 0, "Tartak", "tartak.qmsh", 0.f, 0.f),
	Obj("iron_ore", OBJ_UZYWALNY|OBJ_ZYLA_ZELAZA|OBJ_KONWERSJA_V0, 0, "Ruda ¿elaza", "iron_ore.qmsh"),
	Obj("gold_ore", OBJ_UZYWALNY|OBJ_ZYLA_ZLOTA|OBJ_KONWERSJA_V0, 0, "Ruda z³ota", "gold_ore.qmsh"),
	Obj("portal", OBJ_PODWOJNA_FIZYKA|OBJ_WAZNE|OBJ_WYMAGANE|OBJ_NA_SRODKU, 0, "Portal", "portal.qmsh"),
	Obj("magic_thing", OBJ_NA_SRODKU|OBJ_SWIATLO, 0, "Magiczne coœ", "magiczne_cos.qmsh", 1.122f/2, 0.844f, -1, 0.844f),
	Obj("throne", OBJ_WAZNE|OBJ_WYMAGANE|OBJ_UZYWALNY|OBJ_TRON|OBJ_PRZY_SCIANIE, 0, L_ANG("Tron", "Throne"), "tron.qmsh"),
	Obj("moonwell", OBJ_WODA, 0, "Ksiê¿ycowa studnia", "moonwell.qmsh", 1.031f, 0.755f),
	Obj("moonwell_phy", OBJ_PODWOJNA_FIZYKA|OBJ_OBROT, 0, "Fizyka studni", "moonwell_phy.qmsh"),
	Obj("tomashu_dom", OBJ_BUDYNEK|OBJ_OPCJONALNY, 0, "Mój domek :3", "tomashu_dom.qmsh"),
	Obj("shelves", OBJ_PRZY_SCIANIE, 0, "Pó³ki", "polki.qmsh"),
	Obj("stool", OBJ_UZYWALNY|OBJ_STOLEK, 0, "Sto³ek", "stolek.qmsh", 0.27f, 0.44f),
	Obj("bania", 0, 0, "Bania", "bania.qmsh", 1.f, 1.f),
	Obj("painting_x1", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz_x1.qmsh"),
	Obj("painting_x2", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz_x2.qmsh"),
	Obj("painting_x3", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz_x3.qmsh"),
	Obj("painting_x4", OBJ_PRZY_SCIANIE|OBJ_BRAK_FIZYKI|OBJ_WYSOKO|OBJ_NA_SCIANIE, 0, L_ANG("Obraz", "Painting"), "obraz_x4.qmsh"),
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
	Obj("wall", OBJ_BLOKUJE_WIDOK, 0, "Mur", "mur.qmsh", 0),
	Obj("tower", OBJ_BLOKUJE_WIDOK, 0, "Wie¿a", "wieza.qmsh"),
	Obj("gate", OBJ_PODWOJNA_FIZYKA|OBJ_BLOKUJE_WIDOK, 0, "Brama", "brama.qmsh", 0),
	Obj("grate", 0, 0, "Kraty", "kraty.qmsh"),
	Obj("to_remove", 0, 0, "Do usuniêcia", "snopek.qmsh"),
	Obj("boxes2", 0, 0, "Pud³a", "pudla.qmsh"),
	Obj("barrel_broken", 0, 0, "Beczka rozbita", "beczka_rozbita.qmsh", 0.38f, 1.083f),
	Obj("fern", OBJ_BRAK_FIZYKI, 0, "Paproæ", "paproc.qmsh", 0.f, 0.f, 1),
	Obj("stocks", 0, OBJ2_WIELOFIZYKA, "Dyby", "dyby.qmsh"),
	Obj("winplant", OBJ_BRAK_FIZYKI, 0, "Krzaki przed oknem", "krzaki_okno.qmsh", 0.f, 0.f, 1),
	Obj("smallroof", OBJ_BRAK_FIZYKI, OBJ2_CAM_COLLIDERS, "Daszek", "daszek.qmsh", 0.f, 0.f),
	Obj("rope", OBJ_BRAK_FIZYKI, 0, "Lina", "lina.qmsh", 0.f, 0.f),
	Obj("wheel", OBJ_BRAK_FIZYKI, 0, "Ko³o", "kolo.qmsh", 0.f, 0.f),
	Obj("woodpile", 0, 0, "Drewno", "woodpile.qmsh"),
	Obj("grass2", OBJ_BRAK_FIZYKI, 0, "Trawa", "trawa2.qmsh", 0.f, 0.f, 1),
	Obj("corn", OBJ_BRAK_FIZYKI, 0, "Zbo¿e", "zboze.qmsh", 0.f, 0.f, 1),
};
const uint n_objs = countof(g_objs);

//=================================================================================================
void Object::Save(HANDLE file)
{
	WriteFile(file, &pos, sizeof(pos), &tmp, NULL);
	WriteFile(file, &rot, sizeof(rot), &tmp, NULL);
	WriteFile(file, &scale, sizeof(scale), &tmp, NULL);

	if(base)
	{
		byte len = (byte)strlen(base->id);
		WriteFile(file, &len, sizeof(len), &tmp, NULL);
		WriteFile(file, base->id, len, &tmp, NULL);
	}
	else
	{
		byte len = 0;
		WriteFile(file, &len, sizeof(len), &tmp, NULL);
		len = (byte)mesh->res->filename.length();
		WriteFile(file, &len, sizeof(len), &tmp, NULL);
		WriteFile(file, mesh->res->filename.c_str(), len, &tmp, NULL);
	}
}

//=================================================================================================
bool Object::Load(HANDLE file)
{
	ReadFile(file, &pos, sizeof(pos), &tmp, NULL);
	ReadFile(file, &rot, sizeof(rot), &tmp, NULL);
	ReadFile(file, &scale, sizeof(scale), &tmp, NULL);

	byte len;
	ReadFile(file, &len, sizeof(len), &tmp, NULL);

	if(len)
	{
		ReadFile(file, BUF, len, &tmp, NULL);
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
		mesh = base->ani;
		if(LOAD_VERSION == V_0_2 && IS_SET(base->flagi, OBJ_KONWERSJA_V0))
			return false;
	}
	else
	{
		base = NULL;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		ReadFile(file, BUF, len, &tmp, NULL);
		BUF[len] = 0;
		if(LOAD_VERSION >= V_0_3)
			mesh = Game::Get().LoadMesh(BUF);
		else
		{
			if(strcmp(BUF, "mur.qmsh") == 0 || strcmp(BUF, "mur2.qmsh") == 0 || strcmp(BUF, "brama.qmsh") == 0)
			{
				base = FindObject("to_remove");
				mesh = base->ani;
			}
			else
				mesh = Game::Get().LoadMesh(BUF);
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
		mesh = base->ani;
	}
	else
	{
		// use mesh
		if(!ReadString1(stream))
			return false;
		mesh = Game::Get().LoadMesh(BUF);
		base = NULL;
	}
	return true;
}
