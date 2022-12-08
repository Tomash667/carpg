#include "Pch.h"
#include "Content.h"

#include "AbilityLoader.h"
#include "BitStreamFunc.h"
#include "BuildingLoader.h"
#include "ClassLoader.h"
#include "DialogLoader.h"
#include "ItemLoader.h"
#include "LocationLoader.h"
#include "MusicListLoader.h"
#include "ObjectLoader.h"
#include "PerkLoader.h"
#include "QuestLoader.h"
#include "RequiredLoader.h"
#include "UnitLoader.h"

//-----------------------------------------------------------------------------
Content content;
static cstring content_id[] = {
	"abilities",
	"buildings",
	"classes",
	"dialogs",
	"items",
	"locations",
	"musics",
	"objects",
	"perks",
	"quests",
	"required",
	"units"
};
static_assert(countof(content_id) == (int)Content::Id::Max, "Missing content_id.");

//=================================================================================================
Content::Content() : abilityLoader(new AbilityLoader), buildingLoader(new BuildingLoader), classLoader(new ClassLoader), dialogLoader(new DialogLoader),
itemLoader(new ItemLoader), locationLoader(new LocationLoader), musicLoader(new MusicListLoader), objectLoader(new ObjectLoader), perkLoader(new PerkLoader),
questLoader(new QuestLoader), requiredLoader(new RequiredLoader), unitLoader(new UnitLoader)
{
	questLoader->dialogLoader = dialogLoader;
}

//=================================================================================================
Content::~Content()
{
	if(buildingLoader)
		Cleanup();
}

//=================================================================================================
void Content::Cleanup()
{
	delete abilityLoader;
	delete buildingLoader;
	delete classLoader;
	delete dialogLoader;
	delete itemLoader;
	delete locationLoader;
	delete musicLoader;
	delete objectLoader;
	delete perkLoader;
	delete questLoader;
	delete requiredLoader;
	delete unitLoader;
	buildingLoader = nullptr;
}

//=================================================================================================
void Content::LoadContent(delegate<void(Id)> callback)
{
	LoadVersion();

	Info("Game: Loading items.");
	callback(Id::Items);
	itemLoader->DoLoading();

	Info("Game: Loading dialogs.");
	callback(Id::Dialogs);
	dialogLoader->DoLoading();

	Info("Game: Loading objects.");
	callback(Id::Objects);
	objectLoader->DoLoading();

	Info("Game: Loading locations.");
	callback(Id::Locations);
	locationLoader->DoLoading();

	Info("Game: Loading abilities.");
	callback(Id::Abilities);
	abilityLoader->DoLoading();

	Info("Game: Loading perks.");
	callback(Id::Perks);
	perkLoader->DoLoading();

	Info("Game: Loading classes.");
	callback(Id::Classes);
	classLoader->DoLoading();

	Info("Game: Loading units.");
	callback(Id::Units);
	unitLoader->DoLoading();
	classLoader->ApplyUnits();
	abilityLoader->ApplyUnits();

	Info("Game: Loading buildings.");
	callback(Id::Buildings);
	buildingLoader->DoLoading();

	Info("Game: Loading music.");
	callback(Id::Musics);
	musicLoader->DoLoading();

	Info("Game: Loading quests.");
	callback(Id::Quests);
	questLoader->DoLoading();
	unitLoader->ProcessDialogRequests();

	callback(Id::Required);
	requiredLoader->DoLoading();

	Cleanup();
}

//=================================================================================================
void Content::LoadVersion()
{
	cstring path = Format("%s/system.txt", systemDir.c_str());
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
	AbilityLoader::Cleanup();
	BuildingLoader::Cleanup();
	ClassLoader::Cleanup();
	DialogLoader::Cleanup();
	ItemLoader::Cleanup();
	LocationLoader::Cleanup();
	MusicListLoader::Cleanup();
	ObjectLoader::Cleanup();
	PerkLoader::Cleanup();
	QuestLoader::Cleanup();
	UnitLoader::Cleanup();
}

//=================================================================================================
void Content::WriteCrc(BitStreamWriter& f)
{
	f << crc;
}

//=================================================================================================
void Content::ReadCrc(BitStreamReader& f)
{
	f >> clientCrc;
}

//=================================================================================================
bool Content::GetCrc(Id type, uint& myCrc, cstring& typeCrc)
{
	if(type < (Id)0 || type >= Id::Max)
		return false;

	myCrc = crc[(int)type];
	typeCrc = content_id[(int)type];
	return true;
}

//=================================================================================================
bool Content::ValidateCrc(Id& type, uint& myCrc, uint& playerCrc, cstring& typeStr)
{
	for(uint i = 0; i < (uint)Id::Max; ++i)
	{
		if(crc[i] != clientCrc[i])
		{
			type = (Id)i;
			myCrc = crc[i];
			playerCrc = clientCrc[i];
			typeStr = content_id[i];
			return false;
		}
	}

	return true;
}
