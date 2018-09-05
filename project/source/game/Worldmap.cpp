#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "Terrain.h"
#include "CityGenerator.h"
#include "Inventory.h"
#include "Quest_Sawmill.h"
#include "Quest_Bandits.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "Perlin.h"
#include "LocationHelper.h"
#include "Content.h"
#include "QuestManager.h"
#include "Encounter.h"
#include "CaveLocation.h"
#include "Camp.h"
#include "MultiInsideLocation.h"
#include "WorldMapGui.h"
#include "GameGui.h"
#include "InfoBox.h"
#include "LoadScreen.h"
#include "MainMenu.h"
#include "MultiplayerPanel.h"
#include "AIController.h"
#include "Team.h"
#include "ItemContainer.h"
#include "BuildingScript.h"
#include "BuildingGroup.h"
#include "Stock.h"
#include "UnitGroup.h"
#include "Portal.h"
#include "World.h"
#include "Level.h"
#include "DirectX.h"
#include "ScriptManager.h"
#include "Var.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"

extern Matrix m1, m2, m3, m4;

void Game::GenerateWorld()
{
	if(next_seed != 0)
	{
		Srand(next_seed);
		next_seed = 0;
	}
	else if(force_seed != 0 && force_seed_all)
		Srand(force_seed);

	Info("Generating world, seed %u.", RandVal());

	W.GenerateWorld();

	Info("Randomness integrity: %d", RandVal());
}

bool Game::EnterLocation(int level, int from_portal, bool close_portal)
{
	Location& l = *L.location;
	if(l.type == L_ACADEMY)
	{
		ShowAcademyText();
		return false;
	}

	world_map->visible = false;
	game_gui->visible = true;

	const bool reenter = L.is_open;
	L.is_open = true;
	if(W.GetState() != World::State::INSIDE_ENCOUNTER)
		W.SetState(World::State::INSIDE_LOCATION);
	if(from_portal != -1)
		L.enter_from = ENTER_FROM_PORTAL + from_portal;
	else
		L.enter_from = ENTER_FROM_OUTSIDE;
	if(!reenter)
		L.light_angle = Random(PI * 2);

	dungeon_level = level;
	L.event_handler = nullptr;
	pc_data.before_player = BP_NONE;
	arena_free = true; //zabezpieczenie :3
	unit_views.clear();
	Inventory::lock = nullptr;

	bool first = false;
	int steps;

	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		first = true;
		level_generated = true;
	}
	else
		level_generated = false;

	if(!reenter)
		InitQuadTree();

	if(Net::IsOnline() && players > 1)
	{
		packet_data.resize(4);
		packet_data[0] = ID_CHANGE_LEVEL;
		packet_data[1] = (byte)L.location_index;
		packet_data[2] = 0;
		packet_data[3] = (W.GetState() == World::State::INSIDE_ENCOUNTER ? 1 : 0);
		int ack = peer->Send((cstring)&packet_data[0], 4, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
		StreamWrite(packet_data, Stream_TransferServer, UNASSIGNED_SYSTEM_ADDRESS);
		for(auto info : game_players)
		{
			if(info->id == my_id)
				info->state = PlayerInfo::IN_GAME;
			else
			{
				info->state = PlayerInfo::WAITING_FOR_RESPONSE;
				info->ack = ack;
				info->timer = 5.f;
			}
		}
		Net_FilterServerChanges();
	}

	// calculate number of loading steps for drawing progress bar
	steps = 3; // common txEnteringLocation, txGeneratingMinimap, txLoadingComplete
	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
		++steps; // txGeneratingMap
	switch(l.type)
	{
	case L_CITY:
		if(first)
			steps += 4; // txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems
		else if(!reenter)
		{
			steps += 2; // txGeneratingUnits, txGeneratingPhysics
			if(l.last_visit != W.GetWorldtime())
				++steps; // txGeneratingItems
		}
		if(!reenter)
			++steps; // txRecreatingObjects
		break;
	case L_DUNGEON:
	case L_CRYPT:
	case L_CAVE:
		if(first)
			steps += 2; // txGeneratingObjects, txGeneratingUnits
		else if(!reenter)
			++steps; // txRegeneratingLevel
		break;
	case L_FOREST:
	case L_CAMP:
	case L_MOONWELL:
		if(first)
			steps += 3; // txGeneratingObjects, txGeneratingUnits, txGeneratingItems
		else if(!reenter)
			steps += 2; // txGeneratingUnits, txGeneratingPhysics
		if(!reenter)
			++steps; // txRecreatingObjects
		break;
	case L_ENCOUNTER:
		steps += 4; // txGeneratingObjects, txRecreatingObjects, txGeneratingUnits, txGeneratingItems
		break;
	default:
		assert(0);
		steps = 10;
		break;
	}

	LoadingStart(steps);
	LoadingStep(txEnteringLocation);

	// generate map on first visit
	if(l.state != LS_ENTERED && l.state != LS_CLEARED)
	{
		if(next_seed != 0)
		{
			Srand(next_seed);
			next_seed = 0;
		}
		else if(force_seed != 0 && force_seed_all)
			Srand(force_seed);

		// log what is required to generate location for testing
		if(l.type == L_CITY)
		{
			City* city = (City*)&l;
			Info("Generating location '%s', seed %u [%d].", l.name.c_str(), RandVal(), city->citizens);
		}
		else
			Info("Generating location '%s', seed %u.", l.name.c_str(), RandVal());
		l.seed = RandVal();
		if(!l.outside)
		{
			InsideLocation* inside = (InsideLocation*)L.location;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				multi->infos[dungeon_level].seed = l.seed;
			}
		}

		// nie odwiedzono, trzeba wygenerowaæ
		l.state = LS_ENTERED;

		LoadingStep(txGeneratingMap);

		switch(l.type)
		{
		case L_CITY:
			if(LocationHelper::IsCity(l))
				GenerateCityMap(l);
			else
				GenerateVillageMap(l);
			break;
		case L_DUNGEON:
		case L_CRYPT:
			{
				InsideLocation* inside = (InsideLocation*)L.location;
				inside->SetActiveLevel(0);
				if(inside->IsMultilevel())
				{
					MultiInsideLocation* multi = (MultiInsideLocation*)inside;
					if(multi->generated == 0)
						++multi->generated;
				}
				Info("Generating dungeon, target %d.", inside->target);
				GenerateDungeon(l);
			}
			break;
		case L_CAVE:
			GenerateCave(l);
			break;
		case L_FOREST:
			if(L.location_index == QM.quest_secret->where2)
				GenerateSecretLocation(l);
			else
				GenerateForest(l);
			break;
		case L_ENCOUNTER:
			GenerateEncounterMap(l);
			break;
		case L_CAMP:
			GenerateCamp(l);
			break;
		case L_MOONWELL:
			GenerateMoonwell(l);
			break;
		default:
			assert(0);
			break;
		}
	}
	else if(l.type != L_DUNGEON && l.type != L_CRYPT)
		Info("Entering location '%s'.", l.name.c_str());

	city_ctx = nullptr;

	switch(l.type)
	{
	case L_CITY:
		{
			// ustaw wskaŸniki
			City* city = (City*)L.location;
			city_ctx = city;
			arena_free = true;

			if(!reenter)
			{
				// ustaw kontekst
				ApplyContext(city, local_ctx);
				// ustaw teren
				ApplyTiles(city->h, city->tiles);
			}

			SetOutsideParams();

			// czy to pierwsza wizyta?
			if(first)
			{
				// stwórz budynki
				LoadingStep(txGeneratingBuildings);
				SpawnBuildings(city->buildings);

				// generuj obiekty
				LoadingStep(txGeneratingObjects);
				SpawnCityObjects();

				// generuj jednostki
				LoadingStep(txGeneratingUnits);
				SpawnUnits(city);
				SpawnTmpUnits(city);

				// generuj przedmioty
				LoadingStep(txGeneratingItems);
				GenerateStockItems();
				GenerateCityPickableItems();
				if(city->IsVillage())
					SpawnForestItems(-2);
			}
			else if(!reenter)
			{
				// resetowanie bohaterów/questowych postaci
				if(city->reset)
					RemoveTmpUnits(city);

				// usuwanie krwii/zw³ok
				int days;
				city->CheckUpdate(days, W.GetWorldtime());
				if(days > 0)
					UpdateLocation(days, 100, false);

				for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
				{
					if((*it)->ctx.require_tmp_ctx && !(*it)->ctx.tmp_ctx)
						(*it)->ctx.SetTmpCtx(tmp_ctx_pool.Get());
				}

				// odtwórz jednostki
				LoadingStep(txGeneratingUnits);
				RespawnUnits();
				RepositionCityUnits();

				// odtwórz fizykê
				LoadingStep(txGeneratingPhysics);
				RespawnObjectColliders();
				RespawnBuildingPhysics();

				if(city->reset)
				{
					SpawnTmpUnits(city);
					city->reset = false;
				}

				// generuj przedmioty
				if(days > 0)
				{
					LoadingStep(txGeneratingItems);
					GenerateStockItems();
					if(days >= 10)
					{
						GenerateCityPickableItems();
						if(city->IsVillage())
							SpawnForestItems(-2);
					}
				}

				OnReenterLevel(local_ctx);
				for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
					OnReenterLevel((*it)->ctx);
			}

			if(!reenter)
			{
				// stwórz obiekt kolizji terenu
				LoadingStep(txRecreatingObjects);
				SpawnTerrainCollider();
				SpawnCityPhysics();
				SpawnOutsideBariers();
			}

			// pojawianie sie questowej jednostki
			if(L.location->active_quest && L.location->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER && !L.location->active_quest->done)
				HandleQuestEvent(L.location->active_quest);

			// generuj minimapê
			LoadingStep(txGeneratingMinimap);
			CreateCityMinimap();

			// dodaj gracza i jego dru¿ynê
			Vec3 spawn_pos;
			float spawn_dir;
			city->GetEntry(spawn_pos, spawn_dir);
			AddPlayerTeam(spawn_pos, spawn_dir, reenter, true);

			if(!reenter)
				GenerateQuestUnits();

			for(Unit* unit : Team.members)
			{
				if(unit->IsHero())
					unit->hero->lost_pvp = false;
			}

			CheckTeamItemShares();

			arena_free = true;

			Quest_Contest* contest = QM.quest_contest;
			if(!contest->generated && L.location_index == contest->where && contest->state == Quest_Contest::CONTEST_TODAY)
				SpawnDrunkmans();
		}
		break;
	case L_DUNGEON:
	case L_CRYPT:
	case L_CAVE:
		EnterLevel(first, reenter, false, from_portal, true);
		break;
	case L_FOREST:
		{
			// ustaw wskaŸniki
			OutsideLocation* forest = (OutsideLocation*)L.location;
			city_ctx = nullptr;
			if(!reenter)
				ApplyContext(forest, local_ctx);

			int days;
			bool need_reset = forest->CheckUpdate(days, W.GetWorldtime());

			// ustaw teren
			if(!reenter)
				ApplyTiles(forest->h, forest->tiles);

			SetOutsideParams();

			if(QM.quest_secret->where2 == L.location_index)
			{
				// czy to pierwsza wizyta?
				if(first)
				{
					// generuj obiekty
					LoadingStep(txGeneratingObjects);
					SpawnSecretLocationObjects();

					// generuj jednostki
					LoadingStep(txGeneratingUnits);
					SpawnSecretLocationUnits();

					// generate items
					LoadingStep(txGeneratingItems);
					SpawnForestItems(0);
				}
				else if(!reenter)
				{
					// odtwórz jednostki
					LoadingStep(txGeneratingUnits);
					RespawnUnits();

					// odtwórz fizykê
					LoadingStep(txGeneratingPhysics);
					RespawnObjectColliders();

					OnReenterLevel(local_ctx);
				}

				// stwórz obiekt kolizji terenu
				if(!reenter)
				{
					LoadingStep(txRecreatingObjects);
					SpawnTerrainCollider();
					SpawnOutsideBariers();
				}

				// generuj minimapê
				LoadingStep(txGeneratingMinimap);
				CreateForestMinimap();

				// dodaj gracza i jego dru¿ynê
				SpawnTeamSecretLocation();
			}
			else
			{
				Vec3 pos;
				float dir;
				W.GetOutsideSpawnPoint(pos, dir);

				// czy to pierwsza wizyta?
				if(first)
				{
					// generuj obiekty
					LoadingStep(txGeneratingObjects);
					SpawnForestObjects();

					// generuj jednostki
					LoadingStep(txGeneratingUnits);
					SpawnForestUnits(pos);

					// generate items
					LoadingStep(txGeneratingItems);
					SpawnForestItems(0);
				}
				else if(!reenter)
				{
					if(days > 0)
						UpdateLocation(days, 100, false);

					if(need_reset)
					{
						// usuñ ¿ywe jednostki
						for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
						{
							if((*it)->IsAlive())
							{
								delete *it;
								*it = nullptr;
							}
						}
						RemoveNullElements(local_ctx.units);
					}

					LoadingStep(txGeneratingUnits);
					bool have_sawmill = false;
					if(L.location_index == QM.quest_sawmill->target_loc)
					{
						// sawmill quest
						if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::InBuild
							&& QM.quest_sawmill->build_state == Quest_Sawmill::BuildState::LumberjackLeft)
						{
							GenerateSawmill(true);
							have_sawmill = true;
							L.location->loaded_resources = false;
						}
						else if(QM.quest_sawmill->sawmill_state == Quest_Sawmill::State::Working
							&& QM.quest_sawmill->build_state != Quest_Sawmill::BuildState::Finished)
						{
							GenerateSawmill(false);
							have_sawmill = true;
							L.location->loaded_resources = false;
						}
						else
							RespawnUnits();
					}
					else
					{
						// respawn units
						RespawnUnits();
					}

					// odtwórz fizykê
					LoadingStep(txGeneratingPhysics);
					RespawnObjectColliders();

					if(need_reset)
					{
						// nowe jednostki
						SpawnForestUnits(pos);

						// spawn new ground items
						SpawnForestItems(have_sawmill ? -1 : 0);
					}

					OnReenterLevel(local_ctx);
				}

				// stwórz obiekt kolizji terenu
				if(!reenter)
				{
					LoadingStep(txRecreatingObjects);
					SpawnTerrainCollider();
					SpawnOutsideBariers();
				}

				// questowe rzeczy
				if(forest->active_quest && forest->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
				{
					Quest_Event* event = forest->active_quest->GetEvent(L.location_index);
					if(event)
					{
						if(!event->done)
							HandleQuestEvent(event);
						L.event_handler = event->location_event_handler;
					}
				}

				// generuj minimapê
				LoadingStep(txGeneratingMinimap);
				CreateForestMinimap();

				// dodaj gracza i jego dru¿ynê
				AddPlayerTeam(pos, dir, reenter, true);
			}
		}
		break;
	case L_ENCOUNTER:
		{
			// ustaw wskaŸniki
			OutsideLocation* enc = (OutsideLocation*)L.location;
			enc->loaded_resources = false;
			city_ctx = nullptr;
			ApplyContext(enc, local_ctx);

			// ustaw teren
			ApplyTiles(enc->h, enc->tiles);

			SetOutsideParams();

			// generuj obiekty
			LoadingStep(txGeneratingObjects);
			SpawnEncounterObjects();

			// stwórz obiekt kolizji terenu
			LoadingStep(txRecreatingObjects);
			SpawnTerrainCollider();
			SpawnOutsideBariers();

			// generuj jednostki
			LoadingStep(txGeneratingUnits);
			GameDialog* dialog;
			Unit* talker;
			Quest* quest;
			SpawnEncounterUnits(dialog, talker, quest);

			// generate items
			LoadingStep(txGeneratingItems);
			SpawnForestItems(-1);

			// generuj minimapê
			LoadingStep(txGeneratingMinimap);
			CreateForestMinimap();

			// dodaj gracza i jego dru¿ynê
			SpawnEncounterTeam();
			if(dialog)
			{
				DialogContext& ctx = *Team.leader->player->dialog_ctx;
				StartDialog2(Team.leader->player, talker, dialog);
				ctx.dialog_quest = quest;
			}
		}
		break;
	case L_CAMP:
		{
			// ustaw wskaŸniki
			OutsideLocation* camp = (OutsideLocation*)L.location;
			city_ctx = nullptr;
			if(!reenter)
				ApplyContext(camp, local_ctx);

			int days;
			camp->CheckUpdate(days, W.GetWorldtime());

			// ustaw teren
			if(!reenter)
				ApplyTiles(camp->h, camp->tiles);

			Vec3 pos;
			float dir;
			W.GetOutsideSpawnPoint(pos, dir);

			SetOutsideParams();

			// czy to pierwsza wizyta?
			if(first)
			{
				// generuj obiekty
				LoadingStep(txGeneratingObjects);
				SpawnCampObjects();

				// generuj jednostki
				LoadingStep(txGeneratingUnits);
				SpawnCampUnits();

				// generate items
				LoadingStep(txGeneratingItems);
				SpawnForestItems(-1);
			}
			else if(!reenter)
			{
				if(days > 0)
					UpdateLocation(days, 100, false);

				// odtwórz jednostki
				LoadingStep(txGeneratingUnits);
				RespawnUnits();

				if(days > 10)
					SpawnForestItems(-1);

				// odtwórz fizykê
				LoadingStep(txGeneratingPhysics);
				RespawnObjectColliders();

				OnReenterLevel(local_ctx);
			}

			// stwórz obiekt kolizji terenu
			if(!reenter)
			{
				LoadingStep(txRecreatingObjects);
				SpawnTerrainCollider();
				SpawnOutsideBariers();
			}

			// questowe rzeczy
			if(camp->active_quest && camp->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
			{
				Quest_Event* event = camp->active_quest->GetEvent(L.location_index);
				if(event)
				{
					if(!event->done)
						HandleQuestEvent(event);
					L.event_handler = event->location_event_handler;
				}
			}

			// generuj minimapê
			LoadingStep(txGeneratingMinimap);
			CreateForestMinimap();

			// dodaj gracza i jego dru¿ynê
			AddPlayerTeam(pos, dir, reenter, true);

			// generate guards for bandits quest
			if(QM.quest_bandits->bandits_state == Quest_Bandits::State::GenerateGuards && L.location_index == QM.quest_bandits->target_loc)
			{
				QM.quest_bandits->bandits_state = Quest_Bandits::State::GeneratedGuards;
				UnitData* ud = UnitData::Get("guard_q_bandyci");
				int ile = Random(4, 5);
				pos += Vec3(sin(dir + PI) * 8, 0, cos(dir + PI) * 8);
				for(int i = 0; i < ile; ++i)
				{
					Unit* u = SpawnUnitNearLocation(local_ctx, pos, *ud, &Team.leader->pos, 6, 4.f);
					u->assist = true;
				}
			}
		}
		break;
	case L_MOONWELL:
		{
			// ustaw wskaŸniki
			OutsideLocation* forest = (OutsideLocation*)L.location;
			city_ctx = nullptr;
			if(!reenter)
				ApplyContext(forest, local_ctx);

			int days;
			bool need_reset = forest->CheckUpdate(days, W.GetWorldtime());

			// ustaw teren
			if(!reenter)
				ApplyTiles(forest->h, forest->tiles);

			Vec3 pos;
			float dir;
			W.GetOutsideSpawnPoint(pos, dir);
			SetOutsideParams();

			// czy to pierwsza wizyta?
			if(first)
			{
				// generuj obiekty
				LoadingStep(txGeneratingObjects);
				SpawnMoonwellObjects();

				// generuj jednostki
				LoadingStep(txGeneratingUnits);
				SpawnMoonwellUnits(pos);

				// generate items
				LoadingStep(txGeneratingItems);
				SpawnForestItems(1);
			}
			else if(!reenter)
			{
				if(days > 0)
					UpdateLocation(days, 100, false);

				if(need_reset)
				{
					// usuñ ¿ywe jednostki
					for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
					{
						if((*it)->IsAlive())
						{
							delete *it;
							*it = nullptr;
						}
					}
					RemoveNullElements(local_ctx.units);
				}

				// odtwórz jednostki
				LoadingStep(txGeneratingUnits);
				RespawnUnits();

				// odtwórz fizykê
				LoadingStep(txGeneratingPhysics);
				RespawnObjectColliders();

				if(need_reset)
				{
					// nowe jednostki
					SpawnMoonwellUnits(pos);
				}

				if(days > 10)
					SpawnForestItems(1);

				OnReenterLevel(local_ctx);
			}

			// stwórz obiekt kolizji terenu
			if(!reenter)
			{
				LoadingStep(txRecreatingObjects);
				SpawnTerrainCollider();
				SpawnOutsideBariers();
			}

			// generuj minimapê
			LoadingStep(txGeneratingMinimap);
			CreateForestMinimap();

			// dodaj gracza i jego dru¿ynê
			AddPlayerTeam(pos, dir, reenter, true);
		}
		break;
	default:
		assert(0);
		break;
	}

	if(L.location->outside)
	{
		SetTerrainTextures();
		CalculateQuadtree();
	}

	bool loaded_resources = RequireLoadingResources(L.location, nullptr);
	LoadResources(txLoadingComplete, false);

	l.last_visit = W.GetWorldtime();
	CheckIfLocationCleared();
	local_ctx_valid = true;
	cam.Reset();
	pc_data.rot_buf = 0.f;
	SetMusic();

	if(close_portal)
	{
		delete L.location->portal;
		L.location->portal = nullptr;
	}

	if(Net::IsOnline())
	{
		net_mode = NM_SERVER_SEND;
		net_state = NetState::Server_Send;
		if(players > 1)
		{
			net_stream.Reset();
			PrepareLevelData(net_stream, loaded_resources);
			Info("Generated location packet: %d.", net_stream.GetNumberOfBytesUsed());
		}
		else
			game_players[0]->state = PlayerInfo::IN_GAME;

		info_box->Show(txWaitingForPlayers);
	}
	else
	{
		clear_color = clear_color2;
		game_state = GS_LEVEL;
		load_screen->visible = false;
		main_menu->visible = false;
		game_gui->visible = true;
	}

	if(L.location->outside)
	{
		OnEnterLevelOrLocation();
		OnEnterLocation();
	}

	Info("Randomness integrity: %d", RandVal());
	Info("Entered location.");

	return true;
}

void Game::ApplyTiles(float* _h, TerrainTile* _tiles)
{
	assert(_h && _tiles);

	TEX splat = terrain->GetSplatTexture();
	D3DLOCKED_RECT lock;
	V(splat->LockRect(0, &lock, nullptr, 0));
	byte* bits = (byte*)lock.pBits;
	for(uint y = 0; y < 256; ++y)
	{
		DWORD* row = (DWORD*)(bits + lock.Pitch * y);
		for(uint x = 0; x < 256; ++x, ++row)
		{
			TerrainTile& t = _tiles[x / 2 + y / 2 * OutsideLocation::size];
			if(t.alpha == 0)
				*row = terrain_tile_info[t.t].mask;
			else
			{
				const TerrainTileInfo& tti1 = terrain_tile_info[t.t];
				const TerrainTileInfo& tti2 = terrain_tile_info[t.t2];
				if(tti1.shift > tti2.shift)
					*row = tti2.mask + ((255 - t.alpha) << tti1.shift);
				else
					*row = tti1.mask + (t.alpha << tti2.shift);
			}
		}
	}
	V(splat->UnlockRect(0));
	splat->GenerateMipSubLevels();

	terrain->SetHeightMap(_h);
	terrain->Rebuild();
	terrain->CalculateBox();
}

Game::ObjectEntity Game::SpawnObjectEntity(LevelContext& ctx, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* out_point,
	int variant)
{
	if(IS_SET(base->flags, OBJ_TABLE_SPAWNER))
	{
		// table & stools
		BaseObject* table = BaseObject::Get(Rand() % 2 == 0 ? "table" : "table2");
		BaseUsable* stool = BaseUsable::Get("stool");

		// table
		Object* o = new Object;
		o->mesh = table->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = 1;
		o->base = table;
		ctx.objects->push_back(o);
		SpawnObjectExtras(ctx, table, pos, rot, o);

		// stools
		int count = Random(2, 4);
		int d[4] = { 0,1,2,3 };
		for(int i = 0; i < 4; ++i)
			std::swap(d[Rand() % 4], d[Rand() % 4]);

		for(int i = 0; i < count; ++i)
		{
			float sdir, slen;
			switch(d[i])
			{
			case 0:
				sdir = 0.f;
				slen = table->size.y + 0.3f;
				break;
			case 1:
				sdir = PI / 2;
				slen = table->size.x + 0.3f;
				break;
			case 2:
				sdir = PI;
				slen = table->size.y + 0.3f;
				break;
			case 3:
				sdir = PI * 3 / 2;
				slen = table->size.x + 0.3f;
				break;
			default:
				assert(0);
				break;
			}

			sdir += rot;

			Usable* u = new Usable;
			ctx.usables->push_back(u);
			u->base = stool;
			u->pos = pos + Vec3(sin(sdir)*slen, 0, cos(sdir)*slen);
			u->rot = sdir;
			u->user = nullptr;
			if(Net::IsOnline())
				u->netid = Usable::netid_counter++;

			SpawnObjectExtras(ctx, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IS_SET(base->flags, OBJ_BUILDING))
	{
		// building
		int roti;
		if(Equal(rot, 0))
			roti = 0;
		else if(Equal(rot, PI / 2))
			roti = 1;
		else if(Equal(rot, PI))
			roti = 2;
		else if(Equal(rot, PI * 3 / 2))
			roti = 3;
		else
		{
			assert(0);
			roti = 0;
			rot = 0.f;
		}

		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		ctx.objects->push_back(o);

		ProcessBuildingObjects(ctx, nullptr, nullptr, o->mesh, nullptr, rot, roti, pos, nullptr, nullptr, false, out_point);

		return o;
	}
	else if(IS_SET(base->flags, OBJ_USABLE))
	{
		// usable object
		BaseUsable* base_use = (BaseUsable*)base;

		Usable* u = new Usable;
		u->base = base_use;
		u->pos = pos;
		u->rot = rot;
		u->user = nullptr;

		if(IS_SET(base_use->use_flags, BaseUsable::CONTAINER))
		{
			u->container = new ItemContainer;
			auto item = GetRandomBook();
			if(item)
				u->container->items.push_back({ item, 1, 1 });
		}

		if(variant == -1)
		{
			if(base->variants)
			{
				// extra code for bench
				if(IS_SET(base_use->use_flags, BaseUsable::IS_BENCH))
				{
					switch(L.location->type)
					{
					case L_CITY:
						variant = 0;
						break;
					case L_DUNGEON:
					case L_CRYPT:
						variant = Rand() % 2;
						break;
					default:
						variant = Rand() % 2 + 2;
						break;
					}
				}
				else
					variant = Random<int>(base->variants->entries.size() - 1);
			}
		}
		u->variant = variant;

		if(Net::IsOnline())
			u->netid = Usable::netid_counter++;
		ctx.usables->push_back(u);

		SpawnObjectExtras(ctx, base, pos, rot, u, scale, flags);

		return u;
	}
	else if(IS_SET(base->flags, OBJ_IS_CHEST))
	{
		// chest
		Chest* chest = new Chest;
		chest->mesh_inst = new MeshInstance(base->mesh);
		chest->rot = rot;
		chest->pos = pos;
		chest->handler = nullptr;
		chest->looted = false;
		ctx.chests->push_back(chest);
		if(Net::IsOnline())
			chest->netid = Chest::netid_counter++;

		SpawnObjectExtras(ctx, base, pos, rot, nullptr, scale, flags);

		return chest;
	}
	else
	{
		// normal object
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		ctx.objects->push_back(o);

		SpawnObjectExtras(ctx, base, pos, rot, o, scale, flags);

		return o;
	}
}

void Game::SpawnBuildings(vector<CityBuilding>& _buildings)
{
	City* city = (City*)L.location;

	const int mur1 = int(0.15f*OutsideLocation::size);
	const int mur2 = int(0.85f*OutsideLocation::size);

	// budynki
	for(vector<CityBuilding>::iterator it = _buildings.begin(), end = _buildings.end(); it != end; ++it)
	{
		Object* o = new Object;

		switch(it->rot)
		{
		case 0:
			o->rot.y = 0.f;
			break;
		case 1:
			o->rot.y = PI * 3 / 2;
			break;
		case 2:
			o->rot.y = PI;
			break;
		case 3:
			o->rot.y = PI / 2;
			break;
		}

		o->pos = Vec3(float(it->pt.x + it->type->shift[it->rot].x) * 2, 1.f, float(it->pt.y + it->type->shift[it->rot].y) * 2);
		terrain->SetH(o->pos);
		o->rot.x = o->rot.z = 0.f;
		o->scale = 1.f;
		o->base = nullptr;
		o->mesh = it->type->mesh;
		local_ctx.objects->push_back(o);
	}

	// create walls, towers & gates
	if(!city->IsVillage())
	{
		const int mid = int(0.5f*OutsideLocation::size);
		BaseObject* oWall = BaseObject::Get("wall"),
			*oTower = BaseObject::Get("tower"),
			*oGate = BaseObject::Get("gate"),
			*oGrate = BaseObject::Get("grate");

		// walls
		for(int i = mur1; i < mur2; i += 3)
		{
			// north
			if(!IS_SET(city->gates, GATE_SOUTH) || i < mid - 1 || i > mid)
			{
				Object* o = new Object;
				o->pos = Vec3(float(i) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f);
				o->rot = Vec3(0, PI, 0);
				o->scale = 1.f;
				o->base = oWall;
				o->mesh = oWall->mesh;
				local_ctx.objects->push_back(o);
				SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
			}

			// south
			if(!IS_SET(city->gates, GATE_NORTH) || i < mid - 1 || i > mid)
			{
				Object* o = new Object;
				o->pos = Vec3(float(i) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f);
				o->rot = Vec3(0, 0, 0);
				o->scale = 1.f;
				o->base = oWall;
				o->mesh = oWall->mesh;
				local_ctx.objects->push_back(o);
				SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
			}

			// west
			if(!IS_SET(city->gates, GATE_WEST) || i < mid - 1 || i > mid)
			{
				Object* o = new Object;
				o->pos = Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
				o->rot = Vec3(0, PI * 3 / 2, 0);
				o->scale = 1.f;
				o->base = oWall;
				o->mesh = oWall->mesh;
				local_ctx.objects->push_back(o);
				SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
			}

			// east
			if(!IS_SET(city->gates, GATE_EAST) || i < mid - 1 || i > mid)
			{
				Object* o = new Object;
				o->pos = Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f);
				o->rot = Vec3(0, PI / 2, 0);
				o->scale = 1.f;
				o->base = oWall;
				o->mesh = oWall->mesh;
				local_ctx.objects->push_back(o);
				SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
			}
		}

		// towers
		{
			// north east
			Object* o = new Object;
			o->pos = Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f);
			o->rot = Vec3(0, 0, 0);
			o->scale = 1.f;
			o->base = oTower;
			o->mesh = oTower->mesh;
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
		}
		{
			// south east
			Object* o = new Object;
			o->pos = Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f);
			o->rot = Vec3(0, PI / 2, 0);
			o->scale = 1.f;
			o->base = oTower;
			o->mesh = oTower->mesh;
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
		}
		{
			// south west
			Object* o = new Object;
			o->pos = Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f);
			o->rot = Vec3(0, PI, 0);
			o->scale = 1.f;
			o->base = oTower;
			o->mesh = oTower->mesh;
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
		}
		{
			// north west
			Object* o = new Object;
			o->pos = Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f);
			o->rot = Vec3(0, PI * 3 / 2, 0);
			o->scale = 1.f;
			o->base = oTower;
			o->mesh = oTower->mesh;
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);
		}

		// gates
		if(IS_SET(city->gates, GATE_NORTH))
		{
			Object* o = new Object;
			o->rot.x = o->rot.z = 0.f;
			o->scale = 1.f;
			o->base = oGate;
			o->mesh = oGate->mesh;
			o->rot.y = 0;
			o->pos = Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.85f*OutsideLocation::size * 2);
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);

			Object* o2 = new Object;
			o2->pos = o->pos;
			o2->rot = o->rot;
			o2->scale = 1.f;
			o2->base = oGrate;
			o2->mesh = oGrate->mesh;
			local_ctx.objects->push_back(o2);
			SpawnObjectExtras(local_ctx, o2->base, o2->pos, o2->rot.y, nullptr, 1.f, 0);
		}

		if(IS_SET(city->gates, GATE_SOUTH))
		{
			Object* o = new Object;
			o->rot.x = o->rot.z = 0.f;
			o->scale = 1.f;
			o->base = oGate;
			o->mesh = oGate->mesh;
			o->rot.y = PI;
			o->pos = Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.15f*OutsideLocation::size * 2);
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);

			Object* o2 = new Object;
			o2->pos = o->pos;
			o2->rot = o->rot;
			o2->scale = 1.f;
			o2->base = oGrate;
			o2->mesh = oGrate->mesh;
			local_ctx.objects->push_back(o2);
			SpawnObjectExtras(local_ctx, o2->base, o2->pos, o2->rot.y, nullptr, 1.f, 0);
		}

		if(IS_SET(city->gates, GATE_WEST))
		{
			Object* o = new Object;
			o->rot.x = o->rot.z = 0.f;
			o->scale = 1.f;
			o->base = oGate;
			o->mesh = oGate->mesh;
			o->rot.y = PI * 3 / 2;
			o->pos = Vec3(0.15f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f);
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);

			Object* o2 = new Object;
			o2->pos = o->pos;
			o2->rot = o->rot;
			o2->scale = 1.f;
			o2->base = oGrate;
			o2->mesh = oGrate->mesh;
			local_ctx.objects->push_back(o2);
			SpawnObjectExtras(local_ctx, o2->base, o2->pos, o2->rot.y, nullptr, 1.f, 0);
		}

		if(IS_SET(city->gates, GATE_EAST))
		{
			Object* o = new Object;
			o->rot.x = o->rot.z = 0.f;
			o->scale = 1.f;
			o->base = oGate;
			o->mesh = oGate->mesh;
			o->rot.y = PI / 2;
			o->pos = Vec3(0.85f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f);
			local_ctx.objects->push_back(o);
			SpawnObjectExtras(local_ctx, o->base, o->pos, o->rot.y, nullptr, 1.f, 0);

			Object* o2 = new Object;
			o2->pos = o->pos;
			o2->rot = o->rot;
			o2->scale = 1.f;
			o2->base = oGrate;
			o2->mesh = oGrate->mesh;
			local_ctx.objects->push_back(o2);
			SpawnObjectExtras(local_ctx, o2->base, o2->pos, o2->rot.y, nullptr, 1.f, 0);
		}
	}

	// obiekty i fizyka budynków
	for(vector<CityBuilding>::iterator it = _buildings.begin(), end = _buildings.end(); it != end; ++it)
	{
		Building* b = it->type;

		int r = it->rot;
		if(r == 1)
			r = 3;
		else if(r == 3)
			r = 1;

		ProcessBuildingObjects(local_ctx, city, nullptr, b->mesh, b->inside_mesh, dir_to_rot(r), r,
			Vec3(float(it->pt.x + b->shift[it->rot].x) * 2, 0.f, float(it->pt.y + b->shift[it->rot].y) * 2), it->type, &*it);
	}
}

void Game::ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
	const Vec3& shift, Building* type, CityBuilding* building, bool recreate, Vec3* out_point)
{
	if(mesh->attach_points.empty())
	{
		building->walk_pt = shift;
		assert(!inside);
		return;
	}

	// https://github.com/Tomash667/carpg/wiki/%5BPL%5D-Buildings-export
	// o_x_[!N!]nazwa_???
	// x (o - obiekt, r - obrócony obiekt, p - fizyka, s - strefa, c - postaæ, m - maska œwiat³a, d - detal wokó³ obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	string token;
	Matrix m1, m2;
	bool have_exit = false, have_spawn = false;
	bool is_inside = (inside != nullptr);
	Vec3 spawn_point;
	static vector<const Mesh::Point*> details;

	for(vector<Mesh::Point>::const_iterator it2 = mesh->attach_points.begin(), end2 = mesh->attach_points.end(); it2 != end2; ++it2)
	{
		const Mesh::Point& pt = *it2;
		if(pt.name.length() < 5 || pt.name[0] != 'o')
			continue;

		char c = pt.name[2];
		if(c == 'o' || c == 'r' || c == 'p' || c == 's' || c == 'c' || c == 'l')
		{
			uint poss = pt.name.find_first_of('_', 4);
			if(poss == string::npos)
			{
				assert(0);
				continue;
			}
			token = pt.name.substr(4, poss - 4);
			for(uint k = 0, len = token.length(); k < len; ++k)
			{
				if(token[k] == '#')
					token[k] = '_';
			}
		}
		else if(c == 'm')
		{
			// light mask
			// nothing to do here
		}
		else if(c == 'd')
		{
			if (!recreate)
			{
				assert(!is_inside);
				details.push_back(&pt);
			}
			continue;
		}
		else
			continue;

		Vec3 pos;
		if(roti != 0)
		{
			m2 = pt.mat * Matrix::RotationY(rot);
			pos = Vec3::TransformZero(m2);
		}
		else
			pos = Vec3::TransformZero(pt.mat);
		pos += shift;

		if(c == 'o' || c == 'r' || c == 'l')
		{
			// obiekt / obrócony obiekt
			if(!recreate)
			{
				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* base = BaseObject::TryGet(name);
				assert(base);

				if(base)
				{
					if(ctx.type == LevelContext::Outside)
						terrain->SetH(pos);
					float r;
					switch(c)
					{
					case 'o':
					default:
						r = Random(MAX_ANGLE);
						break;
					case 'r':
						r = Clip(pt.rot.y + rot);
						break;
					case 'l':
						r = PI / 2 * (Rand() % 4);
						break;
					}
					SpawnObjectEntity(ctx, base, pos, r, 1.f, 0, nullptr, variant);
				}
			}
		}
		else if(c == 'p')
		{
			// fizyka
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.ptr = (is_wall ? CAM_COLLIDER : nullptr);

				if(ctx.type == LevelContext::Outside)
				{
					terrain->SetH(pos);
					pos.y += 2.f;
				}

				btCylinderShape* shape = new btCylinderShape(btVector3(pt.size.x, 4.f, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);
			}
			else if(token == "square" || token == "squarev" || token == "squarevn" || token == "squarevp")
			{
				bool is_wall = (token == "square" || token == "squarevn");
				bool block_camera = (token == "square");

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::RECTANGLE;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.w = pt.size.x;
				cobj.h = pt.size.z;
				cobj.ptr = (block_camera ? CAM_COLLIDER : nullptr);

				btBoxShape* shape;
				if(token != "squarevp")
				{
					shape = new btBoxShape(btVector3(pt.size.x, 16.f, pt.size.z));
					if(ctx.type == LevelContext::Outside)
					{
						terrain->SetH(pos);
						pos.y += 8.f;
					}
					else
						pos.y = 0.f;
				}
				else
				{
					shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
					if(ctx.type == LevelContext::Outside)
						pos.y += terrain->GetH(pos);
				}
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(roti != 0)
				{
					cobj.type = CollisionObject::RECTANGLE_ROT;
					cobj.rot = rot;
					cobj.radius = sqrt(cobj.w*cobj.w + cobj.h*cobj.h);
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
				}
			}
			else if(token == "squarevpa")
			{
				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				if(ctx.type == LevelContext::Outside)
					pos.y += terrain->GetH(pos);
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = CG_COLLIDER;
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(roti != 0)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
			}
			else if(token == "squarecam")
			{
				if(ctx.type == LevelContext::Outside)
					pos.y += terrain->GetH(pos);

				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
				if(roti != 0)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

				float w = pt.size.x, h = pt.size.z;
				if(roti == 1 || roti == 3)
					std::swap(w, h);

				CameraCollider& cc = Add1(cam_colliders);
				cc.box.v1.x = pos.x - w;
				cc.box.v2.x = pos.x + w;
				cc.box.v1.z = pos.z - h;
				cc.box.v2.z = pos.z + h;
				cc.box.v1.y = pos.y - pt.size.y;
				cc.box.v2.y = pos.y + pt.size.y;
			}
			else if(token == "xsphere")
			{
				inside->xsphere_pos = pos;
				inside->xsphere_radius = pt.size.x;
			}
			else
				assert(0);
		}
		else if(c == 's')
		{
			// strefa
			if(!recreate)
			{
				if(token == "enter")
				{
					assert(!inside);

					inside = new InsideBuilding;
					inside->level_shift = city->inside_offset;
					inside->offset = Vec2(512.f*city->inside_offset.x + 256.f, 512.f*city->inside_offset.y + 256.f);
					if(city->inside_offset.x > city->inside_offset.y)
					{
						--city->inside_offset.x;
						++city->inside_offset.y;
					}
					else
					{
						city->inside_offset.x += 2;
						city->inside_offset.y = 0;
					}
					float w, h;
					if(roti == 0 || roti == 2)
					{
						w = pt.size.x;
						h = pt.size.z;
					}
					else
					{
						w = pt.size.z;
						h = pt.size.x;
					}
					inside->enter_area.v1.x = pos.x - w;
					inside->enter_area.v1.y = pos.z - h;
					inside->enter_area.v2.x = pos.x + w;
					inside->enter_area.v2.y = pos.z + h;
					Vec2 mid = inside->enter_area.Midpoint();
					inside->enter_y = terrain->GetH(mid.x, mid.y) + 0.1f;
					inside->type = type;
					inside->outside_rot = rot;
					inside->top = -1.f;
					inside->xsphere_radius = -1.f;
					ApplyContext(inside, inside->ctx);
					inside->ctx.building_id = (int)city->inside_buildings.size();

					city->inside_buildings.push_back(inside);

					assert(inside_mesh);

					if(inside_mesh)
					{
						Vec3 o_pos = Vec3(inside->offset.x, 0.f, inside->offset.y);

						Object* o = new Object;
						o->base = nullptr;
						o->mesh = inside_mesh;
						o->pos = o_pos;
						o->rot = Vec3(0, 0, 0);
						o->scale = 1.f;
						o->require_split = true;
						inside->ctx.objects->push_back(o);

						ProcessBuildingObjects(inside->ctx, city, inside, inside_mesh, nullptr, 0.f, 0, o->pos, nullptr, nullptr);
					}

					have_exit = true;
				}
				else if(token == "exit")
				{
					assert(inside);

					inside->exit_area.v1.x = pos.x - pt.size.x;
					inside->exit_area.v1.y = pos.z - pt.size.z;
					inside->exit_area.v2.x = pos.x + pt.size.x;
					inside->exit_area.v2.y = pos.z + pt.size.z;

					have_exit = true;
				}
				else if(token == "spawn")
				{
					if(is_inside)
						inside->inside_spawn = pos;
					else
					{
						spawn_point = pos;
						terrain->SetH(spawn_point);
					}

					have_spawn = true;
				}
				else if(token == "top")
				{
					assert(is_inside);
					inside->top = pos.y;
				}
				else if(token == "door" || token == "doorc" || token == "doorl" || token == "door2")
				{
					Door* door = new Door;
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->state = Door::Open;
					door->door2 = (token == "door2");
					door->mesh_inst = new MeshInstance(door->door2 ? aDoor2 : aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					door->netid = Door::netid_counter++;

					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += 1.319f;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(token != "door") // door2 are closed now, this is intended
					{
						door->state = Door::Closed;
						if(token == "doorl")
							door->locked = 1;
					}
					else
					{
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}

					ctx.doors->push_back(door);
				}
				else if(token == "arena")
				{
					assert(inside);

					inside->arena1.v1.x = pos.x - pt.size.x;
					inside->arena1.v1.y = pos.z - pt.size.z;
					inside->arena1.v2.x = pos.x + pt.size.x;
					inside->arena1.v2.y = pos.z + pt.size.z;
				}
				else if(token == "arena2")
				{
					assert(inside);

					inside->arena2.v1.x = pos.x - pt.size.x;
					inside->arena2.v1.y = pos.z - pt.size.z;
					inside->arena2.v2.x = pos.x + pt.size.x;
					inside->arena2.v2.y = pos.z + pt.size.z;
				}
				else if(token == "viewer")
				{
					// ten punkt jest u¿ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(building)
					{
						building->walk_pt = pos;
						terrain->SetH(building->walk_pt);
					}
					else if(out_point)
						*out_point = pos;
				}
				else
					assert(0);
			}
		}
		else if(c == 'c')
		{
			if(!recreate)
			{
				UnitData* ud = UnitData::TryGet(token.c_str());
				assert(ud);
				if(ud)
				{
					Unit* u = SpawnUnitNearLocation(ctx, pos, *ud, nullptr, -2);
					u->rot = Clip(pt.rot.y + rot);
					u->ai->start_rot = u->rot;
				}
			}
		}
		else if(c == 'm')
		{
			LightMask& mask = Add1(inside->masks);
			mask.size = Vec2(pt.size.x, pt.size.z);
			mask.pos = Vec2(pos.x, pos.z);
		}
	}

	if(!details.empty() && !is_inside)
	{
		int c = Rand() % 80 + details.size() * 2, count;
		if(c < 10)
			count = 0;
		else if(c < 30)
			count = 1;
		else if(c < 60)
			count = 2;
		else if(c < 90)
			count = 3;
		else
			count = 4;
		if(count > 0)
		{
			std::random_shuffle(details.begin(), details.end(), MyRand);
			while(count && !details.empty())
			{
				const Mesh::Point& pt = *details.back();
				details.pop_back();
				--count;

				uint poss = pt.name.find_first_of('_', 4);
				if(poss == string::npos)
				{
					assert(0);
					continue;
				}
				token = pt.name.substr(4, poss - 4);
				for(uint k = 0, len = token.length(); k < len; ++k)
				{
					if(token[k] == '#')
						token[k] = '_';
				}

				Vec3 pos;
				if(roti != 0)
				{
					m1 = Matrix::RotationY(rot);
					m2 = pt.mat * m1;
					pos = Vec3::TransformZero(m2);
				}
				else
					pos = Vec3::TransformZero(pt.mat);
				pos += shift;

				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* obj = BaseObject::TryGet(name);
				assert(obj);

				if(obj)
				{
					if(ctx.type == LevelContext::Outside)
						terrain->SetH(pos);
					SpawnObjectEntity(ctx, obj, pos, Clip(pt.rot.y + rot), 1.f, 0, nullptr, variant);
				}
			}
		}
		details.clear();
	}

	if(!recreate)
	{
		if(is_inside || inside)
			assert(have_exit && have_spawn);

		if(!is_inside && inside)
			inside->outside_spawn = spawn_point;
	}

	if(inside)
		inside->ctx.masks = (!inside->masks.empty() ? &inside->masks : nullptr);
}

void Game::SpawnUnits(City* city)
{
	assert(city);

	for(CityBuilding& b : city->buildings)
	{
		UnitData* ud = b.type->unit;
		if(!ud)
			continue;

		Unit* u = CreateUnit(*ud, -2);

		switch(b.rot)
		{
		case 0:
			u->rot = 0.f;
			break;
		case 1:
			u->rot = PI * 3 / 2;
			break;
		case 2:
			u->rot = PI;
			break;
		case 3:
			u->rot = PI / 2;
			break;
		}

		u->pos = Vec3(float(b.unit_pt.x) * 2 + 1, 0, float(b.unit_pt.y) * 2 + 1);
		terrain->SetH(u->pos);
		UpdateUnitPhysics(*u, u->pos);
		u->visual_pos = u->pos;

		if(b.type->group == BuildingGroup::BG_ARENA)
			city->arena_pos = u->pos;

		local_ctx.units->push_back(u);

		AIController* ai = new AIController;
		ai->Init(u);
		ais.push_back(ai);
	}

	UnitData* dweller = UnitData::Get(LocationHelper::IsCity(L.location) ? "citizen" : "villager");

	// pijacy w karczmie
	for(int i = 0, ile = Random(1, city_ctx->citizens / 3); i < ile; ++i)
	{
		if(!SpawnUnitInsideInn(*dweller, -2))
			break;
	}

	// wêdruj¹cy mieszkañcy
	const int a = int(0.15f*OutsideLocation::size) + 3;
	const int b = int(0.85f*OutsideLocation::size) - 3;

	for(int i = 0; i < city_ctx->citizens; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				SpawnUnitNearLocation(local_ctx, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *dweller, nullptr, -2, 2.f);
				break;
			}
		}
	}

	// stra¿nicy
	UnitData* guard = UnitData::Get("guard_move");
	for(int i = 0, ile = city_ctx->IsVillage() ? 3 : 6; i < ile; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				SpawnUnitNearLocation(local_ctx, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *guard, nullptr, -2, 2.f);
				break;
			}
		}
	}
}

void Game::RespawnUnits()
{
	RespawnUnits(local_ctx);
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			RespawnUnits((*it)->ctx);
	}
}

void Game::RespawnUnits(LevelContext& ctx)
{
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
	{
		Unit* u = *it;
		if(u->player)
			continue;

		// model
		u->action = A_NONE;
		u->talking = false;
		u->CreateMesh(Unit::CREATE_MESH::NORMAL);

		// fizyka
		CreateUnitPhysics(*u, true);

		// ai
		AIController* ai = new AIController;
		ai->Init(u);
		ais.push_back(ai);
	}
}

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int price_limit, int exclude_flags, uint count = 1)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > price_limit || IS_SET(item->flags, exclude_flags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

void Game::GenerateStockItems()
{
	Location& loc = *L.location;

	assert(loc.type == L_CITY);

	City& city = (City&)loc;
	int price_limit, price_limit2, count_mod;
	bool is_city;

	if(!city.IsVillage())
	{
		price_limit = Random(2000, 2500);
		price_limit2 = 99999;
		count_mod = 0;
		is_city = true;
	}
	else
	{
		price_limit = Random(500, 1000);
		price_limit2 = Random(1250, 2500);
		count_mod = -Random(1, 3);
		is_city = false;
	}

	// merchant
	if(IS_SET(city.flags, City::HaveMerchant))
		GenerateMerchantItems(chest_merchant, price_limit);

	// blacksmith
	if(IS_SET(city.flags, City::HaveBlacksmith))
	{
		chest_blacksmith.clear();
		for(int i = 0, count = Random(12, 18) + count_mod; i < count; ++i)
		{
			switch(Rand() % 4)
			{
			case IT_WEAPON:
				InsertRandomItem(chest_blacksmith, Weapon::weapons, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_BOW:
				InsertRandomItem(chest_blacksmith, Bow::bows, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_SHIELD:
				InsertRandomItem(chest_blacksmith, Shield::shields, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			case IT_ARMOR:
				InsertRandomItem(chest_blacksmith, Armor::armors, price_limit2, ITEM_NOT_SHOP | ITEM_NOT_BLACKSMITH);
				break;
			}
		}
		// basic equipment
		Stock::Get("blacksmith")->Parse(5, is_city, chest_blacksmith);
		SortItems(chest_blacksmith);
	}

	// alchemist
	if(IS_SET(city.flags, City::HaveAlchemist))
	{
		chest_alchemist.clear();
		for(int i = 0, count = Random(8, 12) + count_mod; i < count; ++i)
			InsertRandomItem(chest_alchemist, Consumable::consumables, 99999, ITEM_NOT_SHOP | ITEM_NOT_ALCHEMIST, Random(3, 6));
		SortItems(chest_alchemist);
	}

	// innkeeper
	if(IS_SET(city.flags, City::HaveInn))
	{
		chest_innkeeper.clear();
		Stock::Get("innkeeper")->Parse(5, is_city, chest_innkeeper);
		const ItemList* lis2 = ItemList::Get("normal_food").lis;
		for(uint i = 0, count = Random(10, 20) + count_mod; i < count; ++i)
			InsertItemBare(chest_innkeeper, lis2->Get());
		SortItems(chest_innkeeper);
	}

	// food seller
	if(IS_SET(city.flags, City::HaveFoodSeller))
	{
		chest_food_seller.clear();
		const ItemList* lis = ItemList::Get("food_and_drink").lis;
		for(const Item* item : lis->items)
		{
			uint value = Random(50, 100);
			if(!is_city)
				value /= 2;
			InsertItemBare(chest_food_seller, item, value / item->value);
		}
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("frying_pan"));
		if(Rand() % 4 == 0)
			InsertItemBare(chest_food_seller, Item::Get("ladle"));
		SortItems(chest_food_seller);
	}
}

void Game::GenerateMerchantItems(vector<ItemSlot>& items, int price_limit)
{
	items.clear();
	InsertItemBare(items, Item::Get("p_nreg"), Random(5, 10));
	InsertItemBare(items, Item::Get("p_hp"), Random(5, 10));
	for(int i = 0, count = Random(15, 20); i < count; ++i)
	{
		switch(Rand() % 6)
		{
		case IT_WEAPON:
			InsertRandomItem(items, Weapon::weapons, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_BOW:
			InsertRandomItem(items, Bow::bows, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_SHIELD:
			InsertRandomItem(items, Shield::shields, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_ARMOR:
			InsertRandomItem(items, Armor::armors, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		case IT_CONSUMABLE:
			InsertRandomItem(items, Consumable::consumables, price_limit / 5, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT, Random(2, 5));
			break;
		case IT_OTHER:
			InsertRandomItem(items, OtherItem::others, price_limit, ITEM_NOT_SHOP | ITEM_NOT_MERCHANT);
			break;
		}
	}
	SortItems(items);
}

// dru¿yna opuœci³a lokacje
void Game::LeaveLocation(bool clear, bool end_buffs)
{
	if(Net::IsLocal())
	{
		// zawody
		if(QM.quest_tournament->state != Quest_Tournament::TOURNAMENT_NOT_DONE)
			CleanTournament();
		// arena
		if(!arena_free)
			CleanArena();
	}

	if(clear)
	{
		if(L.is_open)
			LeaveLevel(true);
		return;
	}

	if(!L.is_open)
		return;

	Info("Leaving location.");

	pvp_response.ok = false;
	QM.quest_tournament->generated = false;

	if(Net::IsLocal() && (QM.quest_crazies->check_stone
		|| (QM.quest_crazies->crazies_state >= Quest_Crazies::State::PickedStone && QM.quest_crazies->crazies_state < Quest_Crazies::State::End)))
		CheckCraziesStone();

	// drinking contest
	Quest_Contest* contest = QM.quest_contest;
	if(contest->state >= Quest_Contest::CONTEST_STARTING)
	{
		for(vector<Unit*>::iterator it = contest->units.begin(), end = contest->units.end(); it != end; ++it)
		{
			Unit& u = **it;
			u.busy = Unit::Busy_No;
			u.look_target = nullptr;
			u.event_handler = nullptr;
		}

		InsideBuilding* inn = city_ctx->FindInn();
		Unit* innkeeper = inn->FindUnit(UnitData::Get("innkeeper"));

		innkeeper->talking = false;
		innkeeper->mesh_inst->need_update = true;
		innkeeper->busy = Unit::Busy_No;
		contest->state = Quest_Contest::CONTEST_DONE;
		contest->units.clear();
		W.AddNews(txContestNoWinner);
	}

	// clear blood & bodies from orc base
	if(Net::IsLocal() && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::ClearDungeon && L.location_index == QM.quest_orcs2->target_loc)
	{
		QM.quest_orcs2->orcs_state = Quest_Orcs2::State::End;
		UpdateLocation(31, 100, false);
	}

	if(city_ctx && game_state != GS_EXIT_TO_MENU && Net::IsLocal())
	{
		// opuszczanie miasta
		BuyTeamItems();
	}

	LeaveLevel();

	if(L.is_open)
	{
		if(Net::IsLocal())
		{
			// usuñ questowe postacie
			RemoveQuestUnits(true);
		}

		L.ProcessRemoveUnits(true);

		if(L.location->type == L_ENCOUNTER)
		{
			OutsideLocation* outside = (OutsideLocation*)L.location;
			outside->bloods.clear();
			DeleteElements(outside->objects);
			DeleteElements(outside->chests);
			DeleteElements(outside->items);
			outside->objects.clear();
			DeleteElements(outside->usables);
			for(vector<Unit*>::iterator it = outside->units.begin(), end = outside->units.end(); it != end; ++it)
				delete *it;
			outside->units.clear();
		}
	}

	if(Team.crazies_attack)
	{
		Team.crazies_attack = false;
		if(Net::IsOnline())
			Net::PushChange(NetChange::CHANGE_FLAGS);
	}

	if(!Net::IsLocal())
		pc = nullptr;
	else if(end_buffs)
	{
		// usuñ tymczasowe bufy
		for(Unit* unit : Team.members)
			unit->EndEffects();
	}

	L.is_open = false;
	city_ctx = nullptr;
}

void Game::GenerateDungeon(Location& _loc)
{
	assert(_loc.type == L_DUNGEON || _loc.type == L_CRYPT);

	InsideLocation* inside = (InsideLocation*)&_loc;
	BaseLocation& base = g_base_locations[inside->target];
	InsideLocationLevel& lvl = inside->GetLevelData();

	assert(!lvl.map);

	if(!IS_SET(base.options, BLO_LABIRYNTH))
	{
		OpcjeMapy opcje;
		opcje.korytarz_szansa = base.corridor_chance;
		opcje.w = opcje.h = base.size + base.size_lvl * dungeon_level;
		opcje.rozmiar_korytarz = base.corridor_size;
		opcje.rozmiar_pokoj = base.room_size;
		opcje.rooms = &lvl.rooms;
		opcje.polacz_korytarz = base.join_corridor;
		opcje.polacz_pokoj = base.join_room;
		opcje.ksztalt = (IS_SET(base.options, BLO_ROUND) ? OpcjeMapy::OKRAG : OpcjeMapy::PROSTOKAT);
		opcje.schody_gora = (inside->HaveUpStairs() ? OpcjeMapy::LOSOWO : OpcjeMapy::BRAK);
		opcje.schody_dol = (inside->HaveDownStairs() ? OpcjeMapy::LOSOWO : OpcjeMapy::BRAK);
		opcje.kraty_szansa = base.bars_chance;
		opcje.devmode = devmode;

		// ostatni poziom krypty
		if(inside->type == L_CRYPT && !inside->HaveDownStairs())
		{
			opcje.schody_gora = OpcjeMapy::NAJDALEJ;
			Room& r = Add1(lvl.rooms);
			r.target = RoomTarget::Treasury;
			r.size = Int2(7, 7);
			r.pos.x = r.pos.y = (opcje.w - 7) / 2;
			inside->special_room = 0;
		}
		else if(inside->type == L_DUNGEON && (inside->target == THRONE_FORT || inside->target == THRONE_VAULT) && !inside->HaveDownStairs())
		{
			// sala tronowa
			opcje.schody_gora = OpcjeMapy::NAJDALEJ;
			Room& r = Add1(lvl.rooms);
			r.target = RoomTarget::Throne;
			r.size = Int2(13, 7);
			r.pos.x = (opcje.w - 13) / 2;
			r.pos.y = (opcje.w - 7) / 2;
			inside->special_room = 0;
		}
		else if(L.location_index == QM.quest_secret->where && QM.quest_secret->state == Quest_Secret::SECRET_DROPPED_STONE && !inside->HaveDownStairs())
		{
			// sekret
			opcje.schody_gora = OpcjeMapy::NAJDALEJ;
			Room& r = Add1(lvl.rooms);
			r.target = RoomTarget::PortalCreate;
			r.size = Int2(7, 7);
			r.pos.x = r.pos.y = (opcje.w - 7) / 2;
			inside->special_room = 0;
		}
		else if(L.location_index == QM.quest_evil->target_loc && QM.quest_evil->evil_state == Quest_Evil::State::GeneratedCleric)
		{
			// schody w krypcie 0 jak najdalej od œrodka
			opcje.schody_gora = OpcjeMapy::NAJDALEJ;
		}

		if(QM.quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted && L.location_index == QM.quest_orcs->target_loc && dungeon_level == L.location->GetLastLevel())
		{
			opcje.stop = true;
			bool first = true;
			int proby = 0;
			vector<int> mozliwe_pokoje;

			while(true)
			{
				if(!generuj_mape2(opcje, !first))
				{
					assert(0);
					throw Format("Failed to generate dungeon map (%d)!", opcje.blad);
				}

				first = false;

				// szukaj pokoju dla celi
				int index = 0;
				for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it, ++index)
				{
					if(it->target == RoomTarget::None && it->connected.size() == 1)
						mozliwe_pokoje.push_back(index);
				}

				if(mozliwe_pokoje.empty())
				{
					++proby;
					if(proby == 100)
						throw "Failed to generate special room in dungeon!";
					else
						continue;
				}

				int id = mozliwe_pokoje[Rand() % mozliwe_pokoje.size()];
				lvl.rooms[id].target = RoomTarget::Prison;
				// dodaj drzwi
				Int2 pt = pole_laczace(id, lvl.rooms[id].connected.front());
				Pole& p = opcje.mapa[pt.x + pt.y*opcje.w];
				p.type = DRZWI;
				p.flags |= Pole::F_SPECJALNE;

				if(!kontynuuj_generowanie_mapy(opcje))
				{
					assert(0);
					throw Format("Failed to generate dungeon map 2 (%d)!", opcje.blad);
				}

				break;
			}
		}
		else
		{
			if(!generuj_mape2(opcje))
			{
				assert(0);
				throw Format("Failed to generate dungeon map (%d)!", opcje.blad);
			}
		}

		lvl.w = lvl.h = opcje.w;
		lvl.map = opcje.mapa;
		lvl.staircase_up = opcje.schody_gora_pozycja;
		lvl.staircase_up_dir = opcje.schody_gora_kierunek;
		lvl.staircase_down = opcje.schody_dol_pozycja;
		lvl.staircase_down_dir = opcje.schody_dol_kierunek;
		lvl.staircase_down_in_wall = opcje.schody_dol_w_scianie;

		// inna tekstura pokoju w krypcie
		if(inside->type == L_CRYPT && !inside->HaveDownStairs())
		{
			Room& r = lvl.rooms[0];
			for(int y = 0; y < r.size.y; ++y)
			{
				for(int x = 0; x < r.size.x; ++x)
					lvl.map[r.pos.x + x + (r.pos.y + y)*lvl.w].flags |= Pole::F_DRUGA_TEKSTURA;
			}
		}

		// portal
		if(inside->from_portal)
		{
		powtorz:
			int id = Rand() % lvl.rooms.size();
			while(true)
			{
				Room& room = lvl.rooms[id];
				if(room.target != RoomTarget::None || room.size.x <= 4 || room.size.y <= 4)
					id = (id + 1) % lvl.rooms.size();
				else
					break;
			}

			Room& r = lvl.rooms[id];
			vector<std::pair<Int2, int> > good_pts;

			for(int y = 1; y < r.size.y - 1; ++y)
			{
				for(int x = 1; x < r.size.x - 1; ++x)
				{
					if(lvl.At(r.pos + Int2(x, y)).type == PUSTE)
					{
						// opcje:
						// ___ #__
						// _?_ #?_
						// ### ###
#define P(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == PUSTE)
#define B(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == SCIANA)

						int dir = -1;

						// __#
						// _?#
						// __#
						if(P(-1, 0) && B(1, 0) && B(1, -1) && B(1, 1) && ((P(-1, -1) && P(0, -1)) || (P(-1, 1) && P(0, 1)) || (B(-1, -1) && B(0, -1) && B(-1, 1) && B(0, 1))))
						{
							dir = 1;
						}
						// #__
						// #?_
						// #__
						else if(P(1, 0) && B(-1, 0) && B(-1, 1) && B(-1, -1) && ((P(0, -1) && P(1, -1)) || (P(0, 1) && P(1, 1)) || (B(0, -1) && B(1, -1) && B(0, 1) && B(1, 1))))
						{
							dir = 3;
						}
						// ###
						// _?_
						// ___
						else if(P(0, 1) && B(0, -1) && B(-1, -1) && B(1, -1) && ((P(-1, 0) && P(-1, 1)) || (P(1, 0) && P(1, 1)) || (B(-1, 0) && B(-1, 1) && B(1, 0) && B(1, 1))))
						{
							dir = 2;
						}
						// ___
						// _?_
						// ###
						else if(P(0, -1) && B(0, 1) && B(-1, 1) && B(1, 1) && ((P(-1, 0) && P(-1, -1)) || (P(1, 0) && P(1, -1)) || (B(-1, 0) && B(-1, -1) && B(1, 0) && B(1, -1))))
						{
							dir = 0;
						}

						if(dir != -1)
							good_pts.push_back(std::pair<Int2, int>(r.pos + Int2(x, y), dir));
#undef P
#undef B
					}
				}
			}

			if(good_pts.empty())
				goto powtorz;

			std::pair<Int2, int>& pt = good_pts[Rand() % good_pts.size()];

			const Vec3 pos(2.f*pt.first.x + 1, 0, 2.f*pt.first.y + 1);
			float rot = Clip(dir_to_rot(pt.second) + PI);

			inside->portal->pos = pos;
			inside->portal->rot = rot;

			lvl.GetRoom(pt.first)->target = RoomTarget::Portal;
		}
	}
	else
	{
		Int2 pokoj_pos;
		generate_labirynth(lvl.map, Int2(base.size, base.size), base.room_size, lvl.staircase_up, lvl.staircase_up_dir, pokoj_pos, base.bars_chance, devmode);

		lvl.w = lvl.h = base.size;
		Room& r = Add1(lvl.rooms);
		r.target = RoomTarget::None;
		r.pos = pokoj_pos;
		r.size = base.room_size;
	}
}

void Game::GenerateDungeonObjects2()
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	// schody w górê
	if(inside->HaveUpStairs())
	{
		Object* o = new Object;
		o->mesh = aStairsUp;
		o->pos = pt_to_pos(lvl.staircase_up);
		o->rot = Vec3(0, dir_to_rot(lvl.staircase_up_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		local_ctx.objects->push_back(o);
	}
	else
		SpawnObjectEntity(local_ctx, BaseObject::Get("portal"), inside->portal->pos, inside->portal->rot);

	// schody w dó³
	if(inside->HaveDownStairs())
	{
		Object* o = new Object;
		o->mesh = (lvl.staircase_down_in_wall ? aStairsDown2 : aStairsDown);
		o->pos = pt_to_pos(lvl.staircase_down);
		o->rot = Vec3(0, dir_to_rot(lvl.staircase_down_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		local_ctx.objects->push_back(o);
	}

	// kratki, drzwi
	for(int y = 0; y < lvl.h; ++y)
	{
		for(int x = 0; x < lvl.w; ++x)
		{
			POLE p = lvl.map[x + y*lvl.w].type;
			if(p == KRATKA || p == KRATKA_PODLOGA)
			{
				Object* o = new Object;
				o->mesh = aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 0, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);
			}
			if(p == KRATKA || p == KRATKA_SUFIT)
			{
				Object* o = new Object;
				o->mesh = aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 4, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);
			}
			if(p == DRZWI)
			{
				Object* o = new Object;
				o->mesh = aDoorWall;
				if(IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_DRUGA_TEKSTURA))
					o->mesh = aDoorWall2;
				o->pos = Vec3(float(x * 2) + 1, 0, float(y * 2) + 1);
				o->scale = 1;
				o->base = nullptr;
				local_ctx.objects->push_back(o);

				if(czy_blokuje2(lvl.map[x - 1 + y*lvl.w].type))
				{
					o->rot = Vec3(0, 0, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x + (y - 1)*lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + (y + 1)*lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.z += 0.8229f;
					else if(mov == -1)
						o->pos.z -= 0.8229f;
				}
				else
				{
					o->rot = Vec3(0, PI / 2, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x - 1 + y*lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + 1 + y*lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.x += 0.8229f;
					else if(mov == -1)
						o->pos.x -= 0.8229f;
				}

				if(Rand() % 100 < base.door_chance || IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_SPECJALNE))
				{
					Door* door = new Door;
					local_ctx.doors->push_back(door);
					door->pt = Int2(x, y);
					door->pos = o->pos;
					door->rot = o->rot.y;
					door->state = Door::Closed;
					door->mesh_inst = new MeshInstance(aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					door->netid = Door::netid_counter++;
					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += 1.319f;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(IS_SET(lvl.map[x + y*lvl.w].flags, Pole::F_SPECJALNE))
						door->locked = LOCK_ORCS;
					else if(Rand() % 100 < base.door_open)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}
				}
				else
					lvl.map[x + y*lvl.w].type = OTWOR_NA_DRZWI;
			}
		}
	}
}

void Game::SpawnCityPhysics()
{
	City* city = (City*)L.location;
	TerrainTile* tiles = city->tiles;

	for(int z = 0; z < City::size; ++z)
	{
		for(int x = 0; x < City::size; ++x)
		{
			if(tiles[x + z*OutsideLocation::size].mode == TM_BUILDING_BLOCK)
			{
				btCollisionObject* cobj = new btCollisionObject;
				cobj->setCollisionShape(shape_block);
				cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
				cobj->getWorldTransform().setOrigin(btVector3(2.f*x + 1.f, terrain->GetH(2.f*x + 1.f, 2.f*x + 1), 2.f*z + 1.f));
				phy_world->addCollisionObject(cobj, CG_BUILDING);
			}
		}
	}
}

void Game::RespawnBuildingPhysics()
{
	for(vector<CityBuilding>::iterator it = city_ctx->buildings.begin(), end = city_ctx->buildings.end(); it != end; ++it)
	{
		Building* b = it->type;

		int r = it->rot;
		if(r == 1)
			r = 3;
		else if(r == 3)
			r = 1;

		ProcessBuildingObjects(local_ctx, city_ctx, nullptr, b->mesh, nullptr, dir_to_rot(r), r,
			Vec3(float(it->pt.x + b->shift[it->rot].x) * 2, 1.f, float(it->pt.y + b->shift[it->rot].y) * 2), nullptr, &*it, true);
	}

	for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
	{
		ProcessBuildingObjects((*it)->ctx, city_ctx, *it, (*it)->type->inside_mesh, nullptr, 0.f, 0, Vec3((*it)->offset.x, 0.f, (*it)->offset.y), nullptr,
			nullptr, true);
	}
}

struct OutsideObject
{
	cstring name;
	BaseObject* obj;
	Vec2 scale;
};

OutsideObject outside_objects[] = {
	"tree", nullptr, Vec2(3,5),
	"tree2", nullptr, Vec2(3,5),
	"tree3", nullptr, Vec2(3,5),
	"grass", nullptr, Vec2(1.f,1.5f),
	"grass", nullptr, Vec2(1.f,1.5f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"rock", nullptr, Vec2(1.f,1.f),
	"fern", nullptr, Vec2(1,2)
};
const uint n_outside_objects = countof(outside_objects);

void Game::SpawnCityObjects()
{
	if(!outside_objects[0].obj)
	{
		for(uint i = 0; i < n_outside_objects; ++i)
			outside_objects[i].obj = BaseObject::Get(outside_objects[i].name);
	}

	// well
	if(g_have_well)
	{
		Vec3 pos = pt_to_pos(g_well_pt);
		terrain->SetH(pos);
		SpawnObjectEntity(local_ctx, BaseObject::Get("coveredwell"), pos, PI / 2 * (Rand() % 4), 1.f, 0, nullptr);
	}

	TerrainTile* tiles = ((City*)L.location)->tiles;

	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x - 1 + pt.y*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x + 1 + pt.y*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x - 1 + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x - 1 + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x + 1 + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL &&
			tiles[pt.x + 1 + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = outside_objects[Rand() % n_outside_objects];
			SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

void Game::GenerateForest(Location& loc)
{
	OutsideLocation* outside = (OutsideLocation*)&loc;

	// stwórz mapê jeœli jeszcze nie ma
	const int _s = OutsideLocation::size;
	outside->tiles = new TerrainTile[_s*_s];
	outside->h = new float[(_s + 1)*(_s + 1)];
	memset(outside->tiles, 0, sizeof(TerrainTile)*_s*_s);

	Perlin perlin2(4, 4, 1);

	for(int i = 0, y = 0; y < _s; ++y)
	{
		for(int x = 0; x < _s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	Perlin perlin(4, 4, 1);

	// losuj teren
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(x < 15 || x > _s - 15 || y < 15 || y > _s - 15)
				h[x + y*(_s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
			h[x + y*(_s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

OutsideObject trees[] = {
	"tree", nullptr, Vec2(2,6),
	"tree2", nullptr, Vec2(2,6),
	"tree3", nullptr, Vec2(2,6)
};
const uint n_trees = countof(trees);

OutsideObject trees2[] = {
	"tree", nullptr, Vec2(4,8),
	"tree2", nullptr, Vec2(4,8),
	"tree3", nullptr, Vec2(4,8),
	"withered_tree", nullptr, Vec2(1,4)
};
const uint n_trees2 = countof(trees2);

OutsideObject misc[] = {
	"grass", nullptr, Vec2(1.f,1.5f),
	"grass", nullptr, Vec2(1.f,1.5f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"rock", nullptr, Vec2(1.f,1.f),
	"fern", nullptr, Vec2(1,2)
};
const uint n_misc = countof(misc);

void Game::SpawnForestObjects(int road_dir)
{
	if(!trees[0].obj)
	{
		for(uint i = 0; i < n_trees; ++i)
			trees[i].obj = BaseObject::Get(trees[i].name);
		for(uint i = 0; i < n_trees2; ++i)
			trees2[i].obj = BaseObject::Get(trees2[i].name);
		for(uint i = 0; i < n_misc; ++i)
			misc[i].obj = BaseObject::Get(misc[i].name);
	}

	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;

	// obelisk
	if(Rand() % (road_dir == -1 ? 10 : 15) == 0)
	{
		Vec3 pos;
		if(road_dir == -1)
			pos = Vec3(127.f, 0, 127.f);
		else if(road_dir == 0)
			pos = Vec3(127.f, 0, Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f);
		else
			pos = Vec3(Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f, 0, 127.f);
		terrain->SetH(pos);
		pos.y -= 1.f;
		SpawnObjectEntity(local_ctx, BaseObject::Get("obelisk"), pos, 0.f);
	}
	else if(Rand() % 16 == 0)
	{
		// tree with rocks around it
		Vec3 pos(Random(48.f, 208.f), 0, Random(48.f, 208.f));
		pos.y = terrain->GetH(pos) - 1.f;
		SpawnObjectEntity(local_ctx, trees2[3].obj, pos, Random(MAX_ANGLE), 4.f);
		for(int i = 0; i < 12; ++i)
		{
			Vec3 pos2 = pos + Vec3(sin(PI * 2 * i / 12)*8.f, 0, cos(PI * 2 * i / 12)*8.f);
			pos2.y = terrain->GetH(pos2);
			SpawnObjectEntity(local_ctx, misc[4].obj, pos2, Random(MAX_ANGLE));
		}
	}

	// drzewa
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		TERRAIN_TILE co = tiles[pt.x + pt.y*OutsideLocation::size].t;
		if(co == TT_GRASS)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = trees[Rand() % n_trees];
			SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
		else if(co == TT_GRASS3)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			int co;
			if(Rand() % 12 == 0)
				co = 3;
			else
				co = Rand() % 3;
			OutsideObject& o = trees2[co];
			SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}

	// inne
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = misc[Rand() % n_misc];
			SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

void Game::SpawnForestItems(int count_mod)
{
	assert(InRange(count_mod, -2, 1));

	// get count to spawn
	int herbs, green_herbs;
	switch(count_mod)
	{
	case -2:
		green_herbs = 0;
		herbs = Random(1, 3);
		break;
	case -1:
		green_herbs = 0;
		herbs = Random(2, 5);
		break;
	default:
	case 0:
		green_herbs = Random(0, 1);
		herbs = Random(5, 10);
		break;
	case 1:
		green_herbs = Random(1, 2);
		herbs = Random(10, 15);
		break;
	}

	// spawn items
	struct ItemToSpawn
	{
		const Item* item;
		int count;
	};
	ItemToSpawn items_to_spawn[] = {
		Item::Get("green_herb"), green_herbs,
		Item::Get("healing_herb"), herbs
	};
	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;
	Vec2 region_size(2.f, 2.f);
	for(const ItemToSpawn& to_spawn : items_to_spawn)
	{
		for(int i = 0; i < to_spawn.count; ++i)
		{
			for(int tries = 0; tries < 5; ++tries)
			{
				Int2 pt(Random(8, OutsideLocation::size - 9), Random(8, OutsideLocation::size - 9));
				TERRAIN_TILE type = tiles[pt.x + pt.y*OutsideLocation::size].t;
				if(type == TT_GRASS || type == TT_GRASS3)
				{
					SpawnGroundItemInsideRegion(to_spawn.item, Vec2(2.f*pt.x, 2.f*pt.y), region_size, false);
					break;
				}
			}
		}
	}
}

void Game::SpawnForestUnits(const Vec3& team_pos)
{
	// zbierz grupy
	static TmpUnitGroup groups[4] = {
		{ UnitGroup::TryGet("wolfs") },
		{ UnitGroup::TryGet("spiders") },
		{ UnitGroup::TryGet("rats") },
		{ UnitGroup::TryGet("animals") }
	};
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	const int level = GetDungeonLevel();
	static vector<Vec2> poss;
	poss.clear();
	OutsideLocation* outside = (OutsideLocation*)L.location;
	poss.push_back(Vec2(team_pos.x, team_pos.z));

	// ustal wrogów
	for(int i = 0; i < 4; ++i)
		groups[i].Fill(level);

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 24.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// losuj grupe
			TmpUnitGroup& group = groups[Rand() % 4];
			if(group.entries.empty())
				continue;

			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			int levels = level * 2;
			if(Rand() % 5 == 0 && ud_hunter->level.x <= level)
			{
				int enemy_level = Random(ud_hunter->level.x, Min(ud_hunter->level.y, levels, level));
				if(SpawnUnitNearLocation(local_ctx, pos3, *ud_hunter, nullptr, enemy_level, 6.f))
					levels -= enemy_level;
			}
			while(levels > 0)
			{
				int k = Rand() % group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = Random(ud->level.x, Min(ud->level.y, levels, level));
				if(!SpawnUnitNearLocation(local_ctx, pos3, *ud, nullptr, enemy_level, 6.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

void Game::RepositionCityUnits()
{
	const int a = int(0.15f*OutsideLocation::size) + 3;
	const int b = int(0.85f*OutsideLocation::size) - 3;

	UnitData* citizen;
	if(city_ctx->IsVillage())
		citizen = UnitData::Get("villager");
	else
		citizen = UnitData::Get("citizen");
	UnitData* guard = UnitData::Get("guard_move");
	InsideBuilding* inn = city_ctx->FindInn();

	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsAlive() && u.IsAI())
		{
			if(u.ai->goto_inn)
				WarpToArea(inn->ctx, (Rand() % 5 == 0 ? inn->arena2 : inn->arena1), u.GetUnitRadius(), u.pos);
			else if(u.data == citizen || u.data == guard)
			{
				for(int j = 0; j < 50; ++j)
				{
					Int2 pt(Random(a, b), Random(a, b));
					if(city_ctx->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
					{
						WarpUnit(u, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1));
						break;
					}
				}
			}
		}
	}
}

void Game::Event_RandomEncounter(int)
{
	dialog_enc = nullptr;
	if(Net::IsOnline())
		Net::PushChange(NetChange::CLOSE_ENCOUNTER);
	W.StartEncounter();
	EnterLocation();
}

void Game::GenerateEncounterMap(Location& loc)
{
	OutsideLocation* outside = (OutsideLocation*)&loc;

	// stwórz mapê jeœli jeszcze nie ma
	const int _s = OutsideLocation::size;
	if(!outside->tiles)
	{
		outside->tiles = new TerrainTile[_s*_s];
		outside->h = new float[(_s + 1)*(_s + 1)];
		memset(outside->tiles, 0, sizeof(TerrainTile)*_s*_s);
	}

	// 0 - w prawo, 1 - w góre, 2 - w lewo, 3 - w dó³
	enc_kierunek = Rand() % 4;

	Perlin perlin2(4, 4, 1);

	for(int i = 0, y = 0; y < _s; ++y)
	{
		for(int x = 0; x < _s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	Perlin perlin(4, 4, 1);

	// losuj teren
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(x < 15 || x > _s - 15 || y < 15 || y > _s - 15)
				h[x + y*(_s + 1)] += Random(10.f, 15.f);
		}
	}

	if(enc_kierunek == 0 || enc_kierunek == 2)
	{
		for(int y = 62; y < 66; ++y)
		{
			for(int x = 0; x < _s; ++x)
			{
				outside->tiles[x + y*_s].Set(TT_SAND, TM_ROAD);
				h[x + y*(_s + 1)] = 1.f;
			}
		}
		for(int x = 0; x < _s; ++x)
		{
			h[x + 61 * (_s + 1)] = 1.f;
			h[x + 66 * (_s + 1)] = 1.f;
			h[x + 67 * (_s + 1)] = 1.f;
		}
	}
	else
	{
		for(int y = 0; y < _s; ++y)
		{
			for(int x = 62; x < 66; ++x)
			{
				outside->tiles[x + y*_s].Set(TT_SAND, TM_ROAD);
				h[x + y*(_s + 1)] = 1.f;
			}
		}
		for(int y = 0; y < _s; ++y)
		{
			h[61 + y*(_s + 1)] = 1.f;
			h[66 + y*(_s + 1)] = 1.f;
			h[67 + y*(_s + 1)] = 1.f;
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
			h[x + y*(_s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	if(enc_kierunek == 0 || enc_kierunek == 2)
	{
		for(int y = 61; y <= 67; ++y)
		{
			for(int x = 1; x < _s - 1; ++x)
				h[x + y*(_s + 1)] = (h[x + y*(_s + 1)] + h[x + 1 + y*(_s + 1)] + h[x - 1 + y*(_s + 1)] + h[x + (y - 1)*(_s + 1)] + h[x + (y + 1)*(_s + 1)]) / 5;
		}
	}
	else
	{
		for(int y = 1; y < _s - 1; ++y)
		{
			for(int x = 61; x <= 67; ++x)
				h[x + y*(_s + 1)] = (h[x + y*(_s + 1)] + h[x + 1 + y*(_s + 1)] + h[x - 1 + y*(_s + 1)] + h[x + (y - 1)*(_s + 1)] + h[x + (y + 1)*(_s + 1)]) / 5;
		}
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

void Game::SpawnEncounterUnits(GameDialog*& dialog, Unit*& talker, Quest*& quest)
{
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
				GenerateMerchantItems(chest_merchant, 1000);
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
				SpawnObjectNearLocation(local_ctx, BaseObject::Get("wagon"), Vec2(128, 128), Random(MAX_ANGLE));
				Chest* chest = SpawnObjectNearLocation(local_ctx, BaseObject::Get("chest"), Vec2(128, 128), Random(MAX_ANGLE), 6.f);
				if(chest)
				{
					int gold;
					GenerateTreasure(5, 5, chest->items, gold, false);
					InsertItemBare(chest->items, gold_item_ptr, (uint)gold);
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
		talker = SpawnUnitNearLocation(local_ctx, spawn_pos, *essential, &look_pt, Clamp(essential->level.Random(), level / 2, level), 4.f);
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
		SpawnUnitsGroup(local_ctx, spawn_pos, &look_pt, count, group, level, [&](Unit* u)
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
		SpawnUnitsGroup(local_ctx, spawn_pos, &look_pt, count2, group, level2,
			[&](Unit* u) { u->dont_attack = dont_attack; });
	}
}

void Game::SpawnUnitsGroup(LevelContext& ctx, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback)
{
	static TmpUnitGroup tgroup;
	tgroup.group = group;
	tgroup.Fill(level);

	for(uint i = 0; i < count; ++i)
	{
		int x = Rand() % tgroup.total,
			y = 0;
		for(auto& entry : tgroup.entries)
		{
			y += entry.count;
			if(x < y)
			{
				Unit* u = SpawnUnitNearLocation(ctx, pos, *entry.ud, look_at, Clamp(entry.ud->level.Random(), level / 2, level), 4.f);
				if(u && callback)
					callback(u);
				break;
			}
		}
	}
}

void Game::SpawnEncounterObjects()
{
	SpawnForestObjects((enc_kierunek == 0 || enc_kierunek == 2) ? 0 : 1);
}

void Game::SpawnEncounterTeam()
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

	AddPlayerTeam(pos, dir, false, true);
}

// up³yw czasu
// 1-10 dni (usuñ zw³oki)
// 5-30 dni (usuñ krew)
// 1+ zamknij/otwórz drzwi
template<typename T>
struct RemoveRandomPred
{
	int chance, a, b;

	RemoveRandomPred(int chance, int a, int b)
	{
	}

	bool operator () (const T&)
	{
		return Random(a, b) < chance;
	}
};

void Game::UpdateLocation(LevelContext& ctx, int days, int open_chance, bool reset)
{
	if(days <= 10)
	{
		// usuñ niektóre zw³oki i przedmioty
		for(Unit*& u : *ctx.units)
		{
			if(!u->IsAlive() && Random(4, 10) < days)
			{
				delete u;
				u = nullptr;
			}
		}
		RemoveNullElements(ctx.units);
		auto from = std::remove_if(ctx.items->begin(), ctx.items->end(), RemoveRandomPred<GroundItem*>(days, 0, 10));
		auto end = ctx.items->end();
		if(from != end)
		{
			for(vector<GroundItem*>::iterator it = from; it != end; ++it)
				delete *it;
			ctx.items->erase(from, end);
		}
	}
	else
	{
		// usuñ wszystkie zw³oki i przedmioty
		if(reset)
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
				delete *it;
			ctx.units->clear();
		}
		else
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if(!(*it)->IsAlive())
				{
					delete *it;
					*it = nullptr;
				}
			}
			RemoveNullElements(ctx.units);
		}
		DeleteElements(ctx.items);
	}

	// wylecz jednostki
	if(!reset)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if((*it)->IsAlive())
				(*it)->NaturalHealing(days);
		}
	}

	if(days > 30)
	{
		// usuñ krew
		ctx.bloods->clear();
	}
	else if(days >= 5)
	{
		// usuñ czêœciowo krew
		RemoveElements(ctx.bloods, RemoveRandomPred<Blood>(days, 4, 30));
	}

	if(ctx.traps)
	{
		if(days > 30)
		{
			// usuñ wszystkie jednorazowe pu³apki
			for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL)
				{
					delete *it;
					if(it + 1 == end)
					{
						ctx.traps->pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						ctx.traps->pop_back();
						end = ctx.traps->end();
					}
				}
				else
					++it;
			}
		}
		else if(days >= 5)
		{
			// usuñ czêœæ 1razowych pu³apek
			for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL && Rand() % 30 < days)
				{
					delete *it;
					if(it + 1 == end)
					{
						ctx.traps->pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						ctx.traps->pop_back();
						end = ctx.traps->end();
					}
				}
				else
					++it;
			}
		}
	}

	// losowo otwórz/zamknij drzwi
	if(ctx.doors)
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.locked == 0)
			{
				if(Rand() % 100 < open_chance)
					door.state = Door::Open;
				else
					door.state = Door::Closed;
			}
		}
	}
}

void Game::UpdateLocation(int days, int open_chance, bool reset)
{
	UpdateLocation(local_ctx, days, open_chance, reset);

	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			UpdateLocation((*it)->ctx, days, open_chance, reset);
	}
}

void Game::GenerateCamp(Location& loc)
{
	OutsideLocation* outside = (OutsideLocation*)&loc;

	// stwórz mapê jeœli jeszcze nie ma
	const int _s = OutsideLocation::size;
	if(!outside->tiles)
	{
		outside->tiles = new TerrainTile[_s*_s];
		outside->h = new float[(_s + 1)*(_s + 1)];
		memset(outside->tiles, 0, sizeof(TerrainTile)*_s*_s);
	}

	Perlin perlin2(4, 4, 1);

	for(int i = 0, y = 0; y < _s; ++y)
	{
		for(int x = 0; x < _s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	Perlin perlin(4, 4, 1);

	// losuj teren
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(x < 15 || x > _s - 15 || y < 15 || y > _s - 15)
				h[x + y*(_s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
			h[x + y*(_s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

cstring camp_objs[] = {
	"barrel",
	"barrels",
	"box",
	"boxes",
	"bow_target",
	"torch",
	"torch_off",
	"tanning_rack",
	"hay",
	"firewood",
	"bench",
	"chest",
	"melee_target",
	"anvil",
	"cauldron"
};
const uint n_camp_objs = countof(camp_objs);
BaseObject* camp_objs_ptrs[n_camp_objs];

void Game::SpawnCampObjects()
{
	SpawnForestObjects();

	vector<Vec2> pts;
	BaseObject* ognisko = BaseObject::Get("campfire"),
		*ognisko_zgaszone = BaseObject::Get("campfire_off"),
		*namiot = BaseObject::Get("tent"),
		*poslanie = BaseObject::Get("bedding");

	if(!camp_objs_ptrs[0])
	{
		for(uint i = 0; i < n_camp_objs; ++i)
			camp_objs_ptrs[i] = BaseObject::Get(camp_objs[i]);
	}

	for(int i = 0; i < 20; ++i)
	{
		Vec2 pt = Vec2::Random(Vec2(96, 96), Vec2(256 - 96, 256 - 96));

		// sprawdŸ czy nie ma w pobli¿u ogniska
		bool jest = false;
		for(vector<Vec2>::iterator it = pts.begin(), end = pts.end(); it != end; ++it)
		{
			if(Vec2::Distance(pt, *it) < 16.f)
			{
				jest = true;
				break;
			}
		}
		if(jest)
			continue;

		pts.push_back(pt);

		// ognisko
		if(SpawnObjectNearLocation(local_ctx, Rand() % 5 == 0 ? ognisko_zgaszone : ognisko, pt, Random(MAX_ANGLE)))
		{
			// namioty / pos³ania
			for(int j = 0, ile = Random(4, 7); j < ile; ++j)
			{
				float kat = Random(MAX_ANGLE);
				bool czy_namiot = (Rand() % 2 == 0);
				if(czy_namiot)
					SpawnObjectNearLocation(local_ctx, namiot, pt + Vec2(sin(kat), cos(kat))*Random(4.f, 5.5f), pt);
				else
					SpawnObjectNearLocation(local_ctx, poslanie, pt + Vec2(sin(kat), cos(kat))*Random(3.f, 4.f), pt);
			}
		}
	}

	for(int i = 0; i < 100; ++i)
	{
		Vec2 pt = Vec2::Random(Vec2(90, 90), Vec2(256 - 90, 256 - 90));
		bool ok = true;
		for(vector<Vec2>::iterator it = pts.begin(), end = pts.end(); it != end; ++it)
		{
			if(Vec2::Distance(*it, pt) < 4.f)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			BaseObject* obj = camp_objs_ptrs[Rand() % n_camp_objs];
			auto e = SpawnObjectNearLocation(local_ctx, obj, pt, Random(MAX_ANGLE), 2.f);
			if(e.IsChest() && L.location->spawn != SG_NONE) // empty chests for empty camps
			{
				int gold, level = L.location->st;
				Chest* chest = (Chest*)e;

				GenerateTreasure(level, 5, chest->items, gold, false);
				InsertItemBare(chest->items, gold_item_ptr, (uint)gold);
				SortItems(chest->items);
			}
		}
	}
}

void Game::SpawnCampUnits()
{
	static TmpUnitGroup group;
	static vector<Vec2> poss;
	poss.clear();
	OutsideLocation* outside = (OutsideLocation*)L.location;
	int level = outside->st;
	cstring group_name;

	switch(outside->spawn)
	{
	default:
		assert(0);
	case SG_BANDITS:
		group_name = "bandits";
		break;
	case SG_ORCS:
		group_name = "orcs";
		break;
	case SG_GOBLINS:
		group_name = "goblins";
		break;
	case SG_MAGES:
		group_name = "mages";
		break;
	case SG_EVIL:
		group_name = "evil";
		break;
	case SG_NONE:
		// spawn empty camp, no units
		return;
	}

	// ustal wrogów
	group.group = UnitGroup::TryGet(group_name);
	group.Fill(level);

	for(int added = 0, tries = 50; added < 5 && tries>0; --tries)
	{
		Vec2 pos = Vec2::Random(Vec2(90, 90), Vec2(256 - 90, 256 - 90));

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 8.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// losuj grupe
			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			int levels = level * 2;
			while(levels > 0)
			{
				int k = Rand() % group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = Random(ud->level.x, Min(ud->level.y, levels, level));
				if(!SpawnUnitNearLocation(local_ctx, pos3, *ud, nullptr, enemy_level, 6.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

Game::ObjectEntity Game::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range, float margin, float scale)
{
	bool ok = false;
	if(obj->type == OBJ_CYLINDER)
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, obj->r + margin + range);
		float extra_radius = range / 20;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, pt, obj->r + margin))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

Game::ObjectEntity Game::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range, float margin,
	float scale)
{
	if(obj->type == OBJ_CYLINDER)
		return SpawnObjectNearLocation(ctx, obj, pos, Vec2::LookAtAngle(pos, rot_target), range, margin, scale);
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20, rot;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		bool ok = false;
		for(int i = 0; i < 20; ++i)
		{
			rot = Vec2::LookAtAngle(Vec2(pt.x, pt.z), rot_target);
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

void Game::SpawnTmpUnits(City* city)
{
	InsideBuilding* inn = city->FindInn();
	CityBuilding* training_grounds = city->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS);

	// heroes
	uint count;
	Int2 level;
	if(W.CheckFirstCity())
	{
		count = 4;
		level = Int2(2, 5);
	}
	else
	{
		count = Random(1u, 4u);
		level = Int2(2, 15);
	}

	for(uint i = 0; i < count; ++i)
	{
		UnitData& ud = GetHero(ClassInfo::GetRandom());

		if(Rand() % 2 == 0 || !training_grounds)
		{
			// inside inn
			SpawnUnitInsideInn(ud, level.Random(), inn, true);
		}
		else
		{
			// on training grounds
			Unit* u = SpawnUnitNearLocation(local_ctx, Vec3(2.f*training_grounds->unit_pt.x + 1, 0, 2.f*training_grounds->unit_pt.y + 1), ud, nullptr,
				level.Random(), 8.f);
			if(u)
				u->temporary = true;
		}
	}

	// quest traveler (100% chance in city, 50% in village)
	if(!city_ctx->IsVillage() || Rand() % 2 == 0)
		SpawnUnitInsideInn(*UnitData::Get("traveler"), -2, inn, SU_TEMPORARY);
}

void Game::RemoveTmpUnits(City* city)
{
	RemoveTmpUnits(local_ctx);

	for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it)
		RemoveTmpUnits((*it)->ctx);
}

void Game::RemoveTmpUnits(LevelContext& ctx)
{
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end;)
	{
		Unit* u = *it;
		if(u->temporary)
		{
			delete u;

			if(it + 1 == end)
			{
				ctx.units->pop_back();
				return;
			}
			else
			{
				it = ctx.units->erase(it);
				end = ctx.units->end();
			}
		}
		else
			++it;
	}
}

void Game::GenerateMoonwell(Location& loc)
{
	OutsideLocation* outside = (OutsideLocation*)&loc;

	// stwórz mapê jeœli jeszcze nie ma
	const int _s = OutsideLocation::size;
	outside->tiles = new TerrainTile[_s*_s];
	outside->h = new float[(_s + 1)*(_s + 1)];
	memset(outside->tiles, 0, sizeof(TerrainTile)*_s*_s);

	Perlin perlin2(4, 4, 1);

	for(int i = 0, y = 0; y < _s; ++y)
	{
		for(int x = 0; x < _s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	Perlin perlin(4, 4, 1);

	// losuj teren
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(x < 15 || x > _s - 15 || y < 15 || y > _s - 15)
				h[x + y*(_s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
			h[x + y*(_s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
	}

	// ustaw zielon¹ trawê na œrodku
	for(int y = 40; y < _s - 40; ++y)
	{
		for(int x = 40; x < _s - 40; ++x)
		{
			float d;
			if((d = Distance(float(x), float(y), 64.f, 64.f)) < 8.f)
			{
				outside->tiles[x + y*_s].t = TT_GRASS;
				h[x + y*(_s + 1)] += (1.f - d / 8.f) * 5;
			}
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

void Game::SpawnMoonwellObjects()
{
	Vec3 pos(128.f, 0, 128.f);
	terrain->SetH(pos);
	pos.y -= 0.2f;
	SpawnObjectEntity(local_ctx, BaseObject::Get("moonwell"), pos, 0.f);
	SpawnObjectEntity(local_ctx, BaseObject::Get("moonwell_phy"), pos, 0.f);

	if(!trees[0].obj)
	{
		for(uint i = 0; i < n_trees; ++i)
			trees[i].obj = BaseObject::Get(trees[i].name);
		for(uint i = 0; i < n_trees2; ++i)
			trees2[i].obj = BaseObject::Get(trees2[i].name);
		for(uint i = 0; i < n_misc; ++i)
			misc[i].obj = BaseObject::Get(misc[i].name);
	}

	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;

	// drzewa
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			TERRAIN_TILE co = tiles[pt.x + pt.y*OutsideLocation::size].t;
			if(co == TT_GRASS)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = trees[Rand() % n_trees];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
			else if(co == TT_GRASS3)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				int co;
				if(Rand() % 12 == 0)
					co = 3;
				else
					co = Rand() % 3;
				OutsideObject& o = trees2[co];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}

	// inne
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = misc[Rand() % n_misc];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}
}

void Game::SpawnMoonwellUnits(const Vec3& team_pos)
{
	// zbierz grupy
	static TmpUnitGroup groups[4] = {
		{ UnitGroup::TryGet("wolfs") },
		{ UnitGroup::TryGet("spiders") },
		{ UnitGroup::TryGet("rats") },
		{ UnitGroup::TryGet("animals") }
	};
	UnitData* ud_hunter = UnitData::Get("wild_hunter");
	int level = GetDungeonLevel();
	static vector<Vec2> poss;
	poss.clear();
	OutsideLocation* outside = (OutsideLocation*)L.location;
	poss.push_back(Vec2(team_pos.x, team_pos.z));

	// ustal wrogów
	for(int i = 0; i < 4; ++i)
		groups[i].Fill(level);

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();
		if(Vec2::Distance(pos, Vec2(128.f, 128.f)) < 12.f)
			continue;

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 24.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			// losuj grupe
			TmpUnitGroup& group = groups[Rand() % 4];
			if(group.entries.empty())
				continue;

			poss.push_back(pos);
			++added;

			Vec3 pos3(pos.x, 0, pos.y);

			// postaw jednostki
			int levels = level * 2;
			if(Rand() % 5 == 0 && ud_hunter->level.x <= level)
			{
				int enemy_level = Random(ud_hunter->level.x, Min(ud_hunter->level.y, levels, level));
				if(SpawnUnitNearLocation(local_ctx, pos3, *ud_hunter, nullptr, enemy_level, 6.f))
					levels -= enemy_level;
			}
			while(levels > 0)
			{
				int k = Rand() % group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = Random(ud->level.x, Min(ud->level.y, levels, level));
				if(!SpawnUnitNearLocation(local_ctx, pos3, *ud, nullptr, enemy_level, 6.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

void Game::SpawnObjectExtras(LevelContext& ctx, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale, int flags)
{
	assert(obj);

	// ogieñ pochodni
	if(!IS_SET(flags, SOE_DONT_SPAWN_PARTICLES))
	{
		if(IS_SET(obj->flags, OBJ_LIGHT))
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 3, -1);
			pe->speed_max = Vec3(1, 4, 1);
			pe->mode = 1;
			pe->Init();
			ctx.pes->push_back(pe);

			pe->tex = tFlare;
			if(IS_SET(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					pe->tex = tFlare2;
			}

			// œwiat³o
			if(!IS_SET(flags, SOE_DONT_CREATE_LIGHT) && ctx.lights)
			{
				Light& s = Add1(ctx.lights);
				s.pos = pe->pos;
				s.range = 5;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					s.color = Vec3(0.8f, 0.8f, 1.f);
				else
					s.color = Vec3(1.f, 0.9f, 0.9f);
			}
		}
		else if(IS_SET(obj->flags, OBJ_BLOOD_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 4, -1);
			pe->speed_max = Vec3(1, 6, 1);
			pe->mode = 0;
			pe->tex = tKrew[BLOOD_RED];
			pe->size = 0.5f;
			pe->Init();
			ctx.pes->push_back(pe);
		}
		else if(IS_SET(obj->flags, OBJ_WATER_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 500;
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
			pe->particle_life = 3.f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 4;
			pe->spawn_max = 8;
			pe->speed_min = Vec3(-0.6f, 4, -0.6f);
			pe->speed_max = Vec3(0.6f, 7, 0.6f);
			pe->mode = 0;
			pe->tex = tWoda;
			pe->size = 0.05f;
			pe->Init();
			ctx.pes->push_back(pe);
		}
	}

	// fizyka
	if(obj->shape)
	{
		CollisionObject& c = Add1(ctx.colliders);
		c.ptr = user_ptr;

		int group = CG_OBJECT;
		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
			group |= CG_CAMERA_COLLIDER;

		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(obj->shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);

		if(obj->type == OBJ_CYLINDER)
		{
			cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2, pos.z));
			c.type = CollisionObject::SPHERE;
			c.pt = Vec2(pos.x, pos.z);
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			btTransform& tr = cobj->getWorldTransform();
			m1 = Matrix::RotationY(rot);
			m2 = *obj->matrix * m1;
			Vec3 pos2 = Vec3::TransformZero(m2);
			pos2 += pos;
			tr.setOrigin(ToVector3(pos2));
			tr.setRotation(btQuaternion(rot, 0, 0));

			c.pt = Vec2(pos2.x, pos2.z);
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
		else
		{
			m1 = Matrix::RotationY(rot);
			m2 = Matrix::Translation(pos);
			// skalowanie jakimœ sposobem przechodzi do btWorldTransform i przy rysowaniu jest z³a skala (dwukrotnie u¿yta)
			m3 = Matrix::Scale(1.f / obj->size.x, 1.f, 1.f / obj->size.y);
			m3 = m3 * *obj->matrix * m1 * m2;
			cobj->getWorldTransform().setFromOpenGLMatrix(&m3._11);
			Vec3 out_pos = Vec3::TransformZero(m3);
			Quat q = Quat::CreateFromRotationMatrix(m3);

			float yaw = asin(-2 * (q.x*q.z - q.w*q.y));
			c.pt = Vec2(out_pos.x, out_pos.z);
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(yaw))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = yaw;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}

		phy_world->addCollisionObject(cobj, group);

		if(IS_SET(obj->flags, OBJ_PHYSICS_PTR))
		{
			assert(user_ptr);
			cobj->setUserPointer(user_ptr);
		}

		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
			c.ptr = CAM_COLLIDER;

		if(IS_SET(obj->flags, OBJ_DOUBLE_PHYSICS))
			SpawnObjectExtras(ctx, obj->next_obj, pos, rot, user_ptr, scale, flags);
		else if(IS_SET(obj->flags, OBJ_MULTI_PHYSICS))
		{
			for(int i = 0;; ++i)
			{
				if(obj->next_obj[i].shape)
					SpawnObjectExtras(ctx, &obj->next_obj[i], pos, rot, user_ptr, scale, flags);
				else
					break;
			}
		}
	}
	else if(IS_SET(obj->flags, OBJ_SCALEABLE))
	{
		CollisionObject& c = Add1(ctx.colliders);
		c.type = CollisionObject::SPHERE;
		c.pt = Vec2(pos.x, pos.z);
		c.radius = obj->r*scale;

		btCollisionObject* cobj = new btCollisionObject;
		btCylinderShape* shape = new btCylinderShape(btVector3(obj->r*scale, obj->h*scale, obj->r*scale));
		shapes.push_back(shape);
		cobj->setCollisionShape(shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);
		cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2 * scale, pos.z));
		phy_world->addCollisionObject(cobj, CG_OBJECT);
	}

	if(IS_SET(obj->flags, OBJ_CAM_COLLIDERS))
	{
		int roti = (int)round((rot / (PI / 2)));
		for(vector<Mesh::Point>::const_iterator it = obj->mesh->attach_points.begin(), end = obj->mesh->attach_points.end(); it != end; ++it)
		{
			const Mesh::Point& pt = *it;
			if(strncmp(pt.name.c_str(), "camcol", 6) != 0)
				continue;

			m2 = pt.mat * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(m2) + pos;

			btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
			shapes.push_back(shape);
			btCollisionObject* co = new btCollisionObject;
			co->setCollisionShape(shape);
			co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
			co->getWorldTransform().setOrigin(ToVector3(pos2));
			phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
			if(roti != 0)
				co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

			float w = pt.size.x, h = pt.size.z;
			if(roti == 1 || roti == 3)
				std::swap(w, h);

			CameraCollider& cc = Add1(cam_colliders);
			cc.box.v1.x = pos2.x - w;
			cc.box.v2.x = pos2.x + w;
			cc.box.v1.z = pos2.z - h;
			cc.box.v2.z = pos2.z + h;
			cc.box.v1.y = pos2.y - pt.size.y;
			cc.box.v2.y = pos2.y + pt.size.y;
		}
	}
}

void Game::GenerateSecretLocation(Location& loc)
{
	QM.quest_secret->state = Quest_Secret::SECRET_GENERATED2;

	OutsideLocation* outside = (OutsideLocation*)&loc;

	// stwórz mapê jeœli jeszcze nie ma
	const int _s = OutsideLocation::size;
	outside->tiles = new TerrainTile[_s*_s];
	outside->h = new float[(_s + 1)*(_s + 1)];
	memset(outside->tiles, 0, sizeof(TerrainTile)*_s*_s);

	Perlin perlin2(4, 4, 1);

	for(int i = 0, y = 0; y < _s; ++y)
	{
		for(int x = 0; x < _s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}

	Perlin perlin(4, 4, 1);

	// losuj teren
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
			h[x + y*(_s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
	}

	terrain->RoundHeight();

	float h1 = h[64 + 32 * (_s + 1)],
		h2 = h[64 + 96 * (_s + 2)];

	// wyrównaj teren ko³o portalu i budynku
	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(Distance(float(x), float(y), 64.f, 32.f) < 4.f)
				h[x + y*(_s + 1)] = h1;
			else if(Distance(float(x), float(y), 64.f, 96.f) < 12.f)
				h[x + y*(_s + 1)] = h2;
		}
	}

	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(x < 15 || x > _s - 15 || y < 15 || y > _s - 15)
				h[x + y*(_s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();

	for(uint y = 0; y < _s; ++y)
	{
		for(uint x = 0; x < _s; ++x)
		{
			if(Distance(float(x), float(y), 64.f, 32.f) < 4.f)
				h[x + y*(_s + 1)] = h1;
			else if(Distance(float(x), float(y), 64.f, 96.f) < 12.f)
				h[x + y*(_s + 1)] = h2;
		}
	}

	terrain->RemoveHeightMap();
}

void Game::SpawnSecretLocationObjects()
{
	Vec3 pos(128.f, 0, 96.f * 2);
	terrain->SetH(pos);
	BaseObject* o = BaseObject::Get("tomashu_dom");
	pos.y += 0.05f;
	SpawnObjectEntity(local_ctx, o, pos, 0);
	ProcessBuildingObjects(local_ctx, nullptr, nullptr, o->mesh, nullptr, 0.f, 0, Vec3(0, 0, 0), nullptr, nullptr, false);

	pos.z = 64.f;
	terrain->SetH(pos);
	SpawnObjectEntity(local_ctx, BaseObject::Get("portal"), pos, 0);

	Portal* portal = new Portal;
	portal->at_level = 0;
	portal->next_portal = nullptr;
	portal->pos = pos;
	portal->rot = 0.f;
	portal->target = 0;
	portal->target_loc = QM.quest_secret->where;
	L.location->portal = portal;

	if(!trees[0].obj)
	{
		for(uint i = 0; i < n_trees; ++i)
			trees[i].obj = BaseObject::Get(trees[i].name);
		for(uint i = 0; i < n_trees2; ++i)
			trees2[i].obj = BaseObject::Get(trees2[i].name);
		for(uint i = 0; i < n_misc; ++i)
			misc[i].obj = BaseObject::Get(misc[i].name);
	}

	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;

	// drzewa
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 32.f) > 4.f
			&& Distance(float(pt.x), float(pt.y), 64.f, 96.f) > 12.f)
		{
			TERRAIN_TILE co = tiles[pt.x + pt.y*OutsideLocation::size].t;
			if(co == TT_GRASS)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = trees[Rand() % n_trees];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
			else if(co == TT_GRASS3)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				int co;
				if(Rand() % 12 == 0)
					co = 3;
				else
					co = Rand() % 3;
				OutsideObject& o = trees2[co];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}

	// inne
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 32.f) > 4.f
			&& Distance(float(pt.x), float(pt.y), 64.f, 96.f) > 12.f)
		{
			if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = misc[Rand() % n_misc];
				SpawnObjectEntity(local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}
}

void Game::SpawnSecretLocationUnits()
{
	OutsideLocation* outside = (OutsideLocation*)L.location;
	UnitData* golem = UnitData::Get("golem_adamantine");
	static vector<Vec2> poss;

	poss.push_back(Vec2(128.f, 64.f));
	poss.push_back(Vec2(128.f, 256.f - 64.f));

	for(int added = 0, tries = 50; added < 10 && tries>0; --tries)
	{
		Vec2 pos = outside->GetRandomPos();

		bool ok = true;
		for(vector<Vec2>::iterator it = poss.begin(), end = poss.end(); it != end; ++it)
		{
			if(Vec2::Distance(pos, *it) < 32.f)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			SpawnUnitNearLocation(local_ctx, Vec3(pos.x, 0, pos.y), *golem, nullptr, -2);
			poss.push_back(pos);
			++added;
		}
	}

	poss.clear();
}

void Game::SpawnTeamSecretLocation()
{
	AddPlayerTeam(Vec3(128.f, 0.f, 66.f), PI, false, false);
}

void Game::GenerateMushrooms(int days_since)
{
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Int2 pt;
	Vec2 pos;
	int dir;
	const Item* shroom = Item::Get("mushroom");

	for(int i = 0; i < days_since * 20; ++i)
	{
		pt = Int2::Random(Int2(1, 1), Int2(lvl.w - 2, lvl.h - 2));
		if(OR2_EQ(lvl.map[pt.x + pt.y*lvl.w].type, PUSTE, KRATKA_SUFIT) && lvl.IsTileNearWall(pt, dir))
		{
			pos.x = 2.f*pt.x;
			pos.y = 2.f*pt.y;
			switch(dir)
			{
			case GDIR_LEFT:
				pos.x += 0.5f;
				pos.y += Random(-1.f, 1.f);
				break;
			case GDIR_RIGHT:
				pos.x += 1.5f;
				pos.y += Random(-1.f, 1.f);
				break;
			case GDIR_UP:
				pos.x += Random(-1.f, 1.f);
				pos.y += 1.5f;
				break;
			case GDIR_DOWN:
				pos.x += Random(-1.f, 1.f);
				pos.y += 0.5f;
				break;
			}
			SpawnGroundItemInsideRadius(shroom, pos, 0.5f);
		}
	}
}

void Game::GenerateCityPickableItems()
{
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");

	// piwa w karczmie
	InsideBuilding* inn = city_ctx->FindInn();
	const Item* beer = Item::Get("beer");
	const Item* vodka = Item::Get("vodka");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	for(vector<Object*>::iterator it = inn->ctx.objects->begin(), end = inn->ctx.objects->end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			PickableItemBegin(inn->ctx, obj);
			if(Rand() % 2 == 0)
			{
				PickableItemAdd(beer);
				if(Rand() % 4 == 0)
					PickableItemAdd(beer);
			}
			if(Rand() % 3 == 0)
				PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			PickableItemBegin(inn->ctx, obj);
			for(int i = 0, ile = Random(3, 5); i < ile; ++i)
				PickableItemAdd(beer);
			for(int i = 0, ile = Random(1, 3); i < ile; ++i)
				PickableItemAdd(vodka);
		}
	}

	// jedzenie w sklepie
	CityBuilding* food = city_ctx->FindBuilding(BuildingGroup::BG_FOOD_SELLER);
	if(food)
	{
		Object* found_obj = nullptr;
		float best_dist = 9999.f, dist;
		for(vector<Object*>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
		{
			Object& obj = **it;
			if(obj.base == shelves)
			{
				dist = Vec3::Distance(food->walk_pt, obj.pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					found_obj = &obj;
				}
			}
		}

		if(found_obj)
		{
			const ItemList* lis = ItemList::Get("food_and_drink").lis;
			PickableItemBegin(local_ctx, *found_obj);
			for(int i = 0; i < 20; ++i)
				PickableItemAdd(lis->Get());
		}
	}

	// miksturki u alchemika
	CityBuilding* alch = city_ctx->FindBuilding(BuildingGroup::BG_ALCHEMIST);
	if(alch)
	{
		Object* found_obj = nullptr;
		float best_dist = 9999.f, dist;
		for(vector<Object*>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
		{
			Object& obj = **it;
			if(obj.base == shelves)
			{
				dist = Vec3::Distance(alch->walk_pt, obj.pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					found_obj = &obj;
				}
			}
		}

		if(found_obj)
		{
			PickableItemBegin(local_ctx, *found_obj);
			const Item* heal_pot = Item::Get("p_hp");
			PickableItemAdd(heal_pot);
			if(Rand() % 2 == 0)
				PickableItemAdd(heal_pot);
			if(Rand() % 2 == 0)
				PickableItemAdd(Item::Get("p_nreg"));
			if(Rand() % 2 == 0)
				PickableItemAdd(Item::Get("healing_herb"));
		}
	}
}

namespace PickableItem
{
	struct Item
	{
		uint spawn;
		Vec3 pos;
	};
	LevelContext* ctx;
	Object* o;
	vector<Box> spawns;
	vector<Item> items;
}

void Game::PickableItemBegin(LevelContext& ctx, Object& o)
{
	assert(o.base);

	PickableItem::ctx = &ctx;
	PickableItem::o = &o;
	PickableItem::spawns.clear();
	PickableItem::items.clear();

	for(vector<Mesh::Point>::iterator it = o.base->mesh->attach_points.begin(), end = o.base->mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			assert(it->type == Mesh::Point::Box);
			Box box(it->mat._41, it->mat._42, it->mat._43);
			box.v1.x -= it->size.x - 0.05f;
			box.v1.z -= it->size.z - 0.05f;
			box.v2.x += it->size.x - 0.05f;
			box.v2.z += it->size.z - 0.05f;
			box.v1.y = box.v2.y = box.v1.y - it->size.y;
			PickableItem::spawns.push_back(box);
		}
	}

	assert(!PickableItem::spawns.empty());
}

void Game::PickableItemAdd(const Item* item)
{
	assert(item);

	for(int i = 0; i < 20; ++i)
	{
		// pobierz punkt
		uint spawn = Rand() % PickableItem::spawns.size();
		Box& box = PickableItem::spawns[spawn];
		// ustal pozycjê
		Vec3 pos(Random(box.v1.x, box.v2.x), box.v1.y, Random(box.v1.z, box.v2.z));
		// sprawdŸ kolizjê
		bool ok = true;
		for(vector<PickableItem::Item>::iterator it = PickableItem::items.begin(), end = PickableItem::items.end(); it != end; ++it)
		{
			if(it->spawn == spawn)
			{
				if(CircleToCircle(pos.x, pos.z, 0.15f, it->pos.x, it->pos.z, 0.15f))
				{
					ok = false;
					break;
				}
			}
		}
		// dodaj
		if(ok)
		{
			PickableItem::Item& i = Add1(PickableItem::items);
			i.spawn = spawn;
			i.pos = pos;

			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			gi->rot = Random(MAX_ANGLE);
			float rot = PickableItem::o->rot.y,
				s = sin(rot),
				c = cos(rot);
			gi->pos = Vec3(pos.x*c + pos.z*s, pos.y, -pos.x*s + pos.z*c) + PickableItem::o->pos;
			PickableItem::ctx->items->push_back(gi);

			break;
		}
	}
}

void Game::GenerateDungeonFood()
{
	// determine how much food to spawn
	int mod = 3;
	InsideLocation* inside = (InsideLocation*)L.location;
	BaseLocation& base = g_base_locations[inside->target];

	if(IS_SET(base.options, BLO_LESS_FOOD))
		--mod;

	SPAWN_GROUP spawn = L.location->spawn;
	if(spawn == SG_CHALLANGE)
	{
		if(dungeon_level == 0)
			spawn = SG_ORCS;
		else if(dungeon_level == 1)
			spawn = SG_MAGES_AND_GOLEMS;
		else
			spawn = SG_EVIL;
	}
	SpawnGroup& sg = g_spawn_groups[spawn];
	mod += sg.food_mod;

	if(mod <= 0)
		return;

	// get food list and base objects
	const ItemList& lis = *ItemList::Get(sg.orc_food ? "orc_food" : "normal_food").lis;
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	bool spawn_golden_cup = Rand() % 100 == 0;

	// spawn food
	for(vector<Object*>::iterator it = local_ctx.objects->begin(), end = local_ctx.objects->end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			PickableItemBegin(local_ctx, obj);
			if(spawn_golden_cup)
			{
				spawn_golden_cup = false;
				PickableItemAdd(Item::Get("golden_cup"));
			}
			else
			{
				int count = Random(mod / 2, mod);
				if(count)
				{
					for(int i = 0; i < count; ++i)
						PickableItemAdd(lis.Get());
				}
			}
			if(Rand() % 3 == 0)
				PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			int count = Random(mod, mod * 3 / 2);
			if(count)
			{
				PickableItemBegin(local_ctx, obj);
				for(int i = 0; i < count; ++i)
					PickableItemAdd(lis.Get());
			}
		}
	}
}

void Game::GenerateCityMap(Location& loc)
{
	Info("Generating city map.");

	City* city = (City*)L.location;
	if(!city->tiles)
	{
		city->tiles = new TerrainTile[OutsideLocation::size*OutsideLocation::size];
		city->h = new float[(OutsideLocation::size + 1)*(OutsideLocation::size + 1)];
	}

	gen->Init(city->tiles, city->h, OutsideLocation::size, OutsideLocation::size);
	gen->SetRoadSize(3, 32);
	gen->SetTerrainNoise(Random(3, 5), Random(3.f, 8.f), 1.f, Random(1.f, 2.f));

	gen->RandomizeHeight();

	RoadType rtype;
	int swap = 0;
	bool plaza = (Rand() % 2 == 0);
	GAME_DIR dir = (GAME_DIR)(Rand() % 4);

	switch(Rand() % 4)
	{
	case 0:
		rtype = RoadType_Plus;
		break;
	case 1:
		rtype = RoadType_Line;
		break;
	case 2:
	case 3:
		rtype = RoadType_Three;
		swap = Rand() % 6;
		break;
	}

	rtype = RoadType_Plus;

	gen->GenerateMainRoad(rtype, dir, 4, plaza, swap, city->entry_points, city->gates, true);
	gen->FlattenRoadExits();
	gen->GenerateRoads(TT_ROAD, 25);
	for(int i = 0; i < 2; ++i)
		gen->FlattenRoad();

	vector<ToBuild> tobuild;
	PrepareCityBuildings(*city, tobuild);

	gen->GenerateBuildings(tobuild);
	gen->ApplyWallTiles(city->gates);

	gen->SmoothTerrain();
	gen->FlattenRoad();

	if(plaza && Rand() % 4 != 0)
	{
		g_have_well = true;
		g_well_pt = Int2(64, 64);
	}
	else
		g_have_well = false;

	// budynki
	city->buildings.resize(tobuild.size());
	vector<ToBuild>::iterator build_it = tobuild.begin();
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it, ++build_it)
	{
		it->type = build_it->type;
		it->pt = build_it->pt;
		it->rot = build_it->rot;
		it->unit_pt = build_it->unit_pt;
	}

	terrain->SetHeightMap(city->h);

	// set exits y
	for(vector<EntryPoint>::iterator entry_it = city->entry_points.begin(), entry_end = city->entry_points.end(); entry_it != entry_end; ++entry_it)
		entry_it->exit_y = terrain->GetH(entry_it->exit_area.Midpoint()) + 0.1f;

	terrain->RemoveHeightMap();

	gen->CleanBorders();
}

void Game::GenerateVillageMap(Location& loc)
{
	Info("Generating village map.");

	City* village = (City*)L.location;
	CLEAR_BIT(village->flags, City::HaveExit);
	if(!village->tiles)
	{
		village->tiles = new TerrainTile[OutsideLocation::size*OutsideLocation::size];
		village->h = new float[(OutsideLocation::size + 1)*(OutsideLocation::size + 1)];
	}

	gen->Init(village->tiles, village->h, OutsideLocation::size, OutsideLocation::size);
	gen->SetRoadSize(3, 32);
	gen->SetTerrainNoise(Random(3, 5), Random(3.f, 8.f), 1.f, Random(2.f, 10.f));

	gen->RandomizeHeight();

	RoadType rtype;
	int roads, swap = 0;
	bool plaza;
	GAME_DIR dir = (GAME_DIR)(Rand() % 4);
	bool extra_roads;

	switch(Rand() % 6)
	{
	case 0:
		rtype = RoadType_Line;
		roads = Random(0, 2);
		plaza = (Rand() % 3 == 0);
		extra_roads = true;
		break;
	case 1:
		rtype = RoadType_Curve;
		roads = (Rand() % 4 == 0 ? 1 : 0);
		plaza = false;
		extra_roads = false;
		break;
	case 2:
		rtype = RoadType_Oval;
		roads = (Rand() % 4 == 0 ? 1 : 0);
		plaza = false;
		extra_roads = false;
		break;
	case 3:
		rtype = RoadType_Three;
		roads = Random(0, 3);
		plaza = (Rand() % 3 == 0);
		swap = Rand() % 6;
		extra_roads = true;
		break;
	case 4:
		rtype = RoadType_Sin;
		roads = (Rand() % 4 == 0 ? 1 : 0);
		plaza = (Rand() % 3 == 0);
		extra_roads = false;
		break;
	case 5:
		rtype = RoadType_Part;
		roads = (Rand() % 3 == 0 ? 1 : 0);
		plaza = (Rand() % 3 != 0);
		extra_roads = true;
		break;
	}

	gen->GenerateMainRoad(rtype, dir, roads, plaza, swap, village->entry_points, village->gates, extra_roads);
	if(extra_roads)
		gen->GenerateRoads(TT_SAND, 5);
	gen->FlattenRoadExits();
	for(int i = 0; i < 2; ++i)
		gen->FlattenRoad();

	vector<ToBuild> tobuild;
	PrepareCityBuildings(*village, tobuild);

	gen->GenerateBuildings(tobuild);
	gen->GenerateFields();
	gen->SmoothTerrain();
	gen->FlattenRoad();

	g_have_well = false;

	City* city = (City*)L.location;

	// budynki
	city->buildings.resize(tobuild.size());
	vector<ToBuild>::iterator build_it = tobuild.begin();
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it, ++build_it)
	{
		it->type = build_it->type;
		it->pt = build_it->pt;
		it->rot = build_it->rot;
		it->unit_pt = build_it->unit_pt;
	}

	gen->CleanBorders();
}

void Game::PrepareCityBuildings(City& city, vector<ToBuild>& tobuild)
{
	// required buildings
	tobuild.reserve(city.buildings.size());
	for(CityBuilding& cb : city.buildings)
		tobuild.push_back(ToBuild(cb.type, true));
	city.buildings.clear();

	// not required buildings
	LocalVector2<Building*> buildings;
	city.GenerateCityBuildings(buildings.Get(), false);
	tobuild.reserve(tobuild.size() + buildings.size());
	for(Building* b : buildings)
		tobuild.push_back(ToBuild(b, false));

	// set flags
	for(ToBuild& tb : tobuild)
	{
		if(tb.type->group == BuildingGroup::BG_TRAINING_GROUNDS)
			city.flags |= City::HaveTrainingGrounds;
		else if(tb.type->group == BuildingGroup::BG_BLACKSMITH)
			city.flags |= City::HaveBlacksmith;
		else if(tb.type->group == BuildingGroup::BG_MERCHANT)
			city.flags |= City::HaveMerchant;
		else if(tb.type->group == BuildingGroup::BG_ALCHEMIST)
			city.flags |= City::HaveAlchemist;
		else if(tb.type->group == BuildingGroup::BG_FOOD_SELLER)
			city.flags |= City::HaveFoodSeller;
		else if(tb.type->group == BuildingGroup::BG_INN)
			city.flags |= City::HaveInn;
		else if(tb.type->group == BuildingGroup::BG_ARENA)
			city.flags |= City::HaveArena;
	}
}

int Game::FindWorldUnit(Unit* unit, int hint_loc, int hint_loc2, int* out_level)
{
	assert(unit);

	int level;

	if(hint_loc != -1)
	{
		if(W.GetLocation(hint_loc)->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return hint_loc;
		}
	}

	if(hint_loc2 != -1 && hint_loc != hint_loc2)
	{
		if(W.GetLocation(hint_loc2)->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return hint_loc2;
		}
	}

	const vector<Location*>& locations = W.GetLocations();
	for(uint i = 0; i < locations.size(); ++i)
	{
		Location* loc = locations[i];
		if(loc && i != hint_loc && i != hint_loc2 && loc->state >= LS_ENTERED && loc->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return i;
		}
	}

	return -1;
}

void Game::AbadonLocation(Location* loc)
{
	assert(loc);

	// only works for OutsideLocation for now!
	assert(loc->outside && loc->type != L_CITY);

	OutsideLocation* outside = (OutsideLocation*)loc;

	// if location is open
	if(loc == L.location)
	{
		// remove units
		for(Unit*& u : outside->units)
		{
			if(u->IsAlive() && IsEnemy(*pc->unit, *u))
			{
				u->to_remove = true;
				L.to_remove.push_back(u);
			}
		}

		// remove items from chests
		for(Chest* chest : outside->chests)
		{
			if(!chest->looted)
				chest->items.clear();
		}
	}
	else
	{
		// delete units
		for(Unit*& u : outside->units)
		{
			__assume(u != nullptr);
			if(u->IsAlive() && IsEnemy(*pc->unit, *u))
			{
				delete u;
				u = nullptr;
			}
		}
		RemoveNullElements(outside->units);

		// remove items from chests
		for(Chest* chest : outside->chests)
			chest->items.clear();
	}

	loc->spawn = SG_NONE;
	loc->last_visit = W.GetWorldtime();
}
