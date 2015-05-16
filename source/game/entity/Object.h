#pragma once

#include "Animesh.h"

//-----------------------------------------------------------------------------
// Rodzaj fizyki obiektu
enum OBJ_PHY_TYPE
{
	OBJ_CYLINDER,
	OBJ_HITBOX,
	OBJ_HITBOX_ROT
};

//-----------------------------------------------------------------------------
// Flagi obiektu
enum OBJ_FLAGS
{
	OBJ_PRZY_SCIANIE = 1<<0,
	OBJ_BRAK_FIZYKI = 1<<1,
	OBJ_WYSOKO = 1<<2,
	OBJ_SKRZYNIA = 1<<3,
	OBJ_NA_SCIANIE = 1<<4,
	OBJ_BRAK_PRZESUNIECIA = 1<<5,
	OBJ_SWIATLO = 1<<6,
	OBJ_STOL = 1<<7,
	OBJ_OGNISKO = 1<<8,
	OBJ_WAZNE = 1<<9,
	OBJ_CZASTECZKA = 1<<10,
	OBJ_UZYWALNY = 1<<11,
	OBJ_LAWA = 1<<12,
	OBJ_KOWADLO = 1<<13,
	OBJ_KRZESLO = 1<<14,
	OBJ_KOCIOLEK = 1<<15,
	OBJ_SKALOWALNY = 1<<16,
	OBJ_WSKAZNIK_NA_FIZYKE = 1<<17, // ustawia Object::ptr, tylko dla zwyk³ych obiektów!
	OBJ_BUDYNEK = 1<<18,
	OBJ_PODWOJNA_FIZYKA = 1<<19, // obs³ugiwane tylko 2 boxy i RespawnObjectColliders, CreateObject
	OBJ_KREW = 1<<20,
	OBJ_WYMAGANE = 1<<21,
	OBJ_NA_SRODKU = 1<<22,
	OBJ_TRON = 1<<23,
	OBJ_ZYLA_ZELAZA = 1<<24,
	OBJ_ZYLA_ZLOTA = 1<<25,
	OBJ_KONWERSJA_V0 = 1<<26,
	OBJ_BLOKUJE_WIDOK = 1<<27,
	OBJ_OBROT = 1<<28,
	OBJ_WODA = 1<<29,
	OBJ_OPCJONALNY = 1<<30,
	OBJ_STOLEK = 1<<31,
};
enum OBJ_FLAGS2
{
	OBJ2_LAWA_DIR = 1<<0,
	OBJ2_VARIANT = 1<<1, // póki co nie mo¿e byæ skalowalny, musi mieæ fizykê box i byæ u¿ywalny, nie mo¿e byæ opcjonalny
	OBJ2_WIELOFIZYKA = 1<<2, // póki co obs³uguje tylko boxy
	OBJ2_CAM_COLLIDERS = 1<<3,
};

//-----------------------------------------------------------------------------
struct VariantObj
{
	struct Entry
	{
		cstring mesh_name;
		Animesh* mesh;
	};
	Entry* entries;
	uint count;
	bool loaded;
};

//-----------------------------------------------------------------------------
// Bazowy obiekt
struct Obj
{
	cstring id, name, mesh;
	Animesh* ani;
	OBJ_PHY_TYPE type;
	float r, h, centery;
	VEC2 size;
	btCollisionShape* shape;
	MATRIX* matrix;
	int flagi, flagi2, alpha;
	Obj* next_obj;
	VariantObj* variant;
	float extra_dist;

	Obj() : flagi(0), flagi2(0)
	{

	}

	Obj(cstring _id, int _flagi, int flagi2, cstring _name, cstring _mesh, int _alpha=-1, float _centery=0.f, VariantObj* variant=NULL, float extra_dist=0.f) :
		id(_id), name(_name), mesh(_mesh), type(OBJ_HITBOX), ani(NULL), shape(NULL), matrix(NULL), flagi(_flagi), flagi2(flagi2), alpha(_alpha), centery(_centery), next_obj(NULL),
		variant(variant), extra_dist(extra_dist)
	{

	}

	Obj(cstring _id, int _flagi, int flagi2, cstring _name, cstring _mesh, float _radius, float _height, int _alpha=-1, float _centery=0.f, VariantObj* variant=NULL, float extra_dist=0.f) :
		id(_id), name(_name), mesh(_mesh), type(OBJ_CYLINDER), ani(NULL), shape(NULL), r(_radius), h(_height), matrix(NULL), flagi(_flagi), flagi2(flagi2), alpha(_alpha), centery(_centery),
		next_obj(NULL), variant(variant), extra_dist(extra_dist)
	{

	}
};
extern Obj g_objs[];
extern const uint n_objs;

//-----------------------------------------------------------------------------
inline cstring GetRandomPainting()
{
	if(rand2()%100 == 0)
		return "painting3";
	switch(rand2()%23)
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

//-----------------------------------------------------------------------------
// szuka obiektu, is_variant ustawia tylko na true jeœli ma kilka wariantów
inline Obj* FindObjectTry(cstring _id, bool* is_variant=NULL)
{
	assert(_id);

	if(strcmp(_id, "painting") == 0)
	{
		if(is_variant)
			*is_variant = true;
		return FindObjectTry(GetRandomPainting());
	}

	if(strcmp(_id, "tombstone") == 0)
	{
		if(is_variant)
			*is_variant = true;
		int id = random(0,9);
		if(id != 0)
			return FindObjectTry(Format("tombstone_x%d", id));
		else
			return FindObjectTry("tombstone_1");
	}

	if(strcmp(_id, "random") == 0)
	{
		switch(rand2()%3)
		{
		case 0: return FindObjectTry("wheel");
		case 1: return FindObjectTry("rope");
		case 2: return FindObjectTry("woodpile");
		}
	}

	for(uint i=0; i<n_objs; ++i)
	{
		if(strcmp(g_objs[i].id, _id) == 0)
			return &g_objs[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
inline Obj* FindObject(cstring _id, bool* is_variant=NULL)
{
	Obj* obj = FindObjectTry(_id, is_variant);
	assert(obj && "Brak takiego obiektu!");
	return obj;
}


//-----------------------------------------------------------------------------
// Obiekt w grze
struct Object
{
	VEC3 pos, rot;
	float scale;
	Animesh* mesh;
	Obj* base;
	void* ptr;
	bool require_split;

	Object() : require_split(false)
	{

	}

	inline float GetRadius() const
	{
		return mesh->head.radius * scale;
	}
	// -1 - nie, 0 - tak, 1 - tak i bez cullingu
	inline int RequireAlphaTest() const
	{
		if(!base)
			return -1;
		else
			return base->alpha;
	}
	inline bool IsParticle() const
	{
		return base && IS_SET(base->flagi, OBJ_CZASTECZKA);
	}
	void Save(HANDLE file);
	// zwraca false jeœli obiekt trzeba usun¹æ i zast¹piæ czymœ innym
	// aktualnie obs³ugiwane tylko przez InsideLocationLevel
	bool Load(HANDLE file);
	void Swap(Object& o);
};

//-----------------------------------------------------------------------------
// Animowany obiekt w grze
struct ObjectAni
{
	VEC3 pos;
	float rot, scale;
	AnimeshInstance* ani;

	inline float GetRadius() const
	{
		return ani->ani->head.radius * scale;
	}
};
