#include "Pch.h"
#include "Minimap.h"

#include "City.h"
#include "Game.h"
#include "GameGui.h"
#include "InsideLocation.h"
#include "Level.h"
#include "Portal.h"
#include "ResourceManager.h"
#include "Team.h"

enum UnitType
{
	UNIT_ME,
	UNIT_TEAM,
	UNIT_ENEMY,
	UNIT_NPC,
	UNIT_CORPSE
};

//=================================================================================================
Minimap::Minimap()
{
	visible = false;
}

//=================================================================================================
void Minimap::LoadData()
{
	tUnit[UNIT_ME] = resMgr->Load<Texture>("mini_unit.png");
	tUnit[UNIT_TEAM] = resMgr->Load<Texture>("mini_unit2.png");
	tUnit[UNIT_ENEMY] = resMgr->Load<Texture>("mini_unit3.png");
	tUnit[UNIT_NPC] = resMgr->Load<Texture>("mini_unit4.png");
	tUnit[UNIT_CORPSE] = resMgr->Load<Texture>("mini_unit5.png");
	tStairsDown = resMgr->Load<Texture>("schody_dol.png");
	tStairsUp = resMgr->Load<Texture>("schody_gora.png");
	tBag = resMgr->Load<Texture>("mini_bag.png");
	tBagImportant = resMgr->Load<Texture>("mini_bag2.png");
	tPortal = resMgr->Load<Texture>("mini_portal.png");
	tChest = resMgr->Load<Texture>("mini_chest.png");
	tDoor = resMgr->Load<Texture>("mini_door.png");
}

//=================================================================================================
void Minimap::Draw()
{
	LocationPart& locPart = *game_level->localPart;
	LOCATION type = game_level->location->type;

	// map texture
	Rect r = { globalPos.x, globalPos.y, globalPos.x + size.x, globalPos.y + size.y };
	Rect r_part = { 0, 0, minimap_size, minimap_size };
	gui->DrawSpriteRectPart(game->tMinimap, r, r_part, Color::Alpha(140));

	// stairs
	if(type == L_DUNGEON || type == L_CAVE)
	{
		InsideLocation* inside = (InsideLocation*)game_level->location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HaveNextEntry() && IsSet(lvl.map[lvl.nextEntryPt(lvl.w)].flags, Tile::F_REVEALED))
			gui->DrawSprite(GetEntryImage(lvl.nextEntryType), Int2(TileToPoint(lvl.nextEntryPt)) - Int2(16, 16), Color::Alpha(180));
		if(inside->HavePrevEntry() && IsSet(lvl.map[lvl.prevEntryPt(lvl.w)].flags, Tile::F_REVEALED))
			gui->DrawSprite(GetEntryImage(lvl.prevEntryType), Int2(TileToPoint(lvl.prevEntryPt)) - Int2(16, 16), Color::Alpha(180));
	}

	// portals
	Portal* p = game_level->location->portal;
	InsideLocationLevel* lvl = nullptr;
	if(game_level->location->type == L_DUNGEON || game_level->location->type == L_CAVE)
		lvl = &((InsideLocation*)game_level->location)->GetLevelData();
	while(p)
	{
		if(!lvl || (game_level->dungeon_level == p->at_level && lvl->IsTileVisible(p->pos)))
			gui->DrawSprite(tPortal, Int2(TileToPoint(PosToPt(p->pos))) - Int2(24, 8), Color::Alpha(180));
		p = p->next_portal;
	}

	// chests
	for(Chest* chest : locPart.chests)
	{
		if(!lvl || lvl->IsTileVisible(chest->pos))
		{
			const Vec2 center(16);
			const Vec2 scale(0.5f);
			const Vec2 pos(PosToPoint(Vec2(chest->pos.x, chest->pos.z)) - center);
			const Matrix mat = Matrix::Transform2D(&center, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSpriteTransform(tChest, mat, Color::Alpha(140));
		}
	}

	// items
	LocalVector<const GroundItem*> importantItems;
	for(const GroundItem* groundItem : locPart.GetGroundItems())
	{
		if(lvl && !lvl->IsTileVisible(groundItem->pos))
			continue;

		if(IsSet(groundItem->item->flags, ITEM_IMPORTANT))
			importantItems->push_back(groundItem);
		else
		{
			const Vec2 center(16);
			const Vec2 scale(0.3125f);
			const Vec2 pos(PosToPoint(Vec2(groundItem->pos.x, groundItem->pos.z)) - center);
			const Matrix mat = Matrix::Transform2D(&center, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSpriteTransform(tBag, mat, Color::Alpha(140));
		}
	}
	for(const GroundItem* groundItem : importantItems)
	{
		const Vec2 center(16);
		const Vec2 scale(0.3125f);
		const Vec2 pos(PosToPoint(Vec2(groundItem->pos.x, groundItem->pos.z)) - center);
		const Matrix mat = Matrix::Transform2D(&center, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSpriteTransform(tBagImportant, mat, Color::Alpha(200));
	}

	// team members
	for(Unit& unit : team->members)
	{
		const Vec2 center(16);
		const Vec2 scale(0.3125f);
		const Vec2 pos(PosToPoint(GetMapPosition(unit)) - center);
		const Matrix mat = Matrix::Transform2D(&center, 0.f, &scale, &center, unit.rot, &pos);
		gui->DrawSpriteTransform(tUnit[&unit == game->pc->unit ? UNIT_ME : UNIT_TEAM], mat, Color::Alpha(140));
	}

	// other units
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if((u.IsAlive() || u.mark) && !u.IsTeamMember() && (!lvl || lvl->IsTileVisible(u.pos)))
		{
			const Vec2 center(16, 16);
			const Vec2 scale(IsSet(u.data->flags2, F2_BOSS) ? 0.40625f : 0.3125f);
			const Vec2 pos(PosToPoint(GetMapPosition(u)) - center);
			const Matrix mat = Matrix::Transform2D(&center, 0.f, &scale, &center, (*it)->rot, &pos);
			gui->DrawSpriteTransform(tUnit[u.IsAlive() ? (u.IsEnemy(*game->pc->unit) ? UNIT_ENEMY : UNIT_NPC) : UNIT_CORPSE], mat, Color::Alpha(140));
		}
	}

	if(game_level->city_ctx)
	{
		// building names
		for(Text& text : texts)
		{
			Int2 pt(Convert(text.pos));
			Rect rect = { pt.x - text.size.x / 2, pt.y - text.size.y / 2, pt.x + text.size.x / 2, pt.y + text.size.y / 2 };
			gui->DrawText(GameGui::font, text.text, DTF_SINGLELINE, Color::Black, rect);
		}

		// lines from building name to building
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
			gui->DrawLine(Convert(it->pos), Convert(it->anchor), Color(0, 0, 0, 140));
	}

	// location name
	Rect rect = { 0,0,gui->wndSize.x - 8,gui->wndSize.y - 8 };
	gui->DrawText(GameGui::font, game_level->GetCurrentLocationText(), DTF_RIGHT | DTF_OUTLINE, Color(255, 0, 0, 222), rect);
}

//=================================================================================================
void Minimap::Update(float dt)
{
	if(game_level->city_ctx)
	{
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			for(vector<Text>::iterator it2 = texts.begin(); it2 != end; ++it2)
			{
				if(it == it2)
					continue;

				Vec2 pt1 = Convert(it->pos),
					pt2 = Convert(it2->pos);
				float w1 = float(it->size.x + 2) / 2,
					h1 = float(it->size.y + 2) / 2,
					w2 = float(it2->size.x + 2) / 2,
					h2 = float(it2->size.y + 2) / 2;

				if(RectangleToRectangle(pt1.x - w1, pt1.y - h1, pt1.x + w1, pt1.y + h1,
					pt2.x - w2, pt2.y - h2, pt2.x + w2, pt2.y + h2))
				{
					Vec2 dir = it->pos - it2->pos;
					dir.Normalize();
					it->pos += dir * dt;
				}
			}
		}
	}

	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
		Hide();
}

//=================================================================================================
void Minimap::Event(GuiEvent e)
{
	if(e == GuiEvent_Resize)
	{
		city = nullptr;
		Build();
	}
}

//=================================================================================================
void Minimap::Show()
{
	visible = true;
	Build();
}

//=================================================================================================
void Minimap::Hide()
{
	visible = false;
}

//=================================================================================================
void Minimap::Build()
{
	minimap_size = game_level->minimap_size;
	if(game_level->city_ctx && game_level->city_ctx != city)
	{
		city = game_level->city_ctx;
		texts.clear();

		for(CityBuilding& b : city->buildings)
		{
			if(IsSet(b.building->flags, Building::HAVE_NAME))
			{
				Text& text = Add1(texts);
				text.text = b.building->name.c_str();
				text.size = GameGui::font->CalculateSize(b.building->name);
				text.pos = text.anchor = TransformTile(b.pt);
			}
		}
	}
}

//=================================================================================================
Vec2 Minimap::GetMapPosition(Unit& unit)
{
	if(!game_level->city_ctx || unit.locPart->partType == LocationPart::Type::Outside)
		return Vec2(unit.pos.x, unit.pos.z);
	else
	{
		Building* building = static_cast<InsideBuilding*>(unit.locPart)->building;
		for(CityBuilding& b : game_level->city_ctx->buildings)
		{
			if(b.building == building)
				return Vec2(float(b.pt.x * 2), float(b.pt.y * 2));
		}
	}
	assert(0);
	return Vec2(-1000, -1000);
}

//=================================================================================================
Texture* Minimap::GetEntryImage(EntryType type)
{
	switch(type)
	{
	default:
		assert(0);
	case ENTRY_STAIRS_UP:
		return tStairsUp;
	case ENTRY_STAIRS_DOWN:
	case ENTRY_STAIRS_DOWN_IN_WALL:
		return tStairsDown;
	case ENTRY_DOOR:
		return tDoor;
	}
}
