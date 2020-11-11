#include "Pch.h"
#include "Guild.h"

#include "SaveState.h"

Guild* guild;

void Guild::Clear()
{
	created = false;
}

void Guild::Create(const string& _name)
{
	created = true;
	name = _name;
}

void Guild::Save(GameWriter& f)
{
	f << created;
	if(!created)
		return;

	f << name;
}

void Guild::Load(GameReader& f)
{
	if(LOAD_VERSION < V_DEV)
	{
		created = false;
		return;
	}

	f >> created;
	if(!created)
		return;

	f >> name;
}
