#include "Pch.h"
#include "Core.h"
#include "Content.h"
#include "Spell.h"
#include "Dialog.h"
#include "UnitData.h"
#include "Music.h"

static cstring content_id[] = {
	"items",
	"objects",
	"spells",
	"dialogs",
	"units",
	"buildings",
	"musics"
};
static_assert(countof(content_id) == (int)content::Id::Max, "Missing content_id.");

string content::system_dir;
uint content::errors, content::warnings;
uint content::crc[(int)content::Id::Max];
static uint client_crc[(int)content::Id::Max];

void content::LoadContent(delegate<void(Id)> callback)
{
	uint loaded;

	Info("Game: Loading items.");
	LoadItems();
	callback(Id::Items);

	Info("Game: Loading objects.");
	LoadObjects();
	callback(Id::Objects);

	Info("Game: Loading spells.");
	loaded = LoadSpells(crc[(int)Id::Spells], errors);
	Info("Game: Loaded spells: %u (crc %p).", loaded, crc[(int)Id::Spells]);
	callback(Id::Spells);

	Info("Game: Loading dialogs.");
	loaded = LoadDialogs(crc[(int)Id::Dialogs], errors);
	Info("Game: Loaded dialogs: %u (crc %p).", loaded, crc[(int)Id::Dialogs]);
	callback(Id::Dialogs);

	Info("Game: Loading units.");
	loaded = LoadUnits(crc[(int)Id::Units], errors);
	Info("Game: Loaded units: %u (crc %p).", loaded, crc[(int)Id::Units]);
	callback(Id::Units);

	Info("Game: Loading buildings.");
	LoadBuildings();
	callback(Id::Buildings);

	Info("Game: Loading music.");
	loaded = LoadMusics(errors);
	Info("Game: Loaded music: %u.", loaded);
	callback(Id::Musics);
}

void content::CleanupContent()
{
	CleanupItems();
	CleanupObjects();
	CleanupSpells();
	CleanupUnits();
	CleanupBuildings();
	CleanupMusics();
}

bool content::ReadCrc(BitStream& stream)
{
	return stream.Read(client_crc);
}

void content::WriteCrc(BitStream& stream)
{
	stream.Write(crc);
}

bool content::GetCrc(Id type, uint& my_crc, cstring& type_crc)
{
	if(type < (Id)0 || type >= Id::Max)
		return false;

	my_crc = crc[(int)type];
	type_crc = content_id[(int)type];
	return true;
}

bool content::ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str)
{
	for(uint i = 0; i < (uint)Id::Max; ++i)
	{
		if(crc[i] != client_crc[i])
		{
			type = (Id)i;
			my_crc = crc[i];
			player_crc = client_crc[i];
			type_str = content_id[i];
			return false;
		}
	}

	return true;
}
