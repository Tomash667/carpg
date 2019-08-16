#include "Pch.h"
#include "GameCore.h"
#include "Tokenizer.h"
#include "Content.h"
#include "Spell.h"
#include "MusicTrack.h"
#include "BitStreamFunc.h"
#include "BuildingLoader.h"
#include "ClassLoader.h"
#include "DialogLoader.h"
#include "ItemLoader.h"
#include "ObjectLoader.h"
#include "QuestLoader.h"
#include "SpellLoader.h"
#include "UnitLoader.h"

//-----------------------------------------------------------------------------
Content content;
static cstring content_id[] = {
	"items",
	"objects",
	"spells",
	"dialogs",
	"classes",
	"units",
	"buildings",
	"musics",
	"quests"
};
static_assert(countof(content_id) == (int)Content::Id::Max, "Missing content_id.");

//=================================================================================================
Content::Content() : building_loader(new BuildingLoader), class_loader(new ClassLoader), dialog_loader(new DialogLoader), item_loader(new ItemLoader),
object_loader(new ObjectLoader), quest_loader(new QuestLoader), spell_loader(new SpellLoader), unit_loader(new UnitLoader)
{
	quest_loader->dialog_loader = dialog_loader;
}

//=================================================================================================
void Content::LoadContent(delegate<void(Id)> callback)
{
	uint loaded;

	LoadVersion();

	Info("Game: Loading items.");
	callback(Id::Items);
	item_loader->DoLoading();

	Info("Game: Loading dialogs.");
	callback(Id::Dialogs);
	dialog_loader->DoLoading();

	Info("Game: Loading objects.");
	callback(Id::Objects);
	object_loader->DoLoading();

	Info("Game: Loading spells.");
	callback(Id::Spells);
	spell_loader->DoLoading();

	Info("Game: Loading classes.");
	callback(Id::Classes);
	class_loader->DoLoading();

	Info("Game: Loading units.");
	callback(Id::Units);
	unit_loader->DoLoading();
	class_loader->ApplyUnits();

	Info("Game: Loading buildings.");
	callback(Id::Buildings);
	building_loader->DoLoading();

	Info("Game: Loading music.");
	callback(Id::Musics);
	loaded = MusicTrack::Load(errors);
	Info("Game: Loaded music: %u.", loaded);

	Info("Game: Loading quests.");
	callback(Id::Quests);
	quest_loader->DoLoading();

	unit_loader->ProcessDialogRequests();

	delete building_loader;
	delete class_loader;
	delete dialog_loader;
	delete item_loader;
	delete object_loader;
	delete quest_loader;
	delete spell_loader;
	delete unit_loader;
}

//=================================================================================================
void Content::LoadVersion()
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
void Content::CleanupContent()
{
	ItemLoader::Cleanup();
	ObjectLoader::Cleanup();
	SpellLoader::Cleanup();
	DialogLoader::Cleanup();
	UnitLoader::Cleanup();
	BuildingLoader::Cleanup();
	MusicTrack::Cleanup();
	QuestLoader::Cleanup();
	ClassLoader::Cleanup();
}

//=================================================================================================
void Content::WriteCrc(BitStreamWriter& f)
{
	f << crc;
}

//=================================================================================================
void Content::ReadCrc(BitStreamReader& f)
{
	f >> client_crc;
}

//=================================================================================================
bool Content::GetCrc(Id type, uint& my_crc, cstring& type_crc)
{
	if(type < (Id)0 || type >= Id::Max)
		return false;

	my_crc = crc[(int)type];
	type_crc = content_id[(int)type];
	return true;
}

//=================================================================================================
bool Content::ValidateCrc(Id& type, uint& my_crc, uint& player_crc, cstring& type_str)
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
