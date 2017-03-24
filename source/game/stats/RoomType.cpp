// rodzaje pomieszczeñ jakie mo¿na znaleŸæ w podziemiach
#include "Pch.h"
#include "Base.h"
#include "RoomType.h"

//-----------------------------------------------------------------------------
ObjEntry objs_sypialnia[] = {
	"torch", INT2(1,1),
	"bed", INT2(2,3),
	"wardrobe", INT2(2,3),
	"chest", INT2(1,2),
	"shelves", INT2(1,2),
	"table_and_chairs", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_dowodcy[] = {
	"torch", INT2(1,1),
	"bed", INT2(1,1),
	"chest", INT2(1,3),
	"bookshelf", INT2(1,2),
	"painting", INT2(0,3),
	"wardrobe", INT2(1,2),
	"gobelin", INT2(0,1),
	"shelves", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_tron[] = {
	"torch", INT2(1,1),
	"throne", INT2(1,1),
	"chest", INT2(2,3),
	"bookshelf", INT2(1,2),
	"painting", INT2(0,3),
	"wardrobe", INT2(1,2),
	"gobelin", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_biblioteczka[] = {
	"torch", INT2(1,1),
	"wardrobe", INT2(1,2),
	"bookshelf", INT2(3,5),
	"table_and_chairs", INT2(1,2),
	"gobelin", INT2(0,1),
	"painting", INT2(0,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_magiczny[] = {
	"torch", INT2(1,1),
	"magic_thing", INT2(1,1),
	"wardrobe", INT2(1,2),
	"bookshelf", INT2(2,4),
	"painting", INT2(0,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_magazyn[] = {
	"torch", INT2(1,1),
	"barrel", INT2(3,5),
	"barrels", INT2(1,2),
	"box", INT2(3,6),
	"boxes", INT2(1,3),
	"chest", INT2(1,2),
	"shelves", INT2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_skrytka[] = {
	"torch", INT2(1,1),
	"barrel", INT2(2,4),
	"barrels", INT2(1,3),
	"box", INT2(3,5),
	"boxes", INT2(1,2),
	"chest", INT2(1,2),
	"table_and_chairs", INT2(0,1),
	"gobelin", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_sklad_piwa[] = {
	"torch", INT2(1,1),
	"big_barrel", INT2(1,3),
	"barrel", INT2(1,5),
	"barrels", INT2(1,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_kapliczka[] = {
	"torch", INT2(1,1),
	"emblem", INT2(2,4),
	"altar", INT2(1,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_spotkan[] = {
	"torch", INT2(1,1),
	"table_and_chairs", INT2(1,2),
	"barrel", INT2(1,3),
	"bench_dir", INT2(1,3),
	"painting", INT2(1,2),
	"gobelin", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_treningowy[] = {
	"torch", INT2(1,1),
	"melee_target", INT2(1,3),
	"bow_target", INT2(1,3),
	"box", INT2(2,4),
	"boxes", INT2(1,2),
	"chest", INT2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_wyrobu[] = {
	"torch", INT2(1,1),
	"tanning_rack", INT2(1,2),
	"anvil", INT2(1,1),
	"cauldron", INT2(1,1),
	"table_and_chairs", INT2(0,1),
	"box", INT2(2,4),
	"boxes", INT2(1,2),
	"chest", INT2(1,2),
	"shelves", INT2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_kuchnia[] = {
	"torch", INT2(1,1),
	"campfire", INT2(1,1),
	"firewood", INT2(3,5),
	"table_and_chairs", INT2(0,1),
	"barrels", INT2(1,3),
	"chest", INT2(0,1),
	"shelves", INT2(1,2)
};

//-----------------------------------------------------------------------------
ObjEntry objs_schody[] = {
	"torch", INT2(1,1),
	"box", INT2(1,3),
	"gobelin", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_schody_swiatynia[] = {
	"torch", INT2(1,1),
	"box", INT2(1,3),
	"emblem", INT2(2,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_groby[] = {
	"torch", INT2(1,1),
	"grave", INT2(1,3),
	"tombstone", INT2(1,4),
	"emblem", INT2(1,3)
};

//-----------------------------------------------------------------------------
ObjEntry objs_groby2[] = {
	"torch", INT2(1,1),
	"tombstone", INT2(3,5),
	"emblem", INT2(1,3)
};

//-----------------------------------------------------------------------------
ObjEntry objs_pokoj_krypta[] = {
	"torch", INT2(1,1),
	"chest", INT2(1,2),
	"emblem", INT2(2,4)
};

//-----------------------------------------------------------------------------
ObjEntry objs_krypta_skarb[] = {
	"torch", INT2(1,1),
	"chest_r", INT2(1,1),
	"chest", INT2(1,2),
	"emblem", INT2(1,3),
	"box", INT2(1,3),
	"boxes", INT2(1,2),
	"barrel", INT2(1,2),
	"barrels", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_labirynt_skarb[] = {
	"chest_r", INT2(1,1),
	"chest", INT2(1,2),
	"box", INT2(1,3),
	"boxes", INT2(1,2),
	"barrel", INT2(1,2),
	"barrels", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_tut[] = {
	"torch", INT2(1,1),
	"gobelin", INT2(0,1),
	"painting", INT2(0,1)
};

//-----------------------------------------------------------------------------
ObjEntry objs_obrazy[] = {
	"painting", INT2(10,10)
};

//-----------------------------------------------------------------------------
ObjEntry objs_portal[] = {
	"portal", INT2(1,1),
	"chest_r", INT2(1,1),
	"torch", INT2(1,1),
	"emblem_t", INT2(2,3)
};

//-----------------------------------------------------------------------------
RoomType g_room_types[] = {
	"sypialnia",  objs_sypialnia, countof(objs_sypialnia), RT_POWTARZAJ,
	"pokoj_dowodcy", objs_pokoj_dowodcy, countof(objs_pokoj_dowodcy), 0,
	"biblioteczka", objs_biblioteczka, countof(objs_biblioteczka), 0,
	"magazyn", objs_magazyn, countof(objs_magazyn), RT_POWTARZAJ,
	"skrytka", objs_skrytka, countof(objs_skrytka), RT_POWTARZAJ,
	"sklad_piwa", objs_sklad_piwa, countof(objs_sklad_piwa), RT_POWTARZAJ,
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
	"krypta_skarb", objs_krypta_skarb, countof(objs_krypta_skarb), 0,
	"labirynt_skarb", objs_labirynt_skarb, countof(objs_labirynt_skarb), 0,
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

	for(int i = 0; i<n_room_types; ++i)
	{
		if(strcmp(id, g_room_types[i].id) == 0)
			return &g_room_types[i];
	}

	assert(0);
	return &g_room_types[0];
}
