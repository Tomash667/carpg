#include "Pch.h"
#include "GameFile.h"

#include "SaveState.h"
#include "UnitGroup.h"

void GameReader::operator >> (UnitGroup*& group)
{
	if(LOAD_VERSION >= V_0_11)
	{
		const string& id = ReadString1();
		group = UnitGroup::Get(id);
	}
	else
	{
		old::SPAWN_GROUP spawn;
		Read(spawn);
		group = old::OldToNew(spawn);
	}
}
