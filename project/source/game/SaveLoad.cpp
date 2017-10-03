// zapisywanie/wczytywanie gry
#include "Pch.h"
#include "Core.h"
#include "Game.h"
#include "SaveState.h"
#include "Version.h"
#include "QuestManager.h"
#include "Quest_Sawmill.h"
#include "Quest_Bandits.h"
#include "Quest_Mine.h"
#include "Quest_Goblins.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "GameFile.h"
#include "GameStats.h"
#include "MultiInsideLocation.h"
#include "OutsideLocation.h"
#include "City.h"
#include "CaveLocation.h"
#include "Camp.h"
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

// SF_DEV, SF_BETA removed in 0.4
#define SF_ONLINE (1<<0)
//#define SF_DEV (1<<1)
#define SF_DEBUG (1<<2)
//#define SF_BETA (1<<3)

//=================================================================================================
bool Game::CanSaveGame() const
{
	if(game_state == GS_MAIN_MENU || secret_state == SECRET_FIGHT)
		return false;

	if(game_state == GS_WORLDMAP)
	{
		if(world_state == WS_TRAVEL || world_state == WS_ENCOUNTER)
			return false;
	}
	else
	{
		if(in_tutorial || arena_tryb != Arena_Brak || contest_state >= CONTEST_STARTING || tournament_state != TOURNAMENT_NOT_DONE)
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
		// w tej chwili nie mo�na zapisa� gry
		GUI.SimpleDialog(txCantSaveGame, saveload->visible ? saveload : nullptr);
		return false;
	}

	CreateDirectory("saves", nullptr);
	CreateDirectory(Net::IsOnline() ? "saves/multi" : "saves/single", nullptr);

	cstring filename = Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);

	if(io::FileExists(filename))
	{
		cstring bak_filename = Format("%s.bak", filename);
		DeleteFile(bak_filename);
		MoveFile(filename, bak_filename);
	}

	HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		Error(txLoadOpenError, filename, GetLastError());
		GUI.SimpleDialog(txSaveFailed, saveload->visible ? saveload : nullptr);
		return false;
	}
	else
	{
		SaveGame(file);
		CloseHandle(file);

		cstring msg = Format("Game saved '%s'.", filename);
		AddConsoleMsg(msg);
		Info(msg);

		SaveSlot& ss = (Net::IsOnline() ? multi_saves[slot - 1] : single_saves[slot - 1]);
		ss.valid = true;
		ss.game_day = day;
		ss.game_month = month;
		ss.game_year = year;
		ss.location = GetCurrentLocationText();
		ss.player_name = pc->name;
		ss.player_class = pc->clas;
		ss.save_date = time(nullptr);
		ss.text = (text[0] != 0 ? text : Format(txSavedGameN, slot));
		ss.hardcore = hardcore_mode;

		Config cfg;
		cfg.Add("game_day", Format("%d", day));
		cfg.Add("game_month", Format("%d", month));
		cfg.Add("game_year", Format("%d", year));
		cfg.Add("location", ss.location.c_str());
		cfg.Add("player_name", ss.player_name.c_str());
		cfg.Add("player_class", ClassInfo::classes[(int)ss.player_class].id);
		cfg.Add("save_date", Format("%I64d", ss.save_date));
		cfg.Add("text", ss.text.c_str());
		cfg.Add("hardcore", ss.hardcore);

		if(Net::IsOnline())
		{
			ss.multiplayers = players;
			cfg.Add("multiplayers", Format("%d", ss.multiplayers));
		}

		cfg.Save(Format("saves/%s/%d.txt", Net::IsOnline() ? "multi" : "single", slot));

		string path = Format("saves/%s/%d.jpg", Net::IsOnline() ? "multi" : "single", slot);
		CreateSaveImage(path.c_str());

		if(hardcore_mode)
		{
			Info("Hardcore mode, exiting to menu.");
			ExitToMenu();
			return false;
		}

		return true;
	}
}

//=================================================================================================
void Game::LoadGameSlot(int slot)
{
	assert(InRange(slot, 1, MAX_SAVE_SLOTS));

	cstring filename = Format(mp_load ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);

	Info("Loading saved game '%s'.", filename);

	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if(file == INVALID_HANDLE_VALUE)
	{
		auto last_error = GetLastError();
		throw SaveException(Format(txLoadOpenError, filename, last_error), Format("Failed to open file '%s' (%d).", filename, last_error), true);
	}

	prev_game_state = game_state;
	if(mp_load)
		multiplayer_panel->visible = false;
	else
	{
		main_menu->visible = false;
		game_gui->visible = false;
		world_map->visible = false;
	}
	LoadingStart(9);

	try
	{
		LoadGame(file);
	}
	catch(const SaveException&)
	{
		prev_game_state = GS_LOAD;
		CloseHandle(file);
		ExitToMenu();
		throw;
	}
	catch(cstring msg)
	{
		prev_game_state = GS_LOAD;
		CloseHandle(file);
		ExitToMenu();
		throw SaveException(nullptr, msg);
	}

	prev_game_state = GS_LOAD;
	CloseHandle(file);

	if(hardcore_mode)
	{
		SaveSlot& s = single_saves[slot - 1];
		s.valid = false;
		hardcore_savename = s.text;

		Info("Hardcore mode, deleting save.");
		DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot));
		DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.txt" : "saves/single/%d.txt", slot));
		DeleteFile(Format(Net::IsOnline() ? "saves/multi/%d.jpg" : "saves/single/%d.jpg", slot));
	}

	if(!mp_load)
	{
		if(game_state == GS_LEVEL)
		{
			game_gui->visible = true;
			world_map->visible = false;
		}
		else
		{
			game_gui->visible = false;
			world_map->visible = true;
		}
		SetGamePanels();
	}
	else
	{
		saveload->visible = true;
		multiplayer_panel->visible = true;
		main_menu->visible = true;
	}
}

//=================================================================================================
void Game::LoadSaveSlots()
{
	for(int multi = 0; multi < 2; ++multi)
	{
		for(int i = 1; i <= MAX_SAVE_SLOTS; ++i)
		{
			SaveSlot& slot = (multi == 0 ? single_saves : multi_saves)[i - 1];
			cstring filename = Format("saves/%s/%d.sav", multi == 0 ? "single" : "multi", i);
			if(io::FileExists(filename))
			{
				slot.valid = true;
				filename = Format("saves/%s/%d.txt", multi == 0 ? "single" : "multi", i);
				if(io::FileExists(filename))
				{
					Config cfg;
					cfg.Load(filename);
					slot.player_name = cfg.GetString("player_name", "");
					slot.location = cfg.GetString("location", "");
					slot.text = cfg.GetString("text", "");
					slot.game_day = cfg.GetInt("game_day");
					slot.game_month = cfg.GetInt("game_month");
					slot.game_year = cfg.GetInt("game_year");
					slot.hardcore = cfg.GetBool("hardcore");
					if(multi == 1)
						slot.multiplayers = cfg.GetInt("multiplayers");
					else
						slot.multiplayers = -1;
					slot.save_date = cfg.GetInt64("save_date");
					const string& str = cfg.GetString("player_class");
					if(str == "0")
						slot.player_class = Class::WARRIOR;
					else if(str == "1")
						slot.player_class = Class::HUNTER;
					else if(str == "2")
						slot.player_class = Class::ROGUE;
					else
					{
						ClassInfo* ci = ClassInfo::Find(str);
						if(ci && ci->pickable)
							slot.player_class = ci->class_id;
						else
							slot.player_class = Class::INVALID;
					}
				}
				else
				{
					slot.player_name.clear();
					slot.text.clear();
					slot.location.clear();
					slot.game_day = -1;
					slot.game_month = -1;
					slot.game_year = -1;
					slot.player_class = Class::INVALID;
					slot.multiplayers = -1;
					slot.save_date = 0;
					slot.hardcore = false;
				}
			}
			else
				slot.valid = false;

			if(i == MAX_SAVE_SLOTS)
				slot.text = txQuickSave;
		}
	}
}

//=================================================================================================
void Game::SaveGame(HANDLE file)
{
	Info("Saving...");

	GameWriter f(file);

	// przed zapisaniem zaktualizuj minimap�, przenie� jednostki itp
	if(Net::IsOnline())
		Net_PreSave();
	UpdateDungeonMinimap(false);
	ProcessUnitWarps();
	ProcessRemoveUnits();
	if(game_state == GS_WORLDMAP && open_location != -1)
		LeaveLocation(false, false);

	// signature
	byte sign[4] = { 'C','R','S','V' };
	f << sign;

	// version
	f << VERSION;
	f << SAVE_VERSION;
	f << start_version;

	// czy online / dev
	byte flags = (Net::IsOnline() ? SF_ONLINE : 0);
#ifdef _DEBUG
	flags |= SF_DEBUG;
#endif
	WriteFile(file, &flags, sizeof(flags), &tmp, nullptr);

	// czy hardcore
	WriteFile(file, &hardcore_mode, sizeof(hardcore_mode), &tmp, nullptr);
	WriteFile(file, &total_kills, sizeof(total_kills), &tmp, nullptr);

	// world state
	WriteFile(file, &year, sizeof(year), &tmp, nullptr);
	WriteFile(file, &month, sizeof(month), &tmp, nullptr);
	WriteFile(file, &day, sizeof(day), &tmp, nullptr);
	WriteFile(file, &worldtime, sizeof(worldtime), &tmp, nullptr);
	WriteFile(file, &game_state, sizeof(game_state), &tmp, nullptr);
	GameStats::Get().Save(file);

	BuildRefidTables();

	byte check_id = 0;

	// world map
	WriteFile(file, &world_state, sizeof(world_state), &tmp, nullptr);
	WriteFile(file, &current_location, sizeof(current_location), &tmp, nullptr);
	uint ile = locations.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		LOCATION_TOKEN loc_token;

		if(*it)
			loc_token = (*it)->GetToken();
		else
			loc_token = LT_NULL;

		WriteFile(file, &loc_token, sizeof(loc_token), &tmp, nullptr);

		if(loc_token != LT_NULL)
		{
			if(loc_token == LT_MULTI_DUNGEON)
			{
				int levels = ((MultiInsideLocation*)(*it))->levels.size();
				WriteFile(file, &levels, sizeof(levels), &tmp, nullptr);
			}
			(*it)->Save(file, (game_state == GS_LEVEL && location == *it));
		}

		WriteFile(file, &check_id, sizeof(check_id), &tmp, nullptr);
		++check_id;
	}
	WriteFile(file, &empty_locations, sizeof(empty_locations), &tmp, nullptr);
	WriteFile(file, &create_camp, sizeof(create_camp), &tmp, nullptr);
	WriteFile(file, &world_pos, sizeof(world_pos), &tmp, nullptr);
	WriteFile(file, &travel_time2, sizeof(travel_time2), &tmp, nullptr);
	WriteFile(file, &szansa_na_spotkanie, sizeof(szansa_na_spotkanie), &tmp, nullptr);
	WriteFile(file, &settlements, sizeof(settlements), &tmp, nullptr);
	WriteFile(file, &encounter_loc, sizeof(encounter_loc), &tmp, nullptr);
	WriteFile(file, &world_dir, sizeof(world_dir), &tmp, nullptr);
	if(world_state == WS_TRAVEL)
	{
		WriteFile(file, &picked_location, sizeof(picked_location), &tmp, nullptr);
		WriteFile(file, &travel_day, sizeof(travel_day), &tmp, nullptr);
		WriteFile(file, &travel_start, sizeof(travel_start), &tmp, nullptr);
		WriteFile(file, &travel_time, sizeof(travel_time), &tmp, nullptr);
		WriteFile(file, &guards_enc_reward, sizeof(guards_enc_reward), &tmp, nullptr);
	}
	ile = encs.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	if(game_state == GS_LEVEL)
	{
		int location_event_handler_quest_refid = (location_event_handler ? location_event_handler->GetLocationEventHandlerQuestRefid() : -1);
		WriteFile(file, &location_event_handler_quest_refid, sizeof(location_event_handler_quest_refid), &tmp, nullptr);
	}
	else
		Team.SaveOnWorldmap(file);
	WriteFile(file, &first_city, sizeof(first_city), &tmp, nullptr);
	ile = boss_levels.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
	if(ile)
		WriteFile(file, &boss_levels[0], sizeof(Int2)*ile, &tmp, nullptr);
	WriteFile(file, &enter_from, sizeof(enter_from), &tmp, nullptr);
	WriteFile(file, &light_angle, sizeof(light_angle), &tmp, nullptr);

	// kamera
	WriteFile(file, &cam.real_rot.y, sizeof(cam.real_rot.y), &tmp, nullptr);
	WriteFile(file, &cam.dist, sizeof(cam.dist), &tmp, nullptr);

	// zapisz ekwipunek sprzedawc�w w mie�cie
	SaveStock(file, chest_merchant);
	SaveStock(file, chest_blacksmith);
	SaveStock(file, chest_alchemist);
	SaveStock(file, chest_innkeeper);
	SaveStock(file, chest_food_seller);

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
	f << dungeon_level;
	f << portal_anim;
	f << drunk_anim;
	f << ais.size();
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
		(*it)->Save(file);

	// game messages & speech bubbles
	game_gui->game_messages->Save(f);
	game_gui->Save(f);

	// rumors/notes
	game_gui->journal->Save(file);

	WriteFile(file, &check_id, sizeof(check_id), &tmp, nullptr);
	++check_id;

	// zapisz dru�yn�
	Team.Save(file);

	// save quests
	QuestManager::Get().Save(file);
	SaveQuestsData(file);

	// newsy
	uint count = news.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<News*>::iterator it = news.begin(), end = news.end(); it != end; ++it)
	{
		WriteFile(file, &(*it)->add_time, sizeof((*it)->add_time), &tmp, nullptr);
		WriteString2(file, (*it)->text);
	}

	WriteFile(file, &check_id, sizeof(check_id), &tmp, nullptr);
	++check_id;

	if(game_state == GS_LEVEL)
	{
		// gra
		//-----------------------
		// cz�steczki
		count = local_ctx.pes->size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
			(*it)->Save(file);

		count = local_ctx.tpes->size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(vector<TrailParticleEmitter*>::iterator it = local_ctx.tpes->begin(), end = local_ctx.tpes->end(); it != end; ++it)
			(*it)->Save(file);

		// wybuchy
		count = local_ctx.explos->size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
			(*it)->Save(file);

		// elektryczno��
		count = local_ctx.electros->size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
			(*it)->Save(file);

		// wyssania �ycia
		count = local_ctx.drains->size();
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(vector<Drain>::iterator it = local_ctx.drains->begin(), end = local_ctx.drains->end(); it != end; ++it)
			it->Save(file);

		// pociski
		FileWriter f(file);
		f << local_ctx.bullets->size();
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
			it->Save(f);

		WriteFile(file, &check_id, sizeof(check_id), &tmp, nullptr);
		++check_id;
	}

	if(Net::IsOnline())
	{
		WriteString1(file, server_name);
		WriteString1(file, server_pswd);
		WriteFile(file, &players, sizeof(players), &tmp, nullptr);
		WriteFile(file, &max_players, sizeof(max_players), &tmp, nullptr);
		WriteFile(file, &last_id, sizeof(last_id), &tmp, nullptr);
		uint count = 0;
		for(auto info : game_players)
		{
			if(info->left == PlayerInfo::LEFT_NO)
				++count;
		}
		WriteFile(file, &count, sizeof(count), &tmp, nullptr);
		for(auto info : game_players)
		{
			if(info->left == PlayerInfo::LEFT_NO)
				info->Save(file);
		}
		WriteFile(file, &kick_id, sizeof(kick_id), &tmp, nullptr);
		WriteFile(file, &netid_counter, sizeof(netid_counter), &tmp, nullptr);
		WriteFile(file, &item_netid_counter, sizeof(item_netid_counter), &tmp, nullptr);
		WriteFile(file, &chest_netid_counter, sizeof(chest_netid_counter), &tmp, nullptr);
		WriteFile(file, &usable_netid_counter, sizeof(usable_netid_counter), &tmp, nullptr);
		WriteFile(file, &skip_id_counter, sizeof(skip_id_counter), &tmp, nullptr);
		WriteFile(file, &trap_netid_counter, sizeof(trap_netid_counter), &tmp, nullptr);
		WriteFile(file, &door_netid_counter, sizeof(door_netid_counter), &tmp, nullptr);
		WriteFile(file, &electro_netid_counter, sizeof(electro_netid_counter), &tmp, nullptr);
		WriteFile(file, &mp_use_interp, sizeof(mp_use_interp), &tmp, nullptr);
		WriteFile(file, &mp_interp, sizeof(mp_interp), &tmp, nullptr);

		WriteFile(file, &check_id, sizeof(check_id), &tmp, nullptr);
		++check_id;

		Net::PushChange(NetChange::GAME_SAVED);
		AddMultiMsg(txGameSaved);
	}

	WriteFile(file, "EOS", 3, &tmp, nullptr);
}

//=================================================================================================
void Game::SaveStock(HANDLE file, vector<ItemSlot>& cnt)
{
	uint count = cnt.size();
	WriteFile(file, &count, sizeof(count), &tmp, nullptr);
	for(vector<ItemSlot>::iterator it = cnt.begin(), end = cnt.end(); it != end; ++it)
	{
		if(it->item)
		{
			WriteString1(file, it->item->id);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, nullptr);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, nullptr);
		}
	}
}

//=================================================================================================
void Game::SaveQuestsData(HANDLE file)
{
	int refid;

	// sekret
	WriteFile(file, &secret_state, sizeof(secret_state), &tmp, nullptr);
	WriteString1(file, GetSecretNote()->desc);
	WriteFile(file, &secret_where, sizeof(secret_where), &tmp, nullptr);
	WriteFile(file, &secret_where2, sizeof(secret_where2), &tmp, nullptr);

	// drinking contest
	WriteFile(file, &contest_where, sizeof(contest_where), &tmp, nullptr);
	WriteFile(file, &contest_state, sizeof(contest_state), &tmp, nullptr);
	WriteFile(file, &contest_generated, sizeof(contest_generated), &tmp, nullptr);
	refid = (contest_winner ? contest_winner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	if(contest_state >= CONTEST_STARTING)
	{
		WriteFile(file, &contest_state2, sizeof(contest_state2), &tmp, nullptr);
		WriteFile(file, &contest_time, sizeof(contest_time), &tmp, nullptr);
		int ile = contest_units.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(vector<Unit*>::iterator it = contest_units.begin(), end = contest_units.end(); it != end; ++it)
			WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, nullptr);
	}

	// zawody na arenie
	WriteFile(file, &tournament_year, sizeof(tournament_year), &tmp, nullptr);
	WriteFile(file, &tournament_city, sizeof(tournament_city), &tmp, nullptr);
	WriteFile(file, &tournament_city_year, sizeof(tournament_city_year), &tmp, nullptr);
	refid = (tournament_winner ? tournament_winner->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, nullptr);
	WriteFile(file, &tournament_generated, sizeof(tournament_generated), &tmp, nullptr);
}

//=================================================================================================
void Game::LoadGame(HANDLE file)
{
	GameReader f(file);

	ClearGame();
	ClearGameVarsOnLoad();
	StopSounds();
	attached_sounds.clear();
	in_tutorial = false;
	arena_free = true;
	pc_data.autowalk = false;
	ai_bow_targets.clear();
	ai_cast_targets.clear();
	load_location_quest.clear();
	load_unit_handler.clear();
	load_chest_handler.clear();
	units_mesh_load.clear();

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

	// czy online / dev
	byte flags;
	ReadFile(file, &flags, sizeof(flags), &tmp, nullptr);
	bool online_save = IS_SET(flags, SF_ONLINE);
	if(mp_load)
	{
		if(!online_save)
			throw SaveException(txLoadMP, "Save is from singleplayer mode.");
	}
	else
	{
		if(online_save)
			throw SaveException(txLoadSP, "Save is from multiplayer mode.");
	}

	Info("Loading save. Version %s, start %s, format %d, mp %d, debug %d.", VersionToString(version), VersionToString(start_version), LOAD_VERSION,
		online_save ? 1 : 0, IS_SET(flags, SF_DEBUG) ? 1 : 0);

	ReadFile(file, &hardcore_mode, sizeof(hardcore_mode), &tmp, nullptr);
	ReadFile(file, &total_kills, sizeof(total_kills), &tmp, nullptr);

	// world state
	GAME_STATE game_state2;
	ReadFile(file, &year, sizeof(year), &tmp, nullptr);
	ReadFile(file, &month, sizeof(month), &tmp, nullptr);
	ReadFile(file, &day, sizeof(day), &tmp, nullptr);
	ReadFile(file, &worldtime, sizeof(worldtime), &tmp, nullptr);
	ReadFile(file, &game_state2, sizeof(game_state2), &tmp, nullptr);
	GameStats::Get().Load(file);

	Unit::refid_table.clear();
	Usable::refid_table.clear();
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();

	byte check_id = 0, read_id;

	// world map
	LoadingStep(txLoadingLocations);
	ReadFile(file, &world_state, sizeof(world_state), &tmp, nullptr);
	ReadFile(file, &current_location, sizeof(current_location), &tmp, nullptr);
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	locations.resize(ile);
	int index = 0;
	int step = 0;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		LOCATION_TOKEN loc_token;

		ReadFile(file, &loc_token, sizeof(loc_token), &tmp, nullptr);

		if(loc_token != LT_NULL)
		{
			switch(loc_token)
			{
			case LT_OUTSIDE:
				*it = new OutsideLocation;
				break;
			case LT_CITY:
			case LT_VILLAGE_OLD:
				*it = new City;
				break;
			case LT_CAVE:
				*it = new CaveLocation;
				break;
			case LT_SINGLE_DUNGEON:
				*it = new SingleInsideLocation;
				break;
			case LT_MULTI_DUNGEON:
				{
					int levels;
					ReadFile(file, &levels, sizeof(levels), &tmp, nullptr);
					*it = new MultiInsideLocation(levels);
				}
				break;
			case LT_CAMP:
				*it = new Camp;
				break;
			default:
				assert(0);
				*it = new OutsideLocation;
				break;
			}

			(*it)->Load(file, (game_state2 == GS_LEVEL && current_location == index), loc_token);
		}
		else
			*it = nullptr;

		if(step == 0)
		{
			if(index >= int(ile) / 4)
			{
				++step;
				LoadingStep();
			}
		}
		else if(step == 1)
		{
			if(index >= int(ile) / 2)
			{
				++step;
				LoadingStep();
			}
		}
		else if(step == 2)
		{
			if(index >= int(ile) * 3 / 4)
			{
				++step;
				LoadingStep();
			}
		}

		ReadFile(file, &read_id, sizeof(read_id), &tmp, nullptr);
		if(read_id != check_id)
			throw Format("Error while reading location %s (%d).", *it ? (*it)->name.c_str() : "nullptr", index);
		++check_id;
	}
	ReadFile(file, &empty_locations, sizeof(empty_locations), &tmp, nullptr);
	ReadFile(file, &create_camp, sizeof(create_camp), &tmp, nullptr);
	ReadFile(file, &world_pos, sizeof(world_pos), &tmp, nullptr);
	ReadFile(file, &travel_time2, sizeof(travel_time2), &tmp, nullptr);
	ReadFile(file, &szansa_na_spotkanie, sizeof(szansa_na_spotkanie), &tmp, nullptr);
	ReadFile(file, &settlements, sizeof(settlements), &tmp, nullptr);
	ReadFile(file, &encounter_loc, sizeof(encounter_loc), &tmp, nullptr);
	ReadFile(file, &world_dir, sizeof(world_dir), &tmp, nullptr);
	if(world_state == WS_TRAVEL)
	{
		ReadFile(file, &picked_location, sizeof(picked_location), &tmp, nullptr);
		ReadFile(file, &travel_day, sizeof(travel_day), &tmp, nullptr);
		ReadFile(file, &travel_start, sizeof(travel_start), &tmp, nullptr);
		ReadFile(file, &travel_time, sizeof(travel_time), &tmp, nullptr);
		ReadFile(file, &guards_enc_reward, sizeof(guards_enc_reward), &tmp, nullptr);
	}
	else if(world_state == WS_ENCOUNTER)
	{
		world_state = WS_TRAVEL;
		travel_start = world_pos = locations[0]->pos;
		picked_location = 0;
		travel_time = 1.f;
		travel_day = 0;
	}
	if(LOAD_VERSION < V_0_3)
		world_dir = Clip(-world_dir);
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	encs.resize(ile, nullptr);
	int location_event_handler_quest_refid;
	if(game_state2 == GS_LEVEL)
		ReadFile(file, &location_event_handler_quest_refid, sizeof(location_event_handler_quest_refid), &tmp, nullptr);
	else
	{
		location_event_handler_quest_refid = -1;
		// wczytaj dru�yn�
		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		for(uint i = 0; i < ile; ++i)
		{
			Unit* u = new Unit;
			u->Load(file, false);
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
	if(current_location != -1)
	{
		location = locations[current_location];
		if(location->type == L_CITY)
			city_ctx = (City*)location;
		else
			city_ctx = nullptr;
	}
	else
	{
		location = nullptr;
		city_ctx = nullptr;
	}
	ReadFile(file, &first_city, sizeof(first_city), &tmp, nullptr);
	ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
	boss_levels.resize(ile);
	if(ile)
		ReadFile(file, &boss_levels[0], sizeof(Int2)*ile, &tmp, nullptr);
	ReadFile(file, &enter_from, sizeof(enter_from), &tmp, nullptr);
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &light_angle, sizeof(light_angle), &tmp, nullptr);
	else
		light_angle = Random(PI * 2);

	// ustaw wska�niki postaci/u�ywalnych
	LoadingStep(txLoadingData);
	for(vector<std::pair<Unit**, int> >::iterator it = Unit::refid_request.begin(), end = Unit::refid_request.end(); it != end; ++it)
		*(it->first) = Unit::refid_table[it->second];
	Unit::refid_request.clear();
	for(vector<UsableRequest>::iterator it = Usable::refid_request.begin(), end = Usable::refid_request.end(); it != end; ++it)
	{
		Usable* u = Usable::refid_table[it->refid];
		if(u->user != it->user)
		{
			Warn("Invalid usable %s (%d) user %s.", u->base->id.c_str(), u->refid, it->user->data->id.c_str());
			*it->usable = nullptr;
		}
		else
			*it->usable = u;
	}
	Usable::refid_request.clear();

	// camera
	ReadFile(file, &cam.real_rot.y, sizeof(cam.real_rot.y), &tmp, nullptr);
	ReadFile(file, &cam.dist, sizeof(cam.dist), &tmp, nullptr);
	cam.Reset();
	pc_data.rot_buf = 0.f;

	// ekwipunek sprzedawc�w w mie�cie
	LoadStock(file, chest_merchant);
	LoadStock(file, chest_blacksmith);
	LoadStock(file, chest_alchemist);
	LoadStock(file, chest_innkeeper);
	if(LOAD_VERSION >= V_0_2_20)
		LoadStock(file, chest_food_seller);
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
	cam.real_rot.x = pc->unit->rot;
	pc->dialog_ctx = &dialog_context;
	dialog_context.dialog_mode = false;
	dialog_context.is_local = true;
	f >> dungeon_level;
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
	f >> ile;
	ais.resize(ile);
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		*it = new AIController;
		(*it)->Load(file);
	}

	// game messages & speech bubbles
	game_gui->game_messages->Load(f);
	game_gui->Load(f);

	// rumors/notes
	game_gui->journal->Load(file);

	// arena
	arena_tryb = Arena_Brak;

	ReadFile(file, &read_id, sizeof(read_id), &tmp, nullptr);
	if(read_id != check_id)
		throw "Error reading data before team.";
	++check_id;

	// wczytaj dru�yn�
	Team.Load(file);
	CheckCredit(false, true);

	// load quests
	LoadingStep(txLoadingQuests);
	QuestManager& quest_manager = QuestManager::Get();
	quest_manager.Load(file);

	quest_sawmill = (Quest_Sawmill*)quest_manager.FindQuestById(Q_SAWMILL);
	quest_mine = (Quest_Mine*)quest_manager.FindQuestById(Q_MINE);
	quest_bandits = (Quest_Bandits*)quest_manager.FindQuestById(Q_BANDITS);
	quest_goblins = (Quest_Goblins*)quest_manager.FindQuestById(Q_GOBLINS);
	quest_mages = (Quest_Mages*)quest_manager.FindQuestById(Q_MAGES);
	quest_mages2 = (Quest_Mages2*)quest_manager.FindQuestById(Q_MAGES2);
	quest_orcs = (Quest_Orcs*)quest_manager.FindQuestById(Q_ORCS);
	quest_orcs2 = (Quest_Orcs2*)quest_manager.FindQuestById(Q_ORCS2);
	quest_evil = (Quest_Evil*)quest_manager.FindQuestById(Q_EVIL);
	quest_crazies = (Quest_Crazies*)quest_manager.FindQuestById(Q_CRAZIES);

	if(!quest_mages2)
	{
		quest_mages2 = new Quest_Mages2;
		quest_mages2->refid = quest_manager.quest_counter++;
		quest_mages2->Start();
		quest_manager.unaccepted_quests.push_back(quest_mages2);
	}

	for(vector<QuestItemRequest*>::iterator it = quest_manager.quest_item_requests.begin(), end = quest_manager.quest_item_requests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		*qir->item = quest_manager.FindQuestItem(qir->name.c_str(), qir->quest_refid);
		if(qir->items)
		{
			bool ok = true;
			for(vector<ItemSlot>::iterator it2 = qir->items->begin(), end2 = qir->items->end(); it2 != end2; ++it2)
			{
				if(it2->item == QUEST_ITEM_PLACEHOLDER)
				{
					ok = false;
					break;
				}
			}
			if(ok)
				SortItems(*qir->items);
		}
		delete *it;
	}
	quest_manager.quest_item_requests.clear();
	LoadQuestsData(file);

	// newsy
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	news.resize(count);
	for(vector<News*>::iterator it = news.begin(), end = news.end(); it != end; ++it)
	{
		*it = new News;
		ReadFile(file, &(*it)->add_time, sizeof((*it)->add_time), &tmp, nullptr);
		ReadString2(file, (*it)->text);
	}

	ReadFile(file, &read_id, sizeof(read_id), &tmp, nullptr);
	if(read_id != check_id)
		throw "Error reading data after news.";
	++check_id;

	LoadingStep(txLoadingLevel);

	if(game_state2 == GS_LEVEL)
	{
		open_location = current_location;

		if(location->outside)
		{
			OutsideLocation* outside = (OutsideLocation*)location;

			SetOutsideParams();
			SetTerrainTextures();

			ApplyContext(location, local_ctx);
			ApplyTiles(outside->h, outside->tiles);

			RespawnObjectColliders(false);
			SpawnTerrainCollider();

			if(city_ctx)
			{
				RespawnBuildingPhysics();
				SpawnCityPhysics();
				CreateCityMinimap();
			}
			else
			{
				SpawnOutsideBariers();
				CreateForestMinimap();
			}

			InitQuadTree();
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			inside->SetActiveLevel(dungeon_level);
			BaseLocation& base = g_base_locations[inside->target];

			ApplyContext(inside, local_ctx);
			SetDungeonParamsAndTextures(base);

			RespawnObjectColliders(false);
			SpawnDungeonColliders();
			CreateDungeonMinimap();
		}

		// cz�steczki
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		local_ctx.pes->resize(count);
		for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
		{
			*it = new ParticleEmitter;
			ParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		local_ctx.tpes->resize(count);
		for(vector<TrailParticleEmitter*>::iterator it = local_ctx.tpes->begin(), end = local_ctx.tpes->end(); it != end; ++it)
		{
			*it = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		// wybuchy
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		local_ctx.explos->resize(count);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			(*it)->Load(file);
		}

		// elektryczno��
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		local_ctx.electros->resize(count);
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
		{
			*it = new Electro;
			(*it)->Load(file);
		}

		// wyssania �ycia
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		local_ctx.drains->resize(count);
		for(vector<Drain>::iterator it = local_ctx.drains->begin(), end = local_ctx.drains->end(); it != end; ++it)
			it->Load(file);

		// pociski
		FileReader f(file);
		f >> count;
		local_ctx.bullets->resize(count);
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
			it->Load(f);

		ReadFile(file, &read_id, sizeof(read_id), &tmp, nullptr);
		if(read_id != check_id)
			throw "Failed to read level data.";
		++check_id;

		local_ctx_valid = true;
		RemoveUnusedAiAndCheck();
	}
	else
	{
		open_location = -1;
		local_ctx_valid = false;
	}

	// gui
	if(LOAD_VERSION <= V_0_3)
	{
		FileReader f(file);
		LoadGui(f);
	}
	game_gui->PositionPanels();

	// cele ai
	if(!ai_bow_targets.empty())
	{
		BaseObject* tarcza_s = BaseObject::Get("bow_target");
		for(vector<AIController*>::iterator it = ai_bow_targets.begin(), end = ai_bow_targets.end(); it != end; ++it)
		{
			AIController& ai = **it;
			LevelContext& ctx = Game::Get().GetContext(*ai.unit);
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

	// cele czar�w ai
	for(vector<AIController*>::iterator it = ai_cast_targets.begin(), end = ai_cast_targets.end(); it != end; ++it)
	{
		AIController& ai = **it;
		int refid = (int)ai.cast_target;
		if(refid == -1)
		{
			// zapis z wersji < 2, szukaj w pobli�u punktu
			LevelContext& ctx = GetContext(*ai.unit);
			Spell* spell = ai.unit->data->spells->spell[ai.unit->attack_id];
			float dist2, best_dist2 = spell->range;
			ai.cast_target = nullptr;

			if(IS_SET(spell->flags, Spell::Raise))
			{
				// czar o�ywiania
				for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
				{
					if(!(*it2)->to_remove && (*it2)->live_state == Unit::DEAD && !IsEnemy(*ai.unit, **it2) && IS_SET((*it2)->data->flags, F_UNDEAD) &&
						(dist2 = Vec3::Distance(ai.target_last_pos, (*it2)->pos)) < best_dist2 && CanSee(*ai.unit, **it2))
					{
						best_dist2 = dist2;
						ai.cast_target = *it2;
					}
				}
			}
			else
			{
				// czar leczenia
				for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
				{
					if(!(*it2)->to_remove && !IsEnemy(*ai.unit, **it2) && !IS_SET((*it2)->data->flags, F_UNDEAD) && (*it2)->hpmax - (*it2)->hp > 100.f &&
						(dist2 = Vec3::Distance(ai.target_last_pos, (*it2)->pos)) < best_dist2 && CanSee(*ai.unit, **it2))
					{
						best_dist2 = dist2;
						ai.cast_target = *it2;
					}
				}
			}

			if(!ai.cast_target)
			{
				ai.state = AIController::Idle;
				ai.idle_action = AIController::Idle_None;
				ai.timer = Random(1.f, 2.f);
			}
		}
		else
			ai.cast_target = Unit::GetByRefid(refid);
	}

	// questy zwi�zane z lokacjami
	for(vector<Location*>::iterator it = load_location_quest.begin(), end = load_location_quest.end(); it != end; ++it)
	{
		(*it)->active_quest = (Quest_Dungeon*)quest_manager.FindQuest((int)(*it)->active_quest, false);
		assert((*it)->active_quest);
	}
	// unit event handler
	for(vector<Unit*>::iterator it = load_unit_handler.begin(), end = load_unit_handler.end(); it != end; ++it)
	{
		// pierwszy raz musia�em u�y� tego rzutowania �eby dzia�a�o :o
		(*it)->event_handler = dynamic_cast<UnitEventHandler*>(quest_manager.FindQuest((int)(*it)->event_handler, false));
		assert((*it)->event_handler);
	}
	// chest event handler
	for(vector<Chest*>::iterator it = load_chest_handler.begin(), end = load_chest_handler.end(); it != end; ++it)
	{
		(*it)->handler = dynamic_cast<ChestEventHandler*>(quest_manager.FindQuest((int)(*it)->handler, false));
		assert((*it)->handler);
	}

	if(tournament_generated)
		tournament_master = FindUnitByIdLocal("arena_master");
	else
		tournament_master = nullptr;

	minimap_reveal.clear();
	dialog_context.dialog_mode = false;
	if(location_event_handler_quest_refid != -1)
		location_event_handler = dynamic_cast<LocationEventHandler*>(quest_manager.FindQuest(location_event_handler_quest_refid));
	else
		location_event_handler = nullptr;
	team_shares.clear();
	team_share_id = -1;
	fallback_co = FALLBACK::NONE;
	fallback_t = -0.5f;
	inventory_mode = I_NONE;
	pc_data.before_player = BP_NONE;
	pc_data.selected_unit = nullptr;
	pc_data.selected_target = nullptr;
	dialog_context.pc = pc;
	dialog_context.dialog_mode = false;
	game_gui->Setup();

	if(mp_load)
	{
		ReadString1(file, server_name);
		ReadString1(file, server_pswd);
		ReadFile(file, &players, sizeof(players), &tmp, nullptr);
		ReadFile(file, &max_players, sizeof(max_players), &tmp, nullptr);
		ReadFile(file, &last_id, sizeof(last_id), &tmp, nullptr);
		uint count;
		ReadFile(file, &count, sizeof(count), &tmp, nullptr);
		DeleteElements(old_players);
		old_players.resize(count);
		for(uint i = 0; i < count; ++i)
		{
			old_players[i] = new PlayerInfo;
			old_players[i]->Load(file);
		}
		ReadFile(file, &kick_id, sizeof(kick_id), &tmp, nullptr);
		ReadFile(file, &netid_counter, sizeof(netid_counter), &tmp, nullptr);
		ReadFile(file, &item_netid_counter, sizeof(item_netid_counter), &tmp, nullptr);
		ReadFile(file, &chest_netid_counter, sizeof(chest_netid_counter), &tmp, nullptr);
		ReadFile(file, &usable_netid_counter, sizeof(usable_netid_counter), &tmp, nullptr);
		ReadFile(file, &skip_id_counter, sizeof(skip_id_counter), &tmp, nullptr);
		ReadFile(file, &trap_netid_counter, sizeof(trap_netid_counter), &tmp, nullptr);
		ReadFile(file, &door_netid_counter, sizeof(door_netid_counter), &tmp, nullptr);
		ReadFile(file, &electro_netid_counter, sizeof(electro_netid_counter), &tmp, nullptr);
		ReadFile(file, &mp_use_interp, sizeof(mp_use_interp), &tmp, nullptr);
		ReadFile(file, &mp_interp, sizeof(mp_interp), &tmp, nullptr);

		ReadFile(file, &read_id, sizeof(read_id), &tmp, nullptr);
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
		ReadFile(file, eos, 3, &tmp, nullptr);
		if(eos[0] != 'E' || eos[1] != 'O' || eos[2] != 'S')
			throw "Missing EOS.";
	}

	if(enter_from == ENTER_FROM_UNKNOWN && game_state2 == GS_LEVEL)
	{
		// zgadnij sk�d przysz�a dru�yna
		if(current_location == secret_where2)
			enter_from = ENTER_FROM_PORTAL;
		else if(location->type == L_DUNGEON)
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->from_portal)
				enter_from = ENTER_FROM_PORTAL;
			else
			{
				if(dungeon_level == 0)
					enter_from = ENTER_FROM_OUTSIDE;
				else
					enter_from = ENTER_FROM_UP_LEVEL;
			}
		}
		else if(location->type == L_CRYPT)
		{
			if(dungeon_level == 0)
				enter_from = ENTER_FROM_OUTSIDE;
			else
				enter_from = ENTER_FROM_UP_LEVEL;
		}
		else
			enter_from = ENTER_FROM_OUTSIDE;
	}

	if(location->outside)
		CalculateQuadtree();

	// load music
	LoadingStep(txLoadMusic);
	if(!nomusic)
	{
		LoadMusic(MusicType::Boss, false);
		LoadMusic(MusicType::Death, false);
		LoadMusic(MusicType::Travel, false);
	}

	LoadResources(txEndOfLoading, game_state2 == GS_WORLDMAP);
	load_screen->visible = false;

#ifdef _DEBUG
	ValidateTeamItems();
#endif

	Info("Game loaded.");

	if(mp_load)
	{
		game_state = GS_MAIN_MENU;
		return;
	}

	if(game_state2 == GS_LEVEL)
		SetMusic();
	else
	{
		picked_location = -1;
		SetMusic(MusicType::Travel);
	}
	game_state = game_state2;
	clear_color = clear_color2;
}

//=================================================================================================
void Game::LoadStock(HANDLE file, vector<ItemSlot>& cnt)
{
	bool can_sort = true;

	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, nullptr);
	cnt.resize(count);
	for(vector<ItemSlot>::iterator it = cnt.begin(), end = cnt.end(); it != end; ++it)
	{
		ReadString1(file);
		ReadFile(file, &it->count, sizeof(it->count), &tmp, nullptr);
		if(BUF[0] != '$')
			it->item = FindItem(BUF);
		else
		{
			int quest_refid;
			ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, nullptr);
			QuestManager::Get().AddQuestItemRequest(&it->item, BUF, quest_refid, &cnt);
			it->item = QUEST_ITEM_PLACEHOLDER;
			can_sort = false;
		}
	}

	if(can_sort && LOAD_VERSION < V_0_2_20 && !cnt.empty())
		SortItems(cnt);
}

//=================================================================================================
void Game::LoadQuestsData(HANDLE file)
{
	int refid;

	// load quests old data (now are stored inside quest)
	if(LOAD_VERSION < V_0_4)
	{
		quest_sawmill->LoadOld(file);
		quest_mine->LoadOld(file);
		quest_bandits->LoadOld(file);
		quest_mages2->LoadOld(file);
		quest_orcs2->LoadOld(file);
		quest_goblins->LoadOld(file);
		quest_evil->LoadOld(file);
		quest_crazies->LoadOld(file);
	}

	// sekret
	ReadFile(file, &secret_state, sizeof(secret_state), &tmp, nullptr);
	ReadString1(file, GetSecretNote()->desc);
	ReadFile(file, &secret_where, sizeof(secret_where), &tmp, nullptr);
	ReadFile(file, &secret_where2, sizeof(secret_where2), &tmp, nullptr);

	if(secret_state > SECRET_NONE && !BaseObject::Get("tomashu_dom")->mesh)
		throw "Save uses 'data.pak' file which is missing!";

	// drinking contest
	ReadFile(file, &contest_where, sizeof(contest_where), &tmp, nullptr);
	ReadFile(file, &contest_state, sizeof(contest_state), &tmp, nullptr);
	ReadFile(file, &contest_generated, sizeof(contest_generated), &tmp, nullptr);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	contest_winner = Unit::GetByRefid(refid);
	if(contest_state >= CONTEST_STARTING)
	{
		ReadFile(file, &contest_state2, sizeof(contest_state2), &tmp, nullptr);
		ReadFile(file, &contest_time, sizeof(contest_time), &tmp, nullptr);
		int ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, nullptr);
		contest_units.resize(ile);
		for(vector<Unit*>::iterator it = contest_units.begin(), end = contest_units.end(); it != end; ++it)
		{
			ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
			*it = Unit::GetByRefid(refid);
		}
	}

	// zawody na arenie
	ReadFile(file, &tournament_year, sizeof(tournament_year), &tmp, nullptr);
	ReadFile(file, &tournament_city, sizeof(tournament_city), &tmp, nullptr);
	ReadFile(file, &tournament_city_year, sizeof(tournament_city_year), &tmp, nullptr);
	ReadFile(file, &refid, sizeof(refid), &tmp, nullptr);
	tournament_winner = Unit::GetByRefid(refid);
	ReadFile(file, &tournament_generated, sizeof(tournament_generated), &tmp, nullptr);
	tournament_state = TOURNAMENT_NOT_DONE;
	tournament_units.clear();
}

//=================================================================================================
void Game::Quicksave(bool from_console)
{
	if(!CanSaveGame())
	{
		if(from_console)
			AddConsoleMsg("Can't save game now.");
		else
			GUI.SimpleDialog(txCantSaveNow, nullptr);
		return;
	}

	if(SaveGameSlot(MAX_SAVE_SLOTS, txQuickSave))
	{
		if(!from_console)
			AddGameMsg2(txGameSaved, 1.f, GMS_GAME_SAVED);
	}
}

//=================================================================================================
bool Game::Quickload(bool from_console)
{
	if(!CanLoadGame())
	{
		if(from_console)
			AddConsoleMsg("Can't load game now.");
		else
			GUI.SimpleDialog(txCantLoadGame, nullptr);
		return true;
	}

	try
	{
		Net::SetMode(Net::Mode::Singleplayer);
		LoadGameSlot(MAX_SAVE_SLOTS);
	}
	catch(const SaveException& ex)
	{
		if(ex.missing_file)
		{
			Warn("Missing quicksave.");
			return false;
		}

		Error("Failed to load game: %s", ex.msg);
		cstring dialog_text;
		if(ex.localized_msg)
			dialog_text = Format("%s%s", txLoadError, ex.localized_msg);
		else
			dialog_text = txLoadErrorGeneric;
		GUI.SimpleDialog(dialog_text, nullptr);
	}

	return true;
}

//=================================================================================================
void Game::RemoveUnusedAiAndCheck()
{
	uint prev_size = ais.size();
	bool deleted = false;

	// to jest tylko tymczasowe rozwi�zanie nadmiarowych ai, by� mo�e wyst�powa�o tylko w wersji 0.2 ale p�ki co niech zostanie
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		bool ok = false;
		for(vector<Unit*>::iterator it2 = local_ctx.units->begin(), end2 = local_ctx.units->end(); it2 != end2; ++it2)
		{
			if((*it2)->ai == *it)
			{
				ok = true;
				break;
			}
		}
		if(!ok && city_ctx)
		{
			for(vector<InsideBuilding*>::iterator it2 = city_ctx->inside_buildings.begin(), end2 = city_ctx->inside_buildings.end(); it2 != end2 && !ok; ++it2)
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
		AddGameMsg(s, 10.f);
#endif
	}

#ifdef _DEBUG
	int err_count = 0;
	CheckUnitsAi(local_ctx, err_count);
	if(city_ctx)
	{
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			CheckUnitsAi((*it)->ctx, err_count);
	}
	if(err_count)
		AddGameMsg(Format("CheckUnitsAi: %d errors!", err_count), 10.f);
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
