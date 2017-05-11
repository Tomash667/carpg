#include "Pch.h"
#include "Base.h"
#include "Minimap.h"
#include "Game.h"
#include "InsideLocation.h"
#include "City.h"
#include "Team.h"

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
	RECT r = {global_pos.x, global_pos.y, global_pos.x+size.x, global_pos.y+size.y};
	RECT r_part = {0, 0, minimap_size, minimap_size};
	GUI.DrawSpriteRectPart(game.tMinimap, r, r_part, COLOR_RGBA(255,255,255,140));

	// schody w podziemiach
	if(type == L_DUNGEON || type == L_CRYPT || type == L_CAVE)
	{
		InsideLocation* inside = (InsideLocation*)game.location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		if(inside->HaveDownStairs() && IS_SET(lvl.map[lvl.staircase_down(lvl.w)].flags, Pole::F_ODKRYTE))
			GUI.DrawSprite(game.tSchodyDol, INT2(TileToPoint(lvl.staircase_down))-INT2(16,16), COLOR_RGBA(255,255,255,180));
		if(inside->HaveUpStairs() && IS_SET(lvl.map[lvl.staircase_up(lvl.w)].flags, Pole::F_ODKRYTE))
			GUI.DrawSprite(game.tSchodyGora, INT2(TileToPoint(lvl.staircase_up))-INT2(16,16), COLOR_RGBA(255,255,255,180));
	}

	// portale
	Portal* p = game.location->portal;
	InsideLocationLevel* lvl = game.TryGetLevelData();
	while(p)
	{
		if(!lvl || (game.dungeon_level == p->at_level && lvl->IsTileVisible(p->pos)))
			GUI.DrawSprite(game.tMiniportal, INT2(TileToPoint(pos_to_pt(p->pos)))-INT2(24,8), COLOR_RGBA(255,255,255,180));
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
			D3DXMatrixTransformation2D(&m1, &VEC2(16,16), 0.f, &VEC2(0.25f,0.25f), nullptr, 0.f, &(PosToPoint(VEC2((*it)->pos.x, (*it)->pos.z))-VEC2(16,16)));
			GUI.DrawSpriteTransform(game.tMinibag, m1, COLOR_RGBA(255,255,255,140));
		}
	}
	for(vector<GroundItem*>::iterator it = important_items->begin(), end = important_items->end(); it != end; ++it)
	{
		if(!lvl || lvl->IsTileVisible((*it)->pos))
		{
			D3DXMatrixTransformation2D(&m1, &VEC2(16,16), 0.f, &VEC2(0.25f,0.25f), nullptr, 0.f, &(PosToPoint(VEC2((*it)->pos.x, (*it)->pos.z))-VEC2(16,16)));
			GUI.DrawSpriteTransform(game.tMinibag2, m1, COLOR_RGBA(255,255,255,140));
		}
	}

	// obrazek postaci
	for(Unit* unit : Team.members)
	{
		D3DXMatrixTransformation2D(&m1, &VEC2(16,16), 0.f, &VEC2(0.25f,0.25f), &VEC2(16,16), unit->rot, &(PosToPoint(game.GetMapPosition(*unit))-VEC2(16,16)));
		GUI.DrawSpriteTransform((unit == game.pc->unit) ? game.tMiniunit : game.tMiniunit2, m1, COLOR_RGBA(255,255,255,140));
	}

	// obrazki pozosta³ych postaci
	for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
	{
		Unit& u = **it;
		if((u.IsAlive() || IS_SET(u.data->flags2, F2_MARK)) && !u.IsTeamMember() && (!lvl || lvl->IsTileVisible(u.pos)))
		{
			D3DXMatrixTransformation2D(&m1, &VEC2(16,16), 0.f, &VEC2(0.25f,0.25f), &VEC2(16,16), (*it)->rot, &(PosToPoint(game.GetMapPosition(u))-VEC2(16,16)));
			GUI.DrawSpriteTransform(u.IsAlive() ? (game.IsEnemy(u, *game.pc->unit) ? game.tMiniunit3 : game.tMiniunit4) : game.tMiniunit5, m1, COLOR_RGBA(255,255,255,140));
		}
	}

	if(game.city_ctx)
	{
		// teksty w mieœcie
		for(Text& text : texts)
		{
			INT2 pt(Convert(text.pos));
			RECT rect = {pt.x-text.size.x/2, pt.y-text.size.y/2, pt.x+text.size.x/2, pt.y+text.size.y/2};
			GUI.DrawText(GUI.default_font, text.text, DT_SINGLELINE, BLACK, rect);
		}

		// linie do tekstów
		GUI.LineBegin();
		for(vector<Text>::iterator it = texts.begin(), end = texts.end(); it != end; ++it)
		{
			VEC2 pts[2] = {
				Convert(it->pos),
				Convert(it->anchor)
			};
			GUI.DrawLine(pts, 1, COLOR_RGBA(0,0,0,140));
		}
		GUI.LineEnd();
	}

	// nazwa lokacji
	RECT rect = {0,0,GUI.wnd_size.x-8,GUI.wnd_size.y-8};
	GUI.DrawText(GUI.default_font, game.GetCurrentLocationText(), DT_RIGHT|DT_OUTLINE, COLOR_RGBA(255,0,0,222), rect);
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

				VEC2 pt1 = Convert(it->pos),
					 pt2 = Convert(it2->pos);
				float w1 = float(it->size.x)/2,
					  h1 = float(it->size.y)/2,
					  w2 = float(it2->size.x)/2,
					  h2 = float(it2->size.y)/2;

				if(RectangleToRectangle(pt1.x-w1, pt1.y-h1, pt1.x+w1, pt1.y+h1,
					pt2.x-w2, pt2.y-h2, pt2.x+w2, pt2.y+h2))
				{
					VEC2 dir = it->pos - it2->pos;
					D3DXVec2Normalize(&dir, &dir);
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
