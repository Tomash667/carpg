#include "Pch.h"
#include "GameCore.h"
#include "Tokenizer.h"
#include "Content.h"
#include "Spell.h"
#include "Music.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
string content::system_dir;
uint content::errors, content::warnings;
uint content::crc[(int)content::Id::Max];
uint content::version;
bool content::require_update;
static uint client_crc[(int)content::Id::Max];

//=================================================================================================
void content::LoadContent(delegate<void(Id)> callback)
{
	uint loaded;

	LoadVersion();

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
	LoadDialogs();
	callback(Id::Dialogs);

	Info("Game: Loading units.");
	LoadUnits();
	callback(Id::Units);

	Info("Game: Loading buildings.");
	LoadBuildings();
	callback(Id::Buildings);

	Info("Game: Loading music.");
	loaded = LoadMusics(errors);
	Info("Game: Loaded music: %u.", loaded);
	callback(Id::Musics);
}

//=================================================================================================
void content::LoadVersion()
{
	cstring path = Format("%s/system.txt", system_dir.c_str());
	Tokenizer t;

	try
	{
		if(!t.FromFile(path))
			t.Throw("Failed to open file '%s'.", path);
		t.Next();
		t.AssertItem("version");
		t.Next();
		version = t.GetUint();
		t.Next();
		t.AssertEof();

		Info("Game: System version %u.", version);
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Game: Failed to load system version: %s", e.ToString());
	}
}

//=================================================================================================
void content::CleanupContent()
{
	CleanupItems();
	CleanupObjects();
	CleanupSpells();
	CleanupDialogs();
	CleanupUnits();
	CleanupBuildings();
	CleanupMusics();
}

//=================================================================================================
void content::WriteCrc(BitStreamWriter& f)
{
	f << crc;
}

//=================================================================================================
void content::ReadCrc(BitStreamReader& f)
{
	f >> client_crc;
}

//=================================================================================================
bool content::GetCrc(Id type, uint& my_crc, cstring& type_crc)
{
	if(type < (Id)0 || type >= Id::Max)
		return false;

	my_crc = crc[(int)type];
	type_crc = content_id[(int)type];
	return true;
}

//=================================================================================================
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
