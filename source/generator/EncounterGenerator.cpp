#include "Pch.h"
#include "EncounterGenerator.h"
#include "Encounter.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Terrain.h"
#include "Level.h"
#include "World.h"
#include "Chest.h"
#include "ScriptManager.h"
#include "Var.h"
#include "QuestManager.h"
#include "Quest_Crazies.h"
#include "Quest_Scripted.h"
#include "UnitGroup.h"
#include "ItemHelper.h"
#include "Game.h"

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
	enter_dir = (GameDirection)(Rand() % 4);

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
	if(enter_dir == GDIR_LEFT || enter_dir == GDIR_RIGHT)
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

	Perlin perlin(4, 4, 1);
	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	// flatten road
	if(enter_dir == GDIR_LEFT || enter_dir == GDIR_RIGHT)
	{
		for(uint y = 61; y <= 67; ++y)
		{
			for(uint x = 1; x < s - 1; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1)*(s + 1)] + h[x + (y + 1)*(s + 1)]) / 5;
		}
	}
	else
	{
		for(uint y = 1; y < s - 1; ++y)
		{
			for(uint x = 61; x <= 67; ++x)
				h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1)*(s + 1)] + h[x + (y + 1)*(s + 1)]) / 5;
		}
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

//=================================================================================================
void EncounterGenerator::OnEnter()
{
	outside->loaded_resources = false;
	game_level->Apply();

	ApplyTiles();
	game_level->SetOutsideParams();

	// generate objects
	game->LoadingStep(game->txGeneratingObjects);
	SpawnForestObjects((enter_dir == GDIR_LEFT || enter_dir == GDIR_RIGHT) ? 0 : 1);

	// create colliders
	game->LoadingStep(game->txRecreatingObjects);
	game_level->SpawnTerrainCollider();
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
	Vec3 look_pt;
	switch(enter_dir)
	{
	case GDIR_RIGHT:
		look_pt = Vec3(133.f, 0.f, 128.f);
		break;
	case GDIR_UP:
		look_pt = Vec3(128.f, 0.f, 133.f);
		break;
	case GDIR_LEFT:
		look_pt = Vec3(123.f, 0.f, 128.f);
		break;
	case GDIR_DOWN:
		look_pt = Vec3(128.f, 0.f, 123.f);
		break;
	}

	LevelArea& area = *game_level->local_area;
	EncounterData encounter = world->GetCurrentEncounter();
	UnitData* essential = nullptr;
	cstring group_name = nullptr, group_name2 = nullptr;
	bool dont_attack = false, back_attack = false, cursed_stone = false;
	int count = 0, level = encounter.st, count2 = 0, level2 = encounter.st;
	dialog = nullptr;
	quest = nullptr;
	far_encounter = false;

	if(encounter.mode == ENCOUNTER_COMBAT)
	{
		if(encounter.group->id == "bandits")
		{
			dont_attack = true;
			dialog = GameDialog::TryGet("bandits");
		}
		else if(encounter.group->id == "animals")
		{
			if(Rand() % 3 != 0)
				essential = UnitData::Get("wild_hunter");
		}
		group_name = encounter.group->id.c_str();
		count = Random(3, 5);
	}
	else if(encounter.mode == ENCOUNTER_SPECIAL)
	{
		switch(encounter.special)
		{
		case SE_CRAZY_MAGE:
			essential = UnitData::Get("crazy_mage");
			group_name = nullptr;
			count = 1;
			level = Clamp(encounter.st * 2, 10, 16);
			dialog = GameDialog::TryGet("crazy_mage_encounter");
			break;
		case SE_CRAZY_HEROES:
			group_name = "crazies";
			count = Random(3, 4);
			if(level < 5)
				level = 5;
			dialog = GameDialog::TryGet("crazies_encounter");
			break;
		case SE_MERCHANT:
			essential = UnitData::Get("merchant");
			group_name = "merchant_guards";
			count = Random(3, 4);
			level = Clamp(encounter.st, 5, 6);
			break;
		case SE_HEROES:
			group_name = "heroes";
			count = Random(3, 4);
			if(level < 5)
				level = 5;
			break;
		case SE_BANDITS_VS_TRAVELERS:
			{
				far_encounter = true;
				group_name = "bandits";
				count = Random(4, 6);
				group_name2 = "wagon_guards";
				count2 = Random(2, 3);
				level2 = Clamp(encounter.st, 5, 6);
				game_level->SpawnObjectNearLocation(area, BaseObject::Get("wagon"), Vec2(128, 128), Random(MAX_ANGLE));
				Chest* chest = game_level->SpawnObjectNearLocation(area, BaseObject::Get("chest"), Vec2(128, 128), Random(MAX_ANGLE), 6.f);
				if(chest)
				{
					int gold;
					ItemHelper::GenerateTreasure(5, 5, chest->items, gold, false);
					InsertItemBare(chest->items, Item::gold, (uint)gold);
					SortItems(chest->items);
				}
				script_mgr->GetVar("guards_enc_reward") = false;
			}
			break;
		case SE_HEROES_VS_ENEMIES:
			far_encounter = true;
			group_name = "heroes";
			count = Random(3, 4);
			if(level < 5)
				level = 5;
			switch(Rand() % 4)
			{
			case 0:
				group_name2 = "bandits";
				count2 = Random(3, 5);
				break;
			case 1:
				group_name2 = "orcs";
				count2 = Random(3, 5);
				break;
			case 2:
				group_name2 = "goblins";
				count2 = Random(3, 5);
				break;
			case 3:
				group_name2 = "crazies";
				count2 = Random(3, 4);
				if(level2 < 5)
					level2 = 5;
				break;
			}
			break;
		case SE_GOLEM:
			{
				group_name = nullptr;
				essential = UnitData::Get("q_magowie_golem");
				int pts = encounter.st * Random(1, 2);
				count = max(1, pts / 8);
				dont_attack = true;
				dialog = GameDialog::TryGet("q_mages");
			}
			break;
		case SE_CRAZY:
			group_name = nullptr;
			essential = UnitData::Get("q_szaleni_szaleniec");
			level = 13;
			dont_attack = true;
			dialog = GameDialog::TryGet("q_crazies");
			count = 1;
			quest_mgr->quest_crazies->check_stone = true;
			cursed_stone = true;
			break;
		case SE_UNK:
			group_name = "unk";
			level = 13;
			back_attack = true;
			if(quest_mgr->quest_crazies->crazies_state == Quest_Crazies::State::PickedStone)
			{
				quest_mgr->quest_crazies->crazies_state = Quest_Crazies::State::FirstAttack;
				count = 1;
				quest_mgr->quest_crazies->SetProgress(Quest_Crazies::Progress::Started);
			}
			else
			{
				int pts = encounter.st * Random(1, 3);
				count = max(1, pts / 13);
			}
			break;
		case SE_CRAZY_COOK:
			group_name = nullptr;
			essential = UnitData::Get("crazy_cook");
			level = -2;
			dialog = essential->dialog;
			count = 1;
			break;
		case SE_ENEMIES_COMBAT:
			{
				far_encounter = true;
				int group_index = Rand() % 4;
				int group_index2 = Rand() % 4;
				if(group_index == group_index2)
					group_index2 = (group_index2 + 1) % 4;
				level = Max(3, level + Random(-1, +1));
				switch(group_index)
				{
				case 0:
					group_name = "bandits";
					count = Random(3, 5);
					break;
				case 1:
					group_name = "orcs";
					count = Random(3, 5);
					break;
				case 2:
					group_name = "goblins";
					count = Random(3, 5);
					break;
				case 3:
					group_name = "crazies";
					count = Random(3, 4);
					if(level < 5)
						level = 5;
					break;
				}
				level2 = Max(3, level2 + Random(-1, +1));
				switch(group_index2)
				{
				case 0:
					group_name2 = "bandits";
					count2 = Random(3, 5);
					break;
				case 1:
					group_name2 = "orcs";
					count2 = Random(3, 5);
					break;
				case 2:
					group_name2 = "goblins";
					count2 = Random(3, 5);
					break;
				case 3:
					group_name2 = "crazies";
					count2 = Random(3, 4);
					if(level2 < 5)
						level2 = 5;
					break;
				}
			}
			break;
		case SE_TOMIR:
			essential = UnitData::Get("hero_tomir");
			dialog = GameDialog::TryGet("tomir");
			level = -10;
			break;
		}
	}
	else
	{
		Encounter* enc = encounter.encounter;
		if(enc->group->id == "animals")
		{
			if(Rand() % 3 != 0)
				essential = UnitData::Get("wild_hunter");
		}
		group_name = enc->group->id.c_str();

		count = Random(3, 5);
		if(enc->st == -1)
			level = encounter.st;
		else
			level = enc->st;
		dialog = enc->dialog;
		dont_attack = enc->dont_attack;
		quest = enc->quest;
		game_level->event_handler = enc->location_event_handler;

		if(enc->scripted)
		{
			Quest_Scripted* q = (Quest_Scripted*)quest;
			ScriptEvent event(EVENT_ENCOUNTER);
			q->FireEvent(event);
			world->RemoveEncounter(enc->index);
		}
	}

	talker = nullptr;
	float best_dist;

	const float center = (float)OutsideLocation::size;
	Vec3 spawn_pos(center, 0, center);
	if(back_attack)
	{
		const float dist = 12.f;
		switch(enter_dir)
		{
		case GDIR_RIGHT:
			spawn_pos = Vec3(center + dist, 0, center);
			break;
		case GDIR_UP:
			spawn_pos = Vec3(center, 0.f, center + dist);
			break;
		case GDIR_LEFT:
			spawn_pos = Vec3(center - dist, 0.f, center);
			break;
		case GDIR_DOWN:
			spawn_pos = Vec3(center, 0.f, center - dist);
			break;
		}
	}

	if(essential)
	{
		int unit_level;
		if(level < 0)
			unit_level = -level;
		else
			unit_level = Clamp(essential->level.Random(), level / 2, level);
		talker = game_level->SpawnUnitNearLocation(area, spawn_pos, *essential, &look_pt, unit_level, 4.f);
		talker->dont_attack = dont_attack;
		best_dist = Vec3::Distance(talker->pos, look_pt);
		--count;

		if(cursed_stone)
		{
			int slot = talker->FindItem(Item::Get("q_szaleni_kamien"));
			if(slot != -1)
				talker->items[slot].team_count = 0;
		}
	}

	// first group of units
	if(group_name)
	{
		UnitGroup* group = UnitGroup::TryGet(group_name);
		game_level->SpawnUnitsGroup(area, spawn_pos, &look_pt, count, group, level, [&](Unit* u)
		{
			u->dont_attack = dont_attack;
			float dist = Vec3::Distance(u->pos, look_pt);
			if(!talker || dist < best_dist)
			{
				talker = u;
				best_dist = dist;
			}
		});
	}

	// second group of units
	if(group_name2)
	{
		UnitGroup* group = UnitGroup::TryGet(group_name2);
		game_level->SpawnUnitsGroup(area, spawn_pos, &look_pt, count2, group, level2,
			[&](Unit* u) { u->dont_attack = dont_attack; });
	}
}

//=================================================================================================
void EncounterGenerator::SpawnEncounterTeam()
{
	Vec3 pos;
	float dir;

	const float center = (float)OutsideLocation::size;
	float dist = (far_encounter ? 12.f : 7.f);

	Vec3 look_pt;
	switch(enter_dir)
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

	game_level->AddPlayerTeam(pos, dir);
}
