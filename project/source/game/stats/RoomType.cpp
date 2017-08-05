// rodzaje pomieszczeñ jakie mo¿na znaleŸæ w podziemiach
#include "Pch.h"
#include "Core.h"
#include "RoomType.h"

//-----------------------------------------------------------------------------
ObjEntry objs_sypialnia[] = {
	"torch", Int2(1,1),
	"bed", Int2(2,3),
	"wardrobe", Int2(2,3),
	"chest", Int2(1,2),
	"shelves", Int2(1,2),
	"table_and_chairs", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_dowodcy[] = {
	"torch", Int2(1,1),
	"bed", Int2(1,1),
	"chest", Int2(1,3),
	"bookshelf", Int2(1,2),
	"painting", Int2(0,3),
	"wardrobe", Int2(1,2),
	"gobelin", Int2(0,1),
	"shelves", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_tron[] = {
	"torch", Int2(1,1),
	"throne", Int2(1,1),
	"chest", Int2(2,3),
	"bookshelf", Int2(1,2),
	"painting", Int2(0,3),
	"wardrobe", Int2(1,2),
	"gobelin", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_biblioteczka[] = {
	"torch", Int2(1,1),
	"wardrobe", Int2(1,2),
	"bookshelf", Int2(3,5),
	"table_and_chairs", Int2(1,2),
	"gobelin", Int2(0,1),
	"painting", Int2(0,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_magiczny[] = {
	"torch", Int2(1,1),
	"magic_thing", Int2(1,1),
	"wardrobe", Int2(1,2),
	"bookshelf", Int2(2,4),
	"painting", Int2(0,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_magazyn[] = {
	"torch", Int2(1,1),
	"barrel", Int2(3,5),
	"barrels", Int2(1,2),
	"box", Int2(3,6),
	"boxes", Int2(1,3),
	"chest", Int2(1,2),
	"shelves", Int2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_skrytka[] = {
	"torch", Int2(1,1),
	"barrel", Int2(2,4),
	"barrels", Int2(1,3),
	"box", Int2(3,5),
	"boxes", Int2(1,2),
	"chest", Int2(1,2),
	"table_and_chairs", Int2(0,1),
	"gobelin", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_sklad_piwa[] = {
	"torch", Int2(1,1),
	"big_barrel", Int2(1,3),
	"barrel", Int2(1,5),
	"barrels", Int2(1,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_kapliczka[] = {
	"torch", Int2(1,1),
	"emblem", Int2(2,4),
	"altar", Int2(1,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_spotkan[] = {
	"torch", Int2(1,1),
	"table_and_chairs", Int2(1,2),
	"barrel", Int2(1,3),
	"bench_dir", Int2(1,3),
	"painting", Int2(1,2),
	"gobelin", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_treningowy[] = {
	"torch", Int2(1,1),
	"melee_target", Int2(1,3),
	"bow_target", Int2(1,3),
	"box", Int2(2,4),
	"boxes", Int2(1,2),
	"chest", Int2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_wyrobu[] = {
	"torch", Int2(1,1),
	"tanning_rack", Int2(1,2),
	"anvil", Int2(1,1),
	"cauldron", Int2(1,1),
	"table_and_chairs", Int2(0,1),
	"box", Int2(2,4),
	"boxes", Int2(1,2),
	"chest", Int2(1,2),
	"shelves", Int2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_kuchnia[] = {
	"torch", Int2(1,1),
	"campfire", Int2(1,1),
	"firewood", Int2(3,5),
	"table_and_chairs", Int2(0,1),
	"barrels", Int2(1,3),
	"chest", Int2(0,1),
	"shelves", Int2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_schody[] = {
	"torch", Int2(1,1),
	"box", Int2(1,3),
	"gobelin", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_schody_swiatynia[] = {
	"torch", Int2(1,1),
	"box", Int2(1,3),
	"emblem", Int2(2,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_groby[] = {
	"torch", Int2(1,1),
	"grave", Int2(1,3),
	"tombstone", Int2(1,4),
	"emblem", Int2(1,3)
};

//-----------------------------------------------------------------------------
ObjEntry objs_groby2[] = {
	"torch", Int2(1,1),
	"tombstone", Int2(3,5),
	"emblem", Int2(1,3)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_krypta[] = {
	"torch", Int2(1,1),
	"chest", Int2(1,2),
	"emblem", Int2(2,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_krypta_skarb[] = {
	"torch", Int2(1,1),
	"chest_r", Int2(1,1),
	"chest", Int2(1,2),
	"emblem", Int2(1,3),
	"box", Int2(1,3),
	"boxes", Int2(1,2),
	"barrel", Int2(1,2),
	"barrels", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_labirynt_skarb[] = {
	"chest_r", Int2(1,1),
	"chest", Int2(1,2),
	"box", Int2(1,3),
	"boxes", Int2(1,2),
	"barrel", Int2(1,2),
	"barrels", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_tut[] = {
	"torch", Int2(1,1),
	"gobelin", Int2(0,1),
	"painting", Int2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_obrazy[] = {
	"painting", Int2(10,10)
};

//-----------------------------------------------------------------------------
ObjEntry objs_portal[] = {
	"portal", Int2(1,1),
	"chest_r", Int2(1,1),
	"torch", Int2(1,1),
	"emblem_t", Int2(2,3)
};

//-----------------------------------------------------------------------------
RoomType g_room_types[] = {
	"sypialnia",  objs_sypialnia, countof(objs_sypialnia), RT_REPEAT,
	"pokoj_dowodcy", objs_pokoj_dowodcy, countof(objs_pokoj_dowodcy), 0,
	"biblioteczka", objs_biblioteczka, countof(objs_biblioteczka), 0,
	"magazyn", objs_magazyn, countof(objs_magazyn), RT_REPEAT,
	"skrytka", objs_skrytka, countof(objs_skrytka), RT_REPEAT,
	"sklad_piwa", objs_sklad_piwa, countof(objs_sklad_piwa), RT_REPEAT,
	"kapliczka", objs_kapliczka, countof(objs_kapliczka), 0,
	"pokoj_spotkan", objs_pokoj_spotkan, countof(objs_pokoj_spotkan), 0,
	"pokoj_treningowy", objs_pokoj_treningowy, countof(objs_pokoj_treningowy), 0,
	"pokoj_wyrobu", objs_pokoj_wyrobu, countof(objs_pokoj_wyrobu), 0,
	"kuchnia", objs_kuchnia, countof(objs_kuchnia), 0,
	"schody", objs_schody, countof(objs_schody), 0,
	"schody_swiatynia", objs_schody_swiatynia, countof(objs_schody_swiatynia), 0,
	"groby", objs_groby, countof(objs_groby), 0,
	"groby2", objs_groby2, countof(objs_groby2), 0,
	"pokoj_krypta", objs_pokoj_krypta, countof(objs_pokoj_krypta), 0,
	"krypta_skarb", objs_krypta_skarb, countof(objs_krypta_skarb), RT_TREASURE,
	"labirynt_skarb", objs_labirynt_skarb, countof(objs_labirynt_skarb), RT_TREASURE,
	"tut", objs_tut, countof(objs_tut), 0,
	"magiczny", objs_magiczny, countof(objs_magiczny), 0,
	"obrazy", objs_obrazy, countof(objs_obrazy), 0,
	"tron", objs_tron, countof(objs_tron), 0,
	"portal", objs_portal, countof(objs_portal), 0
};
int n_room_types = countof(g_room_types);

RoomType* FindRoomType(cstring id)
{
	assert(id);

	for(int i = 0; i < n_room_types; ++i)
	{
		if(strcmp(id, g_room_types[i].id) == 0)
			return &g_room_types[i];
	}

	assert(0);
	return &g_room_types[0];
}
