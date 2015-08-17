// zapisywanie/wczytywanie gry
#include "Pch.h"
#include "Game.h"
#include "SaveState.h"
#include "Version.h"
#include "Quest_Sawmill.h"
#include "Quest_Bandits.h"
#include "Quest_Mine.h"
#include "Quest_Goblins.h"
#include "Quest_Mages.h"
#include "Quest_Orcs.h"
#include "Quest_Evil.h"
#include "Quest_Crazies.h"
#include "GameFile.h"

#define SF_ONLINE (1<<0)
//#define SF_DEV (1<<1)
#define SF_DEBUG (1<<2)
//#define SF_BETA (1<<3)

//=================================================================================================
bool Game::CanSaveGame() const
{
	if(game_state == GS_MAIN_MENU || sekret_stan == SS2_WALKA)
		return false;

	if(game_state == GS_WORLDMAP)
	{
		if(world_state == WS_TRAVEL || world_state == WS_ENCOUNTER)
			return false;
	}
	else
	{
		if(in_tutorial || arena_tryb != Arena_Brak || chlanie_stan >= 3 || zawody_stan != IS_BRAK)
			return false;
	}

	if(IsOnline())
	{
		if(IsAnyoneAlive() && IsServer())
			return true;
	}
	else if(pc->unit->IsAlive() || pc->unit->in_arena != -1)
		return true;

	return false;
}

//=================================================================================================
bool Game::CanLoadGame() const
{
	if(IsOnline())
		return false;

	return true;
}

//=================================================================================================
bool Game::SaveGameSlot(int slot, cstring text)
{
	assert(in_range(slot, 1, MAX_SAVE_SLOTS));

	if(!CanSaveGame())
	{
		// w tej chwili nie mo¿na zapisaæ gry
		GUI.SimpleDialog(txCantSaveGame, saveload->visible ? saveload : NULL);
		return false;
	}

	CreateDirectory("saves", NULL);
	CreateDirectory(IsOnline() ? "saves/multi" : "saves/single", NULL);

	cstring filename = Format(IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);

	if(FileExists(filename))
	{
		cstring bak_filename = Format("%s.bak", filename);
		DeleteFile(bak_filename);
		MoveFile(filename, bak_filename);
	}

	HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		ERROR(Format(txLoadOpenError, filename, GetLastError()));
		GUI.SimpleDialog(txSaveFailed, saveload->visible ? saveload : NULL);
		return false;
	}
	else
	{
		SaveGame(file);
		CloseHandle(file);

		cstring msg = Format("Game saved '%s'.", filename);
		AddConsoleMsg(msg);
		LOG(msg);

		SaveSlot& ss = (IsOnline() ? multi_saves[slot-1] : single_saves[slot-1]);
		ss.valid = true;
		ss.game_day = day;
		ss.game_month = month;
		ss.game_year = year;
		ss.location = GetCurrentLocationText();
		ss.player_name = pc->name;
		ss.player_class = pc->clas;
		ss.save_date = time(NULL);
		ss.text = (text[0] != 0 ? text : Format(txSavedGameN, slot));
		ss.hardcore = hardcore_mode;

		Config cfg;
		cfg.Add("game_day", Format("%d", day));
		cfg.Add("game_month", Format("%d", month));
		cfg.Add("game_year", Format("%d", year));
		cfg.Add("location", ss.location.c_str());
		cfg.Add("player_name", ss.player_name.c_str());
		cfg.Add("player_class", g_classes[(int)ss.player_class].id);
		cfg.Add("save_date", Format("%I64d", ss.save_date));
		cfg.Add("text", ss.text.c_str());
		cfg.Add("hardcore", ss.hardcore ? "1" : "0");

		if(IsOnline())
		{
			ss.multiplayers = players;
			cfg.Add("multiplayers", Format("%d", ss.multiplayers));
		}

		cfg.Save(Format("saves/%s/%d.txt", IsOnline() ? "multi" : "single", slot));

		cstring path = Format("saves/%s/%d.jpg", IsOnline() ? "multi" : "single", slot);
		CreateSaveImage(path);

		if(hardcore_mode)
		{
			LOG("Hardcore mode, exiting to menu.");
			ExitToMenu();
			return false;
		}

		return true;
	}
}

//=================================================================================================
bool Game::LoadGameSlot(int slot)
{
	assert(in_range(slot, 1, MAX_SAVE_SLOTS));

	cstring filename = Format(mp_load ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot);

	LOG(Format("Loading saved game '%s'.", filename));

	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		ERROR(Format(txLoadOpenError, filename, GetLastError()));
		throw txLoadFailed;
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
	LoadingStart(7);

	try
	{
		LoadGame(file);
	}
	catch(cstring /*err*/)
	{
		prev_game_state = GS_LOAD;
		CloseHandle(file);
		ExitToMenu();
		throw;
	}

	prev_game_state = GS_LOAD;
	CloseHandle(file);

	if(hardcore_mode)
	{
		SaveSlot& s = single_saves[slot-1];
		s.valid = false;
		hardcore_savename = s.text;

		LOG("Hardcore mode, deleting save.");
		DeleteFile(Format(IsOnline() ? "saves/multi/%d.sav" : "saves/single/%d.sav", slot));
		DeleteFile(Format(IsOnline() ? "saves/multi/%d.txt" : "saves/single/%d.txt", slot));
		DeleteFile(Format(IsOnline() ? "saves/multi/%d.jpg" : "saves/single/%d.jpg", slot));
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

	return true;
}

//=================================================================================================
void Game::LoadSaveSlots()
{
	for(int multi=0; multi<2; ++multi)
	{
		for(int i=1; i<=MAX_SAVE_SLOTS; ++i)
		{
			SaveSlot& slot = (multi == 0 ? single_saves : multi_saves)[i-1];
			cstring filename = Format("saves/%s/%d.sav", multi == 0 ? "single" : "multi", i);
			if(FileExists(filename))
			{
				slot.valid = true;
				filename = Format("saves/%s/%d.txt", multi == 0 ? "single" : "multi", i);
				if(FileExists(filename))
				{
					Config cfg;
					cfg.Open(filename);
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
	LOG("Saving...");

	File f(file);

	// przed zapisaniem zaktualizuj minimapê, przenieœ jednostki itp
	if(IsOnline())
		Net_PreSave();
	UpdateDungeonMinimap(false);
	ProcessUnitWarps();
	ProcessRemoveUnits();
	if(game_state == GS_WORLDMAP && open_location != -1)
		LeaveLocation(false);

	// sygnatura
	char sign[4] = {'C','R','S','V'};
	WriteFile(file, sign, 4, &tmp, NULL);

	// wersja gry
	int w = VERSION;
	WriteFile(file, &w, sizeof(w), &tmp, NULL);

	// wersja zapisu
	w = SAVE_VERSION;
	WriteFile(file, &w, sizeof(w), &tmp, NULL);

	// czy online / dev
	byte flags = (sv_online ? SF_ONLINE : 0);
#ifdef _DEBUG
	flags |= SF_DEBUG;
#endif
	WriteFile(file, &flags, sizeof(flags), &tmp, NULL);

	// czy hardcore
	WriteFile(file, &hardcore_mode, sizeof(hardcore_mode), &tmp, NULL);
	WriteFile(file, &total_kills, sizeof(total_kills), &tmp, NULL);

	// world state
	WriteFile(file, &year, sizeof(year), &tmp, NULL);
	WriteFile(file, &month, sizeof(month), &tmp, NULL);
	WriteFile(file, &day, sizeof(day), &tmp, NULL);
	WriteFile(file, &worldtime, sizeof(worldtime), &tmp, NULL);
	WriteFile(file, &game_state, sizeof(game_state), &tmp, NULL);
	WriteFile(file, &gt_hour, sizeof(gt_hour), &tmp, NULL);
	WriteFile(file, &gt_minute, sizeof(gt_minute), &tmp, NULL);
	WriteFile(file, &gt_second, sizeof(gt_second), &tmp, NULL);
	WriteFile(file, &gt_tick, sizeof(gt_tick), &tmp, NULL);

	BuildRefidTables();

	byte check_id = 0;

	// world map
	WriteFile(file, &world_state, sizeof(world_state), &tmp, NULL);
	WriteFile(file, &current_location, sizeof(current_location), &tmp, NULL);
	uint ile = locations.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		LOCATION_TOKEN loc_token;

		if(*it)
			loc_token = (*it)->GetToken();
		else
			loc_token = LT_NULL;

		WriteFile(file, &loc_token, sizeof(loc_token), &tmp, NULL);

		if(loc_token != LT_NULL)
		{
			if(loc_token == LT_MULTI_DUNGEON)
			{
				int levels = ((MultiInsideLocation*)(*it))->levels.size();
				WriteFile(file, &levels, sizeof(levels), &tmp, NULL);
			}
			(*it)->Save(file, (game_state == GS_LEVEL && location == *it));
		}

		WriteFile(file, &check_id, sizeof(check_id), &tmp, NULL);
		++check_id;
	}
	WriteFile(file, &empty_locations, sizeof(empty_locations), &tmp, NULL);
	WriteFile(file, &create_camp, sizeof(create_camp), &tmp, NULL);
	WriteFile(file, &world_pos, sizeof(world_pos), &tmp, NULL);
	WriteFile(file, &travel_time2, sizeof(travel_time2), &tmp, NULL);
	WriteFile(file, &szansa_na_spotkanie, sizeof(szansa_na_spotkanie), &tmp, NULL);
	WriteFile(file, &cities, sizeof(cities), &tmp, NULL);
	WriteFile(file, &encounter_loc, sizeof(encounter_loc), &tmp, NULL);
	WriteFile(file, &world_dir, sizeof(world_dir), &tmp, NULL);
	if(world_state == WS_TRAVEL)
	{
		WriteFile(file, &picked_location, sizeof(picked_location), &tmp, NULL);
		WriteFile(file, &travel_day, sizeof(travel_day), &tmp, NULL);
		WriteFile(file, &travel_start, sizeof(travel_start), &tmp, NULL);
		WriteFile(file, &travel_time, sizeof(travel_time), &tmp, NULL);
		WriteFile(file, &guards_enc_reward, sizeof(guards_enc_reward), &tmp, NULL);
	}
	ile = encs.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	if(game_state == GS_LEVEL)
	{
		int location_event_handler_quest_refid = (location_event_handler ? location_event_handler->GetLocationEventHandlerQuestRefid() : -1);
		WriteFile(file, &location_event_handler_quest_refid, sizeof(location_event_handler_quest_refid), &tmp, NULL);
	}
	else
	{
		// zapisz dru¿ynê
		ile = team.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		{
			(*it)->Save(file, false);
			(*it)->refid = (int)Unit::refid_table.size();
			Unit::refid_table.push_back(*it);
		}
	}
	WriteFile(file, &first_city, sizeof(first_city), &tmp, NULL);
	ile = boss_levels.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	if(ile)
		WriteFile(file, &boss_levels[0], sizeof(INT2)*ile, &tmp, NULL);
	WriteFile(file, &enter_from, sizeof(enter_from), &tmp, NULL);
	WriteFile(file, &light_angle, sizeof(light_angle), &tmp, NULL);

	// kamera
	WriteFile(file, &cam.real_rot.y, sizeof(cam.real_rot.y), &tmp, NULL);
	WriteFile(file, &cam.dist, sizeof(cam.dist), &tmp, NULL);

	// zapisz ekwipunek sprzedawców w mieœcie
	SaveStock(file, chest_merchant);
	SaveStock(file, chest_blacksmith);
	SaveStock(file, chest_alchemist);
	SaveStock(file, chest_innkeeper);
	SaveStock(file, chest_food_seller);

	// zmienne
	WriteFile(file, &used_cheats, sizeof(used_cheats), &tmp, NULL);
	WriteFile(file, &cheats, sizeof(cheats), &tmp, NULL);
	WriteFile(file, &nosound, sizeof(nosound), &tmp, NULL);
	WriteFile(file, &noai, sizeof(noai), &tmp, NULL);
	WriteFile(file, &dont_wander, sizeof(dont_wander), &tmp, NULL);
	WriteFile(file, &pc->unit->refid, sizeof(pc->unit->refid), &tmp, NULL);
	WriteFile(file, &dungeon_level, sizeof(dungeon_level), &tmp, NULL);
	WriteFile(file, &portal_anim, sizeof(portal_anim), &tmp, NULL);
	WriteFile(file, &drunk_anim, sizeof(drunk_anim), &tmp, NULL);
	ile = ais.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
		(*it)->Save(file);

	// game messages & speech bubbles
	game_gui->game_messages->Save(f);
	game_gui->Save(f);

	// zapisz plotki / notatki
	uint count = plotki.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<string>::iterator it = plotki.begin(), end = plotki.end(); it != end; ++it)
	{
		word len = (word)it->length();
		WriteFile(file, &len, sizeof(len), &tmp, NULL);
		WriteFile(file, it->c_str(), len, &tmp, NULL);
	}
	count = notes.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
	{
		word len = (word)it->length();
		WriteFile(file, &len, sizeof(len), &tmp, NULL);
		WriteFile(file, it->c_str(), len, &tmp, NULL);
	}

	WriteFile(file, &check_id, sizeof(check_id), &tmp, NULL);
	++check_id;

	// zapisz dru¿ynê
	count = team.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	count = active_team.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	WriteFile(file, &leader->refid, sizeof(leader->refid), &tmp, NULL);
	WriteFile(file, &team_gold, sizeof(team_gold), &tmp, NULL);
	WriteFile(file, &atak_szalencow, sizeof(atak_szalencow), &tmp, NULL);
	WriteFile(file, &bandyta, sizeof(bandyta), &tmp, NULL);
	WriteFile(file, &free_recruit, sizeof(free_recruit), &tmp, NULL);

	// zapisz questy
	count = quests.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Quest*>::iterator it = quests.begin(), end = quests.end(); it != end; ++it)
		(*it)->Save(file);
	count = unaccepted_quests.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Quest*>::iterator it = unaccepted_quests.begin(), end = unaccepted_quests.end(); it != end; ++it)
		(*it)->Save(file);
	count = quests_timeout.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<Quest_Dungeon*>::iterator it = quests_timeout.begin(), end = quests_timeout.end(); it != end; ++it)
		WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	count = timed_units.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<TimedUnit>::iterator it = timed_units.begin(), end = timed_units.end(); it != end; ++it)
	{
		TimedUnit& tu = *it;
		WriteFile(file, &tu.unit->refid, sizeof(tu.unit->refid), &tmp, NULL);
		WriteFile(file, &tu.location, sizeof(tu.location), &tmp, NULL);
		WriteFile(file, &tu.days, sizeof(tu.days), &tmp, NULL);
	}
	WriteFile(file, &quest_counter, sizeof(quest_counter), &tmp, NULL);
	WriteFile(file, &unique_quests_completed, sizeof(unique_quests_completed), &tmp, NULL);
	WriteFile(file, &unique_completed_show, sizeof(unique_completed_show), &tmp, NULL);
	SaveQuestsData(file);

	// newsy
	count = news.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<News*>::iterator it = news.begin(), end = news.end(); it != end; ++it)
	{
		WriteFile(file, &(*it)->add_time, sizeof((*it)->add_time), &tmp, NULL);
		WriteString2(file, (*it)->text);
	}

	WriteFile(file, &check_id, sizeof(check_id), &tmp, NULL);
	++check_id;

	if(game_state == GS_LEVEL)
	{
		// gra
		//-----------------------
		// cz¹steczki
		count = local_ctx.pes->size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
			(*it)->Save(file);

		count = local_ctx.tpes->size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		for(vector<TrailParticleEmitter*>::iterator it = local_ctx.tpes->begin(), end = local_ctx.tpes->end(); it != end; ++it)
			(*it)->Save(file);

		// wybuchy
		count = local_ctx.explos->size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
			(*it)->Save(file);

		// elektrycznoœæ
		count = local_ctx.electros->size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
			(*it)->Save(file);

		// wyssania ¿ycia
		count = local_ctx.drains->size();
		WriteFile(file, &count, sizeof(count), &tmp, NULL);
		for(vector<Drain>::iterator it = local_ctx.drains->begin(), end = local_ctx.drains->end(); it != end; ++it)
			it->Save(file);

		// pociski
		File f(file);
		f << local_ctx.bullets->size();
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
			it->Save(f);

		WriteFile(file, &check_id, sizeof(check_id), &tmp, NULL);
		++check_id;
	}

	if(IsOnline())
	{
		WriteString1(file, server_name);
		WriteString1(file, server_pswd);
		WriteFile(file, &players, sizeof(players), &tmp, NULL);
		WriteFile(file, &max_players, sizeof(max_players), &tmp, NULL);
		WriteFile(file, &last_id, sizeof(last_id), &tmp, NULL);
		uint ile = 0;
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(!it->left)
				++ile;
		}
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(!it->left)
				it->Save(file);
		}
		WriteFile(file, &kick_id, sizeof(kick_id), &tmp, NULL);
		WriteFile(file, &netid_counter, sizeof(netid_counter), &tmp, NULL);
		WriteFile(file, &item_netid_counter, sizeof(item_netid_counter), &tmp, NULL);
		WriteFile(file, &chest_netid_counter, sizeof(chest_netid_counter), &tmp, NULL);
		WriteFile(file, &useable_netid_counter, sizeof(useable_netid_counter), &tmp, NULL);
		WriteFile(file, &skip_id_counter, sizeof(skip_id_counter), &tmp, NULL);
		WriteFile(file, &trap_netid_counter, sizeof(trap_netid_counter), &tmp, NULL);
		WriteFile(file, &door_netid_counter, sizeof(door_netid_counter), &tmp, NULL);
		WriteFile(file, &electro_netid_counter, sizeof(electro_netid_counter), &tmp, NULL);
		WriteFile(file, &mp_use_interp, sizeof(mp_use_interp), &tmp, NULL);
		WriteFile(file, &mp_interp, sizeof(mp_interp), &tmp, NULL);

		WriteFile(file, &check_id, sizeof(check_id), &tmp, NULL);
		++check_id;

		PushNetChange(NetChange::GAME_SAVED);
		AddMultiMsg(txGameSaved);
	}
}

//=================================================================================================
void Game::SaveStock(HANDLE file, vector<ItemSlot>& cnt)
{
	uint count = cnt.size();
	WriteFile(file, &count, sizeof(count), &tmp, NULL);
	for(vector<ItemSlot>::iterator it = cnt.begin(), end = cnt.end(); it != end; ++it)
	{
		if(it->item)
		{
			WriteString1(file, it->item->id);
			WriteFile(file, &it->count, sizeof(it->count), &tmp, NULL);
			if(it->item->id[0] == '$')
				WriteFile(file, &it->item->refid, sizeof(int), &tmp, NULL);
		}
		else
		{
			byte b = 0;
			WriteFile(file, &b, sizeof(b), &tmp, NULL);
		}
	}
}

//=================================================================================================
void Game::SaveQuestsData(HANDLE file)
{
	int refid;

	// plotki jednorazowe
	WriteFile(file, &ile_plotek_questowych, sizeof(ile_plotek_questowych), &tmp, NULL);
	WriteFile(file, plotka_questowa, sizeof(plotka_questowa), &tmp, NULL);

	// sekret
	WriteFile(file, &sekret_stan, sizeof(sekret_stan), &tmp, NULL);
	WriteString1(file, GetSecretNote()->desc);
	WriteFile(file, &sekret_gdzie, sizeof(sekret_gdzie), &tmp, NULL);
	WriteFile(file, &sekret_gdzie2, sizeof(sekret_gdzie2), &tmp, NULL);

	// zawody w piciu
	WriteFile(file, &chlanie_gdzie, sizeof(chlanie_gdzie), &tmp, NULL);
	WriteFile(file, &chlanie_stan, sizeof(chlanie_stan), &tmp, NULL);
	WriteFile(file, &chlanie_wygenerowano, sizeof(chlanie_wygenerowano), &tmp, NULL);
	refid = (chlanie_zwyciezca ? chlanie_zwyciezca->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	if(chlanie_stan >= 3)
	{
		WriteFile(file, &chlanie_stan2, sizeof(chlanie_stan2), &tmp, NULL);
		WriteFile(file, &chlanie_czas, sizeof(chlanie_czas), &tmp, NULL);
		int ile = chlanie_ludzie.size();
		WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
			WriteFile(file, &(*it)->refid, sizeof((*it)->refid), &tmp, NULL);
	}

	// zawody na arenie
	WriteFile(file, &zawody_rok, sizeof(zawody_rok), &tmp, NULL);
	WriteFile(file, &zawody_miasto, sizeof(zawody_miasto), &tmp, NULL);
	WriteFile(file, &zawody_rok_miasta, sizeof(zawody_rok_miasta), &tmp, NULL);
	refid = (zawody_zwyciezca ? zawody_zwyciezca->refid : -1);
	WriteFile(file, &refid, sizeof(refid), &tmp, NULL);
	WriteFile(file, &zawody_wygenerowano, sizeof(zawody_wygenerowano), &tmp, NULL);
}

//=================================================================================================
void Game::LoadGame(HANDLE file)
{
	File f(file);

	ClearGame();
	ClearGameVarsOnLoad();
	StopSounds();
	attached_sounds.clear();
	in_tutorial = false;
	arena_free = true;
	ai_bow_targets.clear();
	ai_cast_targets.clear();
	load_location_quest.clear();
	load_unit_handler.clear();
	load_chest_handler.clear();
	load_unit_refid.clear();

	// sygnatura
	char sign[4] = {'C','R','S','V'};
	char sign2[4];
	ReadFile(file, sign2, 4, &tmp, NULL);
	for(int i=0; i<4; ++i)
	{
		if(sign2[i] != sign[i])
			throw txLoadSignature;
	}

	// wersja gry
	int w;
	ReadFile(file, &w, sizeof(w), &tmp, NULL);
	if(w > VERSION)
		throw Format(txLoadVersion, VersionToString(w), VERSION_STR);

	// wersja zapisu
	ReadFile(file, &LOAD_VERSION, sizeof(LOAD_VERSION), &tmp, NULL);
	if(LOAD_VERSION < SUPPORT_LOAD_VERSION.x)
		throw Format(txLoadSaveVersionOld, LOAD_VERSION);
	if(LOAD_VERSION > SUPPORT_LOAD_VERSION.y)
		throw Format(txLoadSaveVersionNew, LOAD_VERSION);
	if(LOAD_VERSION >= V_0_2_20 && LOAD_VERSION < V_DEVEL)
	{
		// build - unused
		uint build;
		ReadFile(file, &build, sizeof(build), &tmp, NULL);
	}

	// czy online / dev
	byte flags;
	ReadFile(file, &flags, sizeof(flags), &tmp, NULL);
	bool online_save = IS_SET(flags, SF_ONLINE);
	if(mp_load)
	{
		if(!online_save)
			throw txLoadMP;
	}
	else
	{
		if(online_save)
			throw txLoadSP;
	}

	LOG(Format("Loading save. Version %s, format %d, mp %d, debug %d.", VersionToString(w), LOAD_VERSION, online_save ? 1 : 0, IS_SET(flags, SF_DEBUG) ? 1 : 0));

	if(LOAD_VERSION == V_0_2)
	{
		hardcore_mode = false;
		total_kills = 0;
	}
	else
	{
		ReadFile(file, &hardcore_mode, sizeof(hardcore_mode), &tmp, NULL);
		ReadFile(file, &total_kills, sizeof(total_kills), &tmp, NULL);
	}

	// world state
	GAME_STATE game_state2;
	ReadFile(file, &year, sizeof(year), &tmp, NULL);
	ReadFile(file, &month, sizeof(month), &tmp, NULL);
	ReadFile(file, &day, sizeof(day), &tmp, NULL);
	ReadFile(file, &worldtime, sizeof(worldtime), &tmp, NULL);
	ReadFile(file, &game_state2, sizeof(game_state2), &tmp, NULL);
	ReadFile(file, &gt_hour, sizeof(gt_hour), &tmp, NULL);
	ReadFile(file, &gt_minute, sizeof(gt_minute), &tmp, NULL);
	ReadFile(file, &gt_second, sizeof(gt_second), &tmp, NULL);
	ReadFile(file, &gt_tick, sizeof(gt_tick), &tmp, NULL);

	Unit::refid_table.clear();
	Useable::refid_table.clear();
	ParticleEmitter::refid_table.clear();
	TrailParticleEmitter::refid_table.clear();

	byte check_id = 0, read_id;

	// world map
	LoadingStep(txLoadingLocations);
	ReadFile(file, &world_state, sizeof(world_state), &tmp, NULL);
	ReadFile(file, &current_location, sizeof(current_location), &tmp, NULL);
	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	locations.resize(ile);
	int index = 0;
	int step = 0;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		LOCATION_TOKEN loc_token;

		ReadFile(file, &loc_token, sizeof(loc_token), &tmp, NULL);

		if(loc_token != LT_NULL)
		{
			switch(loc_token)
			{
			case LT_OUTSIDE:
				*it = new OutsideLocation;
				break;
			case LT_CITY:
				*it = new City;
				break;
			case LT_VILLAGE:
				*it = new Village;
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
					ReadFile(file, &levels, sizeof(levels), &tmp, NULL);
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

			(*it)->Load(file, (game_state2 == GS_LEVEL && current_location == index));

			// aktualizuj nadrzwi w krypcie
			if(LOAD_VERSION < V_0_2_10 && (*it)->type == L_CRYPT)
			{
				InsideLocation* inside = ((InsideLocation*)(*it));
				InsideLocationLevel* lvl = inside->GetLastLevelData();
				if(lvl && !lvl->rooms.empty() && lvl->rooms[0].target == POKOJ_CEL_SKARBIEC)
				{
					for(vector<Object>::iterator obj_it = lvl->objects.begin(), obj_end = lvl->objects.end(); obj_it != obj_end; ++obj_it)
					{
						if(obj_it->mesh == aNaDrzwi)
						{
							INT2 pt = pos_to_pt(obj_it->pos);
							if(IS_SET(lvl->map[pt.x+pt.y*lvl->w].flags, Pole::F_DRUGA_TEKSTURA))
								obj_it->mesh = aNaDrzwi2;
						}
					}
				}
			}
		}
		else
			*it = NULL;

		if(step == 0)
		{
			if(index >= int(ile)/4)
			{
				++step;
				LoadingStep(txLoadingLocations);
			}
		}
		else if(step == 1)
		{
			if(index >= int(ile)/2)
			{
				++step;
				LoadingStep(txLoadingLocations);
			}
		}
		else if(step == 2)
		{
			if(index >= int(ile)*3/4)
			{
				++step;
				LoadingStep(txLoadingLocations);
			}
		}

		if(LOAD_VERSION != V_0_2)
		{
			ReadFile(file, &read_id, sizeof(read_id), &tmp, NULL);
			if(read_id != check_id)
				throw Format("Error while reading location %s (%d).", *it ? (*it)->name.c_str() : "NULL", index);
			++check_id;
		}
	}
	ReadFile(file, &empty_locations, sizeof(empty_locations), &tmp, NULL);
	ReadFile(file, &create_camp, sizeof(create_camp), &tmp, NULL);
	ReadFile(file, &world_pos, sizeof(world_pos), &tmp, NULL);
	ReadFile(file, &travel_time2, sizeof(travel_time2), &tmp, NULL);
	ReadFile(file, &szansa_na_spotkanie, sizeof(szansa_na_spotkanie), &tmp, NULL);
	ReadFile(file, &cities, sizeof(cities), &tmp, NULL);
	ReadFile(file, &encounter_loc, sizeof(encounter_loc), &tmp, NULL);
	if(LOAD_VERSION != V_0_2)
		ReadFile(file, &world_dir, sizeof(world_dir), &tmp, NULL);
	if(world_state == WS_TRAVEL)
	{
		ReadFile(file, &picked_location, sizeof(picked_location), &tmp, NULL);
		ReadFile(file, &travel_day, sizeof(travel_day), &tmp, NULL);
		ReadFile(file, &travel_start, sizeof(travel_start), &tmp, NULL);
		ReadFile(file, &travel_time, sizeof(travel_time), &tmp, NULL);
		if(LOAD_VERSION == V_0_2)
			ReadFile(file, &world_dir, sizeof(world_dir), &tmp, NULL);
		ReadFile(file, &guards_enc_reward, sizeof(guards_enc_reward), &tmp, NULL);
	}
	else if(world_state == WS_ENCOUNTER)
	{
		world_state = WS_TRAVEL;
		travel_start = world_pos = locations[0]->pos;
		picked_location = 0;
		travel_time = 1.f;
		travel_day = 0;
	}
	if(LOAD_VERSION == V_0_2 && world_state != WS_TRAVEL)
		world_dir = random(MAX_ANGLE);
	else if(LOAD_VERSION < V_0_3)
		world_dir = clip(-world_dir);
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	encs.resize(ile, NULL);
	int location_event_handler_quest_refid;
	if(game_state2 == GS_LEVEL)
		ReadFile(file, &location_event_handler_quest_refid, sizeof(location_event_handler_quest_refid), &tmp, NULL);
	else
	{
		location_event_handler_quest_refid = -1;
		// wczytaj dru¿ynê
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		for(uint i=0; i<ile; ++i)
		{
			Unit* u = new Unit;
			u->Load(file, false);
			Unit::AddRefid(u);

			if(IS_SET(u->data->flags, F_HUMAN))
			{
				u->ani = new AnimeshInstance(aHumanBase);
				u->human_data->ApplyScale(aHumanBase);
			}
			else
				u->ani = new AnimeshInstance(u->data->ani);
			u->ani->ptr = u;

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
		if(location->type == L_CITY || location->type == L_VILLAGE)
			city_ctx = (City*)location;
		else
			city_ctx = NULL;
	}
	else
	{
		location = NULL;
		city_ctx = NULL;
	}
	ReadFile(file, &first_city, sizeof(first_city), &tmp, NULL);
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	boss_levels.resize(ile);
	if(ile)
		ReadFile(file, &boss_levels[0], sizeof(INT2)*ile, &tmp, NULL);
	if(LOAD_VERSION >= V_0_2_10)
		ReadFile(file, &enter_from, sizeof(enter_from), &tmp, NULL);
	else
		enter_from = ENTER_FROM_UNKNOWN;
	if(LOAD_VERSION >= V_0_3)
		ReadFile(file, &light_angle, sizeof(light_angle), &tmp, NULL);
	else
		light_angle = random(PI*2);

	// ustaw wskaŸniki postaci/u¿ywalnych
	LoadingStep(txLoadingData);
	for(vector<std::pair<Unit**, int> >::iterator it = Unit::refid_request.begin(), end = Unit::refid_request.end(); it != end; ++it)
		*(it->first) = Unit::refid_table[it->second];
	Unit::refid_request.clear();
	for(vector<UseableRequest>::iterator it = Useable::refid_request.begin(), end = Useable::refid_request.end(); it != end; ++it)
	{
		Useable* u = Useable::refid_table[it->refid];
		if(u->user != it->user)
		{
			WARN(Format("Invalid useable %s (%d) user %s.", u->GetBase()->id, u->refid, it->user->data->id.c_str()));
			*it->useable = NULL;
		}
		else
			*it->useable = u;
	}
	Useable::refid_request.clear();

	// camera
	ReadFile(file, &cam.real_rot.y, sizeof(cam.real_rot.y), &tmp, NULL);
	ReadFile(file, &cam.dist, sizeof(cam.dist), &tmp, NULL);
	cam.Reset();
	player_rot_buf = 0.f;

	// ekwipunek sprzedawców w mieœcie
	LoadStock(file, chest_merchant);
	LoadStock(file, chest_blacksmith);
	LoadStock(file, chest_alchemist);
	LoadStock(file, chest_innkeeper);
	if(LOAD_VERSION >= V_0_2_20)
		LoadStock(file, chest_food_seller);
	else
		chest_food_seller.clear();

	// zmienne
	ReadFile(file, &used_cheats, sizeof(used_cheats), &tmp, NULL);
	ReadFile(file, &cheats, sizeof(cheats), &tmp, NULL);
	if(LOAD_VERSION < V_0_2_10)
	{
		bool show_fps;
		ReadFile(file, &show_fps, sizeof(show_fps), &tmp, NULL);
	}
	ReadFile(file, &nosound, sizeof(nosound), &tmp, NULL);
	ReadFile(file, &noai, sizeof(noai), &tmp, NULL);
	ReadFile(file, &dont_wander, sizeof(dont_wander), &tmp, NULL);
	int refid;
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	pc = Unit::GetByRefid(refid)->player;
	cam.real_rot.x = pc->unit->rot;
	pc->dialog_ctx = &dialog_context;
	dialog_context.dialog_mode = false;
	dialog_context.next_talker = NULL;
	dialog_context.is_local = true;
	ReadFile(file, &dungeon_level, sizeof(dungeon_level), &tmp, NULL);
	if(LOAD_VERSION >= V_0_2_20)
	{
		ReadFile(file, &portal_anim, sizeof(portal_anim), &tmp, NULL);
		ReadFile(file, &drunk_anim, sizeof(drunk_anim), &tmp, NULL);
	}
	else
		drunk_anim = 0.f;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	ais.resize(ile);
	for(vector<AIController*>::iterator it = ais.begin(), end = ais.end(); it != end; ++it)
	{
		*it = new AIController;
		(*it)->Load(file);
	}

	// game messages & speech bubbles
	game_gui->game_messages->Load(f);
	game_gui->Load(f);	

	// wczytaj plotki / notatki
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	plotki.resize(ile);
	for(vector<string>::iterator it = plotki.begin(), end = plotki.end(); it != end; ++it)
	{
		word len;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		it->resize(len);
		ReadFile(file, (void*)it->c_str(), len, &tmp, NULL);
	}
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	notes.resize(ile);
	for(vector<string>::iterator it = notes.begin(), end = notes.end(); it != end; ++it)
	{
		word len;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		it->resize(len);
		ReadFile(file, (void*)it->c_str(), len, &tmp, NULL);
	}

	// arena
	arena_tryb = Arena_Brak;

	if(LOAD_VERSION != V_0_2)
	{
		ReadFile(file, &read_id, sizeof(read_id), &tmp, NULL);
		if(read_id != check_id)
			throw "Error reading data before team.";
		++check_id;
	}

	// wczytaj dru¿ynê
	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	team.resize(count);
	for(vector<Unit*>::iterator it = team.begin(), end = team.end(); it != end; ++it)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		*it = Unit::GetByRefid(refid);
	}
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	active_team.resize(count);
	for(vector<Unit*>::iterator it = active_team.begin(), end = active_team.end(); it != end; ++it)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		*it = Unit::GetByRefid(refid);
	}
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	leader = Unit::GetByRefid(refid);
	ReadFile(file, &team_gold, sizeof(team_gold), &tmp, NULL);
	ReadFile(file, &atak_szalencow, sizeof(atak_szalencow), &tmp, NULL);
	ReadFile(file, &bandyta, sizeof(bandyta), &tmp, NULL);
	ReadFile(file, &free_recruit, sizeof(free_recruit), &tmp, NULL);
	CheckCredit(false, true);

	// wczytaj questy
	LoadingStep(txLoadingQuests);
	LoadQuests(quests, file);
	LoadQuests(unaccepted_quests, file);
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	quests_timeout.resize(count);
	for(vector<Quest_Dungeon*>::iterator it = quests_timeout.begin(), end = quests_timeout.end(); it != end; ++it)
	{
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		*it = (Quest_Dungeon*)FindQuest(refid);
	}
	if(LOAD_VERSION == V_0_2)
		timed_units.clear();
	else
	{
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		timed_units.resize(count);
		for(vector<TimedUnit>::iterator it = timed_units.begin(), end = timed_units.end(); it != end; ++it)
		{
			TimedUnit& tu = *it;
			ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
			tu.unit = Unit::GetByRefid(refid);
			ReadFile(file, &tu.location, sizeof(tu.location), &tmp, NULL);
			ReadFile(file, &tu.days, sizeof(tu.days), &tmp, NULL);
		}
	}
	ReadFile(file, &quest_counter, sizeof(quest_counter), &tmp, NULL);
	ReadFile(file, &unique_quests_completed, sizeof(unique_quests_completed), &tmp, NULL);
	ReadFile(file, &unique_completed_show, sizeof(unique_completed_show), &tmp, NULL);
	if(LOAD_VERSION == V_0_2)
		unique_completed_show = false;

	quest_sawmill = (Quest_Sawmill*)FindQuestById(Q_SAWMILL);
	quest_mine = (Quest_Mine*)FindQuestById(Q_MINE);
	quest_bandits = (Quest_Bandits*)FindQuestById(Q_BANDITS);
	quest_goblins = (Quest_Goblins*)FindQuestById(Q_GOBLINS);
	quest_mages = (Quest_Mages*)FindQuestById(Q_MAGES);
	quest_mages2 = (Quest_Mages2*)FindQuestById(Q_MAGES2);
	quest_orcs = (Quest_Orcs*)FindQuestById(Q_ORCS);
	quest_orcs2 = (Quest_Orcs2*)FindQuestById(Q_ORCS2);
	quest_evil = (Quest_Evil*)FindQuestById(Q_EVIL);
	quest_crazies = (Quest_Crazies*)FindQuestById(Q_CRAZIES);

	if(!quest_mages2)
	{
		quest_mages2 = new Quest_Mages2;
		quest_mages2->refid = quest_counter++;
		quest_mages2->Start();
		unaccepted_quests.push_back(quest_mages2);
	}

	for(vector<QuestItemRequest*>::iterator it = quest_item_requests.begin(), end = quest_item_requests.end(); it != end; ++it)
	{
		QuestItemRequest* qir = *it;
		*qir->item = FindQuestItem(qir->name.c_str(), qir->quest_refid);
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
			{
				if(LOAD_VERSION < V_0_2_10)
					RemoveNullItems(*qir->items);
				SortItems(*qir->items);
				if(qir->unit && LOAD_VERSION < V_0_2_10)
					qir->unit->RecalculateWeight();
			}
		}
		delete *it;
	}
	quest_item_requests.clear();
	LoadQuestsData(file);

	// newsy
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	news.resize(count);
	for(vector<News*>::iterator it = news.begin(), end = news.end(); it != end; ++it)
	{
		*it = new News;
		ReadFile(file, &(*it)->add_time, sizeof((*it)->add_time), &tmp, NULL);
		ReadString2(file, (*it)->text);
	}

	if(LOAD_VERSION != V_0_2)
	{
		ReadFile(file, &read_id, sizeof(read_id), &tmp, NULL);
		if(read_id != check_id)
			throw "Error reading data after news.";
		++check_id;
	}

	LoadingStep(txEndOfLoading);

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

		// cz¹steczki
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		local_ctx.pes->resize(count);
		for(vector<ParticleEmitter*>::iterator it = local_ctx.pes->begin(), end = local_ctx.pes->end(); it != end; ++it)
		{
			*it = new ParticleEmitter;
			ParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		local_ctx.tpes->resize(count);
		for(vector<TrailParticleEmitter*>::iterator it = local_ctx.tpes->begin(), end = local_ctx.tpes->end(); it != end; ++it)
		{
			*it = new TrailParticleEmitter;
			TrailParticleEmitter::AddRefid(*it);
			(*it)->Load(file);
		}

		// wybuchy
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		local_ctx.explos->resize(count);
		for(vector<Explo*>::iterator it = local_ctx.explos->begin(), end = local_ctx.explos->end(); it != end; ++it)
		{
			*it = new Explo;
			(*it)->Load(file);
		}

		// elektrycznoœæ
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		local_ctx.electros->resize(count);
		for(vector<Electro*>::iterator it = local_ctx.electros->begin(), end = local_ctx.electros->end(); it != end; ++it)
		{
			*it = new Electro;
			(*it)->Load(file);
		}

		// wyssania ¿ycia
		ReadFile(file, &count, sizeof(count), &tmp, NULL);
		local_ctx.drains->resize(count);
		for(vector<Drain>::iterator it = local_ctx.drains->begin(), end = local_ctx.drains->end(); it != end; ++it)
			it->Load(file);

		// pociski
		File f(file);
		f >> count;
		local_ctx.bullets->resize(count);
		for(vector<Bullet>::iterator it = local_ctx.bullets->begin(), end = local_ctx.bullets->end(); it != end; ++it)
			it->Load(f);

		if(LOAD_VERSION != V_0_2)
		{
			ReadFile(file, &read_id, sizeof(read_id), &tmp, NULL);
			if(read_id != check_id)
				throw "Failed to read level data.";
			++check_id;
		}

		local_ctx_valid = true;
		RemoveUnusedAiAndCheck();
	}
	else
	{
		open_location = -1;
		local_ctx_valid = false;
	}

	// gui
	if(LOAD_VERSION >= V_0_2_10 && LOAD_VERSION <= V_0_3)
	{
		File f(file);
		LoadGui(f);
	}
	game_gui->PositionPanels();

	// cele ai
	if(!ai_bow_targets.empty())
	{
		Obj* tarcza_s = FindObject("bow_target");
		for(vector<AIController*>::iterator it = ai_bow_targets.begin(), end = ai_bow_targets.end(); it != end; ++it)
		{
			AIController& ai = **it;
			LevelContext& ctx = Game::Get().GetContext(*ai.unit);
			Object* ptr = NULL;
			float dist, best_dist;
			for(vector<Object>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
			{
				if(it->base == tarcza_s)
				{
					dist = distance(it->pos, ai.idle_data.pos);
					if(!ptr || dist < best_dist)
					{
						ptr = &*it;
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
		refid = (int)ai.cast_target;
		if(refid == -1)
		{
			// zapis z wersji < 2, szukaj w pobli¿u punktu
			LevelContext& ctx = GetContext(*ai.unit);
			Spell* spell = ai.unit->data->spells->spell[ai.unit->attack_id];
			float dist2, best_dist2 = spell->range;
			ai.cast_target = NULL;

			if(IS_SET(spell->flags, Spell::Raise))
			{
				// czar o¿ywiania
				for(vector<Unit*>::iterator it2 = ctx.units->begin(), end2 = ctx.units->end(); it2 != end2; ++it2)
				{
					if(!(*it2)->to_remove && (*it2)->live_state == Unit::DEAD && !IsEnemy(*ai.unit, **it2) && IS_SET((*it2)->data->flags, F_UNDEAD) &&
						(dist2 = distance(ai.target_last_pos, (*it2)->pos)) < best_dist2 && CanSee(*ai.unit, **it2))
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
						(dist2 = distance(ai.target_last_pos, (*it2)->pos)) < best_dist2 && CanSee(*ai.unit, **it2))
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
				ai.timer = random(1.f,2.f);
			}
		}
		else
			ai.cast_target = Unit::GetByRefid(refid);
	}

	// questy zwi¹zane z lokacjami
	for(vector<Location*>::iterator it = load_location_quest.begin(), end = load_location_quest.end(); it != end; ++it)
	{
		(*it)->active_quest = (Quest_Dungeon*)FindQuest((int)(*it)->active_quest, false);
		assert((*it)->active_quest);
	}
	// unit event handler
	for(vector<Unit*>::iterator it = load_unit_handler.begin(), end = load_unit_handler.end(); it != end; ++it)
	{
		// pierwszy raz musia³em u¿yæ tego rzutowania ¿eby dzia³a³o :o
		(*it)->event_handler = dynamic_cast<UnitEventHandler*>(FindQuest((int)(*it)->event_handler, false));
		assert((*it)->event_handler);
	}
	// chest event handler
	for(vector<Chest*>::iterator it = load_chest_handler.begin(), end = load_chest_handler.end(); it != end; ++it)
	{
		(*it)->handler = dynamic_cast<ChestEventHandler*>(FindQuest((int)(*it)->handler, false));
		assert((*it)->handler);
	}

	// wskaŸniki na jednostki
	for(vector<Unit**>::iterator it = load_unit_refid.begin(), end = load_unit_refid.end(); it != end; ++it)
	{
		Unit** up = *it;
		*up = Unit::GetByRefid(int(*up));
		assert(*up);
	}

	if(zawody_wygenerowano)
		zawody_mistrz = FindUnitByIdLocal("arena_master");
	else
		zawody_mistrz = NULL;

	minimap_reveal.clear();
	dialog_context.dialog_mode = false;
	if(location_event_handler_quest_refid != -1)
		location_event_handler = dynamic_cast<LocationEventHandler*>(FindQuest(location_event_handler_quest_refid));
	else
		location_event_handler = NULL;
	team_shares.clear();
	team_share_id = -1;
	fallback_co = FALLBACK_NONE;
	fallback_t = 0.f;
	inventory_mode = I_NONE;
	before_player = BP_NONE;
	selected_unit = NULL;
	selected_target = NULL;
	dialog_context.pc = pc;
	dialog_context.dialog_mode = false;
	dialog_context.next_talker = NULL;

	if(mp_load)
	{
		ReadString1(file, server_name);
		ReadString1(file, server_pswd);
		ReadFile(file, &players, sizeof(players), &tmp, NULL);
		ReadFile(file, &max_players, sizeof(max_players), &tmp, NULL);
		ReadFile(file, &last_id, sizeof(last_id), &tmp, NULL);
		uint ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		old_players.resize(ile);
		for(uint i = 0; i<ile; ++i)
			old_players[i].Load(file);
		ReadFile(file, &kick_id, sizeof(kick_id), &tmp, NULL);
		ReadFile(file, &netid_counter, sizeof(netid_counter), &tmp, NULL);
		ReadFile(file, &item_netid_counter, sizeof(item_netid_counter), &tmp, NULL);
		ReadFile(file, &chest_netid_counter, sizeof(chest_netid_counter), &tmp, NULL);
		ReadFile(file, &useable_netid_counter, sizeof(useable_netid_counter), &tmp, NULL);
		ReadFile(file, &skip_id_counter, sizeof(skip_id_counter), &tmp, NULL);
		ReadFile(file, &trap_netid_counter, sizeof(trap_netid_counter), &tmp, NULL);
		ReadFile(file, &door_netid_counter, sizeof(door_netid_counter), &tmp, NULL);
		ReadFile(file, &electro_netid_counter, sizeof(electro_netid_counter), &tmp, NULL);

		if(LOAD_VERSION != V_0_2)
		{
			ReadFile(file, &mp_use_interp, sizeof(mp_use_interp), &tmp, NULL);
			ReadFile(file, &mp_interp, sizeof(mp_interp), &tmp, NULL);

			ReadFile(file, &read_id, sizeof(read_id), &tmp, NULL);
			if(read_id != check_id)
				throw "Failed to read multiplayer data.";
			++check_id;
		}
		else
		{
			mp_use_interp = true;
			mp_interp = 0.05f;
		}
	}
	else
		pc->is_local = true;

	if(enter_from == ENTER_FROM_UNKNOWN && game_state2 == GS_LEVEL)
	{
		// zgadnij sk¹d przysz³a dru¿yna
		if(current_location == sekret_gdzie2)
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

	LoadingStep(txEndOfLoading);
	load_screen->visible = false;
	//minimap->R

	LOG("Game loaded.");

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
		SetMusic(MUSIC_TRAVEL);
	}
	game_state = game_state2;
	clear_color = clear_color2;
}

//=================================================================================================
void Game::LoadStock(HANDLE file, vector<ItemSlot>& cnt)
{
	bool can_sort = true;

	uint count;
	ReadFile(file, &count, sizeof(count), &tmp, NULL);
	cnt.resize(count);
	for(vector<ItemSlot>::iterator it = cnt.begin(), end = cnt.end(); it != end; ++it)
	{
		byte len;
		ReadFile(file, &len, sizeof(len), &tmp, NULL);
		if(len)
		{
			BUF[len] = 0;
			ReadFile(file, BUF, len, &tmp, NULL);
			ReadFile(file, &it->count, sizeof(it->count), &tmp, NULL);
			if(BUF[0] != '$')
				it->item = FindItem(BUF);
			else
			{
				int quest_refid;
				ReadFile(file, &quest_refid, sizeof(quest_refid), &tmp, NULL);
				AddQuestItemRequest(&it->item, BUF, quest_refid, &cnt);
				it->item = QUEST_ITEM_PLACEHOLDER;
				can_sort = false;
			}
		}
		else
		{
			assert(LOAD_VERSION < V_0_2_10);
			it->item = NULL;
			it->count = 0;
		}
	}
	
	if(can_sort && LOAD_VERSION < V_0_2_20 && !cnt.empty())
	{
		if(LOAD_VERSION < V_0_2_10)
			RemoveNullItems(cnt);
		SortItems(cnt);
	}
}

//=================================================================================================
void Game::LoadQuestsData(HANDLE file)
{
	int refid;

	// jednorazowe ploki
	ReadFile(file, &ile_plotek_questowych, sizeof(ile_plotek_questowych), &tmp, NULL);
	ReadFile(file, plotka_questowa, sizeof(plotka_questowa), &tmp, NULL);

	// load quests old data (now are stored inside quest)
	if(LOAD_VERSION < V_DEVEL)
	{
		quest_sawmill->LoadOld(file);
		quest_mine->LoadOld(file);
		quest_bandits->LoadOld(file);
		quest_mages2->LoadOld(file);
		quest_orcs2->LoadOld(file);
		quest_goblins->LoadOld(file);
		quest_evil->LoadOld(file);
	}

	// crazies
	if(LOAD_VERSION == V_0_2)
	{
		quest_crazies = new Quest_Crazies;
		quest_crazies->refid = quest_counter++;
		quest_crazies->Start();
		unaccepted_quests.push_back(quest_crazies);
	}
	else if(LOAD_VERSION < V_DEVEL)
		quest_crazies->LoadOld(file);

	// sekret
	if(LOAD_VERSION == V_0_2)
	{
		sekret_stan = (FindObject("tomashu_dom")->ani ? SS2_BRAK : SS2_WYLACZONY);
		GetSecretNote()->desc.clear();
		sekret_gdzie = -1;
		sekret_gdzie2 = -1;
	}
	else
	{
		ReadFile(file, &sekret_stan, sizeof(sekret_stan), &tmp, NULL);
		ReadString1(file, GetSecretNote()->desc);
		ReadFile(file, &sekret_gdzie, sizeof(sekret_gdzie), &tmp, NULL);
		ReadFile(file, &sekret_gdzie2, sizeof(sekret_gdzie2), &tmp, NULL);

		if(sekret_stan > SS2_BRAK && !FindObject("tomashu_dom")->ani)
			throw "Save uses 'data.pak' file which is missing!";
	}

	// zawody w piciu
	ReadFile(file, &chlanie_gdzie, sizeof(chlanie_gdzie), &tmp, NULL);
	ReadFile(file, &chlanie_stan, sizeof(chlanie_stan), &tmp, NULL);
	ReadFile(file, &chlanie_wygenerowano, sizeof(chlanie_wygenerowano), &tmp, NULL);
	ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
	chlanie_zwyciezca = Unit::GetByRefid(refid);
	if(chlanie_stan >= 3)
	{
		ReadFile(file, &chlanie_stan2, sizeof(chlanie_stan2), &tmp, NULL);
		ReadFile(file, &chlanie_czas, sizeof(chlanie_czas), &tmp, NULL);
		int ile;
		ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
		chlanie_ludzie.resize(ile);
		for(vector<Unit*>::iterator it = chlanie_ludzie.begin(), end = chlanie_ludzie.end(); it != end; ++it)
		{
			ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
			*it = Unit::GetByRefid(refid);
		}
	}

	// zawody na arenie
	if(LOAD_VERSION == V_0_2)
	{
		zawody_rok = 0;
		zawody_rok_miasta = year;
		zawody_miasto = GetRandomCity();
		zawody_zwyciezca = NULL;
		zawody_wygenerowano = false;
	}
	else
	{
		ReadFile(file, &zawody_rok, sizeof(zawody_rok), &tmp, NULL);
		ReadFile(file, &zawody_miasto, sizeof(zawody_miasto), &tmp, NULL);
		ReadFile(file, &zawody_rok_miasta, sizeof(zawody_rok_miasta), &tmp, NULL);
		ReadFile(file, &refid, sizeof(refid), &tmp, NULL);
		zawody_zwyciezca = Unit::GetByRefid(refid);
		ReadFile(file, &zawody_wygenerowano, sizeof(zawody_wygenerowano), &tmp, NULL);
	}
	zawody_stan = IS_BRAK;
	zawody_ludzie.clear();
}

//=================================================================================================
void Game::Quicksave(bool from_console)
{
	if(!CanSaveGame())
	{
		if(from_console)
			AddConsoleMsg(txCantSaveNow);
		else
			GUI.SimpleDialog(txCantSaveNow, NULL);
		return;
	}

	if(SaveGameSlot(MAX_SAVE_SLOTS, txQuickSave))
	{
		if(from_console)
			AddConsoleMsg(txGameSaved);
		else
			AddGameMsg2(txGameSaved, 1.f, GMS_GAME_SAVED);
	}
}

//=================================================================================================
void Game::Quickload(bool from_console)
{
	if(!CanLoadGame())
	{
		if(from_console)
			AddConsoleMsg(txCantLoadGame);
		else
			GUI.SimpleDialog(txCantLoadGame, NULL);
		return;
	}

	try
	{
		sv_server = false;
		sv_online = false;
		LoadGameSlot(MAX_SAVE_SLOTS);
	}
	catch(cstring err)
	{
		err = Format("%s%s", txLoadError, err);
		ERROR(err);
		GUI.SimpleDialog(err, NULL);
	}
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
			*it = NULL;
			deleted = true;
		}
	}

	if(deleted)
	{
		RemoveNullElements(ais);
		cstring s = Format("Removed unused ais: %u.", prev_size - ais.size());
		WARN(s);
#ifdef IS_DEV
		AddGameMsg(s, 10.f);
#endif
	}

#ifdef IS_DEV
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
			ERROR(Format("Unit %s is player 0x%p and ai 0x%p.", u.data->id.c_str(), u.player, u.ai));
		}
		else if(u.player && u.hero)
		{
			++err_count;
			ERROR(Format("Unit %s is player 0x%p and hero 0x%p.", u.data->id.c_str(), u.player, u.hero));
		}
		else if(!u.player && !u.ai)
		{
			++err_count;
			ERROR(Format("Unit %s is neither player or ai.", u.data->id.c_str()));
		}
	}
}
