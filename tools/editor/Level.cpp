#include "Pch.h"
#include "Level.h"

#include <File.h>

void Level::Save(FileWriter& f)
{
	f.WriteStringF("LVL");
	f << (byte)0;
}

bool Level::Load(FileReader& f)
{
	char sign[3];
	byte ver;
	f.Read(sign, 3);
	f >> ver;
	if(!f || strncmp("LVL", sign, 3) != 0 || ver != 0)
		return false;
	return true;
}
