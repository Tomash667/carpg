#include "Pch.h"
#include "GameCore.h"
#include "WorldMapGui.h"
#include "Game.h"
#include "Language.h"
#include "Journal.h"
#include "Version.h"
#include "Quest_Mages.h"
#include "Quest_Crazies.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "City.h"
#include "GameGui.h"
#include "MpBox.h"
#include "GameMessages.h"
#include "DialogBox.h"
#include "World.h"
#include "Level.h"
#include "GameStats.h"
#include "DirectX.h"

//-----------------------------------------------------------------------------
extern const float TRAVEL_SPEED;

//=================================================================================================
WorldMapGui::WorldMapGui() : game(Game::Get())
{
	focusable = true;
	visible = false;

	txGameTimeout = Str("gameTimeout");
	txWorldDate = Str("worldDate");
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
	txEncCrazyCook = Str("encCrazyCook");
	txEncMerchant = Str("encMerchant");
	txEncHeroes = Str("encHeroes");
	txEncBanditsAttackTravelers = Str("encBanditsAttackTravelers");
	txEncHeroesAttack = Str("encHeroesAttack");
	txEncGolem = Str("encGolem");
	txEncCrazy = Str("encCrazy");
	txEncUnk = Str("encUnk");
	txEncBandits = Str("encBandits");
	txEncAnimals = Str("encAnimals");
	txEncOrcs = Str("encOrcs");
	txEncGoblins = Str("encGoblins");
	txBuildings = Str("buildings");

	mp_box = game.game_gui->mp_box;
	journal = game.game_gui->journal;
	game_messages = game.game_gui->game_messages;
}

//=================================================================================================
void WorldMapGui::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("camp.png", tMapIcon[LI_CAMP]);
	tex_mgr.AddLoadTask("village.png", tMapIcon[LI_VILLAGE]);
	tex_mgr.AddLoadTask("city.png", tMapIcon[LI_CITY]);
	tex_mgr.AddLoadTask("dungeon.png", tMapIcon[LI_DUNGEON]);
	tex_mgr.AddLoadTask("crypt.png", tMapIcon[LI_CRYPT]);
	tex_mgr.AddLoadTask("cave.png", tMapIcon[LI_CAVE]);
	tex_mgr.AddLoadTask("forest.png", tMapIcon[LI_FOREST]);
	tex_mgr.AddLoadTask("moonwell.png", tMapIcon[LI_MOONWELL]);
	tex_mgr.AddLoadTask("academy.png", tMapIcon[LI_ACADEMY]);
	tex_mgr.AddLoadTask("worldmap.jpg", tWorldMap);
	tex_mgr.AddLoadTask("selected.png", tSelected[0]);
	tex_mgr.AddLoadTask("selected2.png", tSelected[1]);
	tex_mgr.AddLoadTask("mover.png", tMover);
	tex_mgr.AddLoadTask("old_map.png", tMapBg);
	tex_mgr.AddLoadTask("enc.png", tEnc);
}

//=================================================================================================
void WorldMapGui::Draw(ControlDrawData*)
{
	// t³o
	Rect rect0(Int2::Zero, game.GetWindowSize());
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	GUI.DrawSpriteRectPart(tMapBg, rect0, rect0);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	game.device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	// mapa
	Matrix mat;
	mat = Matrix::Transform2D(&Vec2(0, 0), 0.f, &Vec2(600.f / 512.f, 600.f / 512.f), nullptr, 0.f, nullptr);
	GUI.DrawSpriteTransform(tWorldMap, mat);

	// obrazki lokacji
	for(vector<Location*>::iterator it = W.locations.begin(), end = W.locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;
		Location& loc = **it;
		if(loc.state == LS_UNKNOWN || loc.state == LS_HIDDEN)
			continue;
		GUI.DrawSprite(tMapIcon[loc.image], WorldPosToScreen(Int2(loc.pos.x - 16.f, loc.pos.y + 16.f)), loc.state == LS_KNOWN ? 0x70707070 : Color::White);
	}

	// lokacje spotkañ
	if(game.devmode)
	{
		for(vector<Encounter*>::iterator it = W.encounters.begin(), end = W.encounters.end(); it != end; ++it)
		{
			if(*it)
				GUI.DrawSprite(tEnc, WorldPosToScreen(Int2((*it)->pos.x - 16.f, (*it)->pos.y + 16.f)));
		}
	}

	LocalString s = Format(txWorldDate, W.GetDate());

	// opis aktualnej lokacji
	if(W.current_location)
	{
		Location& current = *W.current_location;
		GUI.DrawSprite(tSelected[1], WorldPosToScreen(Int2(current.pos.x - 32.f, current.pos.y + 32.f)), 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		AppendLocationText(current, s.get_ref());
	}

	// opis zaznaczonej lokacji
	if(picked_location != -1)
	{
		Location& picked = *W.locations[picked_location];

		if(picked_location != W.current_location_index)
		{
			float odl = Vec2::Distance(W.world_pos, picked.pos) / 600.f * 200;
			int koszt = int(ceil(odl / TRAVEL_SPEED));
			s += Format("\n\n%s: %s", txTarget, picked.name.c_str());
			AppendLocationText(picked, s.get_ref());
			s += Format("\n%s: %g km\n%s: %d %s", txDistance, ceil(odl * 10) / 10, txTravelTime, koszt, koszt == 1 ? txDay : txDays);
		}
		GUI.DrawSprite(tSelected[0], WorldPosToScreen(Int2(picked.pos.x - 32.f, picked.pos.y + 32.f)), 0xAAFFFFFF);
	}

	// aktualna pozycja dru¿yny w czasie podró¿y
	if(Any(W.state, World::State::TRAVEL, World::State::ENCOUNTER))
		GUI.DrawSprite(tMover, WorldPosToScreen(Int2(W.world_pos.x - 8, W.world_pos.y + 8)), 0xBBFFFFFF);

	// szansa na spotkanie
	if(game.devmode && Net::IsLocal())
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)W.encounter_chance - 25)) * 100 / 500), W.encounter_chance);

	// tekst
	Rect rect(Int2(608, 8), game.GetWindowSize() - Int2(8, 8));
	GUI.DrawText(GUI.default_font, s, 0, Color::Black, rect);

	// kreska
	if(picked_location != -1 && picked_location != W.current_location_index)
	{
		Location& picked = *W.locations[picked_location];
		Vec2 pts[2] = { WorldPosToScreen(W.world_pos), WorldPosToScreen(picked.pos) };

		GUI.LineBegin();
		GUI.DrawLine(pts, 1, 0xAA000000);
		GUI.LineEnd();
	}

	if(game.koniec_gry)
	{
		// czarne t³o
		int color;
		if(game.death_fade < 1.f)
			color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
		else
			color = Color::White;

		GUI.DrawSpriteFull(game.tCzern, color);

		// obrazek
		D3DSURFACE_DESC desc;
		V(game.tEmerytura->GetLevelDesc(0, &desc));
		GUI.DrawSprite(game.tEmerytura, Center(desc.Width, desc.Height), color);

		// tekst
		cstring text = Format(txGameTimeout, game.pc->kills, GameStats::Get().total_kills - game.pc->kills);
		Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
		GUI.DrawText(GUI.default_font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
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

	if(W.travel_location_index != -1)
		picked_location = W.travel_location_index;

	if(W.state == World::State::TRAVEL)
	{
		if(game.paused || (!Net::IsOnline() && GUI.HavePauseDialog()))
			return;

		// ruch po mapie
		W.travel_timer += dt;
		const Vec2& end_pt = W.locations[W.travel_location_index]->pos;
		float dist = Vec2::Distance(W.travel_start_pos, end_pt);
		if(W.travel_timer > W.travel_day)
		{
			// min¹³ kolejny dzieñ w podró¿y
			++W.travel_day;
			if(Net::IsLocal())
				game.WorldProgress(1, Game::WPM_TRAVEL);
		}

		if(W.travel_timer * 3 >= dist / TRAVEL_SPEED)
		{
			if(game.IsLeader())
			{
				// end of travel
				if(Net::IsOnline())
					Net::PushChange(NetChange::END_TRAVEL);
				W.state = World::State::ON_MAP;
				W.current_location_index = W.travel_location_index;
				W.current_location = W.locations[W.current_location_index];
				W.travel_location_index = -1;
				L.location_index = W.current_location_index;
				L.location = W.current_location;
				Location& loc = *W.current_location;
				if(loc.state == LS_KNOWN)
					game.SetLocationVisited(loc);
				W.world_pos = end_pt;
				goto update_worldmap;
			}
			else
				W.world_pos = end_pt;
		}
		else
		{
			// ruch
			Vec2 dir = end_pt - W.travel_start_pos;
			W.world_pos = W.travel_start_pos + dir * (W.travel_timer / dist * TRAVEL_SPEED * 3);

			W.encounter_timer += dt;

			// odkryj pobliskie miejsca / ataki
			if(Net::IsLocal() && W.encounter_timer >= 0.25f)
			{
				W.encounter_timer = 0;
				int co = -2, enc = -1, index = 0;
				for(Location* ploc : W.locations)
				{
					if(!ploc)
					{
						++index;
						continue;
					}
					Location& loc = *ploc;
					if(Vec2::Distance(W.world_pos, loc.pos) <= 32.f)
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
							W.encounter_chance += szansa;
						}

						if(loc.state == LS_UNKNOWN)
						{
							loc.state = LS_KNOWN;
							if(Net::IsOnline())
								game.Net_ChangeLocationState(index, false);
						}
					}
					++index;
				}

				int id = 0;
				for(vector<Encounter*>::iterator it = W.encounters.begin(), end = W.encounters.end(); it != end; ++it, ++id)
				{
					if(!*it)
						continue;
					Encounter& enc0 = **it;
					if(Vec2::Distance(enc0.pos, W.world_pos) < enc0.zasieg)
					{
						if(!enc0.check_func || enc0.check_func())
						{
							W.encounter_chance += enc0.szansa;
							enc = id;
						}
					}
				}

				W.encounter_chance += 1;

				if(Rand() % 500 < ((int)W.encounter_chance) - 25 || (DEBUG_BOOL && Key.Focus() && Key.Down('E')))
				{
					W.encounter_chance = 0.f;
					W.locations[W.GetEncounterLocationIndex()]->state = LS_UNKNOWN;

					if(enc != -1)
					{
						// questowe spotkanie
						W.state = World::State::ENCOUNTER;
						game.game_enc = W.encounters[enc];
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

						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::ENCOUNTER;
							c.str = StringPool.Get();
							*c.str = game.game_enc->text;

							// jeœli gracz nie jest przywódc¹ to nie mo¿e wcisn¹æ przycisku
							if(!game.IsLeader())
								game.dialog_enc->bts[0].state = Button::DISABLED;

							Net::PushChange(NetChange::UPDATE_MAP_POS);
						}
					}
					else
					{
						Quest_Crazies::State c_state = game.quest_crazies->crazies_state;

						bool golemy = (game.quest_mages2->mages_state >= Quest_Mages2::State::Encounter && game.quest_mages2->mages_state < Quest_Mages2::State::Completed && Rand() % 3 == 0)
							|| (DEBUG_BOOL && Key.Focus() && Key.Down('G'));
						bool szalony = (c_state == Quest_Crazies::State::TalkedWithCrazy && (Rand() % 2 == 0 || (DEBUG_BOOL && Key.Focus() && Key.Down('S'))));
						bool unk = (c_state >= Quest_Crazies::State::PickedStone && c_state < Quest_Crazies::State::End && (Rand() % 3 == 0 || (DEBUG_BOOL && Key.Focus() && Key.Down('S'))));
						if(game.quest_mages2->mages_state == Quest_Mages2::State::Encounter && Rand() % 2 == 0)
							golemy = true;
						if(c_state == Quest_Crazies::State::PickedStone && Rand() % 2 == 0)
							unk = true;

						if(co == -2)
						{
							if(Rand() % 6 == 0)
								co = SG_BANDYCI;
							else
								co = -3;
						}
						else if(Rand() % 3 == 0)
							co = -3;

						if(szalony || unk || golemy || co == -3 || (DEBUG_BOOL && Key.Focus() && Key.Down(VK_SHIFT)))
						{
							// losowe spotkanie
							game.spotkanie = Rand() % 6;
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
								else if(Key.Down('C'))
									game.spotkanie = 9;
							}
							game.enc_tryb = 1;

							if((game.spotkanie == 0 || game.spotkanie == 1) && Rand() % 10 == 0)
								game.spotkanie = 9;

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
							case 9:
								info.text = txEncCrazyCook;
								break;
							}
							info.type = DIALOG_OK;
							game.dialog_enc = GUI.ShowDialog(info);
							W.state = World::State::ENCOUNTER;

							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::ENCOUNTER;
								c.str = StringPool.Get();
								*c.str = info.text;

								if(!game.IsLeader())
									game.dialog_enc->bts[0].state = Button::DISABLED;

								Net::PushChange(NetChange::UPDATE_MAP_POS);
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
							W.state = World::State::ENCOUNTER;

							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::ENCOUNTER;
								c.str = StringPool.Get();
								*c.str = info.text;

								if(!game.IsLeader())
									game.dialog_enc->bts[0].state = Button::DISABLED;

								Net::PushChange(NetChange::UPDATE_MAP_POS);
							}
						}
					}
				}
			}
		}
	}
	else if(W.state != World::State::ENCOUNTER && !journal->visible)
	{
	update_worldmap:
		Vec2 cursor_pos(float(GUI.cursor_pos.x), float(GUI.cursor_pos.y));
		Location* loc = nullptr;
		float dist = 17.f, dist2;
		int i = 0, index;

		if(focus)
		{
			for(vector<Location*>::iterator it = W.locations.begin(), end = W.locations.end(); it != end; ++it, ++i)
			{
				if(!*it || (*it)->state == LS_UNKNOWN || (*it)->state == LS_HIDDEN)
					continue;
				Vec2 pt = WorldPosToScreen((*it)->pos);
				dist2 = Vec2::Distance(pt, cursor_pos);
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
			picked_location = index;
			if(W.state == World::State::ON_MAP && Key.Focus())
			{
				if(Key.PressedRelease(VK_LBUTTON))
				{
					if(game.IsLeader())
					{
						if(picked_location != W.current_location_index)
						{
							// opuœæ aktualn¹ lokalizacje
							if(L.is_open)
							{
								game.LeaveLocation();
								L.is_open = false;
							}

							// rozpocznij wêdrówkê po mapie œwiata
							W.state = World::State::TRAVEL;
							W.current_location_index = -1;
							W.current_location = nullptr;
							W.travel_location_index = picked_location;
							L.location_index = -1;
							L.location = nullptr;
							W.travel_timer = 0.f;
							W.travel_day = 0;
							W.travel_start_pos = W.world_pos;
							Location& l = *W.locations[picked_location];
							W.travel_dir = Clip(Angle(W.world_pos.x, W.world_pos.y, l.pos.x, l.pos.y) + PI);
							W.encounter_timer = 0.f;

							if(Net::IsOnline())
							{
								NetChange& c = Add1(Net::changes);
								c.type = NetChange::TRAVEL;
								c.id = picked_location;
							}
						}
						else
						{
							if(Net::IsLocal())
								game.EnterLocation();
							else
								Net::PushChange(NetChange::ENTER_LOCATION);
						}
					}
					else
						game.AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game.devmode && picked_location != W.current_location_index && Key.PressedRelease('T'))
				{
					if(game.IsLeader())
					{
						if(L.is_open)
						{
							game.LeaveLocation(false, false);
							L.is_open = false;
						}

						W.current_location_index = picked_location;
						W.current_location = W.locations[W.current_location_index];
						L.location_index = W.current_location_index;
						L.location = W.current_location;
						Location& loc = *W.current_location;
						if(loc.state == LS_KNOWN)
							game.SetLocationVisited(loc);
						W.world_pos = loc.pos;
						
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_TRAVEL;
							c.id = picked_location;
						}
					}
					else
						game.AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
		else
			picked_location = -1;
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

//=================================================================================================
void WorldMapGui::AppendLocationText(Location& loc, string& s)
{
	if(game.devmode && Net::IsLocal())
	{
		s += " (";
		if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
		{
			InsideLocation* inside = (InsideLocation*)&loc;
			s += Format("%s, %s, st %d, levels %d, ",
				g_base_locations[inside->target].name, g_spawn_groups[inside->spawn].id, inside->st, inside->GetLastLevel() + 1);
		}
		else if(loc.type == L_FOREST || loc.type == L_CAMP || loc.type == L_CAVE || loc.type == L_MOONWELL)
			s += Format("%s, st %d, ", g_spawn_groups[loc.spawn].id, loc.st);
		s += Format("quest 0x%p)", loc.active_quest);
	}
	if(loc.state >= LS_VISITED && loc.type == L_CITY)
		GetCityText((City&)loc, s);
}

//=================================================================================================
void WorldMapGui::GetCityText(City& city, string& s)
{
	// Citizens: X
	s += Format("\n%s: %d", txCitizens, city.citizens_world);

	// Buildings
	LocalVector3<std::pair<string*, uint>> items;

	// list buildings
	for(CityBuilding& b : city.buildings)
	{
		if(IS_SET(b.type->flags, Building::LIST))
		{
			bool found = false;
			for(auto& i : items)
			{
				if(*i.first == b.type->name)
				{
					++i.second;
					found = true;
					break;
				}
			}
			if(!found)
			{
				string* s = StringPool.Get();
				*s = b.type->name;
				items.push_back(std::pair<string*, uint>(s, 1u));
			}
		}
	}

	// sort
	if(items.empty())
		return;
	std::sort(items.begin(), items.end(), [](const std::pair<string*, uint>& a, const std::pair<string*, uint>& b) { return *a.first < *b.first; });

	// create string
	s += Format("\n%s: ", txBuildings);
	for(auto it = items.begin(), last = items.end() - 1; it != last; ++it)
	{
		auto& i = *it;
		if(i.second == 1u)
			s += *i.first;
		else
			s += Format("%s(%u)", *i.first, i.second);
		s += ", ";
	}
	auto& i = items.back();
	if(i.second == 1u)
		s += *i.first;
	else
		s += Format("%s(%u)", *i.first, i.second);

	// cleanup
	for(auto& i : items)
		StringPool.Free(i.first);
}
