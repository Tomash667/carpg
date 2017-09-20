#include "Pch.h"
#include "Core.h"
#include "GameGui.h"
#include "Game.h"
#include "Language.h"
#include "Inventory.h"
#include "StatsPanel.h"
#include "TeamPanel.h"
#include "MpBox.h"
#include "Journal.h"
#include "Minimap.h"
#include "GameMessages.h"
#include "Controls.h"
#include "AIController.h"
#include "Chest.h"
#include "Door.h"
#include "Team.h"
#include "Class.h"
#include "Action.h"
#include "ActionPanel.h"
#include "BookPanel.h"

//-----------------------------------------------------------------------------
enum class TooltipGroup
{
	Sidebar,
	Buff,
	Bar,
	Action,
	Invalid = -1
};
enum Bar
{
	BAR_HP,
	BAR_STAMINA
};

//-----------------------------------------------------------------------------
static ObjectPool<SpeechBubble> SpeechBubblePool;

//=================================================================================================
GameGui::GameGui() : debug_info_size(0, 0), profiler_size(0, 0), use_cursor(false), game(Game::Get())
{
	txDeath = Str("death");
	txDeathAlone = Str("deathAlone");
	txGameTimeout = Str("gameTimeout");
	txChest = Str("chest");
	txDoor = Str("door");
	txDoorLocked = Str("doorLocked");
	txPressEsc = Str("pressEsc");
	txMenu = Str("menu");
	txHp = Str("hp");
	txStamina = Str("stamina");
	BuffInfo::LoadText();

	scrollbar.parent = this;
	visible = false;

	tooltip.Init(TooltipGetText(this, &GameGui::GetTooltip));

	// add panels
	mp_box = new MpBox;
	Add(mp_box);

	Inventory::LoadText();
	inventory = new Inventory;
	inventory->InitTooltip();
	inventory->title = Inventory::txInventory;
	inventory->mode = Inventory::INVENTORY;
	inventory->visible = false;
	Add(inventory);

	stats = new StatsPanel;
	Add(stats);

	team_panel = new TeamPanel;
	Add(team_panel);

	gp_trade = new GamePanelContainer;
	Add(gp_trade);

	inv_trade_mine = new Inventory;
	inv_trade_mine->title = Inventory::txInventory;
	inv_trade_mine->focus = true;
	gp_trade->Add(inv_trade_mine);

	inv_trade_other = new Inventory;
	inv_trade_other->title = Inventory::txInventory;
	gp_trade->Add(inv_trade_other);

	gp_trade->Hide();

	journal = new Journal;
	Add(journal);

	minimap = new Minimap;
	Add(minimap);

	game_messages = new GameMessages;

	action_panel = new ActionPanel;
	Add(action_panel);

	book_panel = new BookPanel;
	Add(book_panel);
}

//=================================================================================================
GameGui::~GameGui()
{
	delete game_messages;
	delete inventory;
	delete stats;
	delete journal;
	delete team_panel;
	delete minimap;
	delete mp_box;
	delete inv_trade_mine;
	delete inv_trade_other;
	delete gp_trade;
	delete action_panel;
	delete book_panel;

	SpeechBubblePool.Free(speech_bbs);
}

//=================================================================================================
void GameGui::Draw(ControlDrawData*)
{
	DrawFront();

	Container::Draw();

	DrawBack();

	game_messages->Draw();
}

//=================================================================================================
void GameGui::DrawFront()
{
	// death screen
	if(!Team.IsAnyoneAlive())
	{
		DrawDeathScreen();
		return;
	}

	// end of game screen
	if(game.koniec_gry)
	{
		DrawEndOfGameScreen();
		return;
	}

	// celownik
	if(game.pc->unit->action == A_SHOOT)
		GUI.DrawSprite(tCelownik, Center(32, 32));

	// obwódka bólu
	if(game.pc->dmgc > 0.f)
		GUI.DrawSpriteFull(tObwodkaBolu, COLOR_RGBA(255, 255, 255, (int)Clamp<float>(game.pc->dmgc / game.pc->unit->hp * 5 * 255, 0.f, 255.f)));

	// debug info
	if(game.debug_info && (!Net::Net::IsLocal() || !game.devmode))
		game.debug_info = false;
	if(game.debug_info)
	{
		sorted_units.clear();
		vector<Unit*>& units = *game.GetContext(*game.pc->unit).units;
		for(auto unit : units)
		{
			if(!unit->IsAlive())
				continue;
			float dist = Vec3::DistanceSquared(game.cam.from, unit->pos);
			sorted_units.push_back({ unit, dist, 255, nullptr });
		}

		SortUnits();

		for(auto& it : sorted_units)
		{
			Unit& u = *it.unit;
			Vec3 text_pos = u.visual_pos;
			text_pos.y += u.GetUnitHeight();
			if(u.IsAI())
			{
				AIController& ai = *u.ai;
				DrawUnitInfo(Format("%s (%s)\nB:%d, F:%d, LVL:%d\nAni:%d, A:%s %.2f\n%s, %d %.2f %d", u.GetName(), u.data->id.c_str(), u.busy, u.frozen, u.level,
					u.animation, str_ai_state[ai.state], ai.timer, str_ai_idle[ai.idle_action], ai.city_wander ? 1 : 0, ai.loc_timer, ai.unit->run_attack ? 1 : 0),
					u, text_pos, -1);
			}
			else
			{
				DrawUnitInfo(Format("%s (%s)\nB:%d, F:%d, Ani:%d, A:%d", u.GetName(), u.data->id.c_str(), u.busy, u.frozen, u.animation, u.player->action),
					u, text_pos, -1);
			}
		}
	}
	else
	{
		// near enemies/allies
		sorted_units.clear();
		for(vector<UnitView>::iterator it = game.unit_views.begin(), end = game.unit_views.end(); it != end; ++it)
		{
			int alpha;

			// 0.0 -> 0.1 niewidoczne
			// 0.1 -> 0.2 alpha 0->255
			// -0.2 -> -0.1 widoczne
			// -0.1 -> 0.0 alpha 255->0
			if(it->time > UNIT_VIEW_A)
			{
				if(it->time > UNIT_VIEW_B)
					alpha = 255;
				else
					alpha = int((it->time - UNIT_VIEW_A) * 255 * UNIT_VIEW_MUL);
			}
			else if(it->time < 0.f)
			{
				if(it->time < -UNIT_VIEW_A)
					alpha = 255;
				else
					alpha = int(-it->time * 255 * UNIT_VIEW_MUL);
			}
			else
				alpha = 0;

			if(alpha)
			{
				float dist = Vec3::DistanceSquared(game.cam.from, it->unit->pos);
				sorted_units.push_back({ it->unit, dist, alpha, &it->last_pos });
			}
		}

		SortUnits();

		for(auto& it : sorted_units)
			DrawUnitInfo(it.unit->GetName(), *it.unit, *it.last_pos, it.alpha);
	}

	// napis nad wybranym obiektem/postaci¹
	switch(game.pc_data.before_player)
	{
	case BP_UNIT:
		if(!game.debug_info)
		{
			Unit* u = game.pc_data.before_player_ptr.unit;
			bool dont_draw = false;
			for(vector<UnitView>::iterator it = game.unit_views.begin(), end = game.unit_views.end(); it != end; ++it)
			{
				if(it->unit == u)
				{
					if(it->time > UNIT_VIEW_B || it->time < -UNIT_VIEW_A)
						dont_draw = true;
					break;
				}
			}
			if(!dont_draw)
				DrawUnitInfo(u->GetName(), *u, u->GetUnitTextPos(), -1);
		}
		break;
	case BP_CHEST:
		{
			Vec3 text_pos = game.pc_data.before_player_ptr.chest->pos;
			text_pos.y += 0.75f;
			GUI.DrawText3D(GUI.default_font, txChest, DT_OUTLINE, WHITE, text_pos);
		}
		break;
	case BP_DOOR:
		{
			Vec3 text_pos = game.pc_data.before_player_ptr.door->pos;
			text_pos.y += 1.75f;
			GUI.DrawText3D(GUI.default_font, game.pc_data.before_player_ptr.door->locked == LOCK_NONE ? txDoor : txDoorLocked, DT_OUTLINE, WHITE, text_pos);
		}
		break;
	case BP_ITEM:
		{
			GroundItem& item = *game.pc_data.before_player_ptr.item;
			Mesh* mesh;
			if(IS_SET(item.item->flags, ITEM_GROUND_MESH))
				mesh = item.item->mesh;
			else
				mesh = game.aBag;
			Vec3 text_pos = item.pos;
			text_pos.y += mesh->head.bbox.v2.y;
			cstring text;
			if(item.count == 1)
				text = item.item->name.c_str();
			else
				text = Format("%s (%d)", item.item->name.c_str(), item.count);
			GUI.DrawText3D(GUI.default_font, text, DT_OUTLINE, WHITE, text_pos);
		}
		break;
	case BP_USABLE:
		{
			Usable& u = *game.pc_data.before_player_ptr.usable;
			BaseUsable& bu = BaseUsable::base_usables[u.type];
			Vec3 text_pos = u.pos;
			text_pos.y += u.GetMesh()->head.radius;
			GUI.DrawText3D(GUI.default_font, bu.name, DT_OUTLINE, WHITE, text_pos);
		}
		break;
	}

	// dymki z tekstem
	DrawSpeechBubbles();

	// dialog
	if(game.dialog_context.dialog_mode)
	{
		Int2 dsize(GUI.wnd_size.x - 256 - 8, 104);
		Int2 offset((GUI.wnd_size.x - dsize.x) / 2, 32);
		GUI.DrawItem(tDialog, offset, dsize, 0xAAFFFFFF, 16);

		Rect r = { offset.x + 6, offset.y + 6, offset.x + dsize.x - 12, offset.y + dsize.y - 4 };
		if(game.dialog_context.show_choices)
		{
			int off = int(scrollbar.offset);

			// zaznaczenie
			Rect r_img = Rect::Create(Int2(offset.x, offset.y + game.dialog_context.choice_selected*GUI.default_font->height - off + 6),
				Int2(dsize.x - 16, GUI.default_font->height));
			if(r_img.Bottom() >= r.Top() && r_img.Top() < r.Bottom())
			{
				if(r_img.Top() < r.Top())
					r_img.Top() = r.Top();
				else if(r_img.Bottom() > r.Bottom())
					r_img.Bottom() = r.Bottom();
				GUI.DrawSpriteRect(game.tEquipped, r_img, 0xAAFFFFFF);
			}

			// tekst
			LocalString s;
			if(Net::Net::IsLocal())
			{
				for(uint i = 0; i < game.dialog_context.choices.size(); ++i)
				{
					s += game.dialog_context.choices[i].msg;
					s += '\n';
				}
			}
			else
			{
				for(uint i = 0; i < game.dialog_choices.size(); ++i)
				{
					s += game.dialog_choices[i];
					s += '\n';
				}
			}
			Rect r2 = r;
			r2 -= Int2(0, off);
			GUI.DrawText(GUI.default_font, s, 0, BLACK, r2, &r);

			// pasek przewijania
			scrollbar.Draw();
		}
		else if(game.dialog_context.dialog_text)
			GUI.DrawText(GUI.default_font, game.dialog_context.dialog_text, DT_CENTER | DT_VCENTER, BLACK, r);
	}

	// get buffs
	int buffs;
	if(Net::Net::IsLocal())
		buffs = game.pc->unit->GetBuffs();
	else
		buffs = game.GetPlayerInfo(game.pc).buffs;

	// healthbar
	float wnd_scale = float(GUI.wnd_size.x) / 800;
	float hpp = Clamp(game.pc->unit->hp / game.pc->unit->hpmax, 0.f, 1.f);
	Rect part = { 0, 0, int(hpp * 256), 16 };
	Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 35));
	if(part.Right() > 0)
		GUI.DrawSprite2(!IS_SET(buffs, BUFF_POISON) ? tHpBar : tPoisonedHpBar, mat, &part, nullptr, WHITE);
	GUI.DrawSprite2(tBar, mat, nullptr, nullptr, WHITE);

	// stamina bar
	float stamina_p = game.pc->unit->stamina / game.pc->unit->stamina_max;
	part.Right() = int(stamina_p * 256);
	mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 17));
	if(part.Right() > 0)
		GUI.DrawSprite2(tStaminaBar, mat, &part, nullptr, WHITE);
	GUI.DrawSprite2(tBar, mat, nullptr, nullptr, WHITE);

	// buffs
	for(BuffImage& img : buff_images)
	{
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(buff_scale, buff_scale), nullptr, 0.f, &img.pos);
		GUI.DrawSprite2(img.tex, mat, nullptr, nullptr, WHITE);
	}

	float scale;
	int offset;

	int img_size = 64 * GUI.wnd_size.x / 1920;
	offset = img_size + 2;
	scale = float(img_size) / 64;
	Int2 spos(256.f*wnd_scale + offset, GUI.wnd_size.y - offset);

	// action
	if(!game.in_tutorial)
	{
		auto& action = game.pc->GetAction();
		PlayerController& pc = *game.pc;
		const float pad = 2.f;
		const float img_size = 32.f;
		const float img_ratio = 0.25f;

		float charge;
		if(pc.action_charges > 0 || pc.action_cooldown >= pc.action_recharge)
		{
			if(action.cooldown == 0)
				charge = 0.f;
			else
				charge = pc.action_cooldown / action.cooldown;
		}
		else
			charge = pc.action_recharge / action.recharge;

		if(charge == 0.f)
		{
			mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale * img_ratio, wnd_scale * img_ratio), nullptr, 0.f,
				&Vec2((256.f + pad) * wnd_scale, GUI.wnd_size.y - (img_size + pad) * wnd_scale));
			GUI.DrawSprite2(action.tex->tex, mat);
		}
		else
		{
			mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale * img_ratio, wnd_scale * img_ratio), nullptr, 0.f,
				&Vec2((256.f + pad) * wnd_scale, GUI.wnd_size.y - (img_size + pad) * wnd_scale));
			GUI.UseGrayscale(true);
			GUI.DrawSprite2(action.tex->tex, mat);
			GUI.UseGrayscale(false);
			if(charge < 1.f)
			{
				Rect part = { 0, 128 - int((1.f - charge) * 128), 128, 128 };
				GUI.DrawSprite2(action.tex->tex, mat, &part);
			}
			GUI.DrawSprite2(tActionCooldown, mat);
		}

		// charges
		if(action.charges > 1)
		{
			Rect r(int(wnd_scale * (256 + pad * 2)), int(GUI.wnd_size.y - (pad * 2) * wnd_scale) - 12, 0, 0);
			GUI.DrawText(GUI.fSmall, Format("%d/%d", pc.action_charges, action.charges), DT_SINGLELINE, BLACK, r);
		}
	}

	// shortcuts
	/*for(int i = 0; i<10; ++i)
	{
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(float(spos.x), float(spos.y)));
		GUI.DrawSprite2(tShortcut, &mat, nullptr, nullptr, WHITE);
		spos.x += offset;
	}*/

	// sidebar
	if(sidebar > 0.f)
	{
		int max = (int)SideButtonId::Max;
		if(Net::IsOnline())
			--max;
		int total = offset * max;
		spos.y = GUI.wnd_size.y - (GUI.wnd_size.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			TEX t;
			if(sidebar_state[i] == 0)
				t = tShortcut;
			else if(sidebar_state[i] == 1)
				t = tShortcutHover;
			else
				t = tShortcutDown;
			mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(float(GUI.wnd_size.x) - sidebar * offset, float(spos.y - i*offset)));
			GUI.DrawSprite2(t, mat, nullptr, nullptr, WHITE);
			GUI.DrawSprite2(tSideButton[i], mat, nullptr, nullptr, WHITE);
		}
	}

	// œciemnianie
	if(game.fallback_co != FALLBACK::NO)
	{
		int alpha;
		if(game.fallback_t < 0.f)
		{
			if(game.fallback_co == FALLBACK::NONE)
				alpha = 255;
			else
				alpha = int((1.f + game.fallback_t) * 255);
		}
		else
			alpha = int((1.f - game.fallback_t) * 255);

		GUI.DrawSpriteFull(game.tCzern, COLOR_RGBA(255, 255, 255, alpha));
	}
}

//=================================================================================================
void GameGui::DrawBack()
{
	// debug info
	if(game.debug_info2)
	{
		Unit& u = *game.pc->unit;
		cstring text;
		if(game.devmode)
		{
			text = Format("Pos: %g; %g; %g (%d; %d)\nRot: %g %s\nFps: %g", FLT_1(u.pos.x), FLT_1(u.pos.y), FLT_1(u.pos.z), int(u.pos.x / 2), int(u.pos.z / 2), FLT_2(u.rot),
				kierunek_nazwa_s[AngleToDir(Clip(u.rot))], FLT_1(game.GetFps()));
		}
		else
			text = Format("Fps: %g", FLT_1(game.GetFps()));
		Int2 s = GUI.default_font->CalculateSize(text);
		if(Int2::Distance(s, debug_info_size) < 32)
			debug_info_size = Int2::Max(s, debug_info_size);
		else
			debug_info_size = s;
		GUI.DrawItem(tDialog, Int2(0, 0), debug_info_size + Int2(24, 24), COLOR_RGBA(255, 255, 255, 128));
		Rect r = { 12, 12, 12 + s.x, 12 + s.y };
		GUI.DrawText(GUI.default_font, text, DT_NOCLIP, BLACK, r);
	}

	// profiler
	const string& str = Profiler::g_profiler.GetString();
	if(!str.empty())
	{
		Int2 block_size = GUI.default_font->CalculateSize(str) + Int2(24, 24);
		profiler_size = Int2::Max(block_size, profiler_size);
		GUI.DrawItem(tDialog, Int2(GUI.wnd_size.x - profiler_size.x, 0), profiler_size, COLOR_RGBA(255, 255, 255, 128));
		Rect rect = { GUI.wnd_size.x - profiler_size.x + 12, 12, GUI.wnd_size.x, GUI.wnd_size.y };
		GUI.DrawText(GUI.default_font, str, DT_LEFT, BLACK, rect);
	}

	// tooltip
	tooltip.Draw();
}

//=================================================================================================
void GameGui::DrawDeathScreen()
{
	if(game.death_screen <= 0)
		return;

	// czarne t³o
	int color;
	if(game.death_screen == 1)
		color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
	else
		color = WHITE;

	if((color & 0xFF000000) != 0)
		GUI.DrawSpriteFull(game.tCzern, color);

	// obrazek i tekst
	if(game.death_screen > 1)
	{
		if(game.death_screen == 2)
			color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
		else
			color = WHITE;

		if((color & 0xFF000000) != 0)
		{
			D3DSURFACE_DESC desc;
			V(game.tRip->GetLevelDesc(0, &desc));

			GUI.DrawSprite(game.tRip, Center(desc.Width, desc.Height), color);

			cstring text = Format(game.death_solo ? txDeathAlone : txDeath, game.pc->kills, game.total_kills - game.pc->kills);
			cstring text2 = Format("%s\n\n%s", text, game.death_screen == 3 ? txPressEsc : "\n");
			Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
			GUI.DrawText(GUI.default_font, text2, DT_CENTER | DT_BOTTOM, color, rect);
		}
	}
}

//=================================================================================================
void GameGui::DrawEndOfGameScreen()
{
	int color;
	if(game.death_fade < 1.f)
		color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
	else
		color = WHITE;

	GUI.DrawSpriteFull(game.tCzern, color);

	// obrazek
	D3DSURFACE_DESC desc;
	V(game.tEmerytura->GetLevelDesc(0, &desc));
	GUI.DrawSprite(game.tEmerytura, Center(desc.Width, desc.Height), color);

	// tekst
	cstring text = Format(txGameTimeout, game.pc->kills, game.total_kills - game.pc->kills);
	cstring text2 = Format("%s\n\n%s", text, game.death_fade >= 1.f ? txPressEsc : "\n");
	Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
	GUI.DrawText(GUI.default_font, text2, DT_CENTER | DT_BOTTOM, color, rect);
}

//=================================================================================================
void GameGui::DrawSpeechBubbles()
{
	// get list to sort
	sorted_speech_bbs.clear();
	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		SpeechBubble& sb = **it;

		Vec3 pos;
		if(sb.unit)
			pos = sb.last_pos = sb.unit->GetHeadPoint();
		else
			pos = sb.last_pos;

		if(Vec3::Distance(game.pc->unit->visual_pos, pos) > 20.f || !game.CanSee(game.pc->unit->pos, sb.last_pos))
		{
			sb.visible = false;
			continue;
		}

		Int2 pt;
		if(GUI.To2dPoint(pos, pt))
		{
			sb.visible = true;
			if(sb.time > 0.25f)
			{
				float cam_dist = Vec3::Distance(game.cam.from, pos);
				sorted_speech_bbs.push_back({ &sb, cam_dist, pt });
			}
		}
		else
			sb.visible = false;
	}

	// sort
	std::sort(sorted_speech_bbs.begin(), sorted_speech_bbs.end(), [](const SortedSpeechBubble& a, const SortedSpeechBubble& b)
	{
		return a.dist > b.dist;
	});

	// draw
	for(auto& it : sorted_speech_bbs)
	{
		auto& sb = *it.bubble;
		int a1, a2;
		if(sb.time >= 0.5f)
		{
			a1 = 0x80FFFFFF;
			a2 = 0xFFFFFFFF;
		}
		else
		{
			float alpha = (min(sb.time, 0.5f) - 0.25f) * 4;
			a1 = COLOR_RGBA(255, 255, 255, int(alpha * 0x80));
			a2 = COLOR_RGBA(255, 255, 255, int(alpha * 0xFF));
		}
		if(it.pt.x < sb.size.x / 2)
			it.pt.x = sb.size.x / 2;
		else if(it.pt.x > GUI.wnd_size.x - sb.size.x / 2)
			it.pt.x = GUI.wnd_size.x - sb.size.x / 2;
		if(it.pt.y < sb.size.y / 2)
			it.pt.y = sb.size.y / 2;
		else if(it.pt.y > GUI.wnd_size.y - sb.size.y / 2)
			it.pt.y = GUI.wnd_size.y - sb.size.y / 2;

		Rect rect = Rect::Create(Int2(it.pt.x - sb.size.x / 2, it.pt.y - sb.size.y / 2), sb.size);
		GUI.DrawItem(tBubble, rect.LeftTop(), sb.size, a1);
		GUI.DrawText(GUI.fSmall, sb.text, DT_CENTER | DT_VCENTER, a2, rect);
	}
}

//=================================================================================================
void GameGui::DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha)
{
	DWORD text_color;
	if(alpha == -1)
	{
		text_color = WHITE;
		alpha = 255;
	}
	else if(game.IsEnemy(unit, *game.pc->unit))
		text_color = COLOR_RGBA(255, 0, 0, alpha);
	else
		text_color = COLOR_RGBA(0, 255, 0, alpha);

	// text
	Rect r;
	GUI.DrawText3D(GUI.default_font, text, DT_OUTLINE, text_color, pos, &r);

	float hpp;
	if(!unit.IsAlive() && !unit.IsFollower())
		hpp = -1.f;
	else
		hpp = max(unit.GetHpp(), 0.f);
	DWORD color = COLOR_RGBA(255, 255, 255, alpha);

	if(hpp >= 0.f)
	{
		// hp background
		Rect r2(r.Left(), r.Bottom(), r.Right(), r.Bottom() + 4);
		GUI.DrawSpriteRect(tMinihp[0], r2, color);

		// hp
		int sizex = r2.SizeX();
		r2.Right() = r2.Left() + int(hpp * sizex);
		Rect r3 = { 0, 0, int(hpp * 64), 4 };
		GUI.DrawSpriteRectPart(tMinihp[1], r2, r3, color);

		// stamina
		if(game.devmode)
		{
			float stamina = max(unit.GetStaminap(), 0.f);
			r2 += Int2(0, 4);
			r3.Right() = int(stamina * 64);
			r2.Right() = r2.Left() + int(stamina * sizex);
			GUI.DrawSpriteRectPart(tMinistamina, r2, r3, color);
		}
	}
}

//=================================================================================================
void GameGui::Update(float dt)
{
	TooltipGroup group = TooltipGroup::Invalid;
	int id = -1;

	Container::Update(dt);

	UpdateSpeechBubbles(dt);

	game_messages->Update(dt);

	if(!GUI.HaveDialog() && !Game::Get().dialog_context.dialog_mode && Key.Down(VK_MENU))
		use_cursor = true;
	else
		use_cursor = false;

	const bool have_manabar = false;
	float hp_scale = float(GUI.wnd_size.x) / 800;

	// buffs
	int buffs;
	if(Net::Net::IsLocal())
		buffs = game.pc->unit->GetBuffs();
	else
		buffs = game.GetPlayerInfo(game.pc).buffs;

	buff_scale = GUI.wnd_size.x / 1024.f;
	float off = buff_scale * 33;
	float buf_posy = float(GUI.wnd_size.y - 5) - off - hp_scale * 35;
	Int2 buff_size(int(buff_scale * 32), int(buff_scale * 32));

	buff_images.clear();

	for(int i = 0; i < BUFF_COUNT; ++i)
	{
		int buff_bit = 1 << i;
		if(IS_SET(buffs, buff_bit))
		{
			auto& info = BuffInfo::info[i];
			buff_images.push_back(BuffImage(Vec2(2, buf_posy), info.img, i));
			buf_posy -= off;
		}
	}

	// sidebar
	int max = (int)SideButtonId::Max;
	if(Net::IsOnline())
		--max;

	sidebar_state[(int)SideButtonId::Inventory] = (inventory->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Journal] = (journal->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Stats] = (stats->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Team] = (team_panel->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Minimap] = (minimap->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Action] = (action_panel->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Talk] = 0;
	sidebar_state[(int)SideButtonId::Menu] = 0;

	bool anything = use_cursor;
	if(gp_trade->visible)
		anything = true;
	bool show_tooltips = anything;
	if(!anything)
	{
		for(int i = 0; i < (int)SideButtonId::Max; ++i)
		{
			if(sidebar_state[i] == 2)
			{
				anything = true;
				if(i != (int)SideButtonId::Minimap)
					show_tooltips = true;
				break;
			}
		}
	}

	// check cursor over item to show tooltip
	if(show_tooltips)
	{
		// for buffs
		for(BuffImage& img : buff_images)
		{
			if(PointInRect(GUI.cursor_pos, Int2(img.pos), buff_size))
			{
				group = TooltipGroup::Buff;
				id = img.id;
				break;
			}
		}

		// for healthbar
		float wnd_scale = float(GUI.wnd_size.x) / 800;
		Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 35));
		Rect r = GUI.GetSpriteRect(tBar, mat);
		if(r.IsInside(GUI.cursor_pos))
		{
			group = TooltipGroup::Bar;
			id = Bar::BAR_HP;
		}

		// for stamina bar
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 17));
		r = GUI.GetSpriteRect(tBar, mat);
		if(r.IsInside(GUI.cursor_pos))
		{
			group = TooltipGroup::Bar;
			id = Bar::BAR_STAMINA;
		}

		// action
		if(!game.in_tutorial)
		{
			auto& action = game.pc->GetAction();
			const float pad = 2.f;
			const float img_size = 32.f;
			const float img_ratio = 0.25f;
			mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale * img_ratio, wnd_scale * img_ratio), nullptr, 0.f,
				&Vec2((256.f + pad) * wnd_scale, GUI.wnd_size.y - (img_size + pad) * wnd_scale));
			r = GUI.GetSpriteRect(action.tex->tex, mat);
			if(r.IsInside(GUI.cursor_pos))
				group = TooltipGroup::Action;
		}
	}

	if(anything)
		sidebar += dt * 5;
	else
		sidebar -= dt * 5;
	sidebar = Clamp(sidebar, 0.f, 1.f);

	if(sidebar > 0.f && !GUI.HaveDialog())
	{
		int img_size = 64 * GUI.wnd_size.x / 1920;
		int offset = img_size + 2;
		int total = offset * max;
		int sposy = GUI.wnd_size.y - (GUI.wnd_size.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			if(PointInRect(GUI.cursor_pos, Int2(int(float(GUI.wnd_size.x) - sidebar * offset), sposy - i * offset), Int2(img_size, img_size)))
			{
				group = TooltipGroup::Sidebar;
				id = i;

				if(sidebar_state[i] == 0)
					sidebar_state[i] = 1;
				if(!GUI.HaveDialog() && Key.PressedRelease(VK_LBUTTON))
				{
					switch((SideButtonId)i)
					{
					case SideButtonId::Menu:
						game.ShowMenu();
						use_cursor = false;
						break;
					case SideButtonId::Team:
						ShowPanel(OpenPanel::Team);
						break;
					case SideButtonId::Minimap:
						ShowPanel(OpenPanel::Minimap);
						if(minimap->visible)
							use_cursor = true;
						break;
					case SideButtonId::Journal:
						ShowPanel(OpenPanel::Journal);
						break;
					case SideButtonId::Inventory:
						ShowPanel(OpenPanel::Inventory);
						break;
					case SideButtonId::Action:
						ShowPanel(OpenPanel::Action);
						break;
					case SideButtonId::Stats:
						ShowPanel(OpenPanel::Stats);
						break;
					case SideButtonId::Talk:
						mp_box->visible = !mp_box->visible;
						break;
					}
				}
			}
		}
	}

	tooltip.UpdateTooltip(dt, (int)group, id);
}

//=================================================================================================
void GameGui::UpdateSpeechBubbles(float dt)
{
	bool removes = false;

	dt *= game.game_speed;

	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		SpeechBubble& sb = **it;
		sb.length -= dt;
		if(sb.length > 0.f)
		{
			if(sb.visible)
				sb.time = min(sb.time + dt * 2, 0.75f);
			else
				sb.time = max(sb.time - dt * 2, 0.f);
		}
		else
		{
			sb.time -= dt;
			if(sb.time <= 0.25f)
			{
				if(sb.unit)
				{
					sb.unit->talking = false;
					sb.unit->bubble = nullptr;
					// fix na crash, powody dla których ani jest NULLem nie s¹ znane :S
					if(sb.unit->mesh_inst)
						sb.unit->mesh_inst->need_update = true;
				}
				SpeechBubblePool.Free(*it);
				*it = nullptr;
				removes = true;
			}
		}
	}

	if(removes)
		RemoveNullElements(speech_bbs);
}

//=================================================================================================
bool GameGui::NeedCursor() const
{
	if(game.dialog_context.dialog_mode || use_cursor)
		return true;
	return Container::NeedCursor();
}

//=================================================================================================
void GameGui::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		Int2 dsize(GUI.wnd_size.x - 256 - 8, 104);
		Int2 offset((GUI.wnd_size.x - dsize.x) / 2, 32);
		scrollbar.size = Int2(12, 104);
		scrollbar.global_pos = scrollbar.pos = Int2(dsize.x + offset.x - 16, offset.y);
		dialog_pos = offset;
		dialog_size = dsize;
		tooltip.Clear();
	}
}

//=================================================================================================
void GameGui::AddSpeechBubble(Unit* unit, cstring text)
{
	assert(unit && text);

	// create new if there isn't already created
	if(!unit->bubble)
	{
		unit->bubble = SpeechBubblePool.Get();
		speech_bbs.push_back(unit->bubble);
	}

	// calculate size
	Int2 s = GUI.fSmall->CalculateSize(text);
	int total = s.x;
	int lines = 1 + total / 400;

	// setup
	unit->bubble->text = text;
	unit->bubble->unit = unit;
	unit->bubble->size = Int2(total / lines + 20, s.y*lines + 20);
	unit->bubble->time = 0.f;
	unit->bubble->length = 1.5f + float(strlen(text)) / 20;
	unit->bubble->visible = false;
	unit->bubble->last_pos = unit->GetHeadPoint();

	unit->talking = true;
	unit->talk_timer = 0.f;
}

//=================================================================================================
void GameGui::AddSpeechBubble(const Vec3& pos, cstring text)
{
	assert(text);

	SpeechBubble* sb = SpeechBubblePool.Get();

	Int2 size = GUI.fSmall->CalculateSize(text);
	int total = size.x;
	int lines = 1 + total / 400;

	sb->text = text;
	sb->unit = nullptr;
	sb->size = Int2(total / lines + 20, size.y*lines + 20);
	sb->time = 0.f;
	sb->length = 1.5f + float(strlen(text)) / 20;
	sb->visible = true;
	sb->last_pos = pos;

	speech_bbs.push_back(sb);
}

//=================================================================================================
void GameGui::Reset()
{
	SpeechBubblePool.Free(speech_bbs);
	Event(GuiEvent_Show);
	use_cursor = false;
	sidebar = 0.f;
}

//=================================================================================================
bool GameGui::UpdateChoice(DialogContext& ctx, int choices)
{
	Int2 dsize(GUI.wnd_size.x - 256 - 8, 104);
	Int2 offset((GUI.wnd_size.x - dsize.x) / 2, 32 + 6);

	// element pod kursorem
	int cursor_choice = -1;
	if(GUI.cursor_pos.x >= offset.x && GUI.cursor_pos.x < offset.x + dsize.x - 16 && GUI.cursor_pos.y >= offset.y && GUI.cursor_pos.y < offset.y + dsize.y - 12)
	{
		int w = (GUI.cursor_pos.y - offset.y + int(scrollbar.offset)) / GUI.default_font->height;
		if(w < choices)
			cursor_choice = w;
	}

	// zmiana zaznaczonego elementu myszk¹
	if(GUI.cursor_pos != game.dialog_cursor_pos)
	{
		game.dialog_cursor_pos = GUI.cursor_pos;
		if(cursor_choice != -1)
			ctx.choice_selected = cursor_choice;
	}

	// strza³ka w górê/dó³
	bool moved = false;
	if(ctx.choice_selected != 0 && game.KeyPressedReleaseAllowed(GK_MOVE_FORWARD))
	{
		--ctx.choice_selected;
		moved = true;
	}
	if(ctx.choice_selected != choices - 1 && game.KeyPressedReleaseAllowed(GK_MOVE_BACK))
	{
		++ctx.choice_selected;
		moved = true;
	}
	if(moved && choices > 5)
	{
		scrollbar.offset = float(GUI.default_font->height*(ctx.choice_selected - 2));
		if(scrollbar.offset < 0.f)
			scrollbar.offset = 0.f;
		else if(scrollbar.offset + scrollbar.part > scrollbar.total)
			scrollbar.offset = float(scrollbar.total - scrollbar.part);
	}

	// wybór opcji dialogowej z klawiatury (1,2,3,..,9)
	if(game.AllowKeyboard() && !Key.Down(VK_SHIFT))
	{
		for(int i = 0; i <= min(8, choices); ++i)
		{
			if(Key.PressedRelease((byte)'1' + i))
			{
				ctx.choice_selected = i;
				return true;
			}
		}
	}

	// wybieranie enterem/esc/spacj¹
	if(game.KeyPressedReleaseAllowed(GK_SELECT_DIALOG))
		return true;
	else if(ctx.dialog_esc != -1 && game.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
	{
		ctx.choice_selected = ctx.dialog_esc;
		return true;
	}

	// wybieranie klikniêciem
	if(game.AllowMouse() && cursor_choice != -1 && Key.PressedRelease(VK_LBUTTON))
	{
		if(ctx.is_local)
			game.pc_data.wasted_key = VK_LBUTTON;
		ctx.choice_selected = cursor_choice;
		return true;
	}

	// aktualizacja paska przewijania
	scrollbar.mouse_focus = focus;
	if(Key.Focus() && PointInRect(GUI.cursor_pos, dialog_pos, dialog_size) && scrollbar.ApplyMouseWheel())
		game.dialog_cursor_pos = Int2(-1, -1);
	scrollbar.Update(0.f);

	return false;
}

//=================================================================================================
void GameGui::UpdateScrollbar(int choices)
{
	scrollbar.part = 104 - 12;
	scrollbar.offset = 0.f;
	scrollbar.total = choices * GUI.default_font->height;
	scrollbar.LostFocus();
}

//=================================================================================================
void GameGui::GetTooltip(TooltipController*, int _group, int id)
{
	TooltipGroup group = (TooltipGroup)_group;

	if(group == TooltipGroup::Invalid)
	{
		tooltip.anything = false;
		return;
	}

	tooltip.anything = true;
	tooltip.img = nullptr;
	tooltip.big_text.clear();
	tooltip.small_text.clear();

	switch(group)
	{
	case TooltipGroup::Buff:
		{
			auto& info = BuffInfo::info[id];
			tooltip.text = info.text;
		}
		break;
	case TooltipGroup::Sidebar:
		{
			GAME_KEYS gk;

			switch((SideButtonId)id)
			{
			case SideButtonId::Menu:
				tooltip.text = Format("%s (%s)", txMenu, game.controls->key_text[VK_ESCAPE]);
				return;
			case SideButtonId::Team:
				gk = GK_TEAM_PANEL;
				break;
			case SideButtonId::Minimap:
				gk = GK_MINIMAP;
				break;
			case SideButtonId::Journal:
				gk = GK_JOURNAL;
				break;
			case SideButtonId::Inventory:
				gk = GK_INVENTORY;
				break;
			case SideButtonId::Action:
				gk = GK_ACTION_PANEL;
				break;
			case SideButtonId::Stats:
				gk = GK_STATS;
				break;
			case SideButtonId::Talk:
				gk = GK_TALK_BOX;
				break;
			}

			tooltip.text = Game::Get().GetShortcutText(gk);
		}
		break;
	case TooltipGroup::Bar:
		{
			Unit* u = game.pc->unit;
			if(id == BAR_HP)
			{
				int hp = (int)u->hp;
				if(hp == 0 && u->hp > 0.f)
					hp = 1;
				int hpmax = (int)u->hpmax;
				tooltip.text = Format("%s: %d/%d", txHp, hp, hpmax);
			}
			else if(id == BAR_STAMINA)
			{
				int stamina = (int)u->stamina;
				int stamina_max = (int)u->stamina_max;
				tooltip.text = Format("%s: %d/%d", txStamina, stamina, stamina_max);
			}
		}
		break;
	case TooltipGroup::Action:
		{
			auto& action = game.pc->GetAction();
			tooltip.text = action.name;
			tooltip.small_text = action.desc;
		}
		break;
	}
}

//=================================================================================================
bool GameGui::HavePanelOpen() const
{
	return stats->visible || inventory->visible || team_panel->visible || gp_trade->visible || journal->visible || minimap->visible || action_panel->visible;
}

//=================================================================================================
void GameGui::ClosePanels(bool close_mp_box)
{
	if(stats->visible)
		stats->Hide();
	if(inventory->visible)
		inventory->Hide();
	if(team_panel->visible)
		team_panel->Hide();
	if(journal->visible)
		journal->Hide();
	if(minimap->visible)
		minimap->Hide();
	if(gp_trade->visible)
		gp_trade->Hide();
	if(close_mp_box && mp_box->visible)
		mp_box->visible = false;
	if(action_panel->visible)
		action_panel->Hide();
}

//=================================================================================================
void GameGui::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("celownik.png", tCelownik);
	tex_mgr.AddLoadTask("bubble.png", tBubble);
	tex_mgr.AddLoadTask("czerwono.png", tObwodkaBolu);
	tex_mgr.AddLoadTask("bar.png", tBar);
	tex_mgr.AddLoadTask("hp_bar.png", tHpBar);
	tex_mgr.AddLoadTask("poisoned_hp_bar.png", tPoisonedHpBar);
	tex_mgr.AddLoadTask("stamina_bar.png", tStaminaBar);
	tex_mgr.AddLoadTask("mana_bar.png", tManaBar);
	tex_mgr.AddLoadTask("shortcut.png", tShortcut);
	tex_mgr.AddLoadTask("shortcut_hover.png", tShortcutHover);
	tex_mgr.AddLoadTask("shortcut_down.png", tShortcutDown);
	tex_mgr.AddLoadTask("bt_menu.png", tSideButton[(int)SideButtonId::Menu]);
	tex_mgr.AddLoadTask("bt_team.png", tSideButton[(int)SideButtonId::Team]);
	tex_mgr.AddLoadTask("bt_minimap.png", tSideButton[(int)SideButtonId::Minimap]);
	tex_mgr.AddLoadTask("bt_journal.png", tSideButton[(int)SideButtonId::Journal]);
	tex_mgr.AddLoadTask("bt_inventory.png", tSideButton[(int)SideButtonId::Inventory]);
	tex_mgr.AddLoadTask("bt_action.png", tSideButton[(int)SideButtonId::Action]);
	tex_mgr.AddLoadTask("bt_stats.png", tSideButton[(int)SideButtonId::Stats]);
	tex_mgr.AddLoadTask("bt_talk.png", tSideButton[(int)SideButtonId::Talk]);
	tex_mgr.AddLoadTask("minihp.png", tMinihp[0]);
	tex_mgr.AddLoadTask("minihp2.png", tMinihp[1]);
	tex_mgr.AddLoadTask("ministamina.png", tMinistamina);
	tex_mgr.AddLoadTask("action_cooldown.png", tActionCooldown);

	BuffInfo::LoadImages();
	journal->LoadData();
	minimap->LoadData();
	team_panel->LoadData();
	action_panel->LoadData();
	book_panel->LoadData();
}

//=================================================================================================
void GameGui::GetGamePanels(vector<GamePanel*>& panels)
{
	panels.push_back(inventory);
	panels.push_back(stats);
	panels.push_back(team_panel);
	panels.push_back(journal);
	panels.push_back(minimap);
	panels.push_back(inv_trade_mine);
	panels.push_back(inv_trade_other);
	panels.push_back(mp_box);
	panels.push_back(action_panel);
}

//=================================================================================================
OpenPanel GameGui::GetOpenPanel()
{
	if(stats->visible)
		return OpenPanel::Stats;
	else if(inventory->visible)
		return OpenPanel::Inventory;
	else if(team_panel->visible)
		return OpenPanel::Team;
	else if(journal->visible)
		return OpenPanel::Journal;
	else if(minimap->visible)
		return OpenPanel::Minimap;
	else if(gp_trade->visible)
		return OpenPanel::Trade;
	else if(action_panel->visible)
		return OpenPanel::Action;
	else
		return OpenPanel::None;
}

//=================================================================================================
void GameGui::ShowPanel(OpenPanel to_open, OpenPanel open)
{
	if(open == OpenPanel::Unknown)
		open = GetOpenPanel();

	// close current panel
	switch(open)
	{
	case OpenPanel::None:
		break;
	case OpenPanel::Stats:
		stats->Hide();
		break;
	case OpenPanel::Inventory:
		inventory->Hide();
		break;
	case OpenPanel::Team:
		team_panel->Hide();
		break;
	case OpenPanel::Journal:
		journal->Hide();
		break;
	case OpenPanel::Minimap:
		minimap->Hide();
		break;
	case OpenPanel::Trade:
		game.OnCloseInventory();
		gp_trade->Hide();
		break;
	case OpenPanel::Action:
		action_panel->Hide();
		break;
	}

	// open new panel
	if(open != to_open)
	{
		switch(to_open)
		{
		case OpenPanel::Stats:
			stats->Show();
			break;
		case OpenPanel::Inventory:
			inventory->Show();
			break;
		case OpenPanel::Team:
			team_panel->Show();
			break;
		case OpenPanel::Journal:
			journal->Show();
			break;
		case OpenPanel::Minimap:
			minimap->Show();
			break;
		case OpenPanel::Action:
			action_panel->Show();
			break;
		}
	}
	else
		use_cursor = false;
}

//=================================================================================================
void GameGui::PositionPanels()
{
	float scale = float(GUI.wnd_size.x) / 1024;
	Int2 pos = Int2(int(scale * 48), int(scale * 32));
	Int2 size = Int2(GUI.wnd_size.x - pos.x * 2, GUI.wnd_size.y - pos.x * 2);

	stats->global_pos = stats->pos = pos;
	stats->size = size;
	team_panel->global_pos = team_panel->pos = pos;
	team_panel->size = size;
	inventory->global_pos = inventory->pos = pos;
	inventory->size = size;
	inv_trade_other->global_pos = inv_trade_other->pos = pos;
	inv_trade_other->size = Int2(size.x, (size.y - 32) / 2);
	inv_trade_mine->global_pos = inv_trade_mine->pos = Int2(pos.x, inv_trade_other->pos.y + inv_trade_other->size.y + 16);
	inv_trade_mine->size = inv_trade_other->size;
	minimap->size = Int2(size.y, size.y);
	minimap->global_pos = minimap->pos = Int2((GUI.wnd_size.x - minimap->size.x) / 2, (GUI.wnd_size.y - minimap->size.y) / 2);
	journal->size = minimap->size;
	journal->global_pos = journal->pos = minimap->pos;
	mp_box->size = Int2((GUI.wnd_size.x - 32) / 2, (GUI.wnd_size.y - 64) / 4);
	mp_box->global_pos = mp_box->pos = Int2(GUI.wnd_size.x - pos.x - mp_box->size.x, GUI.wnd_size.y - pos.x - mp_box->size.y);
	action_panel->global_pos = action_panel->pos = pos;
	action_panel->size = size;

	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		(*it)->Event(GuiEvent_Moved);
		(*it)->Event(GuiEvent_Resize);
	}
}

//=================================================================================================
void GameGui::Save(FileWriter& f) const
{
	f << speech_bbs.size();
	for(const SpeechBubble* p_sb : speech_bbs)
	{
		const SpeechBubble& sb = *p_sb;
		f.WriteString2(sb.text);
		f << (sb.unit ? sb.unit->refid : -1);
		f << sb.size;
		f << sb.time;
		f << sb.length;
		f << sb.visible;
		f << sb.last_pos;
	}
}

//=================================================================================================
void GameGui::Load(FileReader& f)
{
	speech_bbs.resize(f.Read<uint>());
	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		*it = SpeechBubblePool.Get();
		SpeechBubble& sb = **it;
		f.ReadString2(sb.text);
		sb.unit = Unit::GetByRefid(f.Read<int>());
		f >> sb.size;
		f >> sb.time;
		f >> sb.length;
		f >> sb.visible;
		f >> sb.last_pos;
	}
}

//=================================================================================================
void GameGui::Setup()
{
	Action* action;
	if(game.in_tutorial)
		action = nullptr;
	else
		action = &game.pc->GetAction();
	action_panel->Init(action);
}

//=================================================================================================
void GameGui::SortUnits()
{
	std::sort(sorted_units.begin(), sorted_units.end(), [](const SortedUnitView& a, const SortedUnitView& b)
	{
		return a.dist > b.dist;
	});
}
