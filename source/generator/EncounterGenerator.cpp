#include "Pch.h"
#include "EncounterGenerator.h"

#include "AIManager.h"
#include "AITeam.h"
#include "Chest.h"
#include "Encounter.h"
#include "Game.h"
#include "ItemHelper.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "Quest_Scripted.h"
#include "ScriptManager.h"
#include "UnitGroup.h"
#include "World.h"

#include <Perlin.h>
#include <Terrain.h>

//=================================================================================================
int EncounterGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	steps += 4; // txGeneratingObjects, txRecreatingObjects, txGeneratingUnits, txGeneratingItems
	return steps;
}

//=================================================================================================
void EncounterGenerator::Generate()
{
	enterDir = (GameDirection)(Rand() % 4);

	CreateMap();
	RandomizeTerrainTexture();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
		{
			if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
				h[x + y * (s + 1)] += Random(10.f, 15.f);
		}
	}

	// create road
	if(enterDir == GDIR_LEFT || enterDir == GDIR_RIGHT)
	{
		for(uint y = 62; y < 66; ++y)
		{
			for(uint x = 0; x < s; ++x)
			{
				outside->tiles[x + y * s].Set(TT_SAND, TM_ROAD);
				h[x + y * (s + 1)] = 1.f;
			}
		}
		for(uint x = 0; x < s; ++x)
		{
			h[x + 61 * (s + 1)] = 1.f;
			h[x + 66 * (s + 1)] = 1.f;
			h[x + 67 * (s + 1)] = 1.f;
		}
	}
	else
	{
		for(uint y = 0; y < s; ++y)
		{
			for(uint x = 62; x < 66; ++x)
			{
				outside->tiles[x + y * s].Set(TT_SAND, TM_ROAD);
				h[x + y * (s + 1)] = 1.f;
			}
		}
		for(uint y = 0; y < s; ++y)
		{
			h[61 + y * (s + 1)] = 1.f;
			h[66 + y * (s + 1)] = 1.f;
			h[67 + y * (s + 1)] = 1.f;
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	Perlin perlin(4, 4);
	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	// flatten road
	if(enterDir == GDIR_LEFT || enterDir == GDIR_RIGHT)
	{
		for(uint y = 61; y <= 67; ++y)
		{
			for(uint x = 1; x < s - 1; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1) * (s + 1)] + h[x + (y + 1) * (s + 1)]) / 5;
		}
	}
	else
	{
		for(uint y = 1; y < s - 1; ++y)
		{
			for(uint x = 61; x <= 67; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1) * (s + 1)] + h[x + (y + 1) * (s + 1)]) / 5;
		}
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
void EncounterGenerator::OnEnter()
{
	outside->loadedResources = false;
	gameLevel->Apply();

	ApplyTiles();
	SetOutsideParams();

	// generate objects
	game->LoadingStep(game->txGeneratingObjects);
	SpawnForestObjects((enterDir == GDIR_LEFT || enterDir == GDIR_RIGHT) ? 0 : 1);

	// create colliders
	game->LoadingStep(game->txRecreatingObjects);
	gameLevel->SpawnTerrainCollider();
	SpawnOutsideBariers();

	// generate units
	game->LoadingStep(game->txGeneratingUnits);
	GameDialog* dialog;
	Unit* talker;
	Quest* quest;
	SpawnEncounterUnits(dialog, talker, quest);

	// generate items
	game->LoadingStep(game->txGeneratingItems);
	SpawnForestItems(-1);

	// generate minimap
	game->LoadingStep(game->txGeneratingMinimap);
	CreateMinimap();

	// add team
	SpawnEncounterTeam();

	// auto talk with leader
	if(dialog)
		talker->OrderAutoTalk(true, dialog, quest);
}

//=================================================================================================
void EncounterGenerator::SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest)
{
	Vec3 lookPt;
	switch(enterDir)
	{
	case GDIR_RIGHT:
		lookPt = Vec3(133.f, 0.f, 128.f);
		break;
	case GDIR_UP:
		lookPt = Vec3(128.f, 0.f, 133.f);
		break;
	case GDIR_LEFT:
		lookPt = Vec3(123.f, 0.f, 128.f);
		break;
	case GDIR_DOWN:
		lookPt = Vec3(128.f, 0.f, 123.f);
		break;
	}

	LocationPart& locPart = *gameLevel->localPart;
	EncounterData encounter = world->GetCurrentEncounter();
	EncounterSpawn spawn(encounter.st);
	quest = nullptr;
	talker = nullptr;

	if(encounter.mode == ENCOUNTER_COMBAT)
	{
		if(encounter.group->id == "bandits")
		{
			spawn.dontAttack = true;
			spawn.dialog = GameDialog::TryGet("bandits");
		}
		else if(encounter.group->id == "animals")
		{
			if(Rand() % 3 != 0)
				spawn.essential = UnitData::Get("wild_hunter");
		}
		spawn.groupName = encounter.group->id.c_str();
		spawn.count = Random(3, 5);
	}
	else if(encounter.mode == ENCOUNTER_SPECIAL)
	{
		switch(encounter.special)
		{
		case SE_CRAZY_MAGE:
			spawn.essential = UnitData::Get("crazy_mage");
			spawn.groupName = nullptr;
			spawn.count = 1;
			spawn.level = Clamp(encounter.st * 2, 10, 16);
			spawn.dialog = GameDialog::TryGet("crazy_mage_encounter");
			break;
		case SE_CRAZY_HEROES:
			spawn.groupName = "crazies";
			spawn.count = Random(3, 4);
			if(spawn.level < 5)
				spawn.level = 5;
			spawn.dialog = GameDialog::TryGet("crazies_encounter");
			spawn.isTeam = true;
			break;
		case SE_MERCHANT:
			spawn.essential = UnitData::Get("traveling_merchant");
			spawn.groupName = "merchant_guards";
			spawn.count = Random(3, 4);
			spawn.level = Clamp(encounter.st, 5, 6);
			break;
		case SE_HEROES:
			spawn.groupName = "heroes";
			spawn.count = Random(3, 4);
			if(spawn.level < 5)
				spawn.level = 5;
			spawn.isTeam = true;
			break;
		case SE_BANDITS_VS_TRAVELERS:
			{
				spawn.farEncounter = true;
				spawn.groupName = "bandits";
				spawn.count = Random(4, 6);
				spawn.groupName2 = "wagon_guards";
				spawn.count2 = Random(2, 3);
				spawn.level2 = Clamp(encounter.st, 5, 6);
				gameLevel->SpawnObjectNearLocation(locPart, BaseObject::Get("wagon"), Vec2(128, 128), Random(MAX_ANGLE));
				Chest* chest = gameLevel->SpawnObjectNearLocation(locPart, BaseObject::Get("chest"), Vec2(128, 128), Random(MAX_ANGLE), 6.f);
				if(chest)
				{
					int gold;
					ItemHelper::GenerateTreasure(5, 5, chest->items, gold, false);
					InsertItemBare(chest->items, Item::gold, (uint)gold);
					SortItems(chest->items);
				}
				scriptMgr->GetVar("guards_enc_reward") = false;
			}
			break;
		case SE_HEROES_VS_ENEMIES:
			spawn.farEncounter = true;
			spawn.groupName = "heroes";
			spawn.count = Random(3, 4);
			if(spawn.level < 5)
				spawn.level = 5;
			spawn.isTeam = true;
			switch(Rand() % 4)
			{
			case 0:
				spawn.groupName2 = "bandits";
				spawn.count2 = Random(3, 5);
				break;
			case 1:
				spawn.groupName2 = "orcs";
				spawn.count2 = Random(3, 5);
				break;
			case 2:
				spawn.groupName2 = "goblins";
				spawn.count2 = Random(3, 5);
				break;
			case 3:
				spawn.groupName2 = "crazies";
				spawn.count2 = Random(3, 4);
				if(spawn.level2 < 5)
					spawn.level2 = 5;
				spawn.isTeam2 = true;
				break;
			}
			break;
		case SE_CRAZY_COOK:
			spawn.groupName = nullptr;
			spawn.essential = UnitData::Get("crazy_cook");
			spawn.level = -2;
			spawn.dialog = spawn.essential->dialog;
			spawn.count = 1;
			break;
		case SE_ENEMIES_COMBAT:
			{
				farEncounter = true;
				int groupIndex = Rand() % 4;
				int groupIndex2 = Rand() % 4;
				if(groupIndex == groupIndex2)
					groupIndex2 = (groupIndex2 + 1) % 4;
				spawn.level = Max(3, spawn.level + Random(-1, +1));
				switch(groupIndex)
				{
				case 0:
					spawn.groupName = "bandits";
					spawn.count = Random(3, 5);
					break;
				case 1:
					spawn.groupName = "orcs";
					spawn.count = Random(3, 5);
					break;
				case 2:
					spawn.groupName = "goblins";
					spawn.count = Random(3, 5);
					break;
				case 3:
					spawn.groupName = "crazies";
					spawn.count = Random(3, 4);
					if(spawn.level < 5)
						spawn.level = 5;
					spawn.isTeam = true;
					break;
				}
				spawn.level2 = Max(3, spawn.level2 + Random(-1, +1));
				switch(groupIndex2)
				{
				case 0:
					spawn.groupName2 = "bandits";
					spawn.count2 = Random(3, 5);
					break;
				case 1:
					spawn.groupName2 = "orcs";
					spawn.count2 = Random(3, 5);
					break;
				case 2:
					spawn.groupName2 = "goblins";
					spawn.count2 = Random(3, 5);
					break;
				case 3:
					spawn.groupName2 = "crazies";
					spawn.count2 = Random(3, 4);
					if(spawn.level2 < 5)
						spawn.level2 = 5;
					spawn.isTeam2 = true;
					break;
				}
			}
			break;
		case SE_TOMIR:
			spawn.essential = UnitData::Get("hero_tomir");
			spawn.dialog = GameDialog::TryGet("tomir");
			spawn.level = -10;
			break;
		}
	}
	else if(encounter.mode == ENCOUNTER_GLOBAL)
	{
		encounter.global->callback(spawn);
	}
	else
	{
		Encounter* enc = encounter.encounter;
		if(enc->group->id == "animals")
		{
			if(Rand() % 3 != 0)
				spawn.essential = UnitData::Get("wild_hunter");
		}
		spawn.groupName = enc->group->id.c_str();

		spawn.count = Random(3, 5);
		if(enc->st == -1)
			spawn.level = encounter.st;
		else
			spawn.level = enc->st;
		spawn.dialog = enc->dialog;
		spawn.dontAttack = enc->dontAttack;
		quest = enc->quest;
		gameLevel->eventHandler = enc->locationEventHandler;

		if(enc->scripted)
		{
			Quest_Scripted* q = (Quest_Scripted*)quest;
			ScriptEvent event(EVENT_ENCOUNTER);
			q->FireEvent(event);
			world->RemoveEncounter(enc->index);
		}
	}

	dialog = spawn.dialog;
	farEncounter = spawn.farEncounter;

	float bestDist;
	const float center = (float)OutsideLocation::size;
	Vec3 spawnPos(center, 0, center);
	if(spawn.backAttack)
	{
		const float dist = 12.f;
		switch(enterDir)
		{
		case GDIR_RIGHT:
			spawnPos = Vec3(center + dist, 0, center);
			break;
		case GDIR_UP:
			spawnPos = Vec3(center, 0.f, center + dist);
			break;
		case GDIR_LEFT:
			spawnPos = Vec3(center - dist, 0.f, center);
			break;
		case GDIR_DOWN:
			spawnPos = Vec3(center, 0.f, center - dist);
			break;
		}
	}

	// first group of units
	AITeam* team = spawn.isTeam ? aiMgr->CreateTeam() : nullptr;
	if(spawn.essential)
	{
		int unitLevel;
		if(spawn.level < 0)
			unitLevel = -spawn.level;
		else
			unitLevel = Clamp(spawn.essential->level.Random(), spawn.level / 2, spawn.level);
		talker = gameLevel->SpawnUnitNearLocation(locPart, spawnPos, *spawn.essential, &lookPt, unitLevel, 4.f);
		talker->dontAttack = spawn.dontAttack;
		bestDist = Vec3::Distance(talker->pos, lookPt);
		--spawn.count;
		if(team)
			team->Add(talker);
	}
	if(spawn.groupName)
	{
		UnitGroup* group = UnitGroup::TryGet(spawn.groupName);
		gameLevel->SpawnUnitsGroup(locPart, spawnPos, &lookPt, spawn.count, group, spawn.level, [&](Unit* u)
		{
			u->dontAttack = spawn.dontAttack;
			float dist = Vec3::Distance(u->pos, lookPt);
			if(!talker || dist < bestDist)
			{
				talker = u;
				bestDist = dist;
			}
			if(team)
				team->Add(u);
		});
	}
	if(team)
		team->SelectLeader();

	// second group of units
	if(spawn.groupName2)
	{
		team = (spawn.isTeam2 ? aiMgr->CreateTeam() : nullptr);
		UnitGroup* group = UnitGroup::TryGet(spawn.groupName2);
		gameLevel->SpawnUnitsGroup(locPart, spawnPos, &lookPt, spawn.count2, group, spawn.level2, [&](Unit* u)
		{
			u->dontAttack = spawn.dontAttack;
			if(team)
				team->Add(u);
		});
		if(team)
			team->SelectLeader();
	}
}

//=================================================================================================
void EncounterGenerator::SpawnEncounterTeam()
{
	Vec3 pos;
	float dir;

	const float center = (float)OutsideLocation::size;
	float dist = (farEncounter ? 12.f : 7.f);

	Vec3 lookPt;
	switch(enterDir)
	{
	default:
	case GDIR_RIGHT:
		pos = Vec3(center + dist, 0.f, center);
		dir = PI / 2;
		break;
	case GDIR_UP:
		pos = Vec3(center, 0.f, center + dist);
		dir = 0;
		break;
	case GDIR_LEFT:
		pos = Vec3(center - dist, 0.f, center);
		dir = 3.f / 2 * PI;
		break;
	case GDIR_DOWN:
		pos = Vec3(center, 0.f, center - dist);
		dir = PI;
		break;
	}

	gameLevel->AddPlayerTeam(pos, dir);
}
