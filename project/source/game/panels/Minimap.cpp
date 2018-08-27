#include "Pch.h"
#include "GameCore.h"
#include "Minimap.h"
#include "Game.h"
#include "InsideLocation.h"
#include "City.h"
#include "Team.h"
#include "Portal.h"

//=================================================================================================
Minimap::Minimap()
{
	visible = false;
}

//=================================================================================================
void Minimap::Draw(ControlDrawData* /*cdd*/)
{
	Game& game = Game::Get();
	LOCATION type = game.location->type;

	// tekstura minimapy
	Rect r = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	Rect r_part = { 0, 0, minimap_size, minimap_size };
	GUI.DrawSpriteRectPart(game.tMinimap, r, r_part, Color::Alpha(140));

	// schody w podziemiach
	if(type == L_DUNGEON || type == L_CRYPT || type == L_CAVE)
	{
		InsideLocation* inside = (InsideLocation*)game.location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HaveDownStairs() && IS_SET(lvl.map[lvl.staircase_down(lvl.w)].flags, Pole::F_ODKRYTE))
			GUI.DrawSprite(tSchodyDol, Int2(TileToPoint(lvl.staircase_down)) - Int2(16, 16), Color::Alpha(180));
		if(inside->HaveUpStairs() && IS_SET(lvl.map[lvl.staircase_up(lvl.w)].flags, Pole::F_ODKRYTE))
			GUI.DrawSprite(tSchodyGora, Int2(TileToPoint(lvl.staircase_up)) - Int2(16, 16), Color::Alpha(180));
	}

	// portale
	Portal* p = game.location->portal;
	InsideLocationLevel* lvl = game.TryGetLevelData();
	while(p)
	{
		if(!lvl || (game.dungeon_level == p->at_level && lvl->IsTileVisible(p->pos)))
			GUI.DrawSprite(tMiniportal, Int2(TileToPoint(pos_to_pt(p->pos))) - Int2(24, 8), Color::Alpha(180));
		p = p->next_portal;
	}

	// przedmioty
	LocalVector<GroundItem*> important_items;
	for(vector<GroundItem*>::iterator it = game.local_ctx.items->begin(), end = game.local_ctx.items->end(); it != end; ++it)
	{
		if(IS_SET((*it)->item->flags, ITEM_IMPORTANT))
			important_items->push_back(*it);
		else if(!lvl || lvl->IsTileVisible((*it)->pos))
		{
			m1 = Matrix::Transform2D(&Vec2(16, 16), 0.f, &Vec2(0.25f, 0.25f), nullptr, 0.f, &(PosToPoint(Vec2((*it)->pos.x, (*it)->pos.z)) - Vec2(16, 16)));
			GUI.DrawSpriteTransform(tMinibag, m1, Color::Alpha(140));
		}
	}
	for(vector<GroundItem*>::iterator it = important_items->begin(), end = important_items->end(); it != end; ++it)
	{
		if(!lvl || lvl->IsTileVisible((*it)->pos))
		{
			m1 = Matrix::Transform2D(&Vec2(16, 16), 0.f, &Vec2(0.25f, 0.25f), nullptr, 0.f, &(PosToPoint(Vec2((*it)->pos.x, (*it)->pos.z)) - Vec2(16, 16)));
			GUI.DrawSpriteTransform(tMinibag2, m1, Color::Alpha(140));
		}
	}

	// obrazek postaci
	for(Unit* unit : Team.members)
	{
		m1 = Matrix::Transform2D(&Vec2(16, 16), 0.f, &Vec2(0.25f, 0.25f), &Vec2(16, 16), unit->rot, &(PosToPoint(game.GetMapPosition(*unit)) - Vec2(16, 16)));
		GUI.DrawSpriteTransform(tMiniunit[unit == game.pc->unit ? 0 : 1], m1, Color::Alpha(140));
	}

	// obrazki pozosta�ych postaci
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if((u.IsAlive() || IS_SET(u.data->flags2, F2_MARK)) && !u.IsTeamMember() && (!lvl || lvl->IsTileVisible(u.pos)))
		{
			m1 = Matrix::Transform2D(&Vec2(16, 16), 0.f, &Vec2(0.25f, 0.25f), &Vec2(16, 16), (*it)->rot, &(PosToPoint(game.GetMapPosition(u)) - Vec2(16, 16)));
			GUI.DrawSpriteTransform(tMiniunit[u.IsAlive() ? (game.IsEnemy(u, *game.pc->unit) ? 2 : 3) : 4], m1, Color::Alpha(140));
		}
	}

	if(game.city_ctx)
	{
		// teksty w mie�cie
		for(Text& text : texts)
		{
			Int2 pt(Convert(text.pos));
			Rect rect = { pt.x - text.size.x / 2, pt.y - text.size.y / 2, pt.x + text.size.x / 2, pt.y + text.size.y / 2 };
			GUI.DrawText(GUI.default_font, text.text, DTF_SINGLELINE, Color::Black, rect);
		}

		// linie do tekst�w
		GUI.LineBegin();
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			Vec2 pts[2] = {
				Convert(it->pos),
				Convert(it->anchor)
			};
			GUI.DrawLine(pts, 1, Color(0, 0, 0, 140));
		}
		GUI.LineEnd();
	}

	// nazwa lokacji
	Rect rect = { 0,0,GUI.wnd_size.x - 8,GUI.wnd_size.y - 8 };
	GUI.DrawText(GUI.default_font, game.GetCurrentLocationText(), DTF_RIGHT | DTF_OUTLINE, Color(255, 0, 0, 222), rect);
}

//=================================================================================================
void Minimap::Update(float dt)
{
	if(Game::Get().city_ctx)
	{
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			for(vector<Text>::iterator it2 = texts.begin(); it2 != end; ++it2)
			{
				if(it == it2)
					continue;

				Vec2 pt1 = Convert(it->pos),
					pt2 = Convert(it2->pos);
				float w1 = float(it->size.x) / 2,
					h1 = float(it->size.y) / 2,
					w2 = float(it2->size.x) / 2,
					h2 = float(it2->size.y) / 2;

				if(RectangleToRectangle(pt1.x - w1, pt1.y - h1, pt1.x + w1, pt1.y + h1,
					pt2.x - w2, pt2.y - h2, pt2.x + w2, pt2.y + h2))
				{
					Vec2 dir = it->pos - it2->pos;
					dir.Normalize();
					it->pos += dir*dt;
				}
			}
		}
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
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
	Game& game = Game::Get();
	if(game.city_ctx)
	{
		if(game.city_ctx != city)
		{
			city = game.city_ctx;
			texts.clear();

			for(CityBuilding& b : city->buildings)
			{
				if(IS_SET(b.type->flags, Building::HAVE_NAME))
				{
					Text& text = Add1(texts);
					text.text = b.type->name.c_str();
					text.size = GUI.default_font->CalculateSize(b.type->name);
					text.pos = text.anchor = TransformTile(b.pt);
				}
			}
		}
	}
}

//=================================================================================================
void Minimap::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("mini_unit.png", tMiniunit[0]);
	tex_mgr.AddLoadTask("mini_unit2.png", tMiniunit[1]);
	tex_mgr.AddLoadTask("mini_unit3.png", tMiniunit[2]);
	tex_mgr.AddLoadTask("mini_unit4.png", tMiniunit[3]);
	tex_mgr.AddLoadTask("mini_unit5.png", tMiniunit[4]);
	tex_mgr.AddLoadTask("schody_dol.png", tSchodyDol);
	tex_mgr.AddLoadTask("schody_gora.png", tSchodyGora);
	tex_mgr.AddLoadTask("mini_bag.png", tMinibag);
	tex_mgr.AddLoadTask("mini_bag2.png", tMinibag2);
	tex_mgr.AddLoadTask("mini_portal.png", tMiniportal);
}
