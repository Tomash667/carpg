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
#include "Team.h"
#include "SaveState.h"
#include "Debug.h"
#include "Render.h"

const float MAP_IMG_SIZE = 512.f;

struct LocationElement : public GuiElement, public ObjectPoolProxy<LocationElement>
{
	Location* loc;
	cstring ToString() override { return loc->name.c_str(); }
};

//=================================================================================================
WorldMapGui::WorldMapGui() : game(Game::Get()), zoom(1.2f), offset(0.f, 0.f), follow(false)
{
	focusable = true;
	visible = false;
	combo_box.parent = this;
	combo_box.destructor = [](GuiElement* e) { static_cast<LocationElement*>(e)->Free(); };
}

//=================================================================================================
void WorldMapGui::LoadLanguage()
{
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
	tex_mgr.AddLoadTask("tower.png", tMapIcon[LI_TOWER]);
	tex_mgr.AddLoadTask("labyrinth.png", tMapIcon[LI_LABYRINTH]);
	tex_mgr.AddLoadTask("mine.png", tMapIcon[LI_MINE]);
	tex_mgr.AddLoadTask("sawmill.png", tMapIcon[LI_SAWMILL]);
	tex_mgr.AddLoadTask("dungeon2.png", tMapIcon[LI_DUNGEON2]);
	tex_mgr.AddLoadTask("worldmap.jpg", tWorldMap);
	tex_mgr.AddLoadTask("selected.png", tSelected[0]);
	tex_mgr.AddLoadTask("selected2.png", tSelected[1]);
	tex_mgr.AddLoadTask("mover.png", tMover);
	tex_mgr.AddLoadTask("old_map.png", tMapBg);
	tex_mgr.AddLoadTask("enc.png", tEnc);
	tex_mgr.AddLoadTask("worldmap_side.png", tSide);
	tex_mgr.AddLoadTask("magnifying-glass-icon.png", tMagnifyingGlass);
	tex_mgr.AddLoadTask("tracking_arrow.png", tTrackingArrow);
}

//=================================================================================================
void WorldMapGui::Draw(ControlDrawData*)
{
	Render* render = game.GetRender();

	// background
	Rect rect0(Int2::Zero, game.GetWindowSize());
	render->SetTextureAddressMode(TEX_ADR_WRAP);
	GUI.DrawSpriteRectPart(tMapBg, rect0, rect0);
	render->SetTextureAddressMode(TEX_ADR_CLAMP);

	// map
	Matrix mat = Matrix::Transform2D(&offset, 0.f, &Vec2(float(W.world_size) / MAP_IMG_SIZE * zoom), nullptr, 0.f, &(GetCameraCenter() - offset));
	GUI.DrawSpriteTransform(tWorldMap, mat);

	// debug tiles
	if(!combo_box.focus && !GUI.HaveDialog() && DebugKey('S') && !Net::IsClient())
	{
		const vector<int>& tiles = W.GetTiles();
		int ts = W.world_size / World::TILE_SIZE;
		for(int y = 0; y < ts; ++y)
		{
			for(int x = 0; x < ts; ++x)
			{
				int st = tiles[x + y * ts];
				Color c;
				if(st <= 8)
					c = Color::Lerp(Color::Green, Color::Yellow, float(st - 2) / 6);
				else
					c = Color::Lerp(Color::Yellow, Color::Red, float(st - 9) / 7);
				Rect rect(Int2(WorldPosToScreen(Vec2((float)x*World::TILE_SIZE, (float)y*World::TILE_SIZE))),
					Int2(WorldPosToScreen(Vec2((float)(x + 1)*World::TILE_SIZE, (float)(y + 1)*World::TILE_SIZE))));
				GUI.DrawSpriteRect(GUI.tPix, rect, c);
			}
		}
	}

	// location images
	for(vector<Location*>::iterator it = W.locations.begin(), end = W.locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;
		Location& loc = **it;
		if(loc.state == LS_UNKNOWN || loc.state == LS_HIDDEN)
			continue;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(loc.pos.x - 16.f, loc.pos.y + 16.f)));
		GUI.DrawSpriteTransform(tMapIcon[loc.image], mat, loc.state == LS_KNOWN ? 0x70707070 : Color::White);
	}

	// encounter locations
	if(game.devmode)
	{
		for(const Encounter* enc : W.GetEncounters())
		{
			if(enc)
			{
				mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(enc->pos.x - 16.f, enc->pos.y + 16.f)));
				GUI.DrawSpriteTransform(tEnc, mat);
			}
		}
	}

	LocalString s = Format(txWorldDate, W.GetDate());
	const Vec2& world_pos = W.GetWorldPos();

	// current location description
	if(W.GetCurrentLocation())
	{
		Location& current = *W.GetCurrentLocation();
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(current.pos.x - 32.f, current.pos.y + 32.f)));
		GUI.DrawSpriteTransform(tSelected[1], mat, 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		AppendLocationText(current, s.get_ref());
	}

	// target location description
	if(picked_location != -1)
	{
		Location& picked = *W.locations[picked_location];
		if(picked_location != W.GetCurrentLocationIndex())
		{
			float distance = Vec2::Distance(world_pos, picked.pos) * World::MAP_KM_RATIO;
			int days_cost = int(floor(distance / World::TRAVEL_SPEED));
			s += Format("\n\n%s: %s", txTarget, picked.name.c_str());
			AppendLocationText(picked, s.get_ref());
			cstring cost;
			if(days_cost == 0)
				cost = Format("<1 %s", txDay);
			else if(days_cost == 1)
				cost = Format("1 %s", txDay);
			else
				cost = Format("%d %s", days_cost, txDays);
			s += Format("\n%s: %g km\n%s: %s", txDistance, ceil(distance * 10) / 10, txTravelTime, cost);
		}
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(picked.pos.x - 32.f, picked.pos.y + 32.f)));
		GUI.DrawSpriteTransform(tSelected[0], mat, 0xAAFFFFFF);
	}
	else if(c_pos_valid)
	{
		float distance = Vec2::Distance(world_pos, c_pos) * World::MAP_KM_RATIO;
		int days_cost = int(floor(distance / World::TRAVEL_SPEED));
		cstring cost;
		if(days_cost == 0)
			cost = Format("<1 %s", txDay);
		else if(days_cost == 1)
			cost = Format("1 %s", txDay);
		else
			cost = Format("%d %s", days_cost, txDays);
		s += Format("\n\n%s:\n%s: %g km\n%s: %s", txTarget, txDistance, ceil(distance * 10) / 10, txTravelTime, cost);
	}

	// team position
	if(W.GetCurrentLocationIndex() == -1)
	{
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(world_pos.x - 8, world_pos.y + 8)));
		GUI.DrawSpriteTransform(tMover, mat, 0xBBFFFFFF);
	}

	// line from team to target position
	Vec2 target_pos;
	bool ok = false;
	if(picked_location != -1 && picked_location != W.GetCurrentLocationIndex())
	{
		target_pos = W.locations[picked_location]->pos;
		ok = true;
	}
	else if(W.GetState() != World::State::ON_MAP)
	{
		target_pos = W.GetTargetPos();
		ok = true;
	}
	else if(c_pos_valid)
	{
		target_pos = c_pos;
		ok = true;
	}
	if(ok)
	{
		Vec2 pts[2] = { WorldPosToScreen(world_pos), WorldPosToScreen(target_pos) };
		GUI.LineBegin();
		GUI.DrawLine(pts, 1, 0xAA000000);
		GUI.LineEnd();
	}

	// encounter chance
	if(game.devmode && Net::IsLocal())
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)W.GetEncounterChance() - 25)) * 100 / 500), W.GetEncounterChance());

	// tracking arrow
	if(tracking != -1)
	{
		Vec2 pos = W.GetLocation(tracking)->pos;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom / 2, zoom / 2), nullptr, 0.f, &WorldPosToScreen(Vec2(pos.x - 40.f, pos.y + 40.f)));
		GUI.DrawSpriteTransform(tTrackingArrow, mat);
	}

	// side images
	GUI.DrawSpriteRect(tSide, Rect(GUI.wnd_size.x * 2 / 3, 0, GUI.wnd_size.x, GUI.wnd_size.y));

	// magnifying glass & combo box
	Rect rect(int(float(GUI.wnd_size.x) * 2 / 3 + 16.f*GUI.wnd_size.x / 1024),
		int(float(GUI.wnd_size.y) - 138.f * GUI.wnd_size.y / 768));
	rect.p2.x += int(32.f * GUI.wnd_size.x / 1024);
	rect.p2.y += int(32.f * GUI.wnd_size.y / 768);
	GUI.DrawSpriteRect(tMagnifyingGlass, rect);
	combo_box.Draw();

	// text
	rect = Rect(int(float(GUI.wnd_size.x) * 2 / 3 + 16.f * GUI.wnd_size.x / 1024),
		int(94.f * GUI.wnd_size.y / 768),
		int(float(GUI.wnd_size.x) - 16.f * GUI.wnd_size.x / 1024),
		int(float(GUI.wnd_size.y) - 90.f * GUI.wnd_size.y / 768));
	GUI.DrawText(GUI.default_font, s, 0, Color::Black, rect);

	if(game.end_of_game)
	{
		// czarne t³o
		int color;
		if(game.death_fade < 1.f)
			color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
		else
			color = Color::White;

		GUI.DrawSpriteFull(game.tCzern, color);

		// obrazek
		GUI.DrawSprite(game.tEmerytura, Center(Texture::GetSize(game.tEmerytura)), color);

		// tekst
		cstring text = Format(txGameTimeout, game.pc->kills, GameStats::Get().total_kills - game.pc->kills);
		Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
		GUI.DrawText(GUI.default_font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
	}

	if(game.gui->mp_box->visible)
		game.gui->mp_box->Draw();

	if(game.gui->journal->visible)
		game.gui->journal->Draw();

	game.gui->messages->Draw();
}

//=================================================================================================
void WorldMapGui::Update(float dt)
{
	c_pos_valid = false;

	MpBox* mp_box = game.gui->mp_box;
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

	if(game.end_of_game)
		return;

	if(game.gui->journal->visible)
	{
		game.gui->journal->focus = true;
		game.gui->journal->Update(dt);
	}
	if(!GUI.HaveDialog() && !(mp_box->visible && mp_box->itb.focus) && Key.Focus() && !combo_box.focus && game.death_screen == 0
		&& GKey.PressedRelease(GK_JOURNAL))
	{
		if(game.gui->journal->visible)
			game.gui->journal->Hide();
		else
			game.gui->journal->Show();
	}

	if(focus && !game.gui->journal->visible)
	{
		if(!combo_box.focus && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			game.gui->mp_box->visible = !game.gui->mp_box->visible;

		if(game.gui->mp_box->have_focus && combo_box.focus)
			combo_box.LostFocus();

		combo_box.mouse_focus = focus;
		if(combo_box.IsInside(GUI.cursor_pos) && Key.PressedRelease(VK_LBUTTON))
			combo_box.GainFocus();
		combo_box.Update(dt);

		zoom = Clamp(zoom + 0.1f * Key.GetMouseWheel(), 0.5f, 2.f);
		if(clicked)
		{
			offset -= Vec2(Key.GetMouseDif()) / (float(W.world_size) / MAP_IMG_SIZE * zoom);
			if(offset.x < 0)
				offset.x = 0;
			if(offset.y < 0)
				offset.y = 0;
			if(offset.x > MAP_IMG_SIZE)
				offset.x = MAP_IMG_SIZE;
			if(offset.y > MAP_IMG_SIZE)
				offset.y = MAP_IMG_SIZE;
			if(Key.Up(VK_RBUTTON))
				clicked = false;
		}
		if(Key.Pressed(VK_RBUTTON))
		{
			clicked = true;
			follow = false;
			tracking = -1;
			combo_box.LostFocus();
		}

		if(!combo_box.focus && Key.Down(VK_SPACE))
		{
			follow = true;
			tracking = -1;
		}
	}

	if(follow)
		CenterView(dt);
	else if(tracking != -1)
		CenterView(dt, &W.GetLocation(tracking)->pos);

	if(W.travel_location_index != -1)
		picked_location = W.travel_location_index;

	if(W.GetState() == World::State::TRAVEL)
	{
		if(game.paused || (!Net::IsOnline() && GUI.HavePauseDialog()))
			return;

		bool stop = false;
		if(focus && Key.Focus() && GUI.cursor_pos.x < GUI.wnd_size.x * 2 / 3 && Key.PressedRelease(VK_LBUTTON))
		{
			if(Team.IsLeader())
			{
				W.StopTravel(W.GetWorldPos(), true);
				stop = true;
			}
			else
				game.gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
		}
		if(!stop)
			W.UpdateTravel(dt);
	}
	else if(W.GetState() != World::State::ENCOUNTER && !game.gui->journal->visible)
	{
		Vec2 cursor_pos(float(GUI.cursor_pos.x), float(GUI.cursor_pos.y));
		Location* loc = nullptr;
		float dist = 17.f;
		int index;

		c_pos = (Vec2(GUI.cursor_pos) - GetCameraCenter()) / zoom+offset * float(W.world_size) / MAP_IMG_SIZE;
		c_pos.y = float(W.world_size) - c_pos.y;
		if(focus && c_pos.x > 0 && c_pos.y > 0 && c_pos.x < W.world_size && c_pos.y < W.world_size && GUI.cursor_pos.x < GUI.wnd_size.x * 2 / 3)
		{
			c_pos_valid = true;
			if(!Key.Down(VK_SHIFT))
			{
				int i = 0;
				for(vector<Location*>::iterator it = W.locations.begin(), end = W.locations.end(); it != end; ++it, ++i)
				{
					if(!*it || (*it)->state == LS_UNKNOWN || (*it)->state == LS_HIDDEN)
						continue;
					Vec2 pt = WorldPosToScreen((*it)->pos);
					float dist2 = Vec2::Distance(pt, cursor_pos) / zoom;
					if(dist2 < dist)
					{
						loc = *it;
						dist = dist2;
						index = i;
					}
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
					combo_box.LostFocus();
					if(Team.IsLeader())
					{
						if(picked_location != W.GetCurrentLocationIndex())
						{
							W.Travel(picked_location, true);
							follow = true;
							tracking = -1;
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
						game.gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game.devmode && picked_location != W.GetCurrentLocationIndex() && !combo_box.focus && Key.PressedRelease('T'))
				{
					if(Team.IsLeader())
					{
						W.Warp(picked_location);
						follow = true;
						tracking = -1;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_TRAVEL;
							c.id = picked_location;
						}
					}
					else
						game.gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
		else
		{
			picked_location = -1;
			if(c_pos_valid && W.GetState() == World::State::ON_MAP && Key.Focus())
			{
				if(Key.PressedRelease(VK_LBUTTON))
				{
					combo_box.LostFocus();
					if(Team.IsLeader())
					{
						W.TravelPos(c_pos, true);
						follow = true;
						tracking = -1;
					}
					else
						game.gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game.devmode && !combo_box.focus && Key.PressedRelease('T'))
				{
					if(Team.IsLeader())
					{
						W.WarpPos(c_pos);
						follow = true;
						tracking = -1;
						if(Net::IsOnline())
						{
							NetChange& c = Add1(Net::changes);
							c.type = NetChange::CHEAT_TRAVEL_POS;
							c.pos.x = c_pos.x;
							c.pos.y = c_pos.y;
						}
					}
					else
						game.gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
	}

	if(focus && Key.PressedRelease(VK_LBUTTON))
		combo_box.LostFocus();

	game.gui->messages->Update(dt);
}

//=================================================================================================
void WorldMapGui::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_LostFocus:
		game.gui->mp_box->LostFocus();
		combo_box.LostFocus();
		break;
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		{
			Rect rect(int(float(GUI.wnd_size.x) * 2 / 3 + 52.f*GUI.wnd_size.x / 1024),
				int(float(GUI.wnd_size.y) - 138.f * GUI.wnd_size.y / 768));
			rect.p2.x = int(float(GUI.wnd_size.x) - 20.f * GUI.wnd_size.y / 1024);
			rect.p2.y += int(32.f * GUI.wnd_size.y / 768);
			combo_box.pos = rect.p1;
			combo_box.global_pos = combo_box.pos;
			combo_box.size = rect.Size();
			if(e == GuiEvent_Show)
			{
				tracking = -1;
				clicked = false;
				CenterView(-1.f);
				follow = (W.GetState() == World::State::TRAVEL);
				combo_box.Reset();
				c_pos_valid = false;
			}
		}
		break;
	case ComboBox::Event_TextChanged:
		{
			LocalString str = combo_box.GetText();
			Trim(str.get_ref());
			combo_box.ClearItems();
			if(str.length() < 3)
				break;
			int count = 0;
			for(Location* loc : W.GetLocations())
			{
				if(!loc || loc->state == LS_HIDDEN || loc->state == LS_UNKNOWN)
					continue;
				if(StringContainsStringI(loc->name.c_str(), str.c_str()))
				{
					LocationElement* le = LocationElement::Get();
					le->loc = loc;
					combo_box.AddItem(le);
					++count;
					if(count == 5)
						break;
				}
			}
		}
		break;
	case ComboBox::Event_Selected:
		{
			LocationElement* le = static_cast<LocationElement*>(combo_box.GetSelectedItem());
			tracking = le->loc->index;
			follow = false;
		}
		break;
	}
}

//=================================================================================================
void WorldMapGui::Save(FileWriter& f)
{
	f << zoom;
}

//=================================================================================================
void WorldMapGui::Load(FileReader& f)
{
	if(LOAD_VERSION >= V_0_8)
		f >> zoom;
}

//=================================================================================================
void WorldMapGui::Clear()
{
	zoom = 1.2f;
	follow = false;
	tracking = -1;
	clicked = false;
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
	LocalVector3<pair<string*, uint>> items;

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
				items.push_back(pair<string*, uint>(s, 1u));
			}
		}
	}

	// sort
	if(items.empty())
		return;
	std::sort(items.begin(), items.end(), [](const pair<string*, uint>& a, const pair<string*, uint>& b) { return *a.first < *b.first; });

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
	dialog_enc = GUI.ShowDialog(info);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ENCOUNTER;
		c.str = StringPool.Get();
		*c.str = text;

		// disable button when server is not leader
		if(!Team.IsLeader())
			dialog_enc->bts[0].state = Button::DISABLED;
	}
}

//=================================================================================================
Vec2 WorldMapGui::WorldPosToScreen(const Vec2& pt) const
{
	return Vec2(pt.x, float(W.world_size) - pt.y) * zoom
		+ GetCameraCenter()
		- offset * zoom * float(W.world_size) / MAP_IMG_SIZE;
}

//=================================================================================================
void WorldMapGui::CenterView(float dt, const Vec2* target)
{
	Vec2 p = (target ? *target : W.world_pos);
	Vec2 pos = Vec2(p.x, float(W.world_size) - p.y) / (float(W.world_size) / MAP_IMG_SIZE);
	if(dt >= 0.f)
		offset += (pos - offset) * dt * 2.f;
	else
		offset = pos;
}

//=================================================================================================
Vec2 WorldMapGui::GetCameraCenter() const
{
	return Vec2(float(GUI.wnd_size.x) / 3, float(GUI.wnd_size.y) / 2);
}
