#include "Pch.h"
#include "Base.h"
#include "Building.h"

#define T SCHEME_GRASS
#define B SCHEME_BUILDING
#define P SCHEME_SAND
#define D SCHEME_PATH
#define U SCHEME_UNIT
#undef C
#define C SCHEME_BUILDING_PART
#define N SCHEME_BUILDING_NO_PHY

int _s_kupiec[] = {
	B,B,B,B,
	B,B,B,B,
	B,B,B,B,
	U,D,P,P,
	P,P,P,P
};

int _s_kowal[] = {
	B,B,B,B,
	B,B,B,B,
	P,D,U,P,
	P,P,P,P
};

int _s_pola[] = {
	T,P,N,N,P,T,
	P,N,N,N,N,P,
	N,N,N,N,N,N,
	N,N,N,N,N,N,
	P,N,N,N,N,P,
	P,P,N,N,P,P,
	P,P,D,U,P,P,
	P,P,P,P,P,P,
	P,P,P,P,P,P,
	P,P,P,P,P,P
};

int _s_karczma[] = {
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,D,P
};

int _s_baraki[] = {
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	B,B,B,B,B,B,
	P,P,U,D,P,P,
	P,P,P,P,P,P,
	P,P,P,P,P,P,
	T,P,P,P,P,T
};

int _s_soltys[] = {
	B,B,B,B,
	B,B,B,B,
	B,B,B,B,
	B,B,B,T,
	B,B,B,T,
	T,D,T,T
};

int _s_ratusz[] = {
	B,B,B,B,B,
	B,B,B,B,B,
	B,B,B,B,B,
	B,B,B,B,B,
	B,B,B,B,B,
	B,B,B,B,B,
	B,B,B,B,B,
	T,P,D,P,T
};

int _s_dom[] = {
	B,B,B,
	B,B,B,
	T,B,B,
	T,B,B,
	T,D,P
};

int _s_dom2[] = {
	B,B,B,B,
	B,B,B,B,
	B,B,B,B,
	T,D,P,T
};

int _s_arena[] = {
	T,T,N,N,N,N,N,N,T,T,
	T,N,N,N,N,N,N,N,N,T,
	N,N,N,N,N,N,N,N,N,N,
	N,N,N,N,N,N,N,N,N,N,
	N,N,N,N,N,N,N,N,N,N,
	N,N,N,N,N,N,N,N,N,N,
	N,N,N,N,N,N,N,N,N,N,
	N,N,N,N,N,N,N,N,N,N,
	T,N,N,N,N,N,N,N,N,T,
	T,T,N,N,N,N,N,N,T,T,
	T,T,T,T,U,D,T,T,T,T
};

int _s_soltys_new[] = {
	C,C,C,C,C,C,C,C,C,C,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,C,C,C,C,C,C,C,C,C,C,
	D,T,T,T,T,T,T,T,T,T,T
};

int _s_chata[] = {
	C,C,C,C,C,C,C,C,C,C,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,C,
	C,C,C,C,C,C,C,C,C,C,C,
	T,T,T,T,T,D,T,T,T,T,T
};

int _s_chata2[] = {
	C,C,C,C,C,C,C,C,
	C,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,C,
	C,C,C,C,C,C,C,C,
	T,T,T,D,T,T,T,T
};

int _s_chata3[] = {
	C,C,C,C,C,C,
	C,N,N,N,N,C,
	C,N,N,N,N,C,
	C,N,N,N,N,C,
	C,C,C,C,C,C,
	P,D,T,T,T,T
};

int _s_karczma2[] = {
	C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,
	C,N,N,N,N,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,N,N,N,N,N,N,N,N,N,N,C,
	C,N,N,N,C,C,C,C,C,C,C,N,N,N,C,
	C,N,N,N,C,T,P,P,P,T,C,N,N,N,C,
	C,N,N,N,C,T,P,P,P,T,C,N,N,N,C,
	C,N,N,N,C,T,T,P,T,T,C,N,N,N,C,
	C,C,C,C,C,T,T,D,T,T,C,C,C,C,C
};

#undef T
#undef B
#undef P
#undef D
#undef U
#undef C
#undef N

// 0 = 0
// 1 = 3/2 PI
// 2 = PI
// 3 = PI/2

Building buildings[] = {
	//		id						model				model_œrodka			ext				shift[4 dó³, prawo, góra, lewo]				scheme		flags
	Building("merchant",			"kupiec.qmsh",		nullptr,					INT2(4,5),		INT2(1,1), INT2(0,1), INT2(1,0), INT2(1,1), _s_kupiec, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME),
	Building("blacksmith",			"kowal.qmsh",		nullptr,					INT2(4,4),		INT2(1,1), INT2(1,1), INT2(1,1), INT2(1,1), _s_kowal, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME),
	Building("alchemist",			"alchemik.qmsh",	nullptr,					INT2(4,4),		INT2(1,1), INT2(1,1), INT2(1,1), INT2(1,1), _s_kowal, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME),
	Building("training_grounds",	"pola.qmsh",		nullptr,					INT2(6,10),		INT2(1,3), INT2(-1,1), INT2(1,-1), INT2(3,1), _s_pola, BF_DRAW_NAME|BF_LOAD_NAME),
	Building("inn",					"karczma.qmsh",		"karczma_srodek.qmsh",	INT2(6,12),		INT2(1,1), INT2(1,1), INT2(1,1), INT2(1,1), _s_karczma, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME, BG_INN),
	Building("town_hall",			"ratusz.qmsh",		"ratusz_srodek.qmsh",	INT2(5,8),		INT2(1,1), INT2(1,1), INT2(0,1), INT2(1,0), _s_ratusz, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME, BG_HALL),
	Building("village_house",		"soltys.qmsh",		"soltys_srodek.qmsh",	INT2(11,6),		INT2(0,2), INT2(0,0), INT2(1,0), INT2(2,1), _s_soltys_new, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME, BG_HALL),
	Building("barracks",			"baraki.qmsh",		nullptr,					INT2(6,15),		INT2(1,2), INT2(-1,1), INT2(1,-1), INT2(2,1), _s_baraki, BF_DRAW_NAME|BF_LOAD_NAME),
	Building("house",				"dom.qmsh",			nullptr,					INT2(3,5),		INT2(0,1), INT2(0,0), INT2(1,0), INT2(1,1), _s_dom, 0),
	Building("house2",				"dom2.qmsh",		nullptr,					INT2(4,4),		INT2(1,1), INT2(1,1), INT2(1,1), INT2(1,1), _s_dom2, 0),
	Building("house3",				"dom3.qmsh",		nullptr,					INT2(3,5),		INT2(0,1), INT2(0,0), INT2(1,0), INT2(1,1), _s_dom, 0),
	Building("arena",				"arena.qmsh",		"arena_srodek.qmsh",	INT2(10,11),	INT2(1,1), INT2(0,1), INT2(1,0), INT2(1,1), _s_arena, BF_FAVOR_CENTER|BF_DRAW_NAME|BF_LOAD_NAME),
	Building("food_seller",			"food_seller.qmsh", nullptr,					INT2(4,5),		INT2(1,1), INT2(0,1), INT2(1,0), INT2(1,1), _s_kupiec, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME),
	Building("cottage",				"chata.qmsh",		nullptr,					INT2(11,6),		INT2(0,2), INT2(0,0), INT2(1,0), INT2(2,1), _s_chata, BF_FAVOR_ROAD),
	Building("cottage2",			"chata2.qmsh",		nullptr,					INT2(8,6),		INT2(0,2), INT2(0,0), INT2(2,0), INT2(2,2), _s_chata2, BF_FAVOR_ROAD),
	Building("cottage3",			"chata3.qmsh",		nullptr,					INT2(6,6),		INT2(1,1), INT2(1,1), INT2(1,1), INT2(1,1), _s_chata3, BF_FAVOR_ROAD),
	Building("village_inn",			"karczma2.qmsh",	"karczma2_srodek.qmsh",	INT2(15,9),		INT2(0,0), INT2(1,0), INT2(1,1), INT2(0,1), _s_karczma2, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME, BG_INN),
	Building("empty",				nullptr,				nullptr,					INT2(1,1),		INT2(0,0), INT2(0,0), INT2(0,0), INT2(0,0), nullptr, 0),
	Building("village_house_old",	"soltys_old.qmsh",	"soltys_srodek_old.qmsh", INT2(4, 6),	INT2(1, 2),	 INT2(0, 1), INT2(1, 0), INT2(2, 1), _s_soltys, BF_FAVOR_CENTER|BF_FAVOR_ROAD|BF_DRAW_NAME|BF_LOAD_NAME, BG_HALL),
};
