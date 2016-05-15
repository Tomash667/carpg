#include "Pch.h"
#include "Base.h"
#include "Building.h"

int BG_INN, BG_HALL;

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
