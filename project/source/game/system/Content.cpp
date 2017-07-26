#include "Pch.h"
#include "Core.h"
#include "Content.h"

string content::system_dir;
uint content::errors;
uint content::warnings;
static uint client_buildings_crc;

void content::LoadContent(delegate<void(Id)> callback)
{
	Info("Game: Loading buildings.");
	LoadBuildings();
	callback(Id::Buildings);
}

void content::CleanupContent()
{
	CleanupBuildings();
}

bool content::ReadCrc(BitStream& stream)
{
	return stream.Read(client_buildings_crc);
}

void content::WriteCrc(BitStream& stream)
{
	stream.Write(buildings_crc);
}

bool content::GetCrc(Id type, uint& my_crc, cstring& type_crc)
{
	switch(type)
	{
	case Id::Buildings:
		my_crc = buildings_crc;
		type_crc = "buildings";
		return true;
	}

	return false;
}

bool content::ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str)
{
	if(buildings_crc != client_buildings_crc)
	{
		type = Id::Buildings;
		my_crc = buildings_crc;
		player_crc = client_buildings_crc;
		type_str = "buildings";
		return false;
	}

	return true;
}
