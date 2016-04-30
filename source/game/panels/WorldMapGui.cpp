#include "Pch.h"
#include "Base.h"
#include "WorldMapGui.h"
#include "Game.h"
#include "Language.h"
#include "Journal.h"
#include "Version.h"
#include "Quest_Mages.h"
#include "Quest_Crazies.h"

//-----------------------------------------------------------------------------
extern const float TRAVEL_SPEED;

//=================================================================================================
WorldMapGui::WorldMapGui() : game(Game::Get())
{
	focusable = true;
	visible = false;

	txGameTimeout = Str("gameTimeout");
	txWorldData = Str("worldData");
	txCurrentLoc = Str("currentLoc");
	txCitizens = Str("citizens");
	txAvailable = Str("available");
	txTarget = Str("target");
	txDistance = Str("distance");
	txTravelTime = Str("travelTime");
	txDay = Str("day");
	txDays = Str("days");
	txOnlyLeaderCanTravel = Str("onlyLeaderCanTravel");
	txEncCrazyMage = Str("encCrazyMage");
	txEncCrazyHeroes = Str("encCrazyHeroes");
	txEncMerchant = Str("encMerchant");
	txEncHeroes  = Str("encHeroes");
	txEncBanditsAttackTravelers = Str("encBanditsAttackTravelers");
	txEncHeroesAttack = Str("encHeroesAttack");
	txEncGolem = Str("encGolem");
	txEncCrazy = Str("encCrazy");
	txEncUnk = Str("encUnk");
	txEncBandits = Str("encBandits");
	txEncAnimals = Str("encAnimals");
	txEncOrcs = Str("encOrcs");
	txEncGoblins = Str("encGoblins");

	mp_box = game.game_gui->mp_box;
	journal = game.game_gui->journal;
	game_messages = game.game_gui->game_messages;
}

//=================================================================================================
void WorldMapGui::LoadData()
{
	ResourceManager& resMgr = ResourceManager::Get();
	resMgr.GetLoadedTexture("camp.png", tMapIcon[L_CAMP]);
	resMgr.GetLoadedTexture("village.png", tMapIcon[L_VILLAGE]);
	resMgr.GetLoadedTexture("city.png", tMapIcon[L_CITY]);
	resMgr.GetLoadedTexture("dungeon.png", tMapIcon[L_DUNGEON]);
	resMgr.GetLoadedTexture("crypt.png", tMapIcon[L_CRYPT]);
	resMgr.GetLoadedTexture("cave.png", tMapIcon[L_CAVE]);
	resMgr.GetLoadedTexture("forest.png", tMapIcon[L_FOREST]);
	resMgr.GetLoadedTexture("forest.png", tMapIcon[L_ENCOUNTER]);
	resMgr.GetLoadedTexture("moonwell.png", tMapIcon[L_MOONWELL]);
	resMgr.GetLoadedTexture("academy.png", tMapIcon[L_ACADEMY]);
	resMgr.GetLoadedTexture("worldmap.jpg", tWorldMap);
	resMgr.GetLoadedTexture("selected.png", tSelected[0]);
	resMgr.GetLoadedTexture("selected2.png", tSelected[1]);
	resMgr.GetLoadedTexture("mover.png", tMover);
	resMgr.GetLoadedTexture("old_map.png", tMapBg);
	resMgr.GetLoadedTexture("enc.png", tEnc);
}

//=================================================================================================
void WorldMapGui::Draw(ControlDrawData*)
{
	// t³o
	RECT rect0 = {0,0,game.wnd_size.x,game.wnd_size.y};
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	GUI.DrawSpriteRectPart(tMapBg, rect0, rect0);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	// mapa
	MATRIX mat;
	D3DXMatrixTransformation2D(&mat, &VEC2(0,0), 0.f, &VEC2(600.f/512.f,600.f/512.f), nullptr, 0.f, nullptr);
	GUI.DrawSpriteTransform(tWorldMap, mat);

	// obrazki lokacji
	for(vector<Location*>::iterator it = game.locations.begin(), end = game.locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;
		Location& loc = **it;
		if(loc.state == LS_UNKNOWN || loc.state == LS_HIDDEN)
			continue;
		GUI.DrawSprite(tMapIcon[loc.type], WorldPosToScreen(INT2(loc.pos.x-16.f,loc.pos.y+16.f)), loc.state == LS_KNOWN ? 0x70707070 : WHITE);
	}

	// lokacje spotkañ
	if(game.devmode)
	{
		for(vector<Encounter*>::iterator it = game.encs.begin(), end = game.encs.end(); it != end; ++it)
		{
			if(*it)
				GUI.DrawSprite(tEnc, WorldPosToScreen(INT2((*it)->pos.x - 16.f, (*it)->pos.y + 16.f)));
		}
	}

	LocalString s = Format(txWorldData, game.year, game.month+1, game.day+1);

	// opis aktualnej lokacji
	if(game.current_location != -1)
	{
		Location& current = *game.locations[game.current_location];
		GUI.DrawSprite(tSelected[1], WorldPosToScreen(INT2(current.pos.x-32.f,current.pos.y+32.f)), 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		if(game.devmode && game.IsLocal())
		{
			if(current.type == L_DUNGEON || current.type == L_CRYPT)
			{
				InsideLocation* inside = (InsideLocation*)&current;
				s += Format(" (%s, %s, st %d)", g_base_locations[inside->target].name, g_spawn_groups[inside->spawn].id, inside->st);
			}
			else if(current.type == L_FOREST || current.type == L_CAMP || current.type == L_CAVE || current.type == L_MOONWELL)
				s += Format(" (st %d)", current.st);
			s += Format(", quest 0x%p", current.active_quest);
		}
		if(current.state >= LS_VISITED)
		{
			if(current.type == L_CITY)
			{
				City* c = (City*)&current;
				s += Format("\n%s: %d", txCitizens, c->citizens_world);
			}
			else if(current.type == L_VILLAGE)
			{
				Village* v = (Village*)&current;
				s += Format("\n%s: %d\n%s: ", txCitizens, v->citizens_world, txAvailable);
				if(v->v_buildings[0] != B_NONE)
				{
					s += buildings[v->v_buildings[0]].name;
					s += ", ";
					if(v->v_buildings[1] != B_NONE)
					{
						s += buildings[v->v_buildings[1]].name;
						s += ", ";
					}
				}
				s += Format("%s, %s, %s, %s", buildings[B_MERCHANT].name, buildings[B_FOOD_SELLER].name, buildings[B_INN].name, buildings[B_VILLAGE_HALL].name);
			}
		}
	}

	// opis zaznaczonej lokacji
	if(game.picked_location != -1)
	{
		Location& picked = *game.locations[game.picked_location];

		if(game.picked_location != game.current_location)
		{
			float odl = distance(game.world_pos, picked.pos)/600.f*200;
			int koszt = int(ceil(odl/TRAVEL_SPEED));
			s += Format("\n\n%s: %s", txTarget, picked.name.c_str());
			if(game.devmode && game.IsLocal())
			{
				if(picked.type == L_DUNGEON || picked.type == L_CRYPT)
				{
					InsideLocation* inside = (InsideLocation*)&picked;
					s += Format(" (%s, %s, %d)", g_base_locations[inside->target].name, g_spawn_groups[inside->spawn].id, inside->st);
				}
				else if(picked.type == L_FOREST || picked.type == L_CAMP || picked.type == L_CAVE || picked.type == L_MOONWELL)
					s += Format(" (st %d)", picked.st);
				s += Format(", quest 0x%p", picked.active_quest);
			}
			s += Format("\n%s: %g km\n%s: %d %s", txDistance, ceil(odl*10)/10, txTravelTime, koszt, koszt == 1 ? txDay : txDays);
			if(picked.state >= LS_VISITED)
			{
				if(picked.type == L_CITY)
				{
					City* c = (City*)&picked;
					s += Format("\n%s: %d", txCitizens, c->citizens_world);
				}
				else if(picked.type == L_VILLAGE)
				{
					Village* v = (Village*)&picked;
					s += Format("\n%s: %d\n%s: ", txCitizens, v->citizens_world, txAvailable);
					if(v->v_buildings[0] != B_NONE)
					{
						s += buildings[v->v_buildings[0]].name;
						s += ", ";
						if(v->v_buildings[1] != B_NONE)
						{
							s += buildings[v->v_buildings[1]].name;
							s += ", ";
						}
					}
					s += Format("%s, %s, %s, %s", buildings[B_MERCHANT].name, buildings[B_FOOD_SELLER].name, buildings[B_INN].name, buildings[B_VILLAGE_HALL].name);
				}
			}
		}
		GUI.DrawSprite(tSelected[0], WorldPosToScreen(INT2(picked.pos.x-32.f,picked.pos.y+32.f)), 0xAAFFFFFF);
	}

	// aktualna pozycja dru¿yny w czasie podró¿y
	if(game.world_state == WS_TRAVEL || game.world_state == WS_ENCOUNTER)
		GUI.DrawSprite(tMover, WorldPosToScreen(INT2(game.world_pos.x-8,game.world_pos.y+8)), 0xBBFFFFFF);

	// szansa na spotkanie
	if(game.devmode)
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)game.szansa_na_spotkanie-25))*100/500), game.szansa_na_spotkanie);

	// tekst
	RECT rect = {608,8,game.wnd_size.x-8,game.wnd_size.y-8};
	GUI.DrawText(GUI.default_font, s, 0, BLACK, rect);

	// kreska
	if(game.picked_location != -1 && game.picked_location != game.current_location)
	{
		Location& picked = *game.locations[game.picked_location];
		VEC2 pts[2] = {WorldPosToScreen(game.world_pos), WorldPosToScreen(picked.pos)};

		GUI.LineBegin();
		GUI.DrawLine(pts, 1, 0xAA000000);
		GUI.LineEnd();
	}

	if(game.koniec_gry)
	{
		// czarne t³o
		int color;
		if(game.death_fade < 1.f)
			color = (int(game.death_fade*255)<<24) | 0x00FFFFFF;
		else
			color = WHITE;

		GUI.DrawSpriteFull(game.tCzern, color);

		// obrazek
		D3DSURFACE_DESC desc;
		V( game.tEmerytura->GetLevelDesc(0, &desc) );
		GUI.DrawSprite(game.tEmerytura, Center(desc.Width,desc.Height), color);

		// tekst
		cstring text = Format(txGameTimeout, game.pc->kills, game.total_kills-game.pc->kills);
		RECT rect = {0, 0, GUI.wnd_size.x, GUI.wnd_size.y};
		GUI.DrawText(GUI.default_font, text, DT_CENTER|DT_BOTTOM, color, rect);
	}

	if(mp_box->visible)
		mp_box->Draw();

	if(journal->visible)
		journal->Draw();

	game_messages->Draw();
}

//=================================================================================================
void WorldMapGui::Update(float dt)
{
	if(mp_box->visible)
	{
		mp_box->focus = true;
		mp_box->Event(GuiEvent_GainFocus);
		if(GUI.HaveDialog())
			mp_box->LostFocus();
		mp_box->Update(dt);

		if(Key.Focus() && !mp_box->have_focus && !GUI.HaveDialog() && Key.PressedRelease(VK_RETURN))
			mp_box->have_focus = true;
	}

	if(game.koniec_gry)
		return;

	if(journal->visible)
	{
		journal->focus = true;
		journal->Update(dt);
	}
	if(!GUI.HaveDialog() && !(mp_box->visible && mp_box->itb.focus) && Key.Focus() && game.death_screen == 0 && !journal->visible && GKey.PressedRelease(GK_JOURNAL))
		journal->Show();

	if(game.world_state == WS_TRAVEL)
	{
		if(game.paused || (!game.IsOnline() && GUI.HavePauseDialog()))
			return;

		// ruch po mapie
		game.travel_time += dt;
		const VEC2& end_pt = game.locations[game.picked_location]->pos;
		float dist = distance(game.travel_start, end_pt);
		if(game.travel_time > game.travel_day)
		{
			// min¹³ kolejny dzieñ w podró¿y
			++game.travel_day;
			if(game.IsLocal())
				game.WorldProgress(1, Game::WPM_TRAVEL);
		}

		if(game.travel_time*3 >= dist/TRAVEL_SPEED)
		{
			// koniec podró¿y
			game.world_state = WS_MAIN;
			game.current_location = game.picked_location;
			Location& loc = *game.locations[game.current_location];
			if(loc.state == LS_KNOWN)
				loc.state = LS_VISITED;
			game.world_pos = end_pt;
			goto update_worldmap;
		}
		else
		{
			// ruch
			VEC2 dir = end_pt - game.travel_start;
			game.world_pos = game.travel_start + dir * (game.travel_time / dist * TRAVEL_SPEED*3);

			game.travel_time2 += dt;

			// odkryj pobliskie miejsca / ataki
			if(game.IsLocal() && game.travel_time2 >= 0.25f)
			{
				game.travel_time2 = 0;
				int co = -2, enc = -1, index = 0;
				for(Location* ploc : game.locations)
				{
					if(!ploc)
					{
						++index;
						continue;
					}
					Location& loc = *ploc;
					if(distance(game.world_pos, loc.pos) <= 32.f)
					{
						if(loc.state != LS_CLEARED)
						{
							int szansa = 0;
							if(loc.type == L_FOREST)
							{
								szansa = 1;
								co = -1;
							}
							else if(loc.spawn == SG_BANDYCI)
							{
								szansa = 3;
								co = loc.spawn;
							}
							else if(loc.spawn == SG_ORKOWIE || loc.spawn == SG_GOBLINY)
							{
								szansa = 2;
								co = loc.spawn;
							}
							game.szansa_na_spotkanie += szansa;
						}

						if(loc.state == LS_UNKNOWN)
						{
							loc.state = LS_KNOWN;
							if(game.IsOnline())
								game.Net_ChangeLocationState(index, false);
						}
					}
					++index;
				}

				int id = 0;
				for(vector<Encounter*>::iterator it = game.encs.begin(), end = game.encs.end(); it != end; ++it, ++id)
				{
					if(!*it)
						continue;
					Encounter& enc0 = **it;
					if(distance(enc0.pos, game.world_pos) < enc0.zasieg)
					{
						if(!enc0.check_func || enc0.check_func())
						{
							game.szansa_na_spotkanie += enc0.szansa;
							enc = id;
						}
					}
				}

				game.szansa_na_spotkanie += 1;

				if(rand2()%500 < ((int)game.szansa_na_spotkanie)-25 || (DEBUG_BOOL && Key.Focus() && Key.Down('E')))
				{
					game.szansa_na_spotkanie = 0.f;
					game.locations[game.encounter_loc]->state = LS_UNKNOWN;

					if(enc != -1)
					{
						// questowe spotkanie
						game.game_enc = game.encs[enc];
						game.enc_tryb = 2;

						DialogInfo info;
						info.event = DialogEvent(&game, &Game::Event_RandomEncounter);
						info.name = "encounter";
						info.order = ORDER_TOPMOST;
						info.parent = this;
						info.pause = true;
						info.text = game.game_enc->text;
						info.type = DIALOG_OK;
						game.dialog_enc = GUI.ShowDialog(info);

						if(game.IsOnline())
						{
							NetChange& c = Add1(game.net_changes);
							c.type = NetChange::ENCOUNTER;
							c.str = StringPool.Get();
							*c.str =game. game_enc->text;

							// jeœli gracz nie jest przywódc¹ to nie mo¿e wcisn¹æ przycisku
							if(!game.IsLeader())
								game.dialog_enc->bts[0].state = Button::DISABLED;

							game.world_state = WS_ENCOUNTER;
							game.PushNetChange(NetChange::UPDATE_MAP_POS);
						}
					}
					else
					{
						Quest_Crazies::State c_state = game.quest_crazies->crazies_state;

						bool golemy = (game.quest_mages2->mages_state >= Quest_Mages2::State::Encounter && game.quest_mages2->mages_state < Quest_Mages2::State::Completed && rand2()%3 == 0)
							|| (DEBUG_BOOL && Key.Focus() && Key.Down('G'));
						bool szalony = (c_state == Quest_Crazies::State::TalkedWithCrazy && (rand2()%2 == 0 || (DEBUG_BOOL && Key.Focus() && Key.Down('S'))));
						bool unk = (c_state >= Quest_Crazies::State::PickedStone && c_state < Quest_Crazies::State::End && (rand2()%3 == 0 || (DEBUG_BOOL && Key.Focus() && Key.Down('S'))));
						if(game.quest_mages2->mages_state == Quest_Mages2::State::Encounter && rand2()%2 == 0)
							golemy = true;
						if(c_state == Quest_Crazies::State::PickedStone && rand2()%2 == 0)
							unk = true;

						if(co == -2)
						{
							if(rand2()%6 == 0)
								co = SG_BANDYCI;
							else
								co = -3;
						}
						else if(rand2()%3 == 0)
							co = -3;

						if(szalony || unk || golemy || co == -3 || (DEBUG_BOOL && Key.Focus() && Key.Down(VK_SHIFT)))
						{
							// losowe spotkanie
							game.spotkanie = rand2()%6;
							if(unk)
								game.spotkanie = 8;
							else if(szalony)
								game.spotkanie = 7;
							else if(golemy)
							{
								game.spotkanie = 6;
								game.quest_mages2->paid = false;
							}
							else if(DEBUG_BOOL && Key.Focus())
							{
								if(Key.Down('I'))
									game.spotkanie = 1;
								else if(Key.Down('B'))
									game.spotkanie = 4;
							}
							game.enc_tryb = 1;

							DialogInfo info;
							info.event = DialogEvent(&game, &Game::Event_RandomEncounter);
							info.name = "encounter";
							info.order = ORDER_TOPMOST;
							info.parent = this;
							info.pause = true;
							switch(game.spotkanie)
							{
							case 0:
								info.text = txEncCrazyMage;
								break;
							case 1:
								info.text = txEncCrazyHeroes;
								break;
							case 2:
								info.text = txEncMerchant;
								break;
							case 3:
								info.text = txEncHeroes;
								break;
							case 4:
								info.text = txEncBanditsAttackTravelers;
								break;
							case 5:
								info.text = txEncHeroesAttack;
								break;
							case 6:
								info.text = txEncGolem;
								break;
							case 7:
								info.text = txEncCrazy;
								break;
							case 8:
								info.text = txEncUnk;
								break;
							}
							info.type = DIALOG_OK;
							game.dialog_enc = GUI.ShowDialog(info);

							if(game.IsOnline())
							{
								NetChange& c = Add1(game.net_changes);
								c.type = NetChange::ENCOUNTER;
								c.str = StringPool.Get();
								*c.str = info.text;

								if(!game.IsLeader())
									game.dialog_enc->bts[0].state = Button::DISABLED;

								game.world_state = WS_ENCOUNTER;
								game.PushNetChange(NetChange::UPDATE_MAP_POS);
							}
						}
						else
						{
							// spotkanie z wrogami
							game.losowi_wrogowie = (SPAWN_GROUP)co;
							game.enc_tryb = 0;
							DialogInfo info;
							info.event = DialogEvent(&game, &Game::Event_RandomEncounter);
							info.name = "encounter";
							info.order = ORDER_TOPMOST;
							info.parent = this;
							info.pause = true;
							switch(co)
							{
							case SG_BANDYCI:
								info.text = txEncBandits;
								break;
							case -1:
								info.text = txEncAnimals;
								break;
							case SG_ORKOWIE:
								info.text = txEncOrcs;
								break;
							case SG_GOBLINY:
								info.text = txEncGoblins;
								break;
							}
							info.type = DIALOG_OK;
							game.dialog_enc = GUI.ShowDialog(info);

							if(game.IsOnline())
							{
								NetChange& c = Add1(game.net_changes);
								c.type = NetChange::ENCOUNTER;
								c.str = StringPool.Get();
								*c.str = info.text;

								if(!game.IsLeader())
									game.dialog_enc->bts[0].state = Button::DISABLED;

								game.world_state = WS_ENCOUNTER;
								game.PushNetChange(NetChange::UPDATE_MAP_POS);
							}
						}
					}
				}
			}
		}
	}
	else if(game.world_state != WS_ENCOUNTER && !journal->visible)
	{
update_worldmap:
		VEC2 cursor_pos(float(GUI.cursor_pos.x), float(GUI.cursor_pos.y));
		Location* loc = nullptr;
		float dist = 17.f, dist2;
		int i = 0, index;

		if(focus)
		{
			for(vector<Location*>::iterator it = game.locations.begin(), end = game.locations.end(); it != end; ++it, ++i)
			{
				if(!*it || (*it)->state == LS_UNKNOWN || (*it)->state == LS_HIDDEN)
					continue;
				VEC2 pt = WorldPosToScreen((*it)->pos);
				dist2 = distance(pt, cursor_pos);
				if(dist2 < dist)
				{
					loc = *it;
					dist = dist2;
					index = i;
				}
			}
		}

		if(loc)
		{
			game.picked_location = index;
			if(game.world_state == WS_MAIN && Key.Focus())
			{
				if(Key.PressedRelease(VK_LBUTTON))
				{
					if(game.IsLeader())
					{
						if(game.picked_location != game.current_location)
						{
							// rozpocznij wêdrówkê po mapie œwiata
							game.world_state = WS_TRAVEL;
							game.current_location = -1;
							game.travel_time = 0.f;
							game.travel_day = 0;
							game.travel_start = game.world_pos;
							Location& l = *game.locations[game.picked_location];
							game.world_dir = clip(angle(game.world_pos.x, game.world_pos.y, l.pos.x, l.pos.y)+PI);
							game.travel_time2 = 0.f;

							// opuœæ aktualn¹ lokalizacje
							if(game.open_location != -1)
							{
								game.LeaveLocation();
								game.open_location = -1;
							}

							if(game.IsOnline())
							{
								NetChange& c = Add1(game.net_changes);
								c.type = NetChange::TRAVEL;
								c.id = game.picked_location;
							}
						}
						else
						{
							if(game.IsLocal())
								game.EnterLocation();
							else
								game.PushNetChange(NetChange::ENTER_LOCATION);
						}
					}
					else
						game.AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game.devmode && game.picked_location != game.current_location && Key.PressedRelease('T'))
				{
					if(game.IsLeader())
					{
						game.current_location = game.picked_location;
						Location& loc = *game.locations[game.current_location];
						if(loc.state == LS_KNOWN)
							loc.state = LS_VISITED;
						game.world_pos = loc.pos;
						if(game.open_location != -1)
						{
							game.LeaveLocation();
							game.open_location = -1;
						}
						if(game.IsOnline())
						{
							NetChange& c = Add1(game.net_changes);
							c.type = NetChange::CHEAT_TRAVEL;
							c.id = game.picked_location;
						}
					}
					else
						game.AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
		else
			game.picked_location = -1;
	}

	game_messages->Update(dt);
}

//=================================================================================================
void WorldMapGui::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
		mp_box->GainFocus();
	else if(e == GuiEvent_LostFocus)
		mp_box->LostFocus();
}
