// bazowe typy podziemi i krypt
#include "Pch.h"
#include "GameCore.h"
#include "BaseLocation.h"
#include "ResourceManager.h"

//-----------------------------------------------------------------------------
RoomStrChance fort_ludzi_pokoje[] = {
	RoomStrChance("sypialnia", 10),
	RoomStrChance("pokoj_dowodcy", 2),
	RoomStrChance("biblioteczka", 5),
	RoomStrChance("magazyn", 10),
	RoomStrChance("skrytka", 2),
	RoomStrChance("sklad_piwa", 5),
	RoomStrChance("pokoj_spotkan", 10),
	RoomStrChance("pokoj_treningowy", 5),
	RoomStrChance("pokoj_wyrobu", 5),
	RoomStrChance("kuchnia", 5)
	//RoomStrChance("obrazy", 1)
};

//-----------------------------------------------------------------------------
RoomStrChance wieza_magow_pokoje[] = {
	RoomStrChance("magiczny", 3),
	RoomStrChance("sypialnia", 10),
	RoomStrChance("pokoj_dowodcy", 2),
	RoomStrChance("biblioteczka", 6),
	RoomStrChance("magazyn", 10),
	RoomStrChance("skrytka", 1),
	RoomStrChance("sklad_piwa", 3),
	RoomStrChance("pokoj_spotkan", 8),
	RoomStrChance("pokoj_treningowy", 4),
	RoomStrChance("pokoj_wyrobu", 4),
	RoomStrChance("kuchnia", 4)
};

//-----------------------------------------------------------------------------
RoomStrChance baza_nekro_pokoje[] = {
	RoomStrChance("sypialnia", 10),
	RoomStrChance("pokoj_dowodcy", 2),
	RoomStrChance("biblioteczka", 5),
	RoomStrChance("magazyn", 10),
	RoomStrChance("skrytka", 2),
	RoomStrChance("sklad_piwa", 5),
	RoomStrChance("kapliczka", 5),
	RoomStrChance("pokoj_spotkan", 10),
	RoomStrChance("pokoj_treningowy", 5),
	RoomStrChance("pokoj_wyrobu", 5),
	RoomStrChance("kuchnia", 5)
};

//-----------------------------------------------------------------------------
RoomStrChance skrytka_pokoje[] = {
	RoomStrChance("sypialnia", 5),
	RoomStrChance("biblioteczka", 2),
	RoomStrChance("magazyn", 10),
	RoomStrChance("skrytka", 5),
	RoomStrChance("sklad_piwa", 5),
	RoomStrChance("pokoj_spotkan", 5),
	RoomStrChance("pokoj_treningowy", 2),
	RoomStrChance("pokoj_wyrobu", 2),
	RoomStrChance("kuchnia", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance krypta_pokoje[] = {
	RoomStrChance("groby", 8),
	RoomStrChance("groby2", 8),
	RoomStrChance("kapliczka", 4),
	RoomStrChance("pokoj_krypta", 2)
};

//-----------------------------------------------------------------------------
RoomStrChance labirynt_pokoje[] = {
	RoomStrChance("labirynt_skarb", 1)
};

//-----------------------------------------------------------------------------
RoomStrChance tut_pokoje[] = {
	RoomStrChance("tut", 1)
};

//-----------------------------------------------------------------------------
// nazwa jest wyœwietlana tylko w trybie DEBUG
BaseLocation g_base_locations[] = {
	//	  NAZWA										POZIOMY			ROZMIAR		PO£¥CZ		KORYTARZ	ROZMIAR		ROZMIAR		INNE	FLOOR/WALL/CEIL
	//																MAPY		KOR	POK					KORYTARZA	POKOJU
	"Fort ludzi", /*"Human fort"*/					Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		fort_ludzi_pokoje, countof(fort_ludzi_pokoje), 0, 50, 25, 2, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, 0, -1,
		LocationTexturePack(),
	"Fort krasnoludów", /*"Dwarf fort"*/			Int2(3, 5),		40, 3,		50, 5,		25,			Int2(5,12),	Int2(4,8),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		fort_ludzi_pokoje, countof(fort_ludzi_pokoje), 0, 60, 20, 6, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile.jpg", "mur078.jpg", "sufit3.jpg"),
	"Wie¿a magów", /*"Mage tower"*/					Int2(4, 6),		30, 0,		0,	33,		0,			Int2(0,0),	Int2(4,7),	BLO_MAGIC_LIGHT | BLO_ROUND, RoomStr("schody"), RoomStr(nullptr),
		Vec3(100.f / 255,0,0), Vec3(-1.f / 255,0,0), Vec3(0.4f,0.3f,0.3f), Vec3(-0.03f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		wieza_magow_pokoje, countof(wieza_magow_pokoje), 0, 100, 0, 3, SG_MAGOWIE, SG_MAGOWIE_I_GOLEMY, SG_LOSOWO, 50, 25, 0, TRAPS_MAGIC, -1,
		LocationTexturePack("floor_pavingStone_ceramic.jpg", "stone01d.jpg", "block02b.jpg"),
	"Kryjówka bandytów", /*"Bandits hideout"*/		Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		fort_ludzi_pokoje, countof(fort_ludzi_pokoje), 0, 80, 10, 3, SG_BANDYCI, SG_LOSOWO, SG_LOSOWO, 75, 0, 0, TRAPS_NORMAL | TRAPS_NEAR_ENTRANCE, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "sup075.jpg"),
	"Krypta bohatera", /*"Hero crypt"*/				Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody"), RoomStr(nullptr),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		krypta_pokoje, countof(krypta_pokoje), 0, 80, 5, 1, SG_NIEUMARLI, SG_NEKRO, SG_LOSOWO, 50, 25, 0, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Krypta potwora", /*"Monster crypt"*/			Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(5,10),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody"), RoomStr(nullptr),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		krypta_pokoje, countof(krypta_pokoje), 0, 80, 5, 1, SG_NIEUMARLI, SG_NEKRO, SG_LOSOWO, 50, 25, 0, TRAPS_NORMAL | TRAPS_NEAR_END, CRYPT_2_TEXTURE,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01a.jpg", "sufit2.jpg"),
	"Stara œwi¹tynia", /*"Old temple"*/				Int2(1, 3),		40, 2,		35, 15,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody_swiatynia"), RoomStr("kapliczka"),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		baza_nekro_pokoje, countof(baza_nekro_pokoje), 0, 80, 5, 1, SG_NIEUMARLI, SG_NEKRO, SG_ZLO, 25, 25, 25, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Skrytka", /*"Safehouse"*/						Int2(1, 1),		30, 0,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		skrytka_pokoje, countof(skrytka_pokoje), 0, 100, 0, 3, SG_BANDYCI, SG_BRAK, SG_LOSOWO, 25, 25, 0, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Baza nekromantów", /*"Necromancers base"*/		Int2(2, 3),		45, 3,		35,	15,		25,			Int2(5,10),	Int2(5,10), BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody_swiatynia"), RoomStr(nullptr),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		baza_nekro_pokoje, countof(baza_nekro_pokoje), 0, 80, 5, 1, SG_NEKRO, SG_ZLO, SG_LOSOWO, 50, 25, 0, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_paving_littleStones3.jpg", "256-03b.jpg", "sufit2.jpg"),
	"Labirynt", /*"Labirynth"*/						Int2(1, 1),		60, 0,		0, 0,		0,			Int2(0,0),	Int2(6,6),	BLO_LABIRYNTH, RoomStr(nullptr), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.33f,0.33f,0.33f), Vec3(0,0,0), Vec2(3.f,15.f), Vec2(0,0), 15.f, 0,
		labirynt_pokoje, countof(labirynt_pokoje), 0, 0, 0, 3, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, 0, -1,
		LocationTexturePack("block01b.jpg", "stone01b.jpg", "block01d.jpg"),
	"Jaskinia", /*"Cave"*/							Int2(0,0),		52, 0,		0, 0,		0,			Int2(0,0),	Int2(0,0),	BLO_LABIRYNTH,	RoomStr(nullptr), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.4f,0.4f,0.4f), Vec3(0,0,0), Vec2(16.f,25.f), Vec2(0,0), 25.f, 0,
		nullptr, 0, 0, 0, 0, 0, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, 0, KOPALNIA_POZIOM,
		LocationTexturePack("rock2.jpg", "rock1.jpg", "rock3.jpg"),
	"Staro¿ytna zbrojownia", /*"Ancient armory"*/	Int2(1,1),		45, 0,		35, 0,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody_swiatynia"), RoomStr(nullptr),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		fort_ludzi_pokoje, countof(fort_ludzi_pokoje), 0, 80, 5, 1, SG_GOLEMY, SG_BRAK, SG_BRAK, 100, 0, 0, TRAPS_MAGIC | TRAPS_NORMAL, -1,
		LocationTexturePack("floor_tile_ceramicBlue.jpg", "block10c.jpg", "woodmgrid1a.jpg"),
	"Samouczek",									Int2(1,1),		22, 0,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		tut_pokoje, countof(tut_pokoje), 0, 100, 0, 0, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, 0, -1,
		LocationTexturePack(),
	"Fort z tronem",								Int2(2, 3),		40, 2,		40, 20,		30,			Int2(3,12),	Int2(5,10),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		fort_ludzi_pokoje, countof(fort_ludzi_pokoje), 0, 50, 25, 2, SG_LOSOWO, SG_LOSOWO, SG_LOSOWO, 0, 0, 0, 0, -1,
		LocationTexturePack(),
	"Skrytka z tronem",								Int2(1, 1),		35, 0,		50,	5,		35,			Int2(5,12),	Int2(4,7),	0, RoomStr("schody"), RoomStr(nullptr),
		Vec3(0,0,0), Vec3(0,0,0), Vec3(0.3f,0.3f,0.3f), Vec3(-0.025f,-0.025f,-0.025f), Vec2(10,20), Vec2(-0.5f,-1.f), 20.f, -1.f,
		skrytka_pokoje, countof(skrytka_pokoje), 0, 100, 0, 3, SG_BANDYCI, SG_BRAK, SG_LOSOWO, 25, 25, 0, TRAPS_NORMAL, -1,
		LocationTexturePack("mad015.jpg", "mad063.jpg", "mad013.jpg"),
	"Krypta 2-tekstura",							Int2(2, 3),		35, 5,		30, 10,		25,			Int2(5,10),	Int2(4,8),	BLO_MAGIC_LIGHT | BLO_LESS_FOOD, RoomStr("schody"), RoomStr(nullptr),
		Vec3(40.f / 255,40.f / 255,40.f / 255), Vec3(5.f / 255,5.f / 255,5.f / 255), Vec3(0.25f,0.25f,0.25f), Vec3(-0.04f,-0.04f,-0.04f), Vec2(5.f,18.f), Vec2(-0.5f,-0.5f), 18.f, -0.5f,
		krypta_pokoje, countof(krypta_pokoje), 0, 80, 5, 1, SG_NIEUMARLI, SG_NEKRO, SG_LOSOWO, 50, 25, 0, TRAPS_NORMAL | TRAPS_NEAR_END, -1,
		LocationTexturePack("floor_pavement_stone5_2.jpg", "256-01b.jpg", "sufit2.jpg")
};
const uint n_base_locations = countof(g_base_locations);

//=================================================================================================
RoomType* BaseLocation::GetRandomRoomType() const
{
	assert(rooms);

	int total = 0, co = Rand() % room_total;

	for(uint i = 0; i < room_count; ++i)
	{
		total += rooms[i].chance;
		if(co < total)
			return rooms[i].room;
	}

	assert(0);
	return rooms[0].room;
}

//=================================================================================================
void BaseLocation::PreloadTextures()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	for(uint i=0; i<n_base_locations; ++i)
	{
		auto& bl = g_base_locations[i];
		if(bl.tex.floor.id)
		{
			bl.tex.floor.tex = tex_mgr.Get(bl.tex.floor.id);
			if(bl.tex.floor.id_normal)
				bl.tex.floor.tex_normal = tex_mgr.Get(bl.tex.floor.id_normal);
			if(bl.tex.floor.id_specular)
				bl.tex.floor.tex_specular = tex_mgr.Get(bl.tex.floor.id_specular);
		}
		if(bl.tex.wall.id)
		{
			bl.tex.wall.tex = tex_mgr.Get(bl.tex.wall.id);
			if(bl.tex.wall.id_normal)
				bl.tex.wall.tex_normal = tex_mgr.Get(bl.tex.wall.id_normal);
			if(bl.tex.wall.id_specular)
				bl.tex.wall.tex_specular = tex_mgr.Get(bl.tex.wall.id_specular);
		}
		if(bl.tex.ceil.id)
		{
			bl.tex.ceil.tex = tex_mgr.Get(bl.tex.ceil.id);
			if(bl.tex.ceil.id_normal)
				bl.tex.ceil.tex_normal = tex_mgr.Get(bl.tex.ceil.id_normal);
			if(bl.tex.ceil.id_specular)
				bl.tex.ceil.tex_specular = tex_mgr.Get(bl.tex.ceil.id_specular);
		}
	}
}
