#include "Pch.h"
#include "GameCore.h"
#include "Quest_Secret.h"
#include "Item.h"
#include "BaseObject.h"
#include "GameFile.h"

inline Item& GetNote()
{
	return *Item::Get("sekret_kartka");
}

void Quest_Secret::Init()
{
	state = (BaseObject::Get("tomashu_dom")->mesh ? SECRET_NONE : SECRET_OFF);
	GetNote().desc.clear();
	where = -1;
	where2 = -1;
}

void Quest_Secret::Save(GameWriter& f)
{
	f << state;
	f << GetNote().desc;
	f << where;
	f << where2;
}

void Quest_Secret::Load(GameReader& f)
{
	f >> state;
	f >> GetNote().desc;
	f >> where;
	f >> where2;
	if(state > SECRET_NONE && !BaseObject::Get("tomashu_dom")->mesh)
		throw "Save uses 'data.pak' file which is missing!";
}
