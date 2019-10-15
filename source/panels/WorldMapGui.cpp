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
#include "GameGui.h"
#include "LevelGui.h"
#include "MpBox.h"
#include "GameMessages.h"
#include "DialogBox.h"
#include "World.h"
#include "Level.h"
#include "GameStats.h"
#include "ResourceManager.h"
#include "Team.h"
#include "SaveState.h"
#include "Render.h"
#include "Engine.h"

const float MAP_IMG_SIZE = 512.f;

struct LocationElement : public GuiElement, public ObjectPoolProxy<LocationElement>
{
	Location* loc;
	cstring ToString() override { return loc->name.c_str(); }
};

//=================================================================================================
WorldMapGui::WorldMapGui() : zoom(1.2f), offset(0.f, 0.f), follow(false)
{
	focusable = true;
	visible = false;
	combo_box.parent = this;
	combo_box.destructor = [](GuiElement* e) { static_cast<LocationElement*>(e)->Free(); };
}

//=================================================================================================
void WorldMapGui::LoadLanguage()
{
	Language::Section s = Language::GetSection("WorldMap");
	txWorldDate = s.Get("worldDate");
	txCurrentLoc = s.Get("currentLoc");
	txCitizens = s.Get("citizens");
	txAvailable = s.Get("available");
	txTarget = s.Get("target");
	txDistance = s.Get("distance");
	txTravelTime = s.Get("travelTime");
	txDay = s.Get("day");
	txDays = s.Get("days");
	txOnlyLeaderCanTravel = s.Get("onlyLeaderCanTravel");
	txBuildings = s.Get("buildings");
}

//=================================================================================================
void WorldMapGui::LoadData()
{
	tMapIcon[LI_CAMP] = res_mgr->Load<Texture>("camp.png");
	tMapIcon[LI_VILLAGE] = res_mgr->Load<Texture>("village.png");
	tMapIcon[LI_CITY] = res_mgr->Load<Texture>("city.png");
	tMapIcon[LI_DUNGEON] = res_mgr->Load<Texture>("dungeon.png");
	tMapIcon[LI_CRYPT] = res_mgr->Load<Texture>("crypt.png");
	tMapIcon[LI_CAVE] = res_mgr->Load<Texture>("cave.png");
	tMapIcon[LI_FOREST] = res_mgr->Load<Texture>("forest.png");
	tMapIcon[LI_MOONWELL] = res_mgr->Load<Texture>("moonwell.png");
	tMapIcon[LI_TOWER] = res_mgr->Load<Texture>("tower.png");
	tMapIcon[LI_LABYRINTH] = res_mgr->Load<Texture>("labyrinth.png");
	tMapIcon[LI_MINE] = res_mgr->Load<Texture>("mine.png");
	tMapIcon[LI_SAWMILL] = res_mgr->Load<Texture>("sawmill.png");
	tMapIcon[LI_DUNGEON2] = res_mgr->Load<Texture>("dungeon2.png");
	tMapIcon[LI_ACADEMY] = res_mgr->Load<Texture>("academy.png");
	tMapIcon[LI_CAPITAL] = res_mgr->Load<Texture>("capital.png");
	tWorldMap = res_mgr->Load<Texture>("worldmap.jpg");
	tSelected[0] = res_mgr->Load<Texture>("selected.png");
	tSelected[1] = res_mgr->Load<Texture>("selected2.png");
	tMover = res_mgr->Load<Texture>("mover.png");
	tMapBg = res_mgr->Load<Texture>("old_map.png");
	tEnc = res_mgr->Load<Texture>("enc.png");
	tSide = res_mgr->Load<Texture>("worldmap_side.png");
	tMagnifyingGlass = res_mgr->Load<Texture>("magnifying-glass-icon.png");
	tTrackingArrow = res_mgr->Load<Texture>("tracking_arrow.png");
}

//=================================================================================================
void WorldMapGui::Draw(ControlDrawData*)
{
	// background
	Rect rect0(Int2::Zero, engine->GetWindowSize());
	render->SetTextureAddressMode(TEX_ADR_WRAP);
	gui->DrawSpriteRectPart(tMapBg, rect0, rect0);
	render->SetTextureAddressMode(TEX_ADR_CLAMP);

	// map
	Matrix mat = Matrix::Transform2D(&offset, 0.f, &Vec2(float(world->world_size) / MAP_IMG_SIZE * zoom), nullptr, 0.f, &(GetCameraCenter() - offset));
	gui->DrawSpriteTransform(tWorldMap, mat);

	// debug tiles
	if(!combo_box.focus && !gui->HaveDialog() && GKey.DebugKey(Key::S) && !Net::IsClient())
	{
		const vector<int>& tiles = world->GetTiles();
		int ts = world->world_size / World::TILE_SIZE;
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
				gui->DrawArea(c, rect);
			}
		}
	}

	// location images
	for(vector<Location*>::iterator it = world->locations.begin(), end = world->locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;
		Location& loc = **it;
		if(loc.state == LS_UNKNOWN || loc.state == LS_HIDDEN)
			continue;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(loc.pos.x - 16.f, loc.pos.y + 16.f)));
		gui->DrawSpriteTransform(tMapIcon[loc.image], mat, loc.state == LS_KNOWN ? Color(0x70707070) : Color::White);
	}

	// encounter locations
	if(game->devmode)
	{
		for(const Encounter* enc : world->GetEncounters())
		{
			if(enc)
			{
				mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(enc->pos.x - 16.f, enc->pos.y + 16.f)));
				gui->DrawSpriteTransform(tEnc, mat);
			}
		}
	}

	LocalString s = Format(txWorldDate, world->GetDate());
	const Vec2& world_pos = world->GetWorldPos();

	// current location description
	if(world->GetCurrentLocation())
	{
		Location& current = *world->GetCurrentLocation();
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(current.pos.x - 32.f, current.pos.y + 32.f)));
		gui->DrawSpriteTransform(tSelected[1], mat, 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		AppendLocationText(current, s.get_ref());
	}

	// target location description
	if(picked_location != -1)
	{
		Location& picked = *world->locations[picked_location];
		if(picked_location != world->GetCurrentLocationIndex())
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
		gui->DrawSpriteTransform(tSelected[0], mat, 0xAAFFFFFF);
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
	if(world->GetCurrentLocationIndex() == -1)
	{
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom, zoom), nullptr, 0.f, &WorldPosToScreen(Vec2(world_pos.x - 8, world_pos.y + 8)));
		gui->DrawSpriteTransform(tMover, mat, 0xBBFFFFFF);
	}

	// line from team to target position
	Vec2 target_pos;
	bool ok = false;
	if(picked_location != -1 && picked_location != world->GetCurrentLocationIndex())
	{
		target_pos = world->locations[picked_location]->pos;
		ok = true;
	}
	else if(world->GetState() != World::State::ON_MAP)
	{
		target_pos = world->GetTargetPos();
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
		gui->LineBegin();
		gui->DrawLine(pts, 1, 0xAA000000);
		gui->LineEnd();
	}

	// encounter chance
	if(game->devmode && Net::IsLocal())
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)world->GetEncounterChance() - 25)) * 100 / 500), world->GetEncounterChance());

	// tracking arrow
	if(tracking != -1)
	{
		Vec2 pos = world->GetLocation(tracking)->pos;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(zoom / 2, zoom / 2), nullptr, 0.f, &WorldPosToScreen(Vec2(pos.x - 40.f, pos.y + 40.f)));
		gui->DrawSpriteTransform(tTrackingArrow, mat);
	}

	// side images
	gui->DrawSpriteRect(tSide, Rect(gui->wnd_size.x * 2 / 3, 0, gui->wnd_size.x, gui->wnd_size.y));

	// magnifying glass & combo box
	Rect rect(int(float(gui->wnd_size.x) * 2 / 3 + 16.f*gui->wnd_size.x / 1024),
		int(float(gui->wnd_size.y) - 138.f * gui->wnd_size.y / 768));
	rect.p2.x += int(32.f * gui->wnd_size.x / 1024);
	rect.p2.y += int(32.f * gui->wnd_size.y / 768);
	gui->DrawSpriteRect(tMagnifyingGlass, rect);
	combo_box.Draw();

	// text
	rect = Rect(int(float(gui->wnd_size.x) * 2 / 3 + 16.f * gui->wnd_size.x / 1024),
		int(94.f * gui->wnd_size.y / 768),
		int(float(gui->wnd_size.x) - 16.f * gui->wnd_size.x / 1024),
		int(float(gui->wnd_size.y) - 90.f * gui->wnd_size.y / 768));
	gui->DrawText(GameGui::font, s, 0, Color::Black, rect);

	if(game->end_of_game)
		game_gui->level_gui->DrawEndOfGameScreen();

	if(game_gui->mp_box->visible)
		game_gui->mp_box->Draw();

	if(game_gui->journal->visible)
		game_gui->journal->Draw();

	game_gui->messages->Draw();
}

//=================================================================================================
void WorldMapGui::Update(float dt)
{
	c_pos_valid = false;

	MpBox* mp_box = game_gui->mp_box;
	if(mp_box->visible)
	{
		mp_box->focus = true;
		mp_box->Event(GuiEvent_GainFocus);
		if(gui->HaveDialog())
			mp_box->LostFocus();
		mp_box->Update(dt);

		if(input->Focus() && !mp_box->have_focus && !gui->HaveDialog() && input->PressedRelease(Key::Enter))
			mp_box->have_focus = true;
	}

	if(game->end_of_game)
		return;

	if(game_gui->journal->visible)
	{
		game_gui->journal->focus = true;
		game_gui->journal->Update(dt);
	}
	if(!gui->HaveDialog() && !(mp_box->visible && mp_box->itb.focus) && input->Focus() && !combo_box.focus && game->death_screen == 0
		&& GKey.PressedRelease(GK_JOURNAL))
	{
		if(game_gui->journal->visible)
			game_gui->journal->Hide();
		else
			game_gui->journal->Show();
	}

	if(focus && !game_gui->journal->visible)
	{
		if(!combo_box.focus && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			game_gui->mp_box->visible = !game_gui->mp_box->visible;

		if(game_gui->mp_box->have_focus && combo_box.focus)
			combo_box.LostFocus();

		combo_box.mouse_focus = focus;
		if(combo_box.IsInside(gui->cursor_pos) && input->PressedRelease(Key::LeftButton))
			combo_box.GainFocus();
		combo_box.Update(dt);

		zoom = Clamp(zoom + 0.1f * input->GetMouseWheel(), 0.5f, 2.f);
		if(clicked)
		{
			offset -= Vec2(input->GetMouseDif()) / (float(world->world_size) / MAP_IMG_SIZE * zoom);
			if(offset.x < 0)
				offset.x = 0;
			if(offset.y < 0)
				offset.y = 0;
			if(offset.x > MAP_IMG_SIZE)
				offset.x = MAP_IMG_SIZE;
			if(offset.y > MAP_IMG_SIZE)
				offset.y = MAP_IMG_SIZE;
			if(input->Up(Key::RightButton))
				clicked = false;
		}
		if(input->Pressed(Key::RightButton))
		{
			clicked = true;
			follow = false;
			tracking = -1;
			combo_box.LostFocus();
		}

		if(!combo_box.focus && input->Down(Key::Spacebar))
		{
			follow = true;
			tracking = -1;
		}
	}

	if(follow)
		CenterView(dt);
	else if(tracking != -1)
		CenterView(dt, &world->GetLocation(tracking)->pos);

	if(world->travel_location_index != -1)
		picked_location = world->travel_location_index;

	if(world->GetState() == World::State::TRAVEL)
	{
		if(game->paused || (!Net::IsOnline() && gui->HavePauseDialog()))
			return;

		bool stop = false;
		if(focus && input->Focus() && gui->cursor_pos.x < gui->wnd_size.x * 2 / 3 && input->PressedRelease(Key::LeftButton))
		{
			if(team->IsLeader())
			{
				world->StopTravel(world->GetWorldPos(), true);
				stop = true;
			}
			else
				game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
		}
		if(!stop)
			world->UpdateTravel(dt);
	}
	else if(world->GetState() != World::State::ENCOUNTER && !game_gui->journal->visible)
	{
		Vec2 cursor_pos(float(gui->cursor_pos.x), float(gui->cursor_pos.y));
		Location* loc = nullptr;
		float dist = 17.f;
		int index;

		c_pos = (Vec2(gui->cursor_pos) - GetCameraCenter()) / zoom + offset * float(world->world_size) / MAP_IMG_SIZE;
		c_pos.y = float(world->world_size) - c_pos.y;
		if(focus && c_pos.x > 0 && c_pos.y > 0 && c_pos.x < world->world_size && c_pos.y < world->world_size && gui->cursor_pos.x < gui->wnd_size.x * 2 / 3)
		{
			c_pos_valid = true;
			if(!input->Down(Key::Shift))
			{
				int i = 0;
				for(vector<Location*>::iterator it = world->locations.begin(), end = world->locations.end(); it != end; ++it, ++i)
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
			if(world->GetState() == World::State::ON_MAP && input->Focus())
			{
				if(input->PressedRelease(Key::LeftButton))
				{
					combo_box.LostFocus();
					if(team->IsLeader())
					{
						if(picked_location != world->GetCurrentLocationIndex())
						{
							world->Travel(picked_location, true);
							follow = true;
							tracking = -1;
						}
						else
						{
							if(Net::IsLocal())
								game->EnterLocation();
							else
								Net::PushChange(NetChange::ENTER_LOCATION);
						}
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game->devmode && picked_location != world->GetCurrentLocationIndex() && !combo_box.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->Warp(picked_location, true);
						follow = true;
						tracking = -1;
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
		else
		{
			picked_location = -1;
			if(c_pos_valid && world->GetState() == World::State::ON_MAP && input->Focus())
			{
				if(input->PressedRelease(Key::LeftButton))
				{
					combo_box.LostFocus();
					if(team->IsLeader())
					{
						world->TravelPos(c_pos, true);
						follow = true;
						tracking = -1;
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game->devmode && !combo_box.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->WarpPos(c_pos, true);
						follow = true;
						tracking = -1;
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
	}

	if(focus && input->PressedRelease(Key::LeftButton))
		combo_box.LostFocus();

	game_gui->messages->Update(dt);
}

//=================================================================================================
void WorldMapGui::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_LostFocus:
		game_gui->mp_box->LostFocus();
		combo_box.LostFocus();
		break;
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		{
			Rect rect(int(float(gui->wnd_size.x) * 2 / 3 + 52.f*gui->wnd_size.x / 1024),
				int(float(gui->wnd_size.y) - 138.f * gui->wnd_size.y / 768));
			rect.p2.x = int(float(gui->wnd_size.x) - 20.f * gui->wnd_size.y / 1024);
			rect.p2.y += int(32.f * gui->wnd_size.y / 768);
			combo_box.pos = rect.p1;
			combo_box.global_pos = combo_box.pos;
			combo_box.size = rect.Size();
			if(e == GuiEvent_Show)
			{
				tracking = -1;
				clicked = false;
				CenterView(-1.f);
				follow = (world->GetState() == World::State::TRAVEL);
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
			for(Location* loc : world->GetLocations())
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
	if(game->devmode && Net::IsLocal())
	{
		s += " (";
		if(loc.type == L_DUNGEON)
		{
			InsideLocation* inside = (InsideLocation*)&loc;
			s += Format("%s, %s, st %d, levels %d, ",
				g_base_locations[inside->target].name, loc.group->id.c_str(), loc.st, inside->GetLastLevel() + 1);
		}
		else if(loc.type == L_OUTSIDE || loc.type == L_CAMP || loc.type == L_CAVE)
			s += Format("%s, st %d, ", loc.group->id.c_str(), loc.st);
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
		if(IsSet(b.building->flags, Building::LIST))
		{
			bool found = false;
			for(auto& i : items)
			{
				if(*i.first == b.building->name)
				{
					++i.second;
					found = true;
					break;
				}
			}
			if(!found)
			{
				string* s = StringPool.Get();
				*s = b.building->name;
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
	info.event = DialogEvent(game, &Game::Event_RandomEncounter);
	info.name = "encounter";
	info.order = ORDER_TOPMOST;
	info.parent = this;
	info.pause = true;
	info.text = text;
	info.type = DIALOG_OK;
	dialog_enc = gui->ShowDialog(info);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::ENCOUNTER;
		c.str = StringPool.Get();
		*c.str = text;

		// disable button when server is not leader
		if(!team->IsLeader())
			dialog_enc->bts[0].state = Button::DISABLED;
	}
}

//=================================================================================================
Vec2 WorldMapGui::WorldPosToScreen(const Vec2& pt) const
{
	return Vec2(pt.x, float(world->world_size) - pt.y) * zoom
		+ GetCameraCenter()
		- offset * zoom * float(world->world_size) / MAP_IMG_SIZE;
}

//=================================================================================================
void WorldMapGui::CenterView(float dt, const Vec2* target)
{
	Vec2 p = (target ? *target : world->world_pos);
	Vec2 pos = Vec2(p.x, float(world->world_size) - p.y) / (float(world->world_size) / MAP_IMG_SIZE);
	if(dt >= 0.f)
		offset += (pos - offset) * dt * 2.f;
	else
		offset = pos;
}

//=================================================================================================
Vec2 WorldMapGui::GetCameraCenter() const
{
	return Vec2(float(gui->wnd_size.x) / 3, float(gui->wnd_size.y) / 2);
}
