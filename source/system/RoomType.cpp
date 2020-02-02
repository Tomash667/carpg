#include "Pch.h"
#include "GameCore.h"
#include "RoomType.h"

vector<RoomType*> RoomType::types;

//=================================================================================================
RoomType* RoomType::Get(Cstring id)
{
	for(RoomType* type : types)
	{
		if(type->id == id)
			return type;
	}

	return nullptr;
}
