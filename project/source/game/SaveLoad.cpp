#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "SaveState.h"
#include "Version.h"
#include "QuestManager.h"
#include "Quest.h"
#include "Quest_Contest.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "GameFile.h"
#include "GameStats.h"
#include "City.h"
#include "Cave.h"
#include "Gui.h"
#include "SaveLoadPanel.h"
#include "MultiplayerPanel.h"
#include "MainMenu.h"
#include "GameGui.h"
#include "WorldMapGui.h"
#include "GameMessages.h"
#include "LoadScreen.h"
#include "AIController.h"
#include "Spell.h"
#include "Team.h"
#include "Journal.h"
#include "SoundManager.h"
#include "ScriptManager.h"
#include "World.h"
#include "Level.h"
#include "LoadingHandler.h"
#include "LocationGeneratorFactory.h"
#include "Arena.h"
#include "ParticleSystem.h"
#include "Inventory.h"
#include "GlobalGui.h"
#include "Console.h"
#include "Pathfinding.h"

enum SaveFlags
{
	// SF_DEV, SF_BETA removed in 0.4 (remove with compability V_0_4)
	SF_ONLINE = 1 << 0,
	//SF_DEV = 1 << 1,
	SF_DEBUG = 1 << 2,
	//SF_BETA = 1 << 3,
	SF_HARDCORE = 1 << 4
};

const int SAVE_VERSION = V_CURRENT;
int LOAD_VERSION;
const int MIN_SUPPORT_LOAD_VERSION = V_0_2_12;

//=================================================================================================
bool Game::CanSaveGame() const
{
	if(game_state == GS_MAIN_MENU || QM.quest_secret->state == Quest_Secret::SECRET_FIGHT)
		return false;

	if(game_state == GS_WORLDMAP)
	{
		if(Any(W.GetState(), World::State::TRAVEL, World::State::ENCOUNTER))
			return false;
	}
	else
	{
		if(QM.quest_tutorial->in_tutorial || arena->mode != Arena::NONE || QM.quest_contest->state >= Quest_Contest::CONTEST_STARTING
			|| QM.quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			return false;
	}

	if(Net::IsOnline())
	{
		if(Team.IsAnyoneAlive() && Net::IsServer())
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
		return false;

	return true;
}

//=================================================================================================
bool Game::SaveGameSlot(int slot, cstring text)
{
	assert(InRange(slot, 1, MAX_SAVE_SLOTS));

	if(!CanSaveGame())
	{
		// w tej chwili nie mo¿na zapisaæ gry
		GUI.SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveGame, gui->saveload->visible ? gui->saveload : nullptr);
		return false;
	}

	cstring filename = Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
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
	CreateDirectory("saves", nullptr);
	CreateDirectory(Net::IsOnline() ? "saves/multi" : "saves/single", nullptr);

	if(io::FileExists(filename))
	{
		cstring bak_filename = Format("%s.bak", filename);
		DeleteFile(bak_filename);
		MoveFile(filename, bak_filename);
	}

	GameWriter f(filename);
	if(!f)
	{
		Error(txLoadOpenError, filename, GetLastError());
		GUI.SimpleDialog(txSaveFailed, gui->saveload->visible ? gui->saveload : nullptr);
		return false;
	}

	SaveGame(f);

	cstring msg = Format("Game saved '%s'.", filename);
	gui->console->AddMsg(msg);
	Info(msg);

	if(slot != -1)
	{
		gui->saveload->UpdateSaveInfo(slot);

		string path = Format("saves/%s/%d.jpg", Net::IsOnline() ? "multi" : "single", slot);
		CreateSaveImage(path.c_str());
	}

	if(hardcore_mode)
	{
		Info("Hardcore mode, exiting to menu.");
		ExitToMenu();
		return false;
	}

	return true;
}

//=================================================================================================
void Game::LoadGameSlot(int slot)
{
	assert(InRange(slot, 1, MAX_SAVE_SLOTS));

	cstring filename = Format(N.mp_load ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);
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

	prev_game_state = game_state;
	if(N.mp_load)
		gui->multiplayer->visible = false;
	else
	{
		gui->main_menu->visible = false;
		gui->game_gui->visible = false;
		gui->world_map->visible = false;
	}
	LoadingStart(9);

	try
	{
		LoadGame(f);
	}
	catch(const SaveException&)
	{
		prev_game_state = GS_LOAD;
		ExitToMenu();
		throw;
	}
	catch(cstring msg)
	{
		prev_game_state = GS_LOAD;
		ExitToMenu();
		throw SaveException(nullptr, msg);
	}

	prev_game_state = GS_LOAD;

	if(hardcore_mode)
	{
		Info("Hardcore mode, deleting save.");

		if(slot == -1)
		{
			gui->saveload->RemoveHardcoreSave(slot);

			DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot));
			DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.txt" : "saves/single/%d.txt", slot));
			DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.jpg" : "saves/single/%d.jpg", slot));
		}
		else
			DeleteFile(filename);
	}

	if(!N.mp_load)
	{
		if(game_state == GS_LEVEL)
		{
			gui->game_gui->visible = true;
			gui->world_map->visible = false;
		}
		else
		{
			gui->game_gui->visible = false;
			gui->world_map->visible = true;
		}
		gui->Setup(pc);
	}
	else
	{
		gui->saveload->visible = true;
		gui->multiplayer->visible = true;
		gui->main_menu->visible = true;
	}
}

//=================================================================================================
void Game::SaveGame(GameWriter& f)
{
	Info("Saving...");

	// before saving update minimap, finish unit warps
	if(Net::IsOnline())
		Net_PreSave();
	L.UpdateDungeonMinimap(false);
	L.ProcessUnitWarps();
	L.ProcessRemoveUnits(false);
	if(game_state == GS_WORLDMAP && L.is_open)
		LeaveLocation(false, false);
	BuildRefidTables();

	// signature
	byte sign[4] = { 'C','R','S','V' };
	f << sign;

	// version
	f << VERSION;
	f << SAVE_VERSION;
	f << start_version;
	f << content::version;

	// save flags
	byte flags = (Net::IsOnline() ? SF_ONLINE : 0);
#ifdef _DEBUG
	flags |= SF_DEBUG;
#endif
	if(hardcore_mode)
		flags |= SF_HARDCORE;
	f << flags;

	// game stats
	GameStats::Get().Save(f);

	f << game_state;
	W.Save(f);


	byte check_id = 0;

	if(game_state == GS_LEVEL)
		f << (L.event_handler ? L.event_handler->GetLocationEventHandlerQuestRefid() : -1);
	else
		Team.SaveOnWorldmap(f);
	f << L.enter_from;
	f << L.light_angle;

	// camera
	f << cam.real_rot.y;
	f << cam.dist;

	// update traders stock
	SaveStock(f, chest_merchant);
	SaveStock(f, chest_blacksmith);
	SaveStock(f, chest_alchemist);
	SaveStock(f, chest_innkeeper);
	SaveStock(f, chest_food_seller);

	// vars
	f << devmode;
	f << noai;
	f << dont_wander;
	f << cl_fog;
	f << cl_lighting;
	f << draw_particle_sphere;
	f << draw_unit_radius;
	f << draw_hitbox;
	f << draw_phy;
	f << draw_col;
	f << game_speed;
	f << next_seed;
	f << draw_flags;
	f << pc->unit->refid;
	f << L.dungeon_level;
	f << portal_anim;
	f << drunk_anim;
	f << ais.size();
	for(AIController* ai : ais)
		ai->Save(f);

	// game messages & speech bubbles
	gui->messages->Save(f);
	gui->game_gui->Save(f);

	// rumors/notes
	gui->journal->Save(f);

	check_id = (byte)W.GetLocations().size();
	f << check_id;
	++check_id;

	// save team
	Team.Save(f);

	// save quests
	QM.Save(f);
	SM.Save(f);

	f << check_id;
	++check_id;

	if(game_state == GS_LEVEL)
	{
		// particles
		f << L.local_ctx.pes->size();
		for(ParticleEmitter* pe : *L.local_ctx.pes)
			pe->Save(f);

		f << L.local_ctx.tpes->size();
		for(TrailParticleEmitter* tpe : *L.local_ctx.tpes)
			tpe->Save(f);

		// explosions
		f << L.local_ctx.explos->size();;
		for(Explo* explo : *L.local_ctx.explos)
			explo->Save(f);

		// electric effects
		f << L.local_ctx.electros->size();
		for(Electro* electro : *L.local_ctx.electros)
			electro->Save(f);

		// drain effects
		f << L.local_ctx.drains->size();
		for(Drain& drain : *L.local_ctx.drains)
			drain.Save(f);

		// bullets
		f << L.local_ctx.bullets->size();
		for(Bullet& bullet : *L.local_ctx.bullets)
			bullet.Save(f);

		f << check_id;
		++check_id;
	}

	if(Net::IsOnline())
	{
		N.Save(f);

		f << check_id;
		++check_id;

		Net::PushChange(NetChange::GAME_SAVED);
		AddMultiMsg(txGameSaved);
	}

	f.Write("EOS", 3);
}

//=================================================================================================
void Game::SaveStock(FileWriter& f, vector<ItemSlot>& cnt)
{
	f << cnt.size();
	for(ItemSlot& slot : cnt)
	{
		if(slot.item)
		{
			f << slot.item->id;
			f << slot.count;
			if(slot.item->id[0] == '$')
				f << slot.item->refid;
		}
		else
			f.Write0();
	}
}

//=================================================================================================
void Game::LoadGame(GameReader& f)
{
	ClearGame();
	ClearGameVars(false);
	StopAllSounds();
	QM.quest_tutorial->in_tutorial = false;
	arena->Reset();
	pc_data.autowalk = false;
	ai_bow_targets.clear();
	ai_cast_targets.clear();
	load_location_quest.clear();
	load_unit_handler.clear();
	load_chest_handler.clear();
	units_mesh_load.clear();
	Unit::refid_table.clear();
	Usable::refid_table.clear();
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();

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
	if(LOAD_VERSION >= V_0_2_20 && LOAD_VERSION < V_0_4)
	{
		// build - unused
		uint build;
		f >> build;
	}

	// start version
	if(LOAD_VERSION >= V_0_4)
		f >> start_version;
	else
		start_version = version;

	// content version
	if(LOAD_VERSION >= V_0_7)
	{
		uint content_version;
		f >> content_version;
		content::require_update = (content::version != content_version);
	}
	else
		content::require_update = true;
	if(content::require_update)
		Info("Loading old system version. Content update required.");

	// save flags
	byte flags;
	f >> flags;
	bool online_save = IS_SET(flags, SF_ONLINE);
	if(N.mp_load)
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
		online_save ? 1 : 0, IS_SET(flags, SF_DEBUG) ? 1 : 0);

	LoadingHandler loading;
	GAME_STATE game_state2;
	if(LOAD_VERSION >= V_DEV)
	{
		hardcore_mode = IS_SET(flags, SF_HARDCORE);

		// game stats
		GameStats::Get().Load(f);

		// game state
		f >> game_state2;

		// world map
		LoadingStep(txLoadingLocations);
		W.Load(f, loading);
	}
	else
	{
		// game stats (is hardcore, total_kills)
		f >> hardcore_mode;
		GameStats::Get().LoadOld(f, 0);

		// world day/month/year/worldtime
		W.LoadOld(f, loading, 0, false);

		// game state
		f >> game_state2;

		// game stats (play time)
		GameStats::Get().LoadOld(f, 1);

		// world map
		LoadingStep(txLoadingLocations);
		W.LoadOld(f, loading, 1, game_state2 == GS_LEVEL);
	}

	uint count;
	int location_event_handler_quest_refid;
	if(game_state2 == GS_LEVEL)
		f >> location_event_handler_quest_refid;
	else
	{
		location_event_handler_quest_refid = -1;
		// load team
		f >> count;
		for(uint i = 0; i < count; ++i)
		{
			Unit* u = new Unit;
			u->Load(f, false);
			Unit::AddRefid(u);
			u->CreateMesh(Unit::CREATE_MESH::ON_WORLDMAP);

			if(!u->IsPlayer())
			{
				u->ai = new AIController;
				u->ai->Init(u);
				ais.push_back(u->ai);
			}
		}
	}
	if(LOAD_VERSION < V_DEV)
		W.LoadOld(f, loading, 3, false);
	f >> L.enter_from;
	if(LOAD_VERSION >= V_0_3)
		f >> L.light_angle;
	else
		L.light_angle = Random(PI * 2);

	// set entities pointers
	LoadingStep(txLoadingData);
	for(vector<std::pair<Unit**, int> >::iterator it = Unit::refid_request.begin(), end = Unit::refid_request.end(); it != end; ++it)
		*(it->first) = Unit::refid_table[it->second];
	Unit::refid_request.clear();
	for(vector<UsableRequest>::iterator it = Usable::refid_request.begin(), end = Usable::refid_request.end(); it != end; ++it)
	{
		Usable* u = Usable::refid_table[it->refid];
		if(it->user && u->user != it->user)
		{
			Warn("Invalid usable %s (%d) user %s.", u->base->id.c_str(), u->refid, it->user->data->id.c_str());
			*it->usable = nullptr;
		}
		else
			*it->usable = u;
	}
	Usable::refid_request.clear();

	// camera
	f >> cam.real_rot.y;
	f >> cam.dist;
	cam.Reset();
	pc_data.rot_buf = 0.f;

	// traders stock
	LoadStock(f, chest_merchant);
	LoadStock(f, chest_blacksmith);
	LoadStock(f, chest_alchemist);
	LoadStock(f, chest_innkeeper);
	if(LOAD_VERSION >= V_0_2_20)
		LoadStock(f, chest_food_seller);
	else
		chest_food_seller.clear();

	// vars
	if(LOAD_VERSION < V_0_5)
	{
		bool used_cheats;
		f >> used_cheats;
	}
	f >> devmode;
	if(LOAD_VERSION < V_0_4)
	{
		bool no_sound;
		f >> no_sound;
	}
	f >> noai;
#ifdef _DEBUG
	noai = true;
#endif
	f >> dont_wander;
	if(LOAD_VERSION >= V_0_4)
	{
		f >> cl_fog;
		f >> cl_lighting;
		f >> draw_particle_sphere;
		f >> draw_unit_radius;
		f >> draw_hitbox;
		f >> draw_phy;
		f >> draw_col;
		f >> game_speed;
		f >> next_seed;
		if(LOAD_VERSION < V_0_5)
		{
			bool next_seed_extra;
			int next_seed_val[3];
			f >> next_seed_extra;
			if(next_seed_extra)
				f >> next_seed_val;
		}
		f >> draw_flags;
	}
	else
	{
		cl_fog = true;
		cl_lighting = true;
		draw_particle_sphere = false;
		draw_unit_radius = false;
		draw_hitbox = false;
		draw_phy = false;
		draw_col = false;
		game_speed = 1.f;
		next_seed = 0;
		draw_flags = 0xFFFFFFFF;
	}
	Unit* player;
	f >> player;
	pc = player->player;
	if(!N.mp_load)
		pc->id = 0;
	cam.real_rot.x = pc->unit->rot;
	pc->dialog_ctx = &dialog_context;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	f >> L.dungeon_level;
	if(LOAD_VERSION >= V_0_2_20)
	{
		f >> portal_anim;
		f >> drunk_anim;
	}
	else
	{
		portal_anim = 0.f;
		drunk_anim = 0.f;
	}
	ais.resize(f.Read<uint>());
	for(AIController*& ai : ais)
	{
		ai = new AIController;
		ai->Load(f);
	}

	// game messages & speech bubbles
	gui->messages->Load(f);
	gui->game_gui->Load(f);

	// rumors/notes
	gui->journal->Load(f);

	check_id = (byte)W.GetLocations().size();
	f >> read_id;
	if(read_id != check_id)
		throw "Error reading data before team.";
	++check_id;

	// wczytaj dru¿ynê
	Team.Load(f);

	// load quests
	LoadingStep(txLoadingQuests);
	QuestManager& quest_manager = QM;
	quest_manager.Load(f);

	SM.Load(f);

	if(LOAD_VERSION < V_DEV)
		W.LoadOld(f, loading, 2, false);

	f >> read_id;
	if(read_id != check_id)
		throw "Error reading data after news.";
	++check_id;

	LoadingStep(txLoadingLevel);

	if(game_state2 == GS_LEVEL)
	{
		L.is_open = true;

		LocationGenerator* loc_gen = loc_gen_factory->Get(L.location);
		loc_gen->OnLoad();

		// particles
		L.local_ctx.pes->resize(f.Read<uint>());
		for(ParticleEmitter*& pe : *L.local_ctx.pes)
		{
			pe = new ParticleEmitter;
			ParticleEmitter::AddRefid(pe);
			pe->Load(f);
		}

		L.local_ctx.tpes->resize(f.Read<uint>());
		for(TrailParticleEmitter* tpe : *L.local_ctx.tpes)
		{
			tpe = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(tpe);
			tpe->Load(f);
		}

		// explosions
		L.local_ctx.explos->resize(f.Read<uint>());
		for(Explo*& explo : *L.local_ctx.explos)
		{
			explo = new Explo;
			explo->Load(f);
		}

		// electric effects
		L.local_ctx.electros->resize(f.Read<uint>());
		for(Electro*& electro : *L.local_ctx.electros)
		{
			electro = new Electro;
			electro->Load(f);
		}

		// drain effects
		L.local_ctx.drains->resize(f.Read<uint>());
		for(Drain& drain : *L.local_ctx.drains)
			drain.Load(f);

		// bullets
		L.local_ctx.bullets->resize(f.Read<uint>());
		for(Bullet& bullet : *L.local_ctx.bullets)
			bullet.Load(f);

		f >> read_id;
		if(read_id != check_id)
			throw "Failed to read level data.";
		++check_id;

		RemoveUnusedAiAndCheck();
	}
	else
		L.is_open = false;

	// gui
	if(LOAD_VERSION <= V_0_3)
		gui->LoadOldGui(f);
	gui->game_gui->PositionPanels();

	// cele ai
	if(!ai_bow_targets.empty())
	{
		BaseObject* tarcza_s = BaseObject::Get("bow_target");
		for(vector<AIController*>::iterator it = ai_bow_targets.begin(), end = ai_bow_targets.end(); it != end; ++it)
		{
			AIController& ai = **it;
			LevelContext& ctx = L.GetContext(*ai.unit);
			Object* ptr = nullptr;
			float dist, best_dist;
			for(vector<Object*>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
			{
				Object& obj = **it;
				if(obj.base == tarcza_s)
				{
					dist = Vec3::Distance(obj.pos, ai.idle_data.pos);
					if(!ptr || dist < best_dist)
					{
						ptr = &obj;
						best_dist = dist;
					}
				}
			}
			assert(ptr);
			ai.idle_data.obj.ptr = ptr;
		}
	}

	// cele czarów ai
	for(vector<AIController*>::iterator it = ai_cast_targets.begin(), end = ai_cast_targets.end(); it != end; ++it)
	{
		AIController& ai = **it;
		int refid = (int)ai.cast_target;
		ai.cast_target = Unit::GetByRefid(refid);
	}

	// questy zwi¹zane z lokacjami
	for(vector<Location*>::iterator it = load_location_quest.begin(), end = load_location_quest.end(); it != end; ++it)
	{
		(*it)->active_quest = (Quest_Dungeon*)quest_manager.FindQuest((int)(*it)->active_quest, false);
		assert((*it)->active_quest);
	}
	// unit event handler
	for(vector<Unit*>::iterator it = load_unit_handler.begin(), end = load_unit_handler.end(); it != end; ++it)
	{
		// pierwszy raz musia³em u¿yæ tego rzutowania ¿eby dzia³a³o :o
		(*it)->event_handler = dynamic_cast<UnitEventHandler*>(quest_manager.FindQuest((int)(*it)->event_handler, false));
		assert((*it)->event_handler);
	}
	// chest event handler
	for(vector<Chest*>::iterator it = load_chest_handler.begin(), end = load_chest_handler.end(); it != end; ++it)
	{
		(*it)->handler = dynamic_cast<ChestEventHandler*>(quest_manager.FindQuest((int)(*it)->handler, false));
		assert((*it)->handler);
	}

	dialog_context.dialog_mode = false;
	if(location_event_handler_quest_refid != -1)
		L.event_handler = dynamic_cast<LocationEventHandler*>(quest_manager.FindQuest(location_event_handler_quest_refid));
	else
		L.event_handler = nullptr;
	Team.ClearOnNewGameOrLoad();
	fallback_type = FALLBACK::NONE;
	fallback_t = -0.5f;
	gui->inventory->mode = I_NONE;
	pc_data.before_player = BP_NONE;
	pc_data.selected_unit = nullptr;
	pc_data.selected_target = nullptr;
	dialog_context.pc = pc;
	dialog_context.dialog_mode = false;
	gui->game_gui->Setup();

	if(N.mp_load)
	{
		N.Load(f);

		f >> read_id;
		if(read_id != check_id)
			throw "Failed to read multiplayer data.";
		++check_id;
	}
	else
		pc->is_local = true;

	// end of save
	if(LOAD_VERSION >= V_0_5)
	{
		char eos[3];
		f.Read(eos);
		if(eos[0] != 'E' || eos[1] != 'O' || eos[2] != 'S')
			throw "Missing EOS.";
	}

	// load music
	LoadingStep(txLoadMusic);
	if(!sound_mgr->IsMusicDisabled())
	{
		LoadMusic(MusicType::Boss, false);
		LoadMusic(MusicType::Death, false);
		LoadMusic(MusicType::Travel, false);
	}

	LoadResources(txEndOfLoading, game_state2 == GS_WORLDMAP);
	gui->load_screen->visible = false;

#ifdef _DEBUG
	Team.ValidateTeamItems();
#endif

	Info("Game loaded.");

	if(N.mp_load)
	{
		game_state = GS_MAIN_MENU;
		return;
	}

	if(game_state2 == GS_LEVEL)
		SetMusic();
	else
		SetMusic(MusicType::Travel);
	game_state = game_state2;
	clear_color = clear_color2;
}

//=================================================================================================
void Game::LoadStock(FileReader& f, vector<ItemSlot>& cnt)
{
	uint count;
	f >> count;
	if(count == 0)
		return;

	bool can_sort = true;
	cnt.resize(count);
	for(ItemSlot& slot : cnt)
	{
		const string& item_id = f.ReadString1();
		f >> slot.count;
		if(item_id[0] != '$')
			slot.item = Item::Get(item_id);
		else
		{
			int quest_refid;
			f >> quest_refid;
			QM.AddQuestItemRequest(&slot.item, item_id.c_str(), quest_refid, &cnt);
			slot.item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && (LOAD_VERSION < V_0_2_20 || content::require_update))
		SortItems(cnt);
}

//=================================================================================================
void Game::Quicksave(bool from_console)
{
	if(!CanSaveGame())
	{
		if(from_console)
			gui->console->AddMsg(Net::IsClient() ? "Only server can save game." : "Can't save game now.");
		else
			GUI.SimpleDialog(Net::IsClient() ? txOnlyServerCanSave : txCantSaveNow, nullptr);
		return;
	}

	if(SaveGameSlot(MAX_SAVE_SLOTS, txQuickSave))
	{
		if(!from_console)
			gui->messages->AddGameMsg3(GMS_GAME_SAVED);
	}
}

//=================================================================================================
bool Game::Quickload(bool from_console)
{
	if(!CanLoadGame())
	{
		if(from_console)
			gui->console->AddMsg(Net::IsClient() ? "Only server can load game." : "Can't load game now.");
		else
			GUI.SimpleDialog(Net::IsClient() ? txOnlyServerCanLoad : txCantLoadGame, nullptr);
		return true;
	}

	Net::SetMode(Net::Mode::Singleplayer);
	return gui->saveload->TryLoad(MAX_SAVE_SLOTS, true);
}

//=================================================================================================
void Game::RemoveUnusedAiAndCheck()
{
	uint prev_size = ais.size();
	bool deleted = false;

	// to jest tylko tymczasowe rozwi¹zanie nadmiarowych ai, byæ mo¿e wystêpowa³o tylko w wersji 0.2 ale póki co niech zostanie
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		bool ok = false;
		for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
		{
			if((*it2)->ai == *it)
			{
				ok = true;
				break;
			}
		}
		if(!ok && L.city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = L.city_ctx->inside_buildings.begin(), end2 = L.city_ctx->inside_buildings.end(); it2 != end2 && !ok; ++it2)
			{
				for(vector<Unit*>::iterator it3 = (*it2)->units.begin(), end3 = (*it2)->units.end(); it3 != end3; ++it3)
				{
					if((*it3)->ai == *it)
					{
						ok = true;
						break;
					}
				}
			}
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
		cstring s = Format("Removed unused ais: %u.", prev_size - ais.size());
		Warn(s);
#ifdef _DEBUG
		gui->messages->AddGameMsg(s, 10.f);
#endif
	}

#ifdef _DEBUG
	int err_count = 0;
	CheckUnitsAi(L.local_ctx, err_count);
	if(L.city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = L.city_ctx->inside_buildings.begin(), end = L.city_ctx->inside_buildings.end(); it != end; ++it)
			CheckUnitsAi((*it)->ctx, err_count);
	}
	if(err_count)
		gui->messages->AddGameMsg(Format("CheckUnitsAi: %d errors!", err_count), 10.f);
#endif
}

//=================================================================================================
void Game::CheckUnitsAi(LevelContext& ctx, int& err_count)
{
	for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
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
