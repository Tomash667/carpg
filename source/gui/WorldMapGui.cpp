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

layout::Button customLayout;

//=================================================================================================
WorldMapGui::WorldMapGui() : zoom(1.2f), offset(0.f, 0.f), follow(false)
{
	focusable = true;
	visible = false;
	comboSearch.parent = this;
	comboSearch.destructor = [](GuiElement* e) { static_cast<LocationElement*>(e)->SafeFree(); };
	tooltip.Init(TooltipController::Callback(this, &WorldMapGui::GetTooltip));

	customLayout.tex[0] = AreaLayout(Color::None, Color::Black);
	customLayout.tex[1] = AreaLayout(Color::Alpha(80), Color::Black);
	customLayout.tex[2] = AreaLayout(Color::Alpha(120), Color::Black);
	customLayout.font = gameGui->font;
	customLayout.padding = 0;

	buttons[0].id = ButtonJournal;
	buttons[0].parent = this;
	buttons[0].SetLayout(&customLayout);

	buttons[1].id = ButtonTalkBox;
	buttons[1].parent = this;
	buttons[1].SetLayout(&customLayout);
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
	tMapIcon[LI_CAMP] = resMgr->Load<Texture>("camp.png");
	tMapIcon[LI_VILLAGE] = resMgr->Load<Texture>("village.png");
	tMapIcon[LI_CITY] = resMgr->Load<Texture>("city.png");
	tMapIcon[LI_DUNGEON] = resMgr->Load<Texture>("dungeon.png");
	tMapIcon[LI_CRYPT] = resMgr->Load<Texture>("crypt.png");
	tMapIcon[LI_CAVE] = resMgr->Load<Texture>("cave.png");
	tMapIcon[LI_FOREST] = resMgr->Load<Texture>("forest.png");
	tMapIcon[LI_MOONWELL] = resMgr->Load<Texture>("moonwell.png");
	tMapIcon[LI_TOWER] = resMgr->Load<Texture>("tower.png");
	tMapIcon[LI_LABYRINTH] = resMgr->Load<Texture>("labyrinth.png");
	tMapIcon[LI_MINE] = resMgr->Load<Texture>("mine.png");
	tMapIcon[LI_SAWMILL] = resMgr->Load<Texture>("sawmill.png");
	tMapIcon[LI_DUNGEON2] = resMgr->Load<Texture>("dungeon2.png");
	tMapIcon[LI_ACADEMY] = resMgr->Load<Texture>("academy.png");
	tMapIcon[LI_CAPITAL] = resMgr->Load<Texture>("capital.png");
	tMapIcon[LI_HUNTERS_CAMP] = resMgr->Load<Texture>("hunters_camp.png");
	tMapIcon[LI_HILLS] = resMgr->Load<Texture>("hills.png");
	tMapIcon[LI_VILLAGE_DESTROYED] = resMgr->Load<Texture>("village_destroyed.png");
	tWorldMap = resMgr->Load<Texture>("worldmap.jpg");
	tSelected[0] = resMgr->Load<Texture>("selected.png");
	tSelected[1] = resMgr->Load<Texture>("selected2.png");
	tMover = resMgr->Load<Texture>("mover.png");
	tMapBg = resMgr->Load<Texture>("old_map.png");
	tEnc = resMgr->Load<Texture>("enc.png");
	tSide = resMgr->Load<Texture>("worldmap_side.png");
	tMagnifyingGlass = resMgr->Load<Texture>("magnifying-glass-icon.png");
	tTrackingArrow = resMgr->Load<Texture>("tracking_arrow.png");
	buttons[0].img = resMgr->Load<Texture>("bt_journal.png");
	buttons[1].img = resMgr->Load<Texture>("bt_talk.png");
}

//=================================================================================================
void WorldMapGui::Draw()
{
	const Vec2 scale(zoom);

	// background
	gui->DrawSpriteFullWrap(tMapBg);

	// map
	{
		const Vec2 scale(float(world->worldSize) / MAP_IMG_SIZE * zoom);
		const Vec2 pos(GetCameraCenter() - offset);
		const Matrix mat = Matrix::Transform2D(&offset, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tWorldMap, mat);
	}

	// debug tiles
	if(!comboSearch.focus && !gui->HaveDialog() && GKey.DebugKey(Key::S) && !Net::IsClient())
	{
		const vector<int>& tiles = world->GetTiles();
		int ts = world->worldSize / World::TILE_SIZE;
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
				Rect rect(Int2(WorldPosToScreen(Vec2((float)x * World::TILE_SIZE, (float)y * World::TILE_SIZE))),
					Int2(WorldPosToScreen(Vec2((float)(x + 1) * World::TILE_SIZE, (float)(y + 1) * World::TILE_SIZE))));
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

		int alpha;
		if(loc.state == LS_KNOWN)
		{
			if(world->currentLocation == &loc)
				alpha = 192;
			else
				alpha = 128;
		}
		else
			alpha = 255;

		const Vec2 pos(WorldPosToScreen(Vec2(loc.pos.x - 16.f, loc.pos.y + 16.f)));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tMapIcon[loc.image], mat, Color::Alpha(alpha));
	}

	// encounter locations
	if(game->devmode)
	{
		for(const Encounter* enc : world->GetEncounters())
		{
			if(!enc)
				continue;
			const Vec2 pos(WorldPosToScreen(Vec2(enc->pos.x - 16.f, enc->pos.y + 16.f)));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSpriteTransform(tEnc, mat);
		}
	}

	LocalString s = Format(txWorldDate, world->GetDate());
	const Vec2& worldPos = world->GetWorldPos();

	// current location icon, set description
	if(world->GetCurrentLocation())
	{
		Location& current = *world->GetCurrentLocation();
		const Vec2 pos(WorldPosToScreen(Vec2(current.pos.x - 32.f, current.pos.y + 32.f)));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tSelected[1], mat, 0xAAFFFFFF);
		s += Format("\n\n%s: %s", txCurrentLoc, current.name.c_str());
		AppendLocationText(current, s.get_ref());
	}

	// target location icon, set description
	if(pickedLocation != -1)
	{
		Location& picked = *world->locations[pickedLocation];
		if(pickedLocation != world->GetCurrentLocationIndex())
		{
			float distance = Vec2::Distance(worldPos, picked.pos) * World::MAP_KM_RATIO;
			int daysCost = int(floor(distance / World::TRAVEL_SPEED));
			s += Format("\n\n%s: %s", txTarget, picked.name.c_str());
			AppendLocationText(picked, s.get_ref());
			cstring cost;
			if(daysCost == 0)
				cost = Format("<1 %s", txDay);
			else if(daysCost == 1)
				cost = Format("1 %s", txDay);
			else
				cost = Format("%d %s", daysCost, txDays);
			s += Format("\n%s: %g km\n%s: %s", txDistance, ceil(distance * 10) / 10, txTravelTime, cost);
		}
		const Vec2 pos(WorldPosToScreen(Vec2(picked.pos.x - 32.f, picked.pos.y + 32.f)));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tSelected[0], mat, 0xAAFFFFFF);
	}
	else if(mapPosValid)
	{
		float distance = Vec2::Distance(worldPos, mapPos) * World::MAP_KM_RATIO;
		int daysCost = int(floor(distance / World::TRAVEL_SPEED));
		cstring cost;
		if(daysCost == 0)
			cost = Format("<1 %s", txDay);
		else if(daysCost == 1)
			cost = Format("1 %s", txDay);
		else
			cost = Format("%d %s", daysCost, txDays);
		s += Format("\n\n%s:\n%s: %g km\n%s: %s", txTarget, txDistance, ceil(distance * 10) / 10, txTravelTime, cost);
	}

	// team position
	if(world->GetCurrentLocationIndex() == -1)
	{
		const Vec2 pos(WorldPosToScreen(Vec2(worldPos.x - 8, worldPos.y + 8)));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tMover, mat, 0xBBFFFFFF);
	}

	// line from team to target position
	Vec2 targetPos;
	bool ok = false;
	if(pickedLocation != -1 && pickedLocation != world->GetCurrentLocationIndex())
	{
		targetPos = world->locations[pickedLocation]->pos;
		ok = true;
	}
	else if(world->GetState() != World::State::ON_MAP)
	{
		targetPos = world->GetTargetPos();
		ok = true;
	}
	else if(mapPosValid)
	{
		targetPos = mapPos;
		ok = true;
	}
	if(ok)
		gui->DrawLine(WorldPosToScreen(worldPos), WorldPosToScreen(targetPos), 0xAA000000);

	// encounter chance
	if(game->devmode && Net::IsLocal())
		s += Format("\n\nEncounter: %d%% (%g)", int(float(max(0, (int)world->GetEncounterChance() - 25)) * 100 / 500), world->GetEncounterChance());

	// tracking arrow
	if(tracking != -1)
	{
		const Vec2 scale(zoom / 2, zoom / 2);
		const Vec2 pos(WorldPosToScreen(world->GetLocation(tracking)->pos + Vec2(-40.f, 40.f)));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tTrackingArrow, mat);
	}

	// side images
	gui->DrawSpriteRect(tSide, Rect(gui->wndSize.x * 2 / 3, 0, gui->wndSize.x, gui->wndSize.y));

	// buttons
	for(int i = 0; i < 2; ++i)
		buttons[i].Draw();

	// magnifying glass & combo box
	Rect rect(int(float(gui->wndSize.x) * 2 / 3 + 16.f * gui->wndSize.x / 1024),
		int(float(gui->wndSize.y) - 138.f * gui->wndSize.y / 768));
	rect.p2.x += int(32.f * gui->wndSize.x / 1024);
	rect.p2.y += int(32.f * gui->wndSize.y / 768);
	gui->DrawSpriteRect(tMagnifyingGlass, rect);
	comboSearch.Draw();

	// text
	rect = Rect(int(float(gui->wndSize.x) * 2 / 3 + 16.f * gui->wndSize.x / 1024),
		int(94.f * gui->wndSize.y / 768),
		int(float(gui->wndSize.x) - 16.f * gui->wndSize.x / 1024),
		int(float(gui->wndSize.y) - 90.f * gui->wndSize.y / 768));
	gui->DrawText(GameGui::font, s, 0, Color::Black, rect);

	if(game->endOfGame)
		gameGui->levelGui->DrawEndOfGameScreen();

	if(gameGui->mpBox->visible)
		gameGui->mpBox->Draw();

	if(gameGui->journal->visible)
		gameGui->journal->Draw();

	gameGui->messages->Draw();

	tooltip.Draw();
}

//=================================================================================================
void WorldMapGui::Update(float dt)
{
	mapPosValid = false;

	MpBox* mpBox = gameGui->mpBox;
	if(mpBox->visible)
	{
		mpBox->focus = true;
		mpBox->Event(GuiEvent_GainFocus);
		if(gui->HaveDialog())
			mpBox->LostFocus();
		mpBox->Update(dt);

		if(input->Focus() && !mpBox->haveFocus && !gui->HaveDialog() && input->PressedRelease(Key::Enter))
			mpBox->haveFocus = true;

		if(mpBox->IsInside(gui->cursorPos))
			focus = false;
	}

	if(game->endOfGame)
		return;

	int group = -1, id = -1;

	if(gameGui->journal->visible)
	{
		gameGui->journal->focus = focus;
		gameGui->journal->Update(dt);
	}
	if(!gui->HaveDialog() && !(mpBox->visible && mpBox->itb.focus) && input->Focus() && !comboSearch.focus && game->deathScreen == 0)
	{
		if(GKey.PressedRelease(GK_JOURNAL))
		{
			if(gameGui->journal->visible)
				gameGui->journal->Hide();
			else
				gameGui->journal->Show();
		}
		if(GKey.PressedRelease(GK_MAP_SEARCH))
		{
			comboSearch.focus = true;
			comboSearch.Event(GuiEvent_GainFocus);
		}
	}

	if(focus && !gameGui->journal->visible)
	{
		if(!comboSearch.focus && GKey.KeyPressedReleaseAllowed(GK_TALK_BOX))
			gameGui->mpBox->visible = !gameGui->mpBox->visible;

		if(gameGui->mpBox->haveFocus && comboSearch.focus)
			comboSearch.LostFocus();

		comboSearch.mouseFocus = focus;
		if(comboSearch.IsInside(gui->cursorPos) && input->PressedRelease(Key::LeftButton))
			comboSearch.GainFocus();
		comboSearch.Update(dt);

		zoom = Clamp(zoom + 0.1f * input->GetMouseWheel(), 0.5f, 2.f);
		if(clicked)
		{
			offset -= Vec2(input->GetMouseDif()) / (float(world->worldSize) / MAP_IMG_SIZE * zoom);
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
			comboSearch.LostFocus();
		}

		if(!comboSearch.focus && input->Down(Key::Spacebar))
		{
			follow = true;
			fastFollow = true;
			tracking = -1;
		}

		// buttons
		for(int i = 0; i < 2; ++i)
		{
			buttons[i].mouseFocus = focus;
			buttons[i].Update(dt);
			if(!comboSearch.focus && buttons[i].IsInside(gui->cursorPos))
			{
				group = 0;
				id = (i == 0 ? GK_JOURNAL : GK_TALK_BOX);
			}
		}

		// search location tooltip
		Rect rect(int(float(gui->wndSize.x) * 2 / 3 + 16.f * gui->wndSize.x / 1024),
			int(float(gui->wndSize.y) - 138.f * gui->wndSize.y / 768));
		rect.p2.x += int(32.f * gui->wndSize.x / 1024);
		rect.p2.y += int(32.f * gui->wndSize.y / 768);
		if(rect.IsInside(gui->cursorPos))
		{
			group = 0;
			id = GK_MAP_SEARCH;
		}
	}

	if(follow)
		CenterView(dt);
	else if(tracking != -1)
	{
		fastFollow = true;
		CenterView(dt, &world->GetLocation(tracking)->pos);
	}

	if(world->GetState() == World::State::TRAVEL)
	{
		if(world->travelLocation)
			pickedLocation = world->travelLocation->index;

		if(game->paused || (!Net::IsOnline() && gui->HavePauseDialog()))
			return;

		bool stop = false;
		if(focus && input->Focus() && gui->cursorPos.x < gui->wndSize.x * 2 / 3 && input->PressedRelease(Key::LeftButton))
		{
			if(team->IsLeader())
			{
				world->StopTravel(world->GetWorldPos(), true);
				stop = true;
			}
			else
				gameGui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
		}
		if(!stop)
			world->UpdateTravel(dt);
	}
	else if(world->GetState() != World::State::ENCOUNTER && !gameGui->journal->visible)
	{
		const Vec2 cursorPos = Vec2(gui->cursorPos);
		Location* loc = nullptr;
		int index;

		mapPos = (cursorPos - GetCameraCenter()) / zoom + offset * float(world->worldSize) / MAP_IMG_SIZE;
		mapPos.y = float(world->worldSize) - mapPos.y;
		if(focus && mapPos.x > 0 && mapPos.y > 0 && mapPos.x < world->worldSize && mapPos.y < world->worldSize && gui->cursorPos.x < gui->wndSize.x * 2 / 3)
		{
			mapPosValid = true;
			if(!input->Down(Key::Shift))
			{
				float dist = 17.f;
				int i = 0;
				for(vector<Location*>::iterator it = world->locations.begin(), end = world->locations.end(); it != end; ++it, ++i)
				{
					if(!*it || (*it)->state == LS_UNKNOWN || (*it)->state == LS_HIDDEN)
						continue;
					Vec2 pt = WorldPosToScreen((*it)->pos);
					float dist2 = Vec2::Distance(pt, cursorPos) / zoom;
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
			pickedLocation = index;
			if(world->GetState() == World::State::ON_MAP && input->Focus())
			{
				if(input->PressedRelease(Key::LeftButton))
				{
					comboSearch.LostFocus();
					if(team->IsLeader())
					{
						if(pickedLocation != world->GetCurrentLocationIndex())
						{
							world->Travel(pickedLocation, true);
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
						gameGui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game->devmode && pickedLocation != world->GetCurrentLocationIndex() && !comboSearch.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->Warp(pickedLocation, true);
						StartTravel(true);
					}
					else
						gameGui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
		else
		{
			pickedLocation = -1;
			if(mapPosValid && world->GetState() == World::State::ON_MAP && input->Focus())
			{
				if(input->PressedRelease(Key::LeftButton))
				{
					comboSearch.LostFocus();
					if(team->IsLeader())
					{
						world->TravelPos(mapPos, true);
						StartTravel();
					}
					else
						gameGui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
				else if(game->devmode && !comboSearch.focus && input->PressedRelease(Key::T))
				{
					if(team->IsLeader())
					{
						world->WarpPos(mapPos, true);
						StartTravel(true);
					}
					else
						gameGui->messages->AddGameMsg2(txOnlyLeaderCanTravel, 3.f, GMS_ONLY_LEADER_CAN_TRAVEL);
				}
			}
		}
	}

	if(focus && input->PressedRelease(Key::LeftButton))
		comboSearch.LostFocus();

	gameGui->messages->Update(dt);
	tooltip.UpdateTooltip(dt, group, id);
}

//=================================================================================================
void WorldMapGui::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_LostFocus:
		gameGui->mpBox->LostFocus();
		comboSearch.LostFocus();
		break;
	case GuiEvent_Show:
	case GuiEvent_WindowResize:
		{
			Rect rect(int(float(gui->wndSize.x) * 2 / 3 + 52.f * gui->wndSize.x / 1024),
				int(float(gui->wndSize.y) - 140.f * gui->wndSize.y / 768));
			rect.p2.x = int(float(gui->wndSize.x) - 20.f * gui->wndSize.y / 1024);
			rect.p2.y += int(32.f * gui->wndSize.y / 768);
			comboSearch.pos = rect.p1;
			comboSearch.globalPos = comboSearch.pos;
			comboSearch.size = rect.Size();

			const int imgSize = int(32.f * gui->wndSize.y / 768);
			buttons[0].pos = Int2(rect.p1.x, rect.p1.y - imgSize - 5);
			buttons[0].globalPos = buttons[0].pos;
			buttons[0].size = Int2(rect.SizeX() / 2 - 2, imgSize);
			buttons[0].forceImgSize = Int2(imgSize);
			buttons[1].pos = Int2(rect.p1.x + rect.SizeX() / 2 + 2, rect.p1.y - imgSize - 5);
			buttons[1].globalPos = buttons[1].pos;
			buttons[1].size = Int2(rect.SizeX() / 2 - 2, imgSize);
			buttons[1].forceImgSize = Int2(imgSize);

			if(e == GuiEvent_Show)
			{
				tracking = -1;
				clicked = false;
				CenterView(-1.f);
				follow = (world->GetState() == World::State::TRAVEL);
				fastFollow = false;
				comboSearch.Reset();
				mapPosValid = false;
				tooltip.Clear();
			}
		}
		break;
	case ComboBox::Event_TextChanged:
		{
			LocalString str = comboSearch.GetText();
			Trim(str.get_ref());
			comboSearch.ClearItems();
			if(str.length() < 3)
				break;

			LocalVector<Location*> matchingLocations;
			for(Location* loc : world->GetLocations())
			{
				if(!loc || loc->state == LS_HIDDEN || loc->state == LS_UNKNOWN)
					continue;
				if(StringContainsStringI(loc->name.c_str(), str.c_str()))
					matchingLocations.push_back(loc);
			}

			std::sort(matchingLocations.begin(), matchingLocations.end(), [](Location* loc1, Location* loc2) -> bool
			{
				return strcmp(loc1->name.c_str(), loc2->name.c_str()) < 0;
			});

			for(uint i = 0; i < min(5u, matchingLocations.size()); ++i)
			{
				LocationElement* le = LocationElement::Get();
				le->loc = matchingLocations[i];
				comboSearch.AddItem(le);
			}
		}
		break;
	case ComboBox::Event_Selected:
		{
			LocationElement* le = static_cast<LocationElement*>(comboSearch.GetSelectedItem());
			tracking = le->loc->index;
			follow = false;
		}
		break;
	case ButtonJournal:
		gameGui->journal->Show();
		break;
	case ButtonTalkBox:
		gameGui->mpBox->visible = !gameGui->mpBox->visible;
		break;
	}
}

//=================================================================================================
void WorldMapGui::Save(GameWriter& f)
{
	f << zoom;
}

//=================================================================================================
void WorldMapGui::Load(GameReader& f)
{
	f >> zoom;
}

//=================================================================================================
void WorldMapGui::Clear()
{
	zoom = 1.2f;
	follow = false;
	tracking = -1;
	clicked = false;
	dialogEnc = nullptr;
	pickedLocation = -1;
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
				gBaseLocations[inside->target].name, loc.group->id.c_str(), loc.st, inside->GetLastLevel() + 1);
		}
		else if(loc.type == L_OUTSIDE || loc.type == L_CAMP || loc.type == L_CAVE)
			s += Format("%s, st %d, ", loc.group->id.c_str(), loc.st);
		s += Format("quest 0x%p)", loc.activeQuest);
	}
	if(loc.state >= LS_VISITED && loc.type == L_CITY)
		GetCityText((City&)loc, s);
}

//=================================================================================================
void WorldMapGui::GetCityText(City& city, string& s)
{
	// Citizens: X
	s += Format("\n%s: %d", txCitizens, city.citizensWorld);

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
	info.order = DialogOrder::Top;
	info.parent = this;
	info.pause = true;
	info.text = text;
	info.type = DIALOG_OK;
	dialogEnc = gui->ShowDialog(info);

	if(Net::IsOnline())
	{
		NetChange& c = Net::PushChange(NetChange::ENCOUNTER);
		c.str = StringPool.Get();
		*c.str = text;

		// disable button when server is not leader
		if(!team->IsLeader())
			dialogEnc->bts[0].state = Button::DISABLED;
	}
}

//=================================================================================================
void WorldMapGui::StartTravel(bool fast)
{
	follow = true;
	tracking = -1;
	fastFollow = fast;
}

//=================================================================================================
Vec2 WorldMapGui::WorldPosToScreen(const Vec2& pt) const
{
	return Vec2(pt.x, float(world->worldSize) - pt.y) * zoom
		+ GetCameraCenter()
		- offset * zoom * float(world->worldSize) / MAP_IMG_SIZE;
}

//=================================================================================================
void WorldMapGui::CenterView(float dt, const Vec2* target)
{
	Vec2 p = (target ? *target : world->worldPos);
	Vec2 pos = Vec2(p.x, float(world->worldSize) - p.y) / (float(world->worldSize) / MAP_IMG_SIZE);
	if(dt >= 0.f)
		offset += (pos - offset) * dt * (fastFollow ? 10.f : 2.f);
	else
		offset = pos;
}

//=================================================================================================
Vec2 WorldMapGui::GetCameraCenter() const
{
	return Vec2(float(gui->wndSize.x) / 3, float(gui->wndSize.y) / 2);
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
