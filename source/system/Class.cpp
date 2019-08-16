#include "Pch.h"
#include "GameCore.h"
#include "Class.h"

vector<Class*> Class::classes;
static WeightContainer<Class*> player_classes, hero_classes, crazy_classes;

//=================================================================================================
Class* Class::TryGet(Cstring id)
{
	for(Class* clas : classes)
	{
		if(clas->id == id)
			return clas;
	}
	return nullptr;
}

//=================================================================================================
Class* Class::GetRandomPlayer()
{
	return player_classes.GetRandom();
}

//=================================================================================================
Class* Class::GetRandomHero()
{
	return hero_classes.GetRandom();
}

//=================================================================================================
Class* Class::GetRandomCrazy()
{
	return crazy_classes.GetRandom();
}

//=================================================================================================
void Class::InitLists()
{
	for(Class* clas : classes)
	{
		if(clas->player && clas->player_weight > 0)
			player_classes.Add(clas, clas->player_weight);
		if(clas->hero && clas->hero_weight > 0)
			hero_classes.Add(clas, clas->hero_weight);
		if(clas->crazy && clas->crazy_weight > 0)
			crazy_classes.Add(clas, clas->crazy_weight);
	}
}
