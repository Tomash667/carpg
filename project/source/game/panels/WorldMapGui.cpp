#include "Pch.h"
#include "GameCore.h"
#include "WorldMapGui.h"
#include "Game.h"
#include "Language.h"
#include "Journal.h"
#include "Version.h"
#include "QuestManager.h"
#include "Quest_Mages.h"
#include "Quest_Crazies.h"
#include "Encounter.h"
#include "InsideLocation.h"
#include "City.h"
#include "GlobalGui.h"
#include "GameGui.h"
#include "MpBox.h"
#include "GameMessages.h"
#include "DialogBox.h"
#include "World.h"
#include "Level.h"
#include "GameStats.h"
#include "ResourceManager.h"
#include "DirectX.h"

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
	txBuildings = Str("buildings");

	mp_box = game.gui->game_gui->mp_box;
	journal = game.gui->game_gui->journal;
	game_messages = game.gui->game_gui->game_messages;
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
		for(const Encounter* enc : W.GetEncounters())
		{
			if(enc)
				GUI.DrawSprite(tEnc, WorldPosToScreen(Int2(enc->pos.x - 16.f, enc->pos.y + 16.f)));
		}
	}

	LocalString s = Format(txWorldDate, W.GetDate());
	const Vec2& world_pos = W.GetWorldPos();

	// opis aktualnej lokacji
	if(W.GetCurrentLocation())
	{
		Location& current = *W.GetCurrentLocation();
		GUI.DrawSprite(tSelected[1], WorldPosToScreen(Int2(current.pos.x - 32.f, current.pos.y + 32.f)), 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		AppendLocationText(current, s.get_ref());
	}

	// opis zaznaczonej lokacji
	if(picked_location != -1)
	{
		Location& picked = *W.locations[picked_location];

		if(picked_location != W.GetCurrentLocationIndex())
		{
			float distance = Vec2::Distance(world_pos, picked.pos) / 600.f * 200;
			int days_cost = int(ceil(distance / World::TRAVEL_SPEED));
			s += Format("\n\n%s: %s", txTarget, picked.name.c_str());
			AppendLocationText(picked, s.get_ref());
			s += Format("\n%s: %g km\n%s: %d %s", txDistance, ceil(distance * 10) / 10, txTravelTime, days_cost, days_cost == 1 ? txDay : txDays);
		}
		GUI.DrawSprite(tSelected[0], WorldPosToScreen(Int2(picked.pos.x - 32.f, picked.pos.y + 32.f)), 0xAAFFFFFF);
	}

	// aktualna pozycja dru¿yny w czasie podró¿y
	if(Any(W.GetState(), World::State::TRAVEL, World::State::ENCOUNTER))
		GUI.DrawSprite(tMover, WorldPosToScreen(Int2(world_pos.x - 8, world_pos.y + 8)), 0xBBFFFFFF);

	// szansa na spotkanie
	if(game.devmode && Net::IsLocal())
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)W.GetEncounterChance() - 25)) * 100 / 500), W.GetEncounterChance());

	// tekst
	Rect rect(Int2(608, 8), game.GetWindowSize() - Int2(8, 8));
	GUI.DrawText(GUI.default_font, s, 0, Color::Black, rect);

	// kreska
	if(picked_location != -1 && picked_location != W.GetCurrentLocationIndex())
	{
		Location& picked = *W.locations[picked_location];
		Vec2 pts[2] = { WorldPosToScreen(world_pos), WorldPosToScreen(picked.pos) };

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

	if(W.GetState() == World::State::TRAVEL)
	{
		if(game.paused || (!Net::IsOnline() && GUI.HavePauseDialog()))
			return;

		W.UpdateTravel(dt);
	}
	else if(W.GetState() != World::State::ENCOUNTER && !journal->visible)
	{
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
			if(W.GetState() == World::State::ON_MAP && Key.Focus())
			{
				if(Key.PressedRelease(VK_LBUTTON))
				{
					if(game.IsLeader())
					{
						if(picked_location != W.GetCurrentLocationIndex())
						{
							// opuœæ aktualn¹ lokalizacje
							if(L.is_open)
							{
								game.LeaveLocation();
								L.is_open = false;
							}

							W.Travel(picked_location);
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
						game_messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game.devmode && picked_location != W.GetCurrentLocationIndex() && Key.PressedRelease('T'))
				{
					if(game.IsLeader())
					{
						if(L.is_open)
						{
							game.LeaveLocation(false, false);
							L.is_open = false;
						}

						W.Warp(picked_location);
						
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_TRAVEL;
							c.id = picked_location;
						}
					}
					else
						game_messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
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

//=================================================================================================
void WorldMapGui::ShowEncounterMessage(cstring text)
{
	DialogInfo info;
	info.event = DialogEvent(&game, &Game::Event_RandomEncounter);
	info.name = "encounter";
	info.order = ORDER_TOPMOST;
	info.parent = this;
	info.pause = true;
	info.text = text;
	info.type = DIALOG_OK;
	game.dialog_enc = GUI.ShowDialog(info);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ENCOUNTER;
		c.str = StringPool.Get();
		*c.str = text;

		// disable button when server is not leader
		if(!game.IsLeader())
			game.dialog_enc->bts[0].state = Button::DISABLED;
	}
}
