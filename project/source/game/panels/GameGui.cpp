#include "Pch.h"
#include "GameCore.h"
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
#include "Profiler.h"
#include "GameStats.h"
#include "Level.h"
#include "GroundItem.h"
#include "ResourceManager.h"
#include "GlobalGui.h"
#include "QuestManager.h"
#include "Quest_Tutorial.h"
#include "PlayerInfo.h"

//-----------------------------------------------------------------------------
GlobalGui* global::gui;
const float UNIT_VIEW_A = 0.2f;
const float UNIT_VIEW_B = 0.4f;
const int UNIT_VIEW_MUL = 5;
extern const int ITEM_IMAGE_SIZE;

cstring order_str[ORDER_MAX] = {
	"NONE",
	"WANDER",
	"WAIT",
	"FOLLOW",
	"LEAVE",
	"MOVE",
	"LOOK_AT",
	"ESCAPE_TO",
	"ESCAPE_TO_UNIT",
	"GOTO_INN"
};

//-----------------------------------------------------------------------------
enum class TooltipGroup
{
	Sidebar,
	Buff,
	Bar,
	Shortcut,
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
	scrollbar.parent = this;
	visible = false;

	tooltip.Init(TooltipGetText(this, &GameGui::GetTooltip));
}

//=================================================================================================
GameGui::~GameGui()
{
	SpeechBubblePool.Free(speech_bbs);
}

//=================================================================================================
void GameGui::LoadLanguage()
{
	Language::Section s = Language::GetSection("GameGui");
	txDeath = s.Get("death");
	txDeathAlone = s.Get("deathAlone");
	txGameTimeout = s.Get("gameTimeout");
	txChest = s.Get("chest");
	txDoor = s.Get("door");
	txDoorLocked = s.Get("doorLocked");
	txPressEsc = s.Get("pressEsc");
	txMenu = s.Get("menu");
	txHp = s.Get("hp");
	txStamina = s.Get("stamina");
	txMeleeWeapon = s.Get("meleeWeapon");
	txRangedWeapon = s.Get("rangedWeapon");
	txPotion = s.Get("potion");
	txMeleeWeaponDesc = s.Get("meleeWeaponDesc");
	txRangedWeaponDesc = s.Get("rangedWeaponDesc");
	txPotionDesc = s.Get("potionDesc");
	BuffInfo::LoadText();
}

//=================================================================================================
void GameGui::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("crosshair.png", tCrosshair);
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
	tex_mgr.AddLoadTask("sword-brandish.png", tMelee);
	tex_mgr.AddLoadTask("bow-arrow.png", tRanged);
	tex_mgr.AddLoadTask("health-potion.png", tPotion);
	tex_mgr.AddLoadTask("emerytura.jpg", tEmerytura);
	tex_mgr.AddLoadTask("equipped.png", tEquipped);

	BuffInfo::LoadImages();
}

//=================================================================================================
void GameGui::Draw(ControlDrawData*)
{
	DrawFront();

	Container::Draw();

	DrawBack();

	game.gui->messages->Draw();
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
	if(game.end_of_game)
	{
		DrawEndOfGameScreen();
		return;
	}

	PlayerController& pc = *game.pc;

	// crosshair
	if(pc.unit->weapon_state == WS_TAKEN && pc.unit->weapon_taken == W_BOW)
		GUI.DrawSprite(tCrosshair, Center(32, 32));

	// obwódka bólu
	if(pc.dmgc > 0.f)
		GUI.DrawSpriteFull(tObwodkaBolu, Color::Alpha((int)Clamp<float>(pc.dmgc / pc.unit->hp * 5 * 255, 0.f, 255.f)));

	// debug info
	if(game.debug_info && !game.devmode)
		game.debug_info = false;
	if(game.debug_info)
	{
		sorted_units.clear();
		vector<Unit*>& units = L.GetArea(*pc.unit).units;
		for(auto unit : units)
		{
			if(!unit->IsAlive())
				continue;
			float dist = Vec3::DistanceSquared(L.camera.from, unit->pos);
			sorted_units.push_back({ unit, dist, 255, nullptr });
		}

		SortUnits();

		for(auto& it : sorted_units)
		{
			Unit& u = *it.unit;
			Vec3 text_pos = u.visual_pos;
			text_pos.y += u.GetUnitHeight();
			LocalString str;
			if(Net::IsOnline())
				str = Format("%d - ", u.netid);
			str += Format("%s (%s) [%u]", u.GetRealName(), u.data->id.c_str(), u.refs);
			if(Net::IsLocal())
			{
				if(u.IsAI())
				{
					AIController& ai = *u.ai;
					str += Format("\nB:%d, F:%d, LVL:%d\nAni:%d, A:%d, Ai:%s%s T:%.2f LT:%.2f\nO:%s", u.busy, u.frozen, u.level,
						u.animation, u.action, str_ai_state[ai.state], ai.state == AIController::Idle ? Format("(%s)", str_ai_idle[ai.idle_action]) : "",
						ai.timer, ai.loc_timer, order_str[u.order]);
					if(u.order_timer > 0.f)
						str += Format(" %.2f", u.order_timer);
					switch(u.order)
					{
					case ORDER_MOVE:
					case ORDER_LOOK_AT:
						str += Format(" Pos:%.1f;%.1f;%.1f", u.order_pos.x, u.order_pos.y, u.order_pos.z);
						break;
					}
				}
				else
					str += Format("\nB:%d, F:%d, LVL:%d, Ani:%d, A:%d", u.busy, u.frozen, u.level, u.animation, u.player->action);
			}
			DrawUnitInfo(str, u, text_pos, -1);
		}
	}
	else
	{
		// near enemies/allies
		sorted_units.clear();
		for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
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
				float dist = Vec3::DistanceSquared(L.camera.from, it->unit->pos);
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
			for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
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
			GUI.DrawText3D(GUI.default_font, txChest, DTF_OUTLINE, Color::White, text_pos);
		}
		break;
	case BP_DOOR:
		{
			Vec3 text_pos = game.pc_data.before_player_ptr.door->pos;
			text_pos.y += 1.75f;
			GUI.DrawText3D(GUI.default_font, game.pc_data.before_player_ptr.door->locked == LOCK_NONE ? txDoor : txDoorLocked, DTF_OUTLINE, Color::White, text_pos);
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
			GUI.DrawText3D(GUI.default_font, text, DTF_OUTLINE, Color::White, text_pos);
		}
		break;
	case BP_USABLE:
		{
			Usable& u = *game.pc_data.before_player_ptr.usable;
			Vec3 text_pos = u.pos;
			text_pos.y += u.GetMesh()->head.radius;
			GUI.DrawText3D(GUI.default_font, u.base->name, DTF_OUTLINE, Color::White, text_pos);
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
			if(Net::IsLocal())
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
			GUI.DrawText(GUI.default_font, s, 0, Color::Black, r2, &r);

			// pasek przewijania
			scrollbar.Draw();
		}
		else if(game.dialog_context.dialog_text)
			GUI.DrawText(GUI.default_font, game.dialog_context.dialog_text, DTF_CENTER | DTF_VCENTER, Color::Black, r);
	}

	// get buffs
	int buffs = pc.unit->GetBuffs();

	// healthbar
	float wnd_scale = float(GUI.wnd_size.x) / 800;
	float hpp = Clamp(pc.unit->hp / pc.unit->hpmax, 0.f, 1.f);
	Rect part = { 0, 0, int(hpp * 256), 16 };
	Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 35));
	if(part.Right() > 0)
		GUI.DrawSprite2(!IS_SET(buffs, BUFF_POISON) ? tHpBar : tPoisonedHpBar, mat, &part, nullptr, Color::White);
	GUI.DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);

	// stamina bar
	float stamina_p = pc.unit->stamina / pc.unit->stamina_max;
	part.Right() = int(stamina_p * 256);
	mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(wnd_scale, wnd_scale), nullptr, 0.f, &Vec2(0.f, float(GUI.wnd_size.y) - wnd_scale * 17));
	if(part.Right() > 0)
		GUI.DrawSprite2(tStaminaBar, mat, &part, nullptr, Color::White);
	GUI.DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);

	// buffs
	for(BuffImage& img : buff_images)
	{
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(buff_scale, buff_scale), nullptr, 0.f, &img.pos);
		GUI.DrawSprite2(img.tex, mat, nullptr, nullptr, Color::White);
	}

	float scale;
	int offset;

	int img_size = 76 * GUI.wnd_size.x / 1920;
	offset = img_size + 2;
	scale = float(img_size) / 64;
	Int2 spos(256.f*wnd_scale, GUI.wnd_size.y - offset);

	// shortcuts
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		const Shortcut& shortcut = pc.shortcuts[i];

		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(float(spos.x), float(spos.y)));
		GUI.DrawSprite2(tShortcut, mat);

		Rect r(spos.x + 2, spos.y + 2);
		r.p2.x += img_size - 4;
		r.p2.y += img_size - 4;

		TEX icon = nullptr;
		int count = 0, icon_size = 128;
		bool enabled = true, is_action = false, equipped = false;
		if(shortcut.type == Shortcut::TYPE_SPECIAL)
		{
			switch(shortcut.value)
			{
			case Shortcut::SPECIAL_MELEE_WEAPON:
				icon = tMelee;
				enabled = pc.unit->HaveWeapon();
				break;
			case Shortcut::SPECIAL_RANGED_WEAPON:
				icon = tRanged;
				enabled = pc.unit->HaveBow();
				break;
			case Shortcut::SPECIAL_ACTION:
				is_action = true;
				break;
			case Shortcut::SPECIAL_HEALING_POTION:
				icon = tPotion;
				enabled = pc.GetHealingPotion() != -1;
				break;
			}
		}
		else if(shortcut.type == Shortcut::TYPE_ITEM)
		{
			icon = shortcut.item->icon;
			icon_size = ITEM_IMAGE_SIZE;
			if(shortcut.item->IsWearable())
			{
				if(pc.unit->HaveItemEquipped(shortcut.item))
				{
					equipped = true;
					enabled = true;
				}
				else
					enabled = pc.unit->HaveItem(shortcut.item, Team.HaveOtherActiveTeamMember());
			}
			else if(shortcut.item->IsStackable())
			{
				count = pc.unit->CountItem(shortcut.item);
				enabled = (count > 0);
			}
			else
				enabled = pc.unit->HaveItem(shortcut.item);
		}

		float scale2 = float(img_size - 2) / icon_size;
		mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale2, scale2), nullptr, 0.f, &Vec2(float(spos.x + 1), float(spos.y + 1)));
		if(icon)
		{
			if(drag_and_drop == 2 && drag_and_drop_type == -1 && drag_and_drop_index == i)
				drag_and_drop_icon = icon;

			if(enabled)
			{
				if(equipped)
					GUI.DrawSprite2(tEquipped, mat);
				GUI.DrawSprite2(icon, mat);
				if(count > 0)
					GUI.DrawText(GUI.fSmall, Format("%d", count), DTF_RIGHT | DTF_BOTTOM, Color::Black, r);
			}
			else
			{
				GUI.UseGrayscale(true);
				GUI.DrawSprite2(icon, mat, nullptr, nullptr, Color::Alpha(128));
				GUI.UseGrayscale(false);
			}
		}
		else if(is_action)
		{
			Action& action = pc.GetAction();
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

			if(drag_and_drop == 2 && drag_and_drop_type == -1 && drag_and_drop_index == i)
				drag_and_drop_icon = action.tex->tex;

			if(charge == 0.f)
				GUI.DrawSprite2(action.tex->tex, mat);
			else
			{
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
				GUI.DrawText(GUI.fSmall, Format("%d/%d", pc.action_charges, action.charges), DTF_RIGHT | DTF_BOTTOM, Color::Black, r);
		}

		const GameKey& gk = GKey[GK_SHORTCUT1 + i];
		if(gk[0] != VK_NONE)
			GUI.DrawText(GUI.fSmall, global::gui->controls->key_text[gk[0]], DTF_SINGLELINE, Color::White, r);

		spos.x += offset;
	}

	// sidebar
	if(sidebar > 0.f)
	{
		int max = (int)SideButtonId::Max;
		if(!Net::IsOnline())
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
			mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(float(GUI.wnd_size.x) - sidebar * offset, float(spos.y - i * offset)));
			GUI.DrawSprite2(t, mat, nullptr, nullptr, Color::White);
			GUI.DrawSprite2(tSideButton[i], mat, nullptr, nullptr, Color::White);
		}
	}

	// œciemnianie
	if(game.fallback_type != FALLBACK::NO)
	{
		int alpha;
		if(game.fallback_t < 0.f)
		{
			if(game.fallback_type == FALLBACK::NONE)
				alpha = 255;
			else
				alpha = int((1.f + game.fallback_t) * 255);
		}
		else
			alpha = int((1.f - game.fallback_t) * 255);

		GUI.DrawSpriteFull(game.tCzern, Color::Alpha(alpha));
	}
}

//=================================================================================================
void GameGui::DrawBack()
{
	if(drag_and_drop == 2 && drag_and_drop_icon)
	{
		int img_size = 76 * GUI.wnd_size.x / 1920;
		float scale = float(img_size) / Texture::GetSize(drag_and_drop_icon).x;
		Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &Vec2(GUI.cursor_pos + Int2(16, 16)));
		GUI.DrawSprite2(drag_and_drop_icon, mat);
	}

	// debug info
	if(game.debug_info2)
	{
		Unit& u = *game.pc->unit;
		cstring text;
		if(game.devmode)
		{
			text = Format("Pos: %g; %g; %g (%d; %d)\nRot: %g %s\nFps: %g", FLT10(u.pos.x), FLT10(u.pos.y), FLT10(u.pos.z), int(u.pos.x / 2), int(u.pos.z / 2),
				FLT100(u.rot), dir_name_short[AngleToDir(Clip(u.rot))], FLT10(game.GetFps()));
		}
		else
			text = Format("Fps: %g", FLT10(game.GetFps()));
		Int2 s = GUI.default_font->CalculateSize(text);
		if(Int2::Distance(s, debug_info_size) < 32)
			debug_info_size = Int2::Max(s, debug_info_size);
		else
			debug_info_size = s;
		GUI.DrawItem(tDialog, Int2(0, 0), debug_info_size + Int2(24, 24), Color::Alpha(128));
		Rect r = { 12, 12, 12 + s.x, 12 + s.y };
		GUI.DrawText(GUI.default_font, text, 0, Color::Black, r);
	}

	// profiler
	const string& str = Profiler::g_profiler.GetString();
	if(!str.empty())
	{
		Int2 block_size = GUI.default_font->CalculateSize(str) + Int2(24, 24);
		profiler_size = Int2::Max(block_size, profiler_size);
		GUI.DrawItem(tDialog, Int2(GUI.wnd_size.x - profiler_size.x, 0), profiler_size, Color::Alpha(128));
		Rect rect = { GUI.wnd_size.x - profiler_size.x + 12, 12, GUI.wnd_size.x, GUI.wnd_size.y };
		GUI.DrawText(GUI.default_font, str, DTF_LEFT, Color::Black, rect);
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
		color = Color::White;

	if((color & 0xFF000000) != 0)
		GUI.DrawSpriteFull(game.tCzern, color);

	// obrazek i tekst
	if(game.death_screen > 1)
	{
		if(game.death_screen == 2)
			color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
		else
			color = Color::White;

		if((color & 0xFF000000) != 0)
		{
			Int2 img_size = gui::GetSize(game.tRip);
			GUI.DrawSprite(game.tRip, Center(img_size), color);

			cstring text = Format(game.death_solo ? txDeathAlone : txDeath, game.pc->kills, GameStats::Get().total_kills - game.pc->kills);
			cstring text2 = Format("%s\n\n%s", text, game.death_screen == 3 ? txPressEsc : "\n");
			Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
			GUI.DrawText(GUI.default_font, text2, DTF_CENTER | DTF_BOTTOM, color, rect);
		}
	}
}

//=================================================================================================
void GameGui::DrawEndOfGameScreen()
{
	// background
	int color;
	if(game.death_fade < 1.f)
		color = (int(game.death_fade * 255) << 24) | 0x00FFFFFF;
	else
		color = Color::White;
	GUI.DrawSpriteFull(game.tCzern, color);

	// image
	Int2 sprite_pos = Center(gui::GetSize(tEmerytura));
	GUI.DrawSprite(tEmerytura, sprite_pos, color);

	// text
	cstring text = Format(txGameTimeout, game.pc->kills, GameStats::Get().total_kills - game.pc->kills);
	cstring text2 = Format("%s\n\n%s", text, game.death_fade >= 1.f ? txPressEsc : "\n");
	Rect rect = { 0, 0, GUI.wnd_size.x, GUI.wnd_size.y };
	GUI.DrawText(GUI.default_font, text2, DTF_CENTER | DTF_BOTTOM, color, rect);
}

//=================================================================================================
void GameGui::DrawSpeechBubbles()
{
	// get list to sort
	LevelArea& area = L.GetArea(*game.pc->unit);
	sorted_speech_bbs.clear();
	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		SpeechBubble& sb = **it;

		Vec3 pos;
		if(sb.unit)
			pos = sb.last_pos = sb.unit->GetHeadPoint();
		else
			pos = sb.last_pos;

		if(Vec3::Distance(game.pc->unit->visual_pos, pos) > 20.f || !L.CanSee(area, game.pc->unit->pos, sb.last_pos))
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
				float cam_dist = Vec3::Distance(L.camera.from, pos);
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
			a1 = Color::Alpha(int(alpha * 0x80));
			a2 = Color::Alpha(int(alpha * 0xFF));
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
		GUI.DrawText(GUI.fSmall, sb.text, DTF_CENTER | DTF_VCENTER, a2, rect);
	}
}

//=================================================================================================
void GameGui::DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha)
{
	Color text_color;
	if(alpha == -1)
	{
		text_color = Color::White;
		alpha = 255;
	}
	else if(unit.IsEnemy(*game.pc->unit))
		text_color = Color(255, 0, 0, alpha);
	else
		text_color = Color(0, 255, 0, alpha);

	// text
	Rect r;
	if(GUI.DrawText3D(GUI.default_font, text, DTF_OUTLINE, text_color, pos, &r))
	{
		float hpp;
		if(!unit.IsAlive() && !unit.IsFollower())
			hpp = -1.f;
		else
			hpp = max(unit.GetHpp(), 0.f);
		Color color = Color::Alpha(alpha);

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
			if(game.devmode && Net::IsLocal())
			{
				float stamina = max(unit.GetStaminap(), 0.f);
				r2 += Int2(0, 4);
				r3.Right() = int(stamina * 64);
				r2.Right() = r2.Left() + int(stamina * sizex);
				GUI.DrawSpriteRectPart(tMinistamina, r2, r3, color);
			}
		}
	}
}

//=================================================================================================
void GameGui::Update(float dt)
{
	TooltipGroup group = TooltipGroup::Invalid;
	int id = -1;

	Container::Update(dt);

	UpdatePlayerView(dt);
	UpdateSpeechBubbles(dt);

	game.gui->messages->Update(dt);

	if(!GUI.HaveDialog() && !Game::Get().dialog_context.dialog_mode && Key.Down(VK_MENU))
		use_cursor = true;
	else
		use_cursor = false;

	const bool have_manabar = false;
	float hp_scale = float(GUI.wnd_size.x) / 800;

	// buffs
	int buffs = game.pc->unit->GetBuffs();

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
	if(!Net::IsOnline())
		--max;

	sidebar_state[(int)SideButtonId::Inventory] = (game.gui->inventory->inv_mine->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Journal] = (game.gui->journal->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Stats] = (game.gui->stats->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Team] = (game.gui->team->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Minimap] = (game.gui->minimap->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Action] = (game.gui->actions->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Talk] = 0;
	sidebar_state[(int)SideButtonId::Menu] = 0;

	bool anything = use_cursor;
	if(game.gui->inventory->gp_trade->visible || game.gui->book->visible)
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

		// shortcuts
		int shortcut_index = GetShortcutIndex();
		if(drag_and_drop == 1)
		{
			if(Int2::Distance(GUI.cursor_pos, drag_and_drop_pos) > 3)
				drag_and_drop = 2;
			if(Key.Released(VK_LBUTTON))
			{
				drag_and_drop = 0;
				game.pc->shortcuts[drag_and_drop_index].trigger = true;
			}
		}
		else if(drag_and_drop == 2)
		{
			if(Key.Released(VK_LBUTTON))
			{
				drag_and_drop = 0;
				if(shortcut_index != -1)
				{
					if(drag_and_drop_type == -1)
					{
						// drag & drop between game gui shortcuts
						if(shortcut_index != drag_and_drop_index)
						{
							game.pc->SetShortcut(shortcut_index, game.pc->shortcuts[drag_and_drop_index].type, game.pc->shortcuts[drag_and_drop_index].value);
							game.pc->SetShortcut(drag_and_drop_index, Shortcut::TYPE_NONE);
						}
					}
					else
					{
						// drag & drop from actions/inventory
						game.pc->SetShortcut(shortcut_index, (Shortcut::Type)drag_and_drop_type, drag_and_drop_index);
					}
				}
			}
		}
		if(shortcut_index != -1 && game.pc->shortcuts[shortcut_index].type != Shortcut::TYPE_NONE)
		{
			group = TooltipGroup::Shortcut;
			id = shortcut_index;
			if(!GUI.HaveDialog())
			{
				if(Key.Pressed(VK_LBUTTON))
				{
					drag_and_drop = 1;
					drag_and_drop_pos = GUI.cursor_pos;
					drag_and_drop_type = -1;
					drag_and_drop_index = shortcut_index;
					drag_and_drop_icon = nullptr;
				}
				else if(Key.PressedRelease(VK_RBUTTON))
					game.pc->SetShortcut(shortcut_index, Shortcut::TYPE_NONE);
			}
		}
	}
	else
		drag_and_drop = 0;

	if(anything)
		sidebar += dt * 5;
	else
		sidebar -= dt * 5;
	sidebar = Clamp(sidebar, 0.f, 1.f);

	if(sidebar > 0.f && !GUI.HaveDialog() && show_tooltips)
	{
		int img_size = 76 * GUI.wnd_size.x / 1920;
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
				if(Key.PressedRelease(VK_LBUTTON))
				{
					switch((SideButtonId)i)
					{
					case SideButtonId::Menu:
						game.gui->ShowMenu();
						use_cursor = false;
						break;
					case SideButtonId::Team:
						ShowPanel(OpenPanel::Team);
						break;
					case SideButtonId::Minimap:
						ShowPanel(OpenPanel::Minimap);
						if(game.gui->minimap->visible)
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
						game.gui->mp_box->visible = !game.gui->mp_box->visible;
						break;
					}
				}

				break;
			}
		}
	}

	if(drag_and_drop != 2)
		tooltip.UpdateTooltip(dt, (int)group, id);
	else
		tooltip.anything = false;
}

//=================================================================================================
int GameGui::GetShortcutIndex()
{
	float wnd_scale = float(GUI.wnd_size.x) / 800;
	int img_size = 76 * GUI.wnd_size.x / 1920;
	int offset = img_size + 2;
	Int2 spos(256.f*wnd_scale, GUI.wnd_size.y - offset);
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		Rect r = Rect::Create(spos, Int2(img_size, img_size));
		if(r.IsInside(GUI.cursor_pos))
			return i;
		spos.x += offset;
	}
	return -1;
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
					if(!sb.unit->mesh_inst)
					{
						game.ReportError(9, Format("Speech bubble for unit without mesh_inst (unit %s, text \"%.100s\").",
							sb.unit->GetRealName(), sb.text.c_str()));
					}
					else
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
	unit->bubble->size = Int2(Max(32, total / lines + 20), s.y*lines + 20);
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

	// try to reuse previous bubble
	SpeechBubble* sb = nullptr;
	for(SpeechBubble* bubble : speech_bbs)
	{
		if(!bubble->unit && Vec3::DistanceSquared(pos, bubble->last_pos) < 0.1f)
		{
			sb = bubble;
			break;
		}
	}
	if(!sb)
	{
		sb = SpeechBubblePool.Get();
		speech_bbs.push_back(sb);
	}

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
}

//=================================================================================================
void GameGui::Reset()
{
	SpeechBubblePool.Free(speech_bbs);
	Event(GuiEvent_Show);
	use_cursor = false;
	sidebar = 0.f;
	unit_views.clear();
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
	if(GUI.cursor_pos != dialog_cursor_pos)
	{
		dialog_cursor_pos = GUI.cursor_pos;
		if(cursor_choice != -1)
			ctx.choice_selected = cursor_choice;
	}

	// strza³ka w górê/dó³
	bool moved = false;
	if(ctx.choice_selected != 0 && GKey.KeyPressedReleaseAllowed(GK_MOVE_FORWARD))
	{
		--ctx.choice_selected;
		moved = true;
	}
	if(ctx.choice_selected != choices - 1 && GKey.KeyPressedReleaseAllowed(GK_MOVE_BACK))
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

	// wybór opcji dialogowej z klawiatury (1,2,3,..,9,0)
	if(GKey.AllowKeyboard() && !Key.Down(VK_SHIFT))
	{
		for(int i = 0; i < min(10, choices); ++i)
		{
			if(Key.PressedRelease((byte)'1' + i))
			{
				ctx.choice_selected = i;
				return true;
			}
		}
		if(choices >= 10)
		{
			if(Key.PressedRelease('0'))
			{
				ctx.choice_selected = 9;
				return true;
			}
		}
	}

	// wybieranie enterem/esc/spacj¹
	if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG))
		return true;
	else if(ctx.dialog_esc != -1 && GKey.AllowKeyboard() && Key.PressedRelease(VK_ESCAPE))
	{
		ctx.choice_selected = ctx.dialog_esc;
		return true;
	}

	// wybieranie klikniêciem
	if(GKey.AllowMouse() && cursor_choice != -1 && Key.PressedRelease(VK_LBUTTON))
	{
		if(ctx.is_local)
			game.pc_data.wasted_key = VK_LBUTTON;
		ctx.choice_selected = cursor_choice;
		return true;
	}

	// aktualizacja paska przewijania
	scrollbar.mouse_focus = focus;
	if(Key.Focus() && PointInRect(GUI.cursor_pos, dialog_pos, dialog_size) && scrollbar.ApplyMouseWheel())
		dialog_cursor_pos = Int2(-1, -1);
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
				tooltip.text = Format("%s (%s)", txMenu, game.gui->controls->key_text[VK_ESCAPE]);
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
	case TooltipGroup::Shortcut:
		{
			const Shortcut& shortcut = game.pc->shortcuts[id];
			cstring title = nullptr, desc = nullptr;
			if(shortcut.type == Shortcut::TYPE_SPECIAL)
			{
				switch(shortcut.value)
				{
				case Shortcut::SPECIAL_MELEE_WEAPON:
					title = txMeleeWeapon;
					desc = txMeleeWeaponDesc;
					break;
				case Shortcut::SPECIAL_RANGED_WEAPON:
					title = txRangedWeapon;
					desc = txRangedWeaponDesc;
					break;
				case Shortcut::SPECIAL_ACTION:
					{
						Action& action = game.pc->GetAction();
						title = action.name.c_str();
						desc = action.desc.c_str();
					}
					break;
				case Shortcut::SPECIAL_HEALING_POTION:
					title = txPotion;
					desc = txPotionDesc;
					break;
				}
			}
			else
			{
				title = shortcut.item->name.c_str();
				desc = shortcut.item->desc.c_str();
			}
			const GameKey& gk = GKey[GK_SHORTCUT1 + id];
			if(gk[0] != VK_NONE)
				title = Format("%s (%s)", title, global::gui->controls->key_text[gk[0]]);
			tooltip.text = title;
			if(desc)
				tooltip.small_text = desc;
			else
				tooltip.small_text.clear();
		}
		break;
	}
}

//=================================================================================================
bool GameGui::HavePanelOpen() const
{
	return game.gui->stats->visible
		|| game.gui->inventory->inv_mine->visible
		|| game.gui->inventory->gp_trade->visible
		|| game.gui->team->visible
		|| game.gui->journal->visible
		|| game.gui->minimap->visible
		|| game.gui->actions->visible;
}

//=================================================================================================
void GameGui::ClosePanels(bool close_mp_box)
{
	if(game.gui->stats->visible)
		game.gui->stats->Hide();
	if(game.gui->inventory->inv_mine->visible)
		game.gui->inventory->inv_mine->Hide();
	if(game.gui->team->visible)
		game.gui->team->Hide();
	if(game.gui->journal->visible)
		game.gui->journal->Hide();
	if(game.gui->minimap->visible)
		game.gui->minimap->Hide();
	if(game.gui->inventory->gp_trade->visible)
		game.gui->inventory->gp_trade->Hide();
	if(close_mp_box && game.gui->mp_box->visible)
		game.gui->mp_box->visible = false;
	if(game.gui->actions->visible)
		game.gui->actions->Hide();
	if(game.gui->book->visible)
		game.gui->book->Hide();
}

//=================================================================================================
void GameGui::GetGamePanels(vector<GamePanel*>& panels)
{
	panels.push_back(game.gui->inventory->inv_mine);
	panels.push_back(game.gui->stats);
	panels.push_back(game.gui->team);
	panels.push_back(game.gui->journal);
	panels.push_back(game.gui->minimap);
	panels.push_back(game.gui->inventory->inv_trade_mine);
	panels.push_back(game.gui->inventory->inv_trade_other);
	panels.push_back(game.gui->mp_box);
	panels.push_back(game.gui->actions);
}

//=================================================================================================
OpenPanel GameGui::GetOpenPanel()
{
	if(game.gui->stats->visible)
		return OpenPanel::Stats;
	else if(game.gui->inventory->inv_mine->visible)
		return OpenPanel::Inventory;
	else if(game.gui->team->visible)
		return OpenPanel::Team;
	else if(game.gui->journal->visible)
		return OpenPanel::Journal;
	else if(game.gui->minimap->visible)
		return OpenPanel::Minimap;
	else if(game.gui->inventory->gp_trade->visible)
		return OpenPanel::Trade;
	else if(game.gui->actions->visible)
		return OpenPanel::Action;
	else if(game.gui->book->visible)
		return OpenPanel::Book;
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
		game.gui->stats->Hide();
		break;
	case OpenPanel::Inventory:
		game.gui->inventory->inv_mine->Hide();
		break;
	case OpenPanel::Team:
		game.gui->team->Hide();
		break;
	case OpenPanel::Journal:
		game.gui->journal->Hide();
		break;
	case OpenPanel::Minimap:
		game.gui->minimap->Hide();
		break;
	case OpenPanel::Trade:
		game.OnCloseInventory();
		game.gui->inventory->gp_trade->Hide();
		break;
	case OpenPanel::Action:
		game.gui->actions->Hide();
		break;
	case OpenPanel::Book:
		game.gui->book->Hide();
		break;
	}

	// open new panel
	if(open != to_open)
	{
		switch(to_open)
		{
		case OpenPanel::Stats:
			game.gui->stats->Show();
			break;
		case OpenPanel::Inventory:
			game.gui->inventory->inv_mine->Show();
			break;
		case OpenPanel::Team:
			game.gui->team->Show();
			break;
		case OpenPanel::Journal:
			game.gui->journal->Show();
			break;
		case OpenPanel::Minimap:
			game.gui->minimap->Show();
			break;
		case OpenPanel::Action:
			game.gui->actions->Show();
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

	game.gui->stats->global_pos = game.gui->stats->pos = pos;
	game.gui->stats->size = size;
	game.gui->team->global_pos = game.gui->team->pos = pos;
	game.gui->team->size = size;
	game.gui->inventory->inv_mine->global_pos = game.gui->inventory->inv_mine->pos = pos;
	game.gui->inventory->inv_mine->size = size;
	game.gui->inventory->inv_trade_other->global_pos = game.gui->inventory->inv_trade_other->pos = pos;
	game.gui->inventory->inv_trade_other->size = Int2(size.x, (size.y - 32) / 2);
	game.gui->inventory->inv_trade_mine->global_pos = game.gui->inventory->inv_trade_mine->pos
		= Int2(pos.x, game.gui->inventory->inv_trade_other->pos.y + game.gui->inventory->inv_trade_other->size.y + 16);
	game.gui->inventory->inv_trade_mine->size = game.gui->inventory->inv_trade_other->size;
	game.gui->minimap->size = Int2(size.y, size.y);
	game.gui->minimap->global_pos = game.gui->minimap->pos = Int2((GUI.wnd_size.x - game.gui->minimap->size.x) / 2, (GUI.wnd_size.y - game.gui->minimap->size.y) / 2);
	game.gui->journal->size = game.gui->minimap->size;
	game.gui->journal->global_pos = game.gui->journal->pos = game.gui->minimap->pos;
	game.gui->mp_box->size = Int2((GUI.wnd_size.x - 32) / 2, (GUI.wnd_size.y - 64) / 4);
	game.gui->mp_box->global_pos = game.gui->mp_box->pos = Int2(GUI.wnd_size.x - pos.x - game.gui->mp_box->size.x, GUI.wnd_size.y - pos.x - game.gui->mp_box->size.y);
	game.gui->actions->global_pos = game.gui->actions->pos = pos;
	game.gui->actions->size = size;

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
		if(sb.unit)
			sb.unit->bubble = &sb;
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
	if(QM.quest_tutorial->in_tutorial)
		action = nullptr;
	else
		action = &game.pc->GetAction();
	game.gui->actions->Init(action);
}

//=================================================================================================
void GameGui::SortUnits()
{
	std::sort(sorted_units.begin(), sorted_units.end(), [](const SortedUnitView& a, const SortedUnitView& b)
	{
		return a.dist > b.dist;
	});
}

//=================================================================================================
void GameGui::UpdatePlayerView(float dt)
{
	LevelArea& area = L.GetArea(*game.pc->unit);
	Unit& u = *game.pc->unit;

	// mark previous views as invalid
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
		it->valid = false;

	// check units inside player view
	for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
	{
		Unit& u2 = **it;
		if(&u == &u2 || u2.to_remove)
			continue;

		bool mark = false;
		if(u.IsEnemy(u2))
		{
			if(u2.IsAlive())
				mark = true;
		}
		else if(u.IsFriend(u2))
			mark = true;

		// oznaczanie pobliskich postaci
		if(mark)
		{
			float dist = Vec3::Distance(u.visual_pos, u2.visual_pos);

			if(dist < ALERT_RANGE && L.camera.frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && L.CanSee(u, u2))
			{
				// dodaj do pobliskich jednostek
				bool jest = false;
				for(vector<UnitView>::iterator it2 = unit_views.begin(), end2 = unit_views.end(); it2 != end2; ++it2)
				{
					if(it2->unit == *it)
					{
						jest = true;
						it2->valid = true;
						it2->last_pos = u2.GetUnitTextPos();
						break;
					}
				}
				if(!jest)
				{
					UnitView& uv = Add1(unit_views);
					uv.valid = true;
					uv.unit = *it;
					uv.time = 0.f;
					uv.last_pos = u2.GetUnitTextPos();
				}
			}
		}
	}

	// aktualizuj pobliskie postacie
	// 0.0 -> 0.1 niewidoczne
	// 0.1 -> 0.2 alpha 0->255
	// -0.2 -> -0.1 widoczne
	// -0.1 -> 0.0 alpha 255->0
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end;)
	{
		bool removed = false;

		if(it->valid)
		{
			if(it->time >= 0.f)
				it->time += dt;
			else if(it->time < -UNIT_VIEW_A)
				it->time = UNIT_VIEW_B;
			else
				it->time = -it->time;
		}
		else
		{
			if(it->time >= 0.f)
			{
				if(it->time < UNIT_VIEW_A)
				{
					// usuñ
					if(it + 1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
				else if(it->time < UNIT_VIEW_B)
					it->time = -it->time;
				else
					it->time = -UNIT_VIEW_B;
			}
			else
			{
				it->time += dt;
				if(it->time >= 0.f)
				{
					// usuñ
					if(it + 1 == end)
					{
						unit_views.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						unit_views.pop_back();
						end = unit_views.end();
						removed = true;
					}
				}
			}
		}

		if(!removed)
			++it;
	}
}

//=================================================================================================
void GameGui::RemoveUnit(Unit* unit)
{
	for(vector<UnitView>::iterator it = unit_views.begin(), end = unit_views.end(); it != end; ++it)
	{
		if(it->unit == unit)
		{
			unit_views.erase(it);
			break;
		}
	}
}

//=================================================================================================
void GameGui::StartDragAndDrop(int type, int value, TEX icon)
{
	drag_and_drop = 2;
	drag_and_drop_type = type;
	drag_and_drop_index = value;
	drag_and_drop_icon = icon;
}
