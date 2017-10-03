#include "Pch.h"
#include "Core.h"
#include "Content.h"

string content::system_dir;
uint content::errors;
uint content::warnings;
static uint client_buildings_crc;
static uint client_objects_crc;

void content::LoadContent(delegate<void(Id)> callback)
{
	Info("Game: Loading buildings.");
	LoadBuildings();
	callback(Id::Buildings);

	Info("Game: Loading objects.");
	LoadObjects();
	callback(Id::Objects);
}

void content::CleanupContent()
{
	CleanupBuildings();
	CleanupObjects();
}

bool content::ReadCrc(BitStream& stream)
{
	return stream.Read(client_buildings_crc)
		&& stream.Read(client_objects_crc);
}

void content::WriteCrc(BitStream& stream)
{
	stream.Write(buildings_crc);
	stream.Write(objects_crc);
}

bool content::GetCrc(Id type, uint& my_crc, cstring& type_crc)
{
	switch(type)
	{
	case Id::Buildings:
		my_crc = buildings_crc;
		type_crc = "buildings";
		return true;
	case Id::Objects:
		my_crc = objects_crc;
		type_crc = "objects";
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

	if(objects_crc != client_objects_crc)
	{
		type = Id::Objects;
		my_crc = objects_crc;
		player_crc = client_objects_crc;
		type_str = "objects";
		return false;
	}

	return true;
}
