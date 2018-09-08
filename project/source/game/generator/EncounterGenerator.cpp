#include "Pch.h"
#include "GameCore.h"
#include "EncounterGenerator.h"
#include "Encounter.h"
#include "OutsideLocation.h"
#include "Perlin.h"
#include "Terrain.h"
#include "Team.h"
#include "Level.h"
#include "World.h"
#include "Chest.h"
#include "ScriptManager.h"
#include "Var.h"
#include "QuestManager.h"
#include "Quest_Crazies.h"
#include "UnitGroup.h"
#include "Game.h"

int EncounterGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	steps += 4; // txGeneratingObjects, txRecreatingObjects, txGeneratingUnits, txGeneratingItems
	return steps;
}

void EncounterGenerator::Generate()
{
	// 0 - right, 1 - up, 2 - left, 3 - down
	enc_kierunek = Rand() % 4;

	CreateMap();

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
	if(enc_kierunek == 0 || enc_kierunek == 2)
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
	if(enc_kierunek == 0 || enc_kierunek == 2)
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

void EncounterGenerator::OnEnter()
{
	Game& game = Game::Get();
	outside->loaded_resources = false;
	game.city_ctx = nullptr;
	game.ApplyContext(outside, game.local_ctx);

	game.ApplyTiles(outside->h, outside->tiles);
	game.SetOutsideParams();

	// generate objects
	game.LoadingStep(game.txGeneratingObjects);
	SpawnForestObjects((enc_kierunek == 0 || enc_kierunek == 2) ? 0 : 1);

	// create colliders
	game.LoadingStep(game.txRecreatingObjects);
	game.SpawnTerrainCollider();
	L.SpawnOutsideBariers();

	// generate units
	game.LoadingStep(game.txGeneratingUnits);
	GameDialog* dialog;
	Unit* talker;
	Quest* quest;
	SpawnEncounterUnits(dialog, talker, quest);

	// generate items
	game.LoadingStep(game.txGeneratingItems);
	SpawnForestItems(-1);

	// generate minimap
	game.LoadingStep(game.txGeneratingMinimap);
	game.CreateForestMinimap();

	// add team
	SpawnEncounterTeam();
	if(dialog)
	{
		DialogContext& ctx = *Team.leader->player->dialog_ctx;
		game.StartDialog2(Team.leader->player, talker, dialog);
		ctx.dialog_quest = quest;
	}
}

void EncounterGenerator::SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest)
{
	Game& game = Game::Get();

	Vec3 look_pt;
	switch(enc_kierunek)
	{
	case 0:
		look_pt = Vec3(133.f, 0.f, 128.f);
		break;
	case 1:
		look_pt = Vec3(128.f, 0.f, 133.f);
		break;
	case 2:
		look_pt = Vec3(123.f, 0.f, 128.f);
		break;
	case 3:
		look_pt = Vec3(128.f, 0.f, 123.f);
		break;
	}

	EncounterData encounter = W.GetCurrentEncounter();
	UnitData* essential = nullptr;
	cstring group_name = nullptr, group_name2 = nullptr;
	bool dont_attack = false, od_tylu = false, kamien = false;
	int count, level, count2, level2;
	dialog = nullptr;
	quest = nullptr;
	far_encounter = false;

	if(encounter.mode == ENCOUNTER_COMBAT)
	{
		switch(encounter.enemy)
		{
		case -1:
			if(Rand() % 3 != 0)
				essential = UnitData::Get("wild_hunter");
			group_name = "animals";
			break;
		case SG_BANDITS:
			group_name = "bandits";
			dont_attack = true;
			dialog = FindDialog("bandits");
			break;
		case SG_GOBLINS:
			group_name = "goblins";
			break;
		case SG_ORCS:
			group_name = "orcs";
			break;
		}

		count = Random(3, 5);
		level = Random(6, 12);
	}
	else if(encounter.mode == ENCOUNTER_SPECIAL)
	{
		switch(encounter.special)
		{
		case SE_CRAZY_MAGE:
			essential = UnitData::Get("crazy_mage");
			group_name = nullptr;
			count = 1;
			level = Random(10, 16);
			dialog = FindDialog("crazy_mage_encounter");
			break;
		case SE_CRAZY_HEROES:
			group_name = "crazies";
			count = Random(2, 4);
			level = Random(2, 15);
			dialog = FindDialog("crazies_encounter");
			break;
		case SE_MERCHANT:
			{
				essential = UnitData::Get("merchant");
				group_name = "merchant_guards";
				count = Random(2, 4);
				level = Random(3, 8);
				game.GenerateMerchantItems(game.chest_merchant, 1000);
			}
			break;
		case SE_HEROES:
			group_name = "heroes";
			count = Random(2, 4);
			level = Random(2, 15);
			break;
		case SE_BANDITS_VS_TRAVELERS:
			{
				far_encounter = true;
				group_name = "bandits";
				count = Random(4, 6);
				level = Random(5, 10);
				group_name2 = "wagon_guards";
				count2 = Random(2, 3);
				level2 = Random(3, 8);
				game.SpawnObjectNearLocation(game.local_ctx, BaseObject::Get("wagon"), Vec2(128, 128), Random(MAX_ANGLE));
				Chest* chest = game.SpawnObjectNearLocation(game.local_ctx, BaseObject::Get("chest"), Vec2(128, 128), Random(MAX_ANGLE), 6.f);
				if(chest)
				{
					int gold;
					game.GenerateTreasure(5, 5, chest->items, gold, false);
					InsertItemBare(chest->items, game.gold_item_ptr, (uint)gold);
					SortItems(chest->items);
				}
				SM.GetVar("guards_enc_reward") = false;
			}
			break;
		case SE_HEROES_VS_ENEMIES:
			far_encounter = true;
			group_name = "heroes";
			count = Random(2, 4);
			level = Random(2, 15);
			switch(Rand() % 4)
			{
			case 0:
				group_name2 = "bandits";
				count2 = Random(3, 5);
				level2 = Random(6, 12);
				break;
			case 1:
				group_name2 = "orcs";
				count2 = Random(3, 5);
				level2 = Random(6, 12);
				break;
			case 2:
				group_name2 = "goblins";
				count2 = Random(3, 5);
				level2 = Random(6, 12);
				break;
			case 3:
				group_name2 = "crazies";
				count2 = Random(2, 4);
				level2 = Random(2, 15);
				break;
			}
			break;
		case SE_GOLEM:
			group_name = nullptr;
			essential = UnitData::Get("q_magowie_golem");
			level = 8;
			dont_attack = true;
			dialog = FindDialog("q_mages");
			count = 1;
			break;
		case SE_CRAZY:
			group_name = nullptr;
			essential = UnitData::Get("q_szaleni_szaleniec");
			level = 13;
			dont_attack = true;
			dialog = FindDialog("q_crazies");
			count = 1;
			QM.quest_crazies->check_stone = true;
			kamien = true;
			break;
		case SE_UNK:
			group_name = "unk";
			level = 13;
			od_tylu = true;
			if(QM.quest_crazies->crazies_state == Quest_Crazies::State::PickedStone)
			{
				QM.quest_crazies->crazies_state = Quest_Crazies::State::FirstAttack;
				count = 1;
				QM.quest_crazies->SetProgress(Quest_Crazies::Progress::Started);
			}
			else
				count = Random(1, 3);
			break;
		case SE_CRAZY_COOK:
			group_name = nullptr;
			essential = UnitData::Get("crazy_cook");
			level = -2;
			dialog = essential->dialog;
			count = 1;
			break;
		}
	}
	else
	{
		Encounter* enc = encounter.encounter;
		switch(enc->group)
		{
		case -1:
			if(Rand() % 3 != 0)
				essential = UnitData::Get("wild_hunter");
			group_name = "animals";
			break;
		case SG_BANDITS:
			group_name = "bandits";
			break;
		case SG_GOBLINS:
			group_name = "goblins";
			break;
		case SG_ORCS:
			group_name = "orcs";
			break;
		}

		count = Random(3, 5);
		level = Random(6, 12);
		dialog = enc->dialog;
		dont_attack = enc->dont_attack;
		quest = enc->quest;
		L.event_handler = enc->location_event_handler;
	}

	talker = nullptr;
	float dist, best_dist;

	Vec3 spawn_pos(128.f, 0, 128.f);
	if(od_tylu)
	{
		switch(enc_kierunek)
		{
		case 0:
			spawn_pos = Vec3(140.f, 0, 128.f);
			break;
		case 1:
			spawn_pos = Vec3(128.f, 0.f, 140.f);
			break;
		case 2:
			spawn_pos = Vec3(116.f, 0.f, 128.f);
			break;
		case 3:
			spawn_pos = Vec3(128.f, 0.f, 116.f);
			break;
		}
	}

	if(essential)
	{
		talker = game.SpawnUnitNearLocation(game.local_ctx, spawn_pos, *essential, &look_pt, Clamp(essential->level.Random(), level / 2, level), 4.f);
		talker->dont_attack = dont_attack;
		best_dist = Vec3::Distance(talker->pos, look_pt);
		--count;

		if(kamien)
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
		game.SpawnUnitsGroup(game.local_ctx, spawn_pos, &look_pt, count, group, level, [&](Unit* u)
		{
			u->dont_attack = dont_attack;
			dist = Vec3::Distance(u->pos, look_pt);
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
		game.SpawnUnitsGroup(game.local_ctx, spawn_pos, &look_pt, count2, group, level2,
			[&](Unit* u) { u->dont_attack = dont_attack; });
	}
}

void EncounterGenerator::SpawnEncounterTeam()
{
	assert(InRange(enc_kierunek, 0, 3));

	Vec3 pos;
	float dir;

	Vec3 look_pt;
	switch(enc_kierunek)
	{
	case 0:
		if(far_encounter)
			pos = Vec3(140.f, 0.f, 128.f);
		else
			pos = Vec3(135.f, 0.f, 128.f);
		dir = PI / 2;
		break;
	case 1:
		if(far_encounter)
			pos = Vec3(128.f, 0.f, 140.f);
		else
			pos = Vec3(128.f, 0.f, 135.f);
		dir = 0;
		break;
	case 2:
		if(far_encounter)
			pos = Vec3(116.f, 0.f, 128.f);
		else
			pos = Vec3(121.f, 0.f, 128.f);
		dir = 3.f / 2 * PI;
		break;
	case 3:
		if(far_encounter)
			pos = Vec3(128.f, 0.f, 116.f);
		else
			pos = Vec3(128.f, 0.f, 121.f);
		dir = PI;
		break;
	}

	Game::Get().AddPlayerTeam(pos, dir, false, true);
}
