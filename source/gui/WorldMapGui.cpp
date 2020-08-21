#include "Pch.h"
#include "WorldMapGui.h"

#include "City.h"
#include "Encounter.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameStats.h"
#include "InsideLocation.h"
#include "Journal.h"
#include "Language.h"
#include "Level.h"
#include "LevelGui.h"
#include "MpBox.h"
#include "SaveState.h"
#include "Team.h"
#include "World.h"

#include <DialogBox.h>
#include <ResourceManager.h>

const float MAP_IMG_SIZE = 512.f;

enum ButtonId
{
	ButtonJournal = GuiEvent_Custom + 10,
	ButtonTalkBox
};

struct LocationElement : public GuiElement, public ObjectPoolProxy<LocationElement>
{
	Location* loc;
	cstring ToString() override { return loc->name.c_str(); }
};

layout::Button custom_layout;

//=================================================================================================
WorldMapGui::WorldMapGui() : zoom(1.2f), offset(0.f, 0.f), follow(false)
{
	focusable = true;
	visible = false;
	combo_search.parent = this;
	combo_search.destructor = [](GuiElement* e) { static_cast<LocationElement*>(e)->SafeFree(); };
	tooltip.Init(TooltipController::Callback(this, &WorldMapGui::GetTooltip));

	custom_layout.tex[0] = AreaLayout(Color::None, Color::Black);
	custom_layout.tex[1] = AreaLayout(Color::Alpha(80), Color::Black);
	custom_layout.tex[2] = AreaLayout(Color::Alpha(120), Color::Black);
	custom_layout.font = game_gui->font;
	custom_layout.padding = 0;

	buttons[0].id = ButtonJournal;
	buttons[0].parent = this;
	buttons[0].SetLayout(&custom_layout);

	buttons[1].id = ButtonTalkBox;
	buttons[1].parent = this;
	buttons[1].SetLayout(&custom_layout);
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
	tMapIcon[LI_HUNTERS_CAMP] = res_mgr->Load<Texture>("hunters_camp.png");
	tMapIcon[LI_HILLS] = res_mgr->Load<Texture>("hills.png");
	tWorldMap = res_mgr->Load<Texture>("worldmap.jpg");
	tSelected[0] = res_mgr->Load<Texture>("selected.png");
	tSelected[1] = res_mgr->Load<Texture>("selected2.png");
	tMover = res_mgr->Load<Texture>("mover.png");
	tMapBg = res_mgr->Load<Texture>("old_map.png");
	tEnc = res_mgr->Load<Texture>("enc.png");
	tSide = res_mgr->Load<Texture>("worldmap_side.png");
	tMagnifyingGlass = res_mgr->Load<Texture>("magnifying-glass-icon.png");
	tTrackingArrow = res_mgr->Load<Texture>("tracking_arrow.png");
	buttons[0].img = res_mgr->Load<Texture>("bt_journal.png");
	buttons[1].img = res_mgr->Load<Texture>("bt_talk.png");
}

//=================================================================================================
void WorldMapGui::Draw(ControlDrawData*)
{
	// background
	gui->DrawSpriteFullWrap(tMapBg);

	// map
	Matrix mat = Matrix::Transform2D(&offset, 0.f, &Vec2(float(world->world_size) / MAP_IMG_SIZE * zoom), nullptr, 0.f, &(GetCameraCenter() - offset));
	gui->DrawSpriteTransform(tWorldMap, mat);

	// debug tiles
	if(!combo_search.focus && !gui->HaveDialog() && GKey.DebugKey(Key::S) && !Net::IsClient())
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
		gui->DrawSpriteTransform(tMapIcon[loc.image], mat, loc.state == LS_KNOWN ? Color::Alpha(128) : Color::White);
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
		gui->DrawLine(WorldPosToScreen(world_pos), WorldPosToScreen(target_pos), 0xAA000000);

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

	// buttons
	for(int i = 0; i < 2; ++i)
		buttons[i].Draw();

	// magnifying glass & combo box
	Rect rect(int(float(gui->wnd_size.x) * 2 / 3 + 16.f*gui->wnd_size.x / 1024),
		int(float(gui->wnd_size.y) - 138.f * gui->wnd_size.y / 768));
	rect.p2.x += int(32.f * gui->wnd_size.x / 1024);
	rect.p2.y += int(32.f * gui->wnd_size.y / 768);
	gui->DrawSpriteRect(tMagnifyingGlass, rect);
	combo_search.Draw();

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

	tooltip.Draw();
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

	int group = -1, id = -1;

	if(game_gui->journal->visible)
	{
		game_gui->journal->focus = focus;
		game_gui->journal->Update(dt);
	}
	if(!gui->HaveDialog() && !(mp_box->visible && mp_box->itb.focus) && input->Focus() && !combo_search.focus && game->death_screen == 0)
	{
		if(GKey.PressedRelease(GK_JOURNAL))
		{
			if(game_gui->journal->visible)
				game_gui->journal->Hide();
			else
				game_gui->journal->Show();
		}
		if(GKey.PressedRelease(GK_MAP_SEARCH))
		{
			combo_search.focus = true;
			combo_search.Event(GuiEvent_GainFocus);
		}
	}

	if(focus && !game_gui->journal->visible)
	{
		if(!combo_search.focus && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			game_gui->mp_box->visible = !game_gui->mp_box->visible;

		if(game_gui->mp_box->have_focus && combo_search.focus)
			combo_search.LostFocus();

		combo_search.mouse_focus = focus;
		if(combo_search.IsInside(gui->cursor_pos) && input->PressedRelease(Key::LeftButton))
			combo_search.GainFocus();
		combo_search.Update(dt);

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
			combo_search.LostFocus();
		}

		if(!combo_search.focus && input->Down(Key::Spacebar))
		{
			follow = true;
			fast_follow = true;
			tracking = -1;
		}

		// buttons
		for(int i = 0; i < 2; ++i)
		{
			buttons[i].mouse_focus = focus;
			buttons[i].Update(dt);
			if(!combo_search.focus && buttons[i].IsInside(gui->cursor_pos))
			{
				group = 0;
				id = (i == 0 ? GK_JOURNAL : GK_TALK_BOX);
			}
		}

		// search location tooltip
		Rect rect(int(float(gui->wnd_size.x) * 2 / 3 + 16.f * gui->wnd_size.x / 1024),
			int(float(gui->wnd_size.y) - 138.f * gui->wnd_size.y / 768));
		rect.p2.x += int(32.f * gui->wnd_size.x / 1024);
		rect.p2.y += int(32.f * gui->wnd_size.y / 768);
		if(rect.IsInside(gui->cursor_pos))
		{
			group = 0;
			id = GK_MAP_SEARCH;
		}
	}

	if(follow)
		CenterView(dt);
	else if(tracking != -1)
	{
		fast_follow = true;
		CenterView(dt, &world->GetLocation(tracking)->pos);
	}

	if(world->GetState() == World::State::TRAVEL)
	{
		if(world->travel_location)
			picked_location = world->travel_location->index;

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
		int index;

		c_pos = (Vec2(gui->cursor_pos) - GetCameraCenter()) / zoom + offset * float(world->world_size) / MAP_IMG_SIZE;
		c_pos.y = float(world->world_size) - c_pos.y;
		if(focus && c_pos.x > 0 && c_pos.y > 0 && c_pos.x < world->world_size && c_pos.y < world->world_size && gui->cursor_pos.x < gui->wnd_size.x * 2 / 3)
		{
			c_pos_valid = true;
			if(!input->Down(Key::Shift))
			{
				float dist = 17.f;
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
					combo_search.LostFocus();
					if(team->IsLeader())
					{
						if(picked_location != world->GetCurrentLocationIndex())
						{
							world->Travel(picked_location, true);
							StartTravel();
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
				else if(game->devmode && picked_location != world->GetCurrentLocationIndex() && !combo_search.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->Warp(picked_location, true);
						StartTravel(true);
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
					combo_search.LostFocus();
					if(team->IsLeader())
					{
						world->TravelPos(c_pos, true);
						StartTravel();
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game->devmode && !combo_search.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->WarpPos(c_pos, true);
						StartTravel(true);
					}
					else
						game_gui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
	}

	if(focus && input->PressedRelease(Key::LeftButton))
		combo_search.LostFocus();

	game_gui->messages->Update(dt);
	tooltip.UpdateTooltip(dt, group, id);
}

//=================================================================================================
void WorldMapGui::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_LostFocus:
		game_gui->mp_box->LostFocus();
		combo_search.LostFocus();
		break;
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		{
			Rect rect(int(float(gui->wnd_size.x) * 2 / 3 + 52.f*gui->wnd_size.x / 1024),
				int(float(gui->wnd_size.y) - 140.f * gui->wnd_size.y / 768));
			rect.p2.x = int(float(gui->wnd_size.x) - 20.f * gui->wnd_size.y / 1024);
			rect.p2.y += int(32.f * gui->wnd_size.y / 768);
			combo_search.pos = rect.p1;
			combo_search.global_pos = combo_search.pos;
			combo_search.size = rect.Size();

			const int img_size = int(32.f * gui->wnd_size.y / 768);
			buttons[0].pos = Int2(rect.p1.x, rect.p1.y - img_size - 5);
			buttons[0].global_pos = buttons[0].pos;
			buttons[0].size = Int2(rect.SizeX() / 2 - 2, img_size);
			buttons[0].force_img_size = Int2(img_size);
			buttons[1].pos = Int2(rect.p1.x + rect.SizeX() / 2 + 2, rect.p1.y - img_size - 5);
			buttons[1].global_pos = buttons[1].pos;
			buttons[1].size = Int2(rect.SizeX() / 2 - 2, img_size);
			buttons[1].force_img_size = Int2(img_size);

			if(e == GuiEvent_Show)
			{
				tracking = -1;
				clicked = false;
				CenterView(-1.f);
				follow = (world->GetState() == World::State::TRAVEL);
				fast_follow = false;
				combo_search.Reset();
				c_pos_valid = false;
				tooltip.Clear();
			}
		}
		break;
	case ComboBox::Event_TextChanged:
		{
			LocalString str = combo_search.GetText();
			Trim(str.get_ref());
			combo_search.ClearItems();
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
					combo_search.AddItem(le);
					++count;
					if(count == 5)
						break;
				}
			}
		}
		break;
	case ComboBox::Event_Selected:
		{
			LocationElement* le = static_cast<LocationElement*>(combo_search.GetSelectedItem());
			tracking = le->loc->index;
			follow = false;
		}
		break;
	case ButtonJournal:
		game_gui->journal->Show();
		break;
	case ButtonTalkBox:
		game_gui->mp_box->visible = !game_gui->mp_box->visible;
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
	dialog_enc = nullptr;
	picked_location = -1;
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
void WorldMapGui::StartTravel(bool fast)
{
	follow = true;
	tracking = -1;
	fast_follow = fast;
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
		offset += (pos - offset) * dt * (fast_follow ? 10.f : 2.f);
	else
		offset = pos;
}

//=================================================================================================
Vec2 WorldMapGui::GetCameraCenter() const
{
	return Vec2(float(gui->wnd_size.x) / 3, float(gui->wnd_size.y) / 2);
}

//=================================================================================================
void WorldMapGui::GetTooltip(TooltipController* tooltip, int group, int id, bool refresh)
{
	if(id == -1)
	{
		tooltip->anything = false;
		return;
	}
	else
	{
		tooltip->anything = true;
		tooltip->text = game->GetShortcutText((GAME_KEYS)id);
	}
}
