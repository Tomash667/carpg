#include "Pch.h"
#include "Game.h"

#include "Ability.h"
#include "AbilityPanel.h"
#include "AIController.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "Cave.h"
#include "City.h"
#include "CommandParser.h"
#include "Console.h"
#include "CraftPanel.h"
#include "CreateServerPanel.h"
#include "GameFile.h"
#include "GameGui.h"
#include "GameMenu.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "Gui.h"
#include "InfoBox.h"
#include "Inventory.h"
#include "ItemHelper.h"
#include "Journal.h"
#include "Level.h"
#include "LevelGui.h"
#include "LoadingHandler.h"
#include "LoadScreen.h"
#include "LocationGeneratorFactory.h"
#include "MainMenu.h"
#include "MpBox.h"
#include "MultiplayerPanel.h"
#include "PlayerInfo.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "SaveLoadPanel.h"
#include "SaveState.h"
#include "ScriptManager.h"
#include "Team.h"
#include "Version.h"
#include "World.h"
#include "WorldMapGui.h"

#include <ParticleSystem.h>
#include <Render.h>
#include <RenderTarget.h>
#include <SceneManager.h>
#include <SoundManager.h>

enum SaveFlags
{
	SF_ONLINE = 1 << 0,
	SF_DEBUG = 1 << 2,
	SF_HARDCORE = 1 << 4,
	SF_ON_WORLDMAP = 1 << 5
};

int LOAD_VERSION;

//=================================================================================================
bool Game::CanSaveGame() const
{
	if(game_state == GS_MAIN_MENU || quest_mgr->quest_secret->state == Quest_Secret::SECRET_FIGHT)
		return false;

	if(game_state == GS_WORLDMAP)
	{
		if(Any(world->GetState(), World::State::TRAVEL, World::State::ENCOUNTER))
			return false;
	}
	else
	{
		if(quest_mgr->quest_tutorial->in_tutorial || arena->mode != Arena::NONE || quest_mgr->quest_contest->state >= Quest_Contest::CONTEST_STARTING
			|| quest_mgr->quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			return false;
	}

	if(Net::IsOnline())
	{
		if(team->IsAnyoneAlive() && Net::IsServer())
			return true;
	}
	else if(pc->unit->IsAlive() || pc->unit->in_arena != -1)
		return true;

	return false;
}

//=================================================================================================
bool Game::CanLoadGame() const
{
	if(Net::IsOnline())
	{
		if(Net::IsClient() || !Any(game_state, GS_LEVEL, GS_WORLDMAP))
			return false;
	}
	else if(hardcore_mode)
		return false;
	return true;
}

//=================================================================================================
bool Game::SaveGameSlot(int slot, cstring text)
{
	assert(InRange(slot, 1, SaveSlot::MAX_SLOTS));

	if(!CanSaveGame())
	{
		// can't save right now
		gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveGame, game_gui->saveload->visible ? game_gui->saveload : nullptr);
		return false;
	}

	LocalString filename = Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
	return SaveGameCommon(filename, slot, text);
}

//=================================================================================================
void Game::SaveGameFilename(const string& name)
{
	string filename = "saves/";
	if(Net::IsOnline())
		filename += "multi/";
	else
		filename += "single/";
	filename += name;
	if(!EndsWith(filename, ".sav"))
		filename += ".sav";

	SaveGameCommon(filename.c_str(), -1, nullptr);
}

//=================================================================================================
bool Game::SaveGameCommon(cstring filename, int slot, cstring text)
{
	io::CreateDirectory("saves");
	io::CreateDirectory(Net::IsOnline() ? "saves/multi" : "saves/single");

	CreateSaveImage();

	LocalString tmp_filename = Format("%s.new", filename);
	GameWriter f(tmp_filename);
	if(!f)
	{
		Error(txLoadOpenError, tmp_filename, GetLastError());
		gui->SimpleDialog(txSaveFailed, game_gui->saveload->visible ? game_gui->saveload : nullptr);
		return false;
	}

	SaveSlot* ss = nullptr;
	if(slot != -1)
	{
		ss = &game_gui->saveload->GetSaveSlot(slot);
		ss->text = text;
	}

	SaveGame(f, ss);
	f.Close();

	if(io::FileExists(filename))
		io::MoveFile(filename, Format("%s.bak", filename));
	io::MoveFile(tmp_filename, filename);

	cstring msg = Format("Game saved '%s'.", filename);
	game_gui->console->AddMsg(msg);
	Info(msg);

	if(hardcore_mode)
	{
		Info("Hardcore mode, exiting to menu.");
		ExitToMenu();
		return false;
	}

	return true;
}

//=================================================================================================
void Game::CreateSaveImage()
{
	int old_flags = draw_flags;
	if(game_state == GS_LEVEL)
		draw_flags = (0xFFFFFFFF & ~DF_GUI & ~DF_MENU);
	else
		draw_flags = (0xFFFFFFFF & ~DF_MENU);
	render->SetRenderTarget(rt_save);
	DrawGame();
	render->SetRenderTarget(nullptr);
	draw_flags = old_flags;
}

//=================================================================================================
void Game::LoadGameSlot(int slot)
{
	assert(InRange(slot, 1, SaveSlot::MAX_SLOTS));
	bool online = (net->mp_load || Net::IsServer());
	cstring filename = Format(online ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
	LoadGameCommon(filename, slot);
}

//=================================================================================================
void Game::LoadGameFilename(const string& name)
{
	string filename = "saves/";
	if(Net::IsOnline())
		filename += "multi/";
	else
		filename += "single/";
	filename += name;
	if(!EndsWith(name, ".sav"))
		filename += ".sav";

	return LoadGameCommon(filename.c_str(), -1);
}

//=================================================================================================
void Game::LoadGameCommon(cstring filename, int slot)
{
	GameReader f(filename);
	if(!f)
	{
		DWORD last_error = GetLastError();
		throw SaveException(Format(txLoadOpenError, filename, last_error), Format("Failed to open file '%s' (%d).", filename, last_error), true);
	}

	Info("Loading save '%s'.", filename);

	net->mp_quickload = (Net::IsOnline() && Any(game_state, GS_LEVEL, GS_WORLDMAP));
	if(net->mp_quickload)
	{
		bool on_worldmap;
		try
		{
			on_worldmap = ValidateNetSaveForLoading(f, slot);
		}
		catch(SaveException& ex)
		{
			throw SaveException(Format(txCantLoadMultiplayer, ex.localized_msg), Format("Can't quick load mp game: %s", ex.msg));
		}

		BitStreamWriter f;
		f << ID_LOADING;
		f << on_worldmap;
		net->SendAll(f, IMMEDIATE_PRIORITY, RELIABLE);

		net->mp_load = true;
		net->ClearChanges();
	}

	prev_game_state = game_state;
	if(net->mp_load && !net->mp_quickload)
		game_gui->multiplayer->visible = false;
	else
	{
		game_gui->main_menu->visible = false;
		game_gui->level_gui->visible = false;
		game_gui->game_menu->CloseDialog();
		game_gui->world_map->Hide();
	}

	try
	{
		in_load = true;
		LoadGame(f);
		in_load = false;
	}
	catch(const SaveException&)
	{
		in_load = false;
		prev_game_state = GS_LOAD;
		ExitToMenu();
		throw;
	}
	catch(cstring msg)
	{
		in_load = false;
		prev_game_state = GS_LOAD;
		ExitToMenu();
		throw SaveException(nullptr, msg);
	}

	f.Close();
	prev_game_state = GS_LOAD;

	if(hardcore_mode)
	{
		Info("Hardcore mode, deleting save.");

		if(slot != -1)
		{
			game_gui->saveload->RemoveHardcoreSave(slot);
			DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot));
		}
		else
			DeleteFile(filename);
	}

	if(net->mp_quickload)
	{
		for(PlayerInfo& info : net->players)
		{
			if(info.left == PlayerInfo::LEFT_NO)
				info.loaded = true;
		}
		game_state = GS_LOAD;
		net_mode = NM_TRANSFER_SERVER;
		net_timer = mp_timeout;
		net_state = NetState::Server_Starting;
		game_gui->info_box->Show("");
	}
	else if(!net->mp_load)
	{
		if(game_state == GS_LEVEL)
		{
			game_gui->level_gui->visible = true;
			game_gui->world_map->Hide();
		}
		else
		{
			game_gui->level_gui->visible = false;
			game_gui->world_map->Show();
		}
		game_gui->Setup(pc);
	}
	else
	{
		game_gui->multiplayer->visible = true;
		game_gui->main_menu->visible = true;
		game_gui->create_server->Show();
	}
}

//=================================================================================================
bool Game::ValidateNetSaveForLoading(GameReader& f, int slot)
{
	SaveSlot ss;
	if(!LoadGameHeader(f, ss))
		throw SaveException(txLoadSignature, "Invalid file signature.");
	f.SetPos(0);
	if(ss.load_version < V_0_9)
		throw SaveException(txTooOldVersion, Format("Too old save version (%d).", ss.load_version));
	for(PlayerInfo& info : net->players)
	{
		bool ok = false;
		if(info.pc->name == ss.player_name)
			continue;
		for(string& str : ss.mp_players)
		{
			if(str == info.name)
			{
				ok = true;
				break;
			}
		}
		if(!ok)
			throw SaveException(Format(txMissingPlayerInSave, info.name.c_str()), Format("Missing player '%s' in save.", info.name.c_str()));
	}
	return ss.on_worldmap;
}

//=================================================================================================
void Game::SaveGame(GameWriter& f, SaveSlot* slot)
{
	Info("Saving...");

	// before saving update minimap, finish unit warps
	if(Net::IsOnline())
		net->ProcessLeftPlayers();
	game_level->UpdateDungeonMinimap(false);
	game_level->ProcessUnitWarps();
	game_level->ProcessRemoveUnits(false);
	if(game_state == GS_WORLDMAP && game_level->is_open)
		LeaveLocation(false, false);

	// signature
	byte sign[4] = { 'C','R','S','V' };
	f << sign;

	// version
	f << VERSION;
	f << V_CURRENT;
	f << start_version;
	f << content.version;

	// save flags
	byte flags = 0;
	if(Net::IsOnline())
		flags |= SF_ONLINE;
	if(IsDebug())
		flags |= SF_DEBUG;
	if(hardcore_mode)
		flags |= SF_HARDCORE;
	if(game_state == GS_WORLDMAP)
		flags |= SF_ON_WORLDMAP;
	f << flags;

	// info
	f << (slot ? slot->text : "");
	f << pc->name;
	f << pc->unit->GetClass()->id;
	if(Net::IsOnline())
	{
		f.WriteCasted<byte>(net->active_players - 1);
		for(PlayerInfo& info : net->players)
		{
			if(!info.pc->is_local && info.left == PlayerInfo::LEFT_NO)
				f << info.pc->name;
		}
	}
	f << time(nullptr);
	f << world->GetDateValue();
	f << game_level->GetCurrentLocationText();
	if(slot)
	{
		slot->valid = true;
		slot->load_version = V_CURRENT;
		slot->player_name = pc->name;
		slot->player_class = pc->unit->data->clas;
		slot->mp_players.clear();
		if(Net::IsOnline())
		{
			for(PlayerInfo& info : net->players)
			{
				if(!info.pc->is_local && info.left == PlayerInfo::LEFT_NO)
					slot->mp_players.push_back(info.pc->name);
			}
		}
		slot->save_date = time(nullptr);
		slot->game_date = world->GetDateValue();
		slot->hardcore = hardcore_mode;
		slot->on_worldmap = (game_state == GS_WORLDMAP);
		slot->location = game_level->GetCurrentLocationText();
		slot->img_offset = f.GetPos() + 4;
	}
	uint img_size = app::render->SaveToFile(rt_save->tex, f);
	if(slot)
		slot->img_size = img_size;

	// ids
	f << ParticleEmitter::impl.id_counter;
	f << TrailParticleEmitter::impl.id_counter;
	f << Unit::impl.id_counter;
	f << GroundItem::impl.id_counter;
	f << Chest::impl.id_counter;
	f << Usable::impl.id_counter;
	f << Trap::impl.id_counter;
	f << Door::impl.id_counter;
	f << Electro::impl.id_counter;
	f << Bullet::impl.id_counter;

	game_stats->Save(f);

	f << game_state;
	world->Save(f);

	byte check_id = 0;

	if(game_state == GS_LEVEL)
		f << (game_level->event_handler ? game_level->event_handler->GetLocationEventHandlerQuestId() : -1);
	else
		team->SaveOnWorldmap(f);
	f << game_level->enter_from;
	f << game_level->light_angle;

	// camera
	f << game_level->camera.real_rot.y;
	f << game_level->camera.dist;
	f << game_level->camera.drunk_anim;

	// vars
	f << devmode;
	f << noai;
	f << dont_wander;
	f << scene_mgr->use_fog;
	f << scene_mgr->use_lighting;
	f << draw_particle_sphere;
	f << draw_unit_radius;
	f << draw_hitbox;
	f << draw_phy;
	f << draw_col;
	f << game_speed;
	f << next_seed;
	f << draw_flags;
	f << pc->unit->id;
	f << game_level->dungeon_level;
	f << portal_anim;
	f << ais.size();
	for(AIController* ai : ais)
		ai->Save(f);
	f << game_level->boss;

	game_gui->Save(f);

	check_id = (byte)world->GetLocations().size();
	f << check_id++;

	// save team
	team->Save(f);

	// save quests
	quest_mgr->Save(f);
	script_mgr->Save(f);

	f << check_id++;

	if(Net::IsOnline())
	{
		net->Save(f);

		f << check_id++;

		Net::PushChange(NetChange::GAME_SAVED);
		game_gui->mp_box->Add(txGameSaved);
	}

	f.Write("EOS", 3);
}

//=================================================================================================
bool Game::LoadGameHeader(GameReader& f, SaveSlot& slot)
{
	if(!f)
		return false;

	// signature
	byte sign[4] = { 'C','R','S','V' };
	byte sign2[4];
	f >> sign2;
	for(int i = 0; i < 4; ++i)
	{
		if(sign2[i] != sign[i])
			return false;
	}

	// version
	int version;
	f >> version;

	// save version
	f >> slot.load_version;
	if(slot.load_version < V_0_4)
		f.Skip<uint>(); // build version

	// start version
	if(slot.load_version >= V_0_4)
		f.Skip<int>();

	// content version
	if(slot.load_version >= V_0_7)
		f.Skip<uint>();

	// save flags
	byte flags;
	f >> flags;

	// info
	if(slot.load_version >= V_0_9)
	{
		slot.hardcore = IsSet(flags, SF_HARDCORE);
		slot.on_worldmap = IsSet(flags, SF_ON_WORLDMAP);
		f >> slot.text;
		f >> slot.player_name;
		slot.player_class = Class::TryGet(f.ReadString1());
		if(IsSet(flags, SF_ONLINE))
			f.ReadStringArray<byte, byte>(slot.mp_players);
		else
			slot.mp_players.clear();
		f >> slot.save_date;
		f >> slot.game_date;
		f >> slot.location;
		f >> slot.img_size;
		slot.img_offset = f.GetPos();
	}

	return true;
}

//=================================================================================================
void Game::LoadGame(GameReader& f)
{
	LoadingStart(8);
	ClearGame();
	ClearGameVars(false);
	StopAllSounds();
	quest_mgr->quest_tutorial->in_tutorial = false;
	arena->Reset();
	pc->data.autowalk = false;
	ai_bow_targets.clear();
	load_location_quest.clear();
	load_unit_handler.clear();
	load_chest_handler.clear();
	units_mesh_load.clear();
	game_level->entering = true;

	byte check_id = 0, read_id;

	// signature
	byte sign[4] = { 'C','R','S','V' };
	byte sign2[4];
	f >> sign2;
	for(int i = 0; i < 4; ++i)
	{
		if(sign2[i] != sign[i])
			throw SaveException(txLoadSignature, "Invalid file signature.");
	}

	// version
	int version;
	f >> version;
	if(version > VERSION)
	{
		cstring ver_str = VersionToString(version);
		throw SaveException(Format(txLoadVersion, ver_str, VERSION_STR), Format("Invalid file version %s, current %s.", ver_str, VERSION_STR));
	}

	// save version
	f >> LOAD_VERSION;
	if(LOAD_VERSION < MIN_SUPPORT_LOAD_VERSION)
	{
		cstring ver_str = VersionToString(version);
		throw SaveException(Format(txLoadSaveVersionOld, ver_str), Format("Unsupported version '%s'.", ver_str));
	}
	f >> start_version;

	// content version
	uint content_version;
	f >> content_version;
	content.require_update = (content.version != content_version);
	if(content.require_update)
		Info("Loading old system version. Content update required.");

	// save flags
	byte flags;
	f >> flags;
	bool online_save = IsSet(flags, SF_ONLINE);
	if(net->mp_load)
	{
		if(!online_save)
			throw SaveException(txLoadSP, "Save is from singleplayer mode.");
	}
	else
	{
		if(online_save)
			throw SaveException(txLoadMP, "Save is from multiplayer mode.");
	}

	Info("Loading save. Version %s, start %s, format %d, mp %d, debug %d.", VersionToString(version), VersionToString(start_version), LOAD_VERSION,
		online_save ? 1 : 0, IsSet(flags, SF_DEBUG) ? 1 : 0);

	// info
	if(LOAD_VERSION >= V_0_9)
	{
		f.SkipString1(); // text
		f.SkipString1(); // player name
		f.SkipString1(); // player class
		if(net->mp_load) // mp players
			f.SkipStringArray<byte, byte>();
		f.Skip<time_t>(); // save_date
		f.Skip<int>(); // game_year
		f.Skip<int>(); // game_month
		f.Skip<int>(); // game_day
		f.SkipString1(); // location
		f.SkipData<uint>(); // image
	}

	// ids
	if(LOAD_VERSION >= V_0_12)
	{
		f >> ParticleEmitter::impl.id_counter;
		f >> TrailParticleEmitter::impl.id_counter;
		f >> Unit::impl.id_counter;
		f >> GroundItem::impl.id_counter;
		f >> Chest::impl.id_counter;
		f >> Usable::impl.id_counter;
		f >> Trap::impl.id_counter;
		f >> Door::impl.id_counter;
		f >> Electro::impl.id_counter;
	}
	if(LOAD_VERSION >= V_0_16)
		f >> Bullet::impl.id_counter;

	LoadingHandler loading;
	GAME_STATE game_state2;
	if(LOAD_VERSION >= V_0_8)
	{
		hardcore_mode = IsSet(flags, SF_HARDCORE);

		game_stats->Load(f);

		// game state
		f >> game_state2;

		// world map
		LoadingStep(txLoadingLocations);
		world->Load(f, loading);
	}
	else
	{
		// game stats (is hardcore, total_kills)
		f >> hardcore_mode;
		game_stats->LoadOld(f, 0);

		// world day/month/year/worldtime
		world->LoadOld(f, loading, 0, false);

		// game state
		f >> game_state2;

		// game stats (play time)
		game_stats->LoadOld(f, 1);

		// world map
		LoadingStep(txLoadingLocations);
		world->LoadOld(f, loading, 1, game_state2 == GS_LEVEL);
	}

	uint count;
	int location_event_handler_quest_id;
	if(game_state2 == GS_LEVEL)
		f >> location_event_handler_quest_id;
	else
	{
		location_event_handler_quest_id = -1;
		// load team
		f >> count;
		for(uint i = 0; i < count; ++i)
		{
			Unit* u = new Unit;
			u->Load(f);
			u->area = nullptr;
			u->CreateMesh(Unit::CREATE_MESH::ON_WORLDMAP);

			if(!u->IsPlayer())
			{
				u->ai = new AIController;
				u->ai->Init(u);
				ais.push_back(u->ai);
			}
		}
	}
	if(LOAD_VERSION < V_0_8)
		world->LoadOld(f, loading, 3, false);
	f >> game_level->enter_from;
	f >> game_level->light_angle;

	// apply entity requests
	LoadingStep(txLoadingData);
	Unit::ApplyRequests();
	Usable::ApplyRequests();

	// camera
	f >> game_level->camera.real_rot.y;
	f >> game_level->camera.dist;
	if(LOAD_VERSION >= V_0_14)
		f >> game_level->camera.drunk_anim;
	game_level->camera.Reset();
	pc->data.rot_buf = 0.f;

	// traders stock
	if(LOAD_VERSION < V_0_8)
	{
		ItemHelper::SkipStock(f); // merchant
		ItemHelper::SkipStock(f); // blacksmith
		ItemHelper::SkipStock(f); // alchemist
		ItemHelper::SkipStock(f); // innkeeper
		ItemHelper::SkipStock(f); // food_seller
	}

	// vars
	f >> devmode;
	f >> noai;
	if(IsDebug())
		noai = true;
	f >> dont_wander;
	f >> scene_mgr->use_fog;
	f >> scene_mgr->use_lighting;
	f >> draw_particle_sphere;
	f >> draw_unit_radius;
	f >> draw_hitbox;
	f >> draw_phy;
	f >> draw_col;
	f >> game_speed;
	f >> next_seed;
	f >> draw_flags;
	Unit* player;
	f >> player;
	pc = player->player;
	if(!net->mp_load)
		pc->id = 0;
	game_level->camera.target = pc->unit;
	game_level->camera.real_rot.x = pc->unit->rot;
	pc->dialog_ctx = &dialog_context;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	f >> game_level->dungeon_level;
	f >> portal_anim;
	if(LOAD_VERSION < V_0_14)
		f >> game_level->camera.drunk_anim;
	ais.resize(f.Read<uint>());
	for(AIController*& ai : ais)
	{
		ai = new AIController;
		ai->Load(f);
	}
	if(LOAD_VERSION >= V_DEV)
	{
		f >> game_level->boss;
		if(game_level->boss)
			game_gui->level_gui->SetBoss(game_level->boss, true);
	}

	game_gui->Load(f);

	check_id = (byte)world->GetLocations().size();
	if(LOAD_VERSION < V_0_14)
		--check_id; // added offscreen location to old saves
	f >> read_id;
	if(read_id != check_id++)
		throw "Error reading data before team.";

	// load team
	team->Load(f);

	// load quests
	LoadingStep(txLoadingQuests);
	quest_mgr->Load(f);

	script_mgr->Load(f);

	if(LOAD_VERSION < V_0_8)
		world->LoadOld(f, loading, 2, false);

	f >> read_id;
	if(read_id != check_id++)
		throw "Error reading data after news.";

	LoadingStep(txLoadingLevel);

	if(game_state2 == GS_LEVEL)
	{
		game_level->is_open = true;

		LocationGenerator* loc_gen = loc_gen_factory->Get(game_level->location);
		loc_gen->OnLoad();

		if(LOAD_VERSION < V_0_11)
		{
			game_level->local_area->tmp->Load(f);

			f >> read_id;
			if(read_id != check_id++)
				throw "Failed to read level data.";
		}

		RemoveUnusedAiAndCheck();
	}
	else
		game_level->is_open = false;

	// gui
	game_gui->level_gui->PositionPanels();

	// set ai bow targets
	if(!ai_bow_targets.empty())
	{
		BaseObject* bow_target = BaseObject::Get("bow_target");
		for(vector<AIController*>::iterator it = ai_bow_targets.begin(), end = ai_bow_targets.end(); it != end; ++it)
		{
			AIController& ai = **it;
			Object* ptr = nullptr;
			float dist, best_dist;
			for(Object* obj : ai.unit->area->objects)
			{
				if(obj->base == bow_target)
				{
					dist = Vec3::Distance(obj->pos, ai.st.idle.pos);
					if(!ptr || dist < best_dist)
					{
						ptr = obj;
						best_dist = dist;
					}
				}
			}
			assert(ptr);
			ai.st.idle.obj.ptr = ptr;
		}
	}

	// location quests
	for(Location* loc : load_location_quest)
	{
		loc->active_quest = static_cast<Quest_Dungeon*>(quest_mgr->FindAnyQuest((int)loc->active_quest));
		assert(loc->active_quest);
	}

	// unit event handlers
	for(Unit* unit : load_unit_handler)
	{
		unit->event_handler = dynamic_cast<UnitEventHandler*>(quest_mgr->FindAnyQuest((int)unit->event_handler));
		assert(unit->event_handler);
	}

	// chest event handlers
	for(Chest* chest : load_chest_handler)
	{
		chest->handler = dynamic_cast<ChestEventHandler*>(quest_mgr->FindAnyQuest((int)chest->handler));
		assert(chest->handler);
	}

	// current location event handler
	if(location_event_handler_quest_id != -1)
	{
		game_level->event_handler = dynamic_cast<LocationEventHandler*>(quest_mgr->FindAnyQuest(location_event_handler_quest_id));
		assert(game_level->event_handler);
	}
	else
		game_level->event_handler = nullptr;

	quest_mgr->UpgradeQuests();
	quest_mgr->ProcessQuestRequests();

	dialog_context.dialog_mode = false;
	team->ClearOnNewGameOrLoad();
	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;
	game_gui->inventory->mode = I_NONE;
	game_gui->ability->Refresh();
	pc->data.before_player = BP_NONE;
	pc->data.selected_unit = pc->unit;
	dialog_context.pc = pc;
	dialog_context.dialog_mode = false;

	if(net->mp_load)
	{
		net->Load(f);

		f >> read_id;
		if(read_id != check_id++)
			throw "Failed to read multiplayer data.";
	}
	else
		pc->is_local = true;

	// end of save
	char eos[3];
	f.Read(eos);
	if(eos[0] != 'E' || eos[1] != 'O' || eos[2] != 'S')
		throw "Missing EOS.";

	game_res->LoadCommonMusic();

	LoadResources(txEndOfLoading, game_state2 == GS_WORLDMAP);
	if(!net->mp_quickload)
		game_gui->load_screen->visible = false;

	Info("Game loaded.");
	game_level->entering = false;

	if(net->mp_load)
	{
		game_state = GS_MAIN_MENU;
		return;
	}

	if(game_state2 == GS_LEVEL)
	{
		SetMusic();
		if(pc->unit->usable && pc->unit->action == A_USE_USABLE && Any(pc->unit->animation_state, AS_USE_USABLE_USING, AS_USE_USABLE_USING_SOUND)
			&& IsSet(pc->unit->usable->base->use_flags, BaseUsable::ALCHEMY))
			game_gui->craft->Show();
	}
	else
		SetMusic(MusicType::Travel);
	game_state = game_state2;
	clear_color = clear_color_next;
}

//=================================================================================================
bool Game::TryLoadGame(int slot, bool quickload, bool from_console)
{
	try
	{
		game->LoadGameSlot(slot);
		return true;
	}
	catch(const SaveException& ex)
	{
		if(quickload && ex.missing_file)
		{
			Warn("Missing quicksave.");
			if(from_console)
				game_gui->console->AddMsg("Missing quicksave.");
			return false;
		}

		cstring msg = Format("Failed to load game: %s", ex.msg);
		Error(msg);
		if(from_console)
			game_gui->console->AddMsg(msg);
		else
		{
			cstring dialog_text;
			if(ex.localized_msg)
				dialog_text = Format("%s%s", txLoadError, ex.localized_msg);
			else
				dialog_text = txLoadErrorGeneric;
			Control* parent = nullptr;
			if(game_gui->game_menu->visible)
				parent = game_gui->game_menu;
			gui->SimpleDialog(dialog_text, parent, "fatal");
		}
		net->mp_load = false;
		return false;
	}
}

//=================================================================================================
void Game::Quicksave(bool from_console)
{
	if(!CanSaveGame())
	{
		if(from_console)
			game_gui->console->AddMsg(Net::IsClient() ? "Only server can save game." : "Can't save game now.");
		else
			gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveNow, nullptr);
		return;
	}

	if(SaveGameSlot(SaveSlot::MAX_SLOTS, txQuickSave))
	{
		if(!from_console)
			game_gui->messages->AddGameMsg3(GMS_GAME_SAVED);
	}
}

//=================================================================================================
void Game::Quickload(bool from_console)
{
	if(!CanLoadGame())
	{
		if(from_console)
			game_gui->console->AddMsg(Net::IsClient() ? "Only server can load game." : "Can't load game now.");
		else
			gui->SimpleDialog(Net::IsClient() ? txOnlyServerCanLoad : txCantLoadGame, nullptr);
		return;
	}
	TryLoadGame(SaveSlot::MAX_SLOTS, true, from_console);
}

//=================================================================================================
void Game::RemoveUnusedAiAndCheck()
{
	uint prev_size = ais.size();
	bool deleted = false;

	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		bool ok = false;
		for(LevelArea& area : game_level->ForEachArea())
		{
			for(Unit* unit : area.units)
			{
				if(unit->ai == *it)
				{
					ok = true;
					break;
				}
			}
			if(ok)
				break;
		}
		if(!ok)
		{
			delete *it;
			*it = nullptr;
			deleted = true;
		}
	}

	if(deleted)
	{
		RemoveNullElements(ais);
		ReportError(5, Format("Removed unused ais: %u.", prev_size - ais.size()));
	}

	if(IsDebug())
	{
		int err_count = 0;
		for(LevelArea& area : game_level->ForEachArea())
			CheckUnitsAi(area, err_count);
		if(err_count)
			game_gui->messages->AddGameMsg(Format("CheckUnitsAi: %d errors!", err_count), 10.f);
	}
}

//=================================================================================================
void Game::CheckUnitsAi(LevelArea& area, int& err_count)
{
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.player && u.ai)
		{
			++err_count;
			Error("Unit %s is player 0x%p and ai 0x%p.", u.data->id.c_str(), u.player, u.ai);
		}
		else if(u.player && u.hero)
		{
			++err_count;
			Error("Unit %s is player 0x%p and hero 0x%p.", u.data->id.c_str(), u.player, u.hero);
		}
		else if(!u.player && !u.ai)
		{
			++err_count;
			Error("Unit %s is neither player or ai.", u.data->id.c_str());
		}
	}
}
