#include "Pch.h"
#include "LevelGui.h"

#include "Ability.h"
#include "AbilityPanel.h"
#include "AIController.h"
#include "BookPanel.h"
#include "Chest.h"
#include "Class.h"
#include "Controls.h"
#include "CraftPanel.h"
#include "Door.h"
#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "GameStats.h"
#include "GroundItem.h"
#include "Inventory.h"
#include "Journal.h"
#include "Language.h"
#include "Level.h"
#include "Minimap.h"
#include "MpBox.h"
#include "PlayerInfo.h"
#include "Quest.h"
#include "StatsPanel.h"
#include "Team.h"
#include "TeamPanel.h"

#include <Engine.h>
#include <ResourceManager.h>

//-----------------------------------------------------------------------------
const float UNIT_VIEW_A = 0.1f;
const float UNIT_VIEW_B = 0.2f;

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
	"GOTO_INN",
	"GUARD",
	"AUTO_TALK"
};

//-----------------------------------------------------------------------------
enum class TooltipGroup
{
	Invalid = -1,
	Sidebar,
	Buff,
	Bar,
	Shortcut
};
enum Bar
{
	BAR_HP,
	BAR_MANA,
	BAR_STAMINA
};

//-----------------------------------------------------------------------------
static ObjectPool<SpeechBubble> SpeechBubblePool;

//=================================================================================================
LevelGui::LevelGui() : debug_info_size(0, 0), use_cursor(false), boss(nullptr)
{
	scrollbar.parent = this;
	visible = false;

	tooltip.Init(TooltipController::Callback(this, &LevelGui::GetTooltip));
}

//=================================================================================================
LevelGui::~LevelGui()
{
	SpeechBubblePool.Free(speech_bbs);
}

//=================================================================================================
void LevelGui::LoadLanguage()
{
	Language::Section s = Language::GetSection("LevelGui");
	txDeath = s.Get("death");
	txDeathAlone = s.Get("deathAlone");
	txGameTimeout = s.Get("gameTimeout");
	txChest = s.Get("chest");
	txDoor = s.Get("door");
	txDoorLocked = s.Get("doorLocked");
	txMenu = s.Get("menu");
	txHp = s.Get("hp");
	txMana = s.Get("mana");
	txStamina = s.Get("stamina");
	txMeleeWeapon = s.Get("meleeWeapon");
	txRangedWeapon = s.Get("rangedWeapon");
	txHealthPotion = s.Get("healthPotion");
	txManaPotion = s.Get("manaPotion");
	txMeleeWeaponDesc = s.Get("meleeWeaponDesc");
	txRangedWeaponDesc = s.Get("rangedWeaponDesc");
	txHealthPotionDesc = s.Get("healthPotionDesc");
	txManaPotionDesc = s.Get("manaPotionDesc");
	txSkipCutscene = s.Get("skipCutscene");
	BuffInfo::LoadText();
}

//=================================================================================================
void LevelGui::LoadData()
{
	tCrosshair = res_mgr->Load<Texture>("crosshair.png");
	tCrosshair2 = res_mgr->Load<Texture>("crosshair2.png");
	tBubble = res_mgr->Load<Texture>("bubble.png");
	tDamageLayer = res_mgr->Load<Texture>("damage_layer.png");
	tBar = res_mgr->Load<Texture>("bar.png");
	tHpBar = res_mgr->Load<Texture>("hp_bar.png");
	tPoisonedHpBar = res_mgr->Load<Texture>("poisoned_hp_bar.png");
	tStaminaBar = res_mgr->Load<Texture>("stamina_bar.png");
	tManaBar = res_mgr->Load<Texture>("mana_bar.png");
	tShortcut = res_mgr->Load<Texture>("shortcut.png");
	tShortcutHover = res_mgr->Load<Texture>("shortcut_hover.png");
	tShortcutDown = res_mgr->Load<Texture>("shortcut_down.png");
	tSideButton[(int)SideButtonId::Menu] = res_mgr->Load<Texture>("bt_menu.png");
	tSideButton[(int)SideButtonId::Team] = res_mgr->Load<Texture>("bt_team.png");
	tSideButton[(int)SideButtonId::Minimap] = res_mgr->Load<Texture>("bt_minimap.png");
	tSideButton[(int)SideButtonId::Journal] = res_mgr->Load<Texture>("bt_journal.png");
	tSideButton[(int)SideButtonId::Inventory] = res_mgr->Load<Texture>("bt_inventory.png");
	tSideButton[(int)SideButtonId::Ability] = res_mgr->Load<Texture>("bt_action.png");
	tSideButton[(int)SideButtonId::Stats] = res_mgr->Load<Texture>("bt_stats.png");
	tSideButton[(int)SideButtonId::Talk] = res_mgr->Load<Texture>("bt_talk.png");
	tMinihp = res_mgr->Load<Texture>("minihp.png");
	tMinistamina = res_mgr->Load<Texture>("ministamina.png");
	tMinimp = res_mgr->Load<Texture>("minimp.png");
	tActionCooldown = res_mgr->Load<Texture>("action_cooldown.png");
	tMelee = res_mgr->Load<Texture>("sword-brandish.png");
	tRanged = res_mgr->Load<Texture>("bow-arrow.png");
	tHealthPotion = res_mgr->Load<Texture>("health-potion.png");
	tManaPotion = res_mgr->Load<Texture>("mana-potion.png");
	tEmerytura = res_mgr->Load<Texture>("emerytura.jpg");
	tEquipped = res_mgr->Load<Texture>("equipped.png");
	tDialog = res_mgr->Load<Texture>("dialog.png");
	tShortcutAction = res_mgr->Load<Texture>("shortcut_action.png");
	tRip = res_mgr->Load<Texture>("rip.jpg");
	tBlack = res_mgr->Load<Texture>("czern.bmp");

	BuffInfo::LoadImages();
}

//=================================================================================================
void LevelGui::Draw(ControlDrawData*)
{
	if(game->cutscene)
	{
		int fallback_alpha = DrawFallback();
		DrawCutscene(fallback_alpha);
	}
	else
	{
		DrawFront();

		Container::Draw();

		DrawBack();

		game_gui->messages->Draw();
	}
}

//=================================================================================================
int LevelGui::DrawFallback()
{
	if(game->fallback_type == FALLBACK::NO)
		return 0;

	int alpha;
	if(game->fallback_t < 0.f)
	{
		if(game->fallback_type == FALLBACK::NONE)
			alpha = 255;
		else
			alpha = int((1.f + game->fallback_t) * 255);
	}
	else
		alpha = int((1.f - game->fallback_t) * 255);

	gui->DrawSpriteFull(tBlack, Color::Alpha(alpha));

	return alpha;
}

//=================================================================================================
void LevelGui::DrawFront()
{
	// death screen
	if(!team->IsAnyoneAlive())
	{
		DrawDeathScreen();
		return;
	}

	// end of game screen
	if(game->end_of_game)
	{
		DrawEndOfGameScreen();
		return;
	}

	PlayerController& pc = *game->pc;

	// crosshair
	if(pc.ShouldUseRaytest())
	{
		const Vec2 center(16, 16);
		const Vec2 scale((1.f - pc.data.range_ratio) + 0.5f);
		const Vec2 pos(0.5f * gui->wnd_size.x - 16.f, 0.5f * gui->wnd_size.y - 16.f);
		const Matrix mat = Matrix::Transform2D(&center, 0, &scale, nullptr, 0, &pos);
		gui->DrawSprite2(pc.IsAbilityPrepared() ? tCrosshair2 : tCrosshair, mat);
	}

	// taking damage layer (red screen)
	if(pc.dmgc > 0.f)
		gui->DrawSpriteFull(tDamageLayer, Color::Alpha((int)Clamp<float>(pc.dmgc / pc.unit->hp * 5 * 255, 0.f, 255.f)));

	// debug info
	if(debug_info && !game->devmode)
		debug_info = false;
	if(debug_info)
	{
		sorted_units.clear();
		for(Unit* unit : pc.unit->area->units)
		{
			if(!unit->IsAlive())
				continue;
			float dist = Vec3::DistanceSquared(game_level->camera.from, unit->pos);
			sorted_units.push_back({ unit, dist, 255, nullptr });
		}

		SortUnits();

		for(auto& it : sorted_units)
		{
			Unit& u = *it.unit;
			Vec3 text_pos = u.visual_pos;
			text_pos.y += u.GetUnitHeight();
			LocalString str = Format("%s (%s, %d", u.GetRealName(), u.data->id.c_str(), u.id);
			if(u.refs != 1)
				str += Format(" x%u", u.refs);
			str += ")";
			if(Net::IsLocal() || u.IsLocalPlayer())
			{
				if(u.IsAI())
				{
					AIController& ai = *u.ai;
					UnitOrder order = u.GetOrder();
					str += Format("\nB:%d, F:%d, LVL:%d, U:%d\nAni:%d, Act:%d, Ai:%s%s T:%.2f LT:%.2f\nO:%s", u.busy, u.frozen, u.level, u.usable ? 1 : 0,
						u.animation, u.action, str_ai_state[ai.state], ai.state == AIController::Idle ? Format("(%s)", str_ai_idle[ai.st.idle.action]) : "",
						ai.timer, ai.loc_timer, order_str[order]);
					if(order != ORDER_NONE && u.order->timer > 0.f)
						str += Format(" %.2f", u.order->timer);
					switch(order)
					{
					case ORDER_MOVE:
					case ORDER_LOOK_AT:
					case ORDER_ESCAPE_TO:
						str += Format(" Pos:%.1f;%.1f;%.1f", u.order->pos.x, u.order->pos.y, u.order->pos.z);
						break;
					case ORDER_FOLLOW:
					case ORDER_GUARD:
					case ORDER_ESCAPE_TO_UNIT:
						if(Unit* unit = u.order->unit)
							str += Format(" %s(%d)", unit->data->id.c_str(), unit->id);
						break;
					case ORDER_AUTO_TALK:
						str += u.order->auto_talk == AutoTalkMode::Leader ? " leader" : " normal";
						if(u.order->auto_talk_dialog)
						{
							str += Format(",%s", u.order->auto_talk_dialog->id.c_str());
							if(u.order->auto_talk_quest)
								str += Format(",%d", u.order->auto_talk_quest->id);
						}
						break;
					}
				}
				else
				{
					str += Format("\nB:%d, F:%d, LVL:%d, U:%d\nAni:%d, Act:%d, PAct:%d",
						u.busy, u.frozen, u.level, u.usable ? 1 : 0, u.animation, u.action, u.player->action);
				}
			}
			DrawUnitInfo(str, u, text_pos);
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
					alpha = int((it->time - UNIT_VIEW_A) * 255 / (UNIT_VIEW_B - UNIT_VIEW_A));
			}
			else if(it->time < 0.f)
			{
				if(it->time < -UNIT_VIEW_A)
					alpha = 255;
				else
					alpha = int(-it->time * 255 / (UNIT_VIEW_B - UNIT_VIEW_A));
			}
			else
				alpha = 0;

			if(alpha)
			{
				float dist = Vec3::DistanceSquared(game_level->camera.from, it->unit->pos);
				sorted_units.push_back({ it->unit, dist, alpha, &it->last_pos });
			}
		}

		SortUnits();

		for(auto& it : sorted_units)
		{
			if(it.unit != boss)
				DrawUnitInfo(it.unit->GetName(), *it.unit, *it.last_pos, it.alpha);
		}
	}

	// text above selected object/units
	switch(game->pc->data.before_player)
	{
	case BP_UNIT:
		if(!debug_info)
		{
			Unit* u = game->pc->data.before_player_ptr.unit;
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
				DrawUnitInfo(u->GetName(), *u, u->GetUnitTextPos());
		}
		break;
	case BP_CHEST:
		{
			Vec3 text_pos = game->pc->data.before_player_ptr.chest->pos;
			text_pos.y += 0.75f;
			DrawObjectInfo(txChest, text_pos);
		}
		break;
	case BP_DOOR:
		{
			Vec3 text_pos = game->pc->data.before_player_ptr.door->pos;
			text_pos.y += 1.75f;
			DrawObjectInfo(game->pc->data.before_player_ptr.door->locked == LOCK_NONE ? txDoor : txDoorLocked, text_pos);
		}
		break;
	case BP_ITEM:
		{
			GroundItem& item = *game->pc->data.before_player_ptr.item;
			Mesh* mesh;
			if(IsSet(item.item->flags, ITEM_GROUND_MESH))
				mesh = item.item->mesh;
			else
				mesh = game_res->aBag;
			Vec3 text_pos = item.pos;
			text_pos.y += mesh->head.bbox.v2.y;
			cstring text;
			if(item.count == 1)
				text = item.item->name.c_str();
			else
				text = Format("%s (%d)", item.item->name.c_str(), item.count);
			DrawObjectInfo(text, text_pos);
		}
		break;
	case BP_USABLE:
		{
			Usable& u = *game->pc->data.before_player_ptr.usable;
			Vec3 text_pos = u.pos;
			text_pos.y += u.GetMesh()->head.radius;
			DrawObjectInfo(u.base->name.c_str(), text_pos);
		}
		break;
	}

	// speech bubbles
	DrawSpeechBubbles();

	// dialog box
	if(game->dialog_context.dialog_mode)
	{
		Int2 dsize(gui->wnd_size.x - 256 - 8, 104);
		Int2 offset((gui->wnd_size.x - dsize.x) / 2, 32);
		gui->DrawItem(tDialog, offset, dsize, 0xAAFFFFFF, 16);

		Rect r = { offset.x + 6, offset.y + 6, offset.x + dsize.x - 12, offset.y + dsize.y - 4 };
		if(Any(game->dialog_context.mode, DialogContext::WAIT_CHOICES, DialogContext::WAIT_MP_RESPONSE))
		{
			int off = int(scrollbar.offset);

			// selection
			Rect r_img = Rect::Create(Int2(offset.x, offset.y + game->dialog_context.choice_selected * GameGui::font->height - off + 6),
				Int2(dsize.x - 16, GameGui::font->height));
			if(r_img.Bottom() >= r.Top() && r_img.Top() < r.Bottom())
			{
				if(r_img.Top() < r.Top())
					r_img.Top() = r.Top();
				else if(r_img.Bottom() > r.Bottom())
					r_img.Bottom() = r.Bottom();
				gui->DrawSpriteRect(tEquipped, r_img, 0xAAFFFFFF);
			}

			// text
			LocalString s;
			for(uint i = 0; i < game->dialog_context.choices.size(); ++i)
			{
				s += game->dialog_context.choices[i].msg;
				s += '\n';
			}
			Rect r2 = r;
			r2 -= Int2(0, off);
			gui->DrawText(GameGui::font, s, 0, Color::Black, r2, &r);

			// pasek przewijania
			scrollbar.Draw();
		}
		else if(game->dialog_context.dialog_text)
			gui->DrawText(GameGui::font, game->dialog_context.dialog_text, DTF_CENTER | DTF_VCENTER, Color::Black, r);
	}

	// health bar
	const Vec2 wndScale(float(gui->wnd_size.x) / 800);
	const bool mp_bar = game->pc->unit->IsUsingMp();
	Rect part = { 0, 0, 0, 16 };
	int bar_offset = (mp_bar ? 3 : 2);
	{
		const float hpp = Clamp(pc.unit->GetHpp(), 0.f, 1.f);
		const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
		part.Right() = int(hpp * 256);
		if(part.Right() > 0)
			gui->DrawSprite2(!IsSet(pc.unit->GetBuffs(), BUFF_POISON) ? tHpBar : tPoisonedHpBar, mat, &part, nullptr, Color::White);
		gui->DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);
		--bar_offset;
	}

	// stamina bar
	{
		const float stamina_p = Clamp(pc.unit->GetStaminap(), 0.f, 1.f);
		const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
		part.Right() = int(stamina_p * 256);
		if(part.Right() > 0)
			gui->DrawSprite2(tStaminaBar, mat, &part, nullptr, Color::White);
		gui->DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);
		--bar_offset;
	}

	// mana bar
	if(mp_bar)
	{
		const float mpp = pc.unit->GetMpp();
		const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
		part.Right() = int(mpp * 256);
		if(part.Right() > 0)
			gui->DrawSprite2(tManaBar, mat, &part, nullptr, Color::White);
		gui->DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);
	}

	// buffs
	for(BuffImage& img : buff_images)
	{
		const Vec2 scale(buff_scale);
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &img.pos);
		gui->DrawSprite2(img.tex, mat, nullptr, nullptr, Color::White);
	}

	// shortcuts
	const int img_size = 76 * gui->wnd_size.x / 1920;
	const int offset = img_size + 2;
	const Vec2 scale(float(img_size) / 64);
	Int2 spos(256.f * wndScale.x, gui->wnd_size.y - offset);
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		const Shortcut& shortcut = pc.shortcuts[i];

		Vec2 pos(spos);
		Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSprite2(tShortcut, mat);

		Rect r(spos.x + 2, spos.y + 2);
		r.p2.x += img_size - 4;
		r.p2.y += img_size - 4;

		Texture* icon = nullptr;
		int count = 0, icon_size = 128;
		Ability* ability = nullptr;
		bool enabled = true, equipped = false;
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
			case Shortcut::SPECIAL_HEALTH_POTION:
				icon = tHealthPotion;
				enabled = pc.unit->FindHealingPotion() != -1;
				break;
			case Shortcut::SPECIAL_MANA_POTION:
				icon = tManaPotion;
				enabled = pc.unit->FindManaPotion() != -1;
				break;
			}
		}
		else if(shortcut.type == Shortcut::TYPE_ITEM)
		{
			icon = shortcut.item->icon;
			icon_size = GameResources::ITEM_IMAGE_SIZE;
			if(shortcut.item->IsWearable())
			{
				if(pc.unit->HaveItemEquipped(shortcut.item))
				{
					equipped = true;
					enabled = true;
				}
				else
					enabled = pc.unit->HaveItem(shortcut.item, team->HaveOtherActiveTeamMember());
			}
			else if(shortcut.item->IsStackable())
			{
				count = pc.unit->CountItem(shortcut.item);
				enabled = (count > 0);
			}
			else
				enabled = pc.unit->HaveItem(shortcut.item);
		}
		else if(shortcut.type == Shortcut::TYPE_ABILITY)
			ability = shortcut.ability;

		const Vec2 scale2(float(img_size - 2) / icon_size);
		pos = Vec2(float(spos.x + 1), float(spos.y + 1));
		mat = Matrix::Transform2D(nullptr, 0.f, &scale2, nullptr, 0.f, &pos);
		if(icon)
		{
			if(drag_and_drop == 2 && drag_and_drop_type == -1 && drag_and_drop_index == i)
				drag_and_drop_icon = icon;

			if(enabled)
			{
				if(equipped)
					gui->DrawSprite2(tEquipped, mat);
				gui->DrawSprite2(icon, mat);
				if(count > 0)
					gui->DrawText(GameGui::font_small, Format("%d", count), DTF_RIGHT | DTF_BOTTOM, Color::Black, r);
			}
			else
			{
				gui->UseGrayscale(true);
				gui->DrawSprite2(icon, mat, nullptr, nullptr, Color::Alpha(128));
				gui->UseGrayscale(false);
			}
		}
		else if(ability)
		{
			const PlayerAbility* ab = pc.GetAbility(ability);
			float charge;
			if(ab)
			{
				if(ab->charges > 0 || ab->cooldown >= ab->recharge)
				{
					if(ability->cooldown.x == 0)
						charge = 0.f;
					else
						charge = ab->cooldown / ability->cooldown.x;
				}
				else
					charge = ab->recharge / ability->recharge;
			}
			else
				charge = 0.f;

			if(drag_and_drop == 2 && drag_and_drop_type == -1 && drag_and_drop_index == i)
				drag_and_drop_icon = ability->tex_icon;

			if(pc.CanUseAbilityPreview(ability))
			{
				if(charge == 0.f)
					gui->DrawSprite2(ability->tex_icon, mat);
				else
				{
					gui->UseGrayscale(true);
					gui->DrawSprite2(ability->tex_icon, mat);
					gui->UseGrayscale(false);
					if(charge < 1.f)
					{
						Rect part = { 0, 128 - int((1.f - charge) * 128), 128, 128 };
						gui->DrawSprite2(ability->tex_icon, mat, &part);
					}
					gui->DrawSprite2(tActionCooldown, mat);
				}
			}
			else
			{
				gui->UseGrayscale(true);
				gui->DrawSprite2(ability->tex_icon, mat);
				gui->UseGrayscale(false);
			}

			// charges
			if(ab && ability->charges > 1)
				gui->DrawText(GameGui::font_small, Format("%d/%d", ab->charges, ability->charges), DTF_RIGHT | DTF_BOTTOM, Color::Black, r);

			// readied ability
			if(game->pc->data.ability_ready == ability)
			{
				pos = Vec2(spos);
				mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
				gui->DrawSprite2(tShortcutAction, mat);
			}
		}

		const GameKey& gk = GKey[GK_SHORTCUT1 + i];
		if(gk[0] != Key::None)
			gui->DrawText(GameGui::font_small, game_gui->controls->GetKeyText(gk[0]), DTF_SINGLELINE, Color::White, r);

		spos.x += offset;
	}

	// sidebar
	if(sidebar > 0.f)
	{
		int max = (int)SideButtonId::Max;
		int total = offset * max;
		spos.y = gui->wnd_size.y - (gui->wnd_size.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			Texture* t;
			if(sidebar_state[i] == 0)
				t = tShortcut;
			else if(sidebar_state[i] == 1)
				t = tShortcutHover;
			else
				t = tShortcutDown;
			const Vec2 pos(float(gui->wnd_size.x) - sidebar * offset, float(spos.y - i * offset));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSprite2(t, mat, nullptr, nullptr, Color::White);
			gui->DrawSprite2(tSideButton[i], mat, nullptr, nullptr, Color::White);
		}
	}

	if(boss)
	{
		gui->DrawText(GameGui::font, boss->GetName(), DTF_OUTLINE | DTF_CENTER, Color(1.f, 0.f, 0.f, bossAlpha), Rect(0, 5, gui->wnd_size.x, 40));
		const float hpp = Clamp(boss->GetHpp(), 0.f, 1.f);
		const Rect part = { 0, 0, int(hpp * 256), 16 };
		const Vec2 pos((float(gui->wnd_size.x) - wndScale.x * 256) / 2, 25);
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
		if(part.Right() > 0)
			gui->DrawSprite2(tHpBar, mat, &part, nullptr, Color::Alpha(bossAlpha));
		gui->DrawSprite2(tBar, mat, nullptr, nullptr, Color::Alpha(bossAlpha));
	}

	DrawFallback();
}

//=================================================================================================
void LevelGui::DrawBack()
{
	if(drag_and_drop == 2 && drag_and_drop_icon)
	{
		const int img_size = 76 * gui->wnd_size.x / 1920;
		const Vec2 scale(float(img_size) / drag_and_drop_icon->GetSize().x);
		const Vec2 pos(gui->cursor_pos + Int2(16, 16));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSprite2(drag_and_drop_icon, mat);
	}

	// debug info
	if(debug_info2)
	{
		cstring text;
		if(game->devmode)
		{
			Unit& u = *game->pc->unit;
			text = Format("Pos: %g; %g; %g (%d; %d)\nRot: %g %s\nFps: %g", FLT10(u.pos.x), FLT10(u.pos.y), FLT10(u.pos.z), int(u.pos.x / 2), int(u.pos.z / 2),
				FLT100(u.rot), dir_name_short[AngleToDir(Clip(u.rot))], FLT10(engine->GetFps()));
		}
		else
			text = Format("Fps: %g", FLT10(engine->GetFps()));
		Int2 s = GameGui::font->CalculateSize(text);
		if(Int2::Distance(s, debug_info_size) < 32)
			debug_info_size = Int2::Max(s, debug_info_size);
		else
			debug_info_size = s;
		gui->DrawItem(tDialog, Int2(0, 0), debug_info_size + Int2(24, 24), Color::Alpha(128));
		Rect r = { 12, 12, 12 + s.x, 12 + s.y };
		gui->DrawText(GameGui::font, text, 0, Color::Black, r);
	}

	// tooltip
	tooltip.Draw();
}

//=================================================================================================
void LevelGui::DrawDeathScreen()
{
	if(game->death_screen <= 0)
		return;

	// black screen
	int color;
	if(game->death_screen == 1)
		color = (int(game->death_fade * 255) << 24) | 0x00FFFFFF;
	else
		color = Color::White;

	if((color & 0xFF000000) != 0)
		gui->DrawSpriteFull(tBlack, color);

	// image & text
	if(game->death_screen > 1)
	{
		if(game->death_screen == 2)
			color = (int(game->death_fade * 255) << 24) | 0x00FFFFFF;
		else
			color = Color::White;

		if((color & 0xFF000000) != 0)
		{
			Int2 img_size = tRip->GetSize();
			gui->DrawSprite(tRip, Center(img_size), color);

			cstring text = Format(game->death_solo ? txDeathAlone : txDeath, game->pc->kills, game_stats->total_kills - game->pc->kills);
			Rect rect = { 0, 0, gui->wnd_size.x, gui->wnd_size.y - 32 };
			gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
		}
	}
}

//=================================================================================================
void LevelGui::DrawEndOfGameScreen()
{
	// background
	int color;
	if(game->death_fade < 1.f)
		color = (int(game->death_fade * 255) << 24) | 0x00FFFFFF;
	else
		color = Color::White;
	gui->DrawSpriteFull(tBlack, color);

	// image
	Int2 sprite_pos = Center(tEmerytura->GetSize());
	gui->DrawSprite(tEmerytura, sprite_pos, color);

	// text
	cstring text = Format(txGameTimeout, game->pc->kills, game_stats->total_kills - game->pc->kills);
	Rect rect = { 0, 0, gui->wnd_size.x, gui->wnd_size.y - 32 };
	gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
}

//=================================================================================================
void LevelGui::DrawSpeechBubbles()
{
	// get list to sort
	LevelArea& area = *game->pc->unit->area;
	sorted_speech_bbs.clear();
	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		SpeechBubble& sb = **it;

		Vec3 pos;
		if(sb.unit)
			pos = sb.last_pos = sb.unit->GetHeadPoint();
		else
			pos = sb.last_pos;

		if(Vec3::Distance(game->pc->unit->visual_pos, pos) > ALERT_RANGE || !game_level->CanSee(area, game->pc->unit->pos, sb.last_pos))
		{
			sb.visible = false;
			continue;
		}

		Int2 pt;
		if(gui->To2dPoint(pos, pt))
		{
			sb.visible = true;
			if(sb.time > 0.25f)
			{
				float cam_dist = Vec3::Distance(game_level->camera.from, pos);
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
		else if(it.pt.x > gui->wnd_size.x - sb.size.x / 2)
			it.pt.x = gui->wnd_size.x - sb.size.x / 2;
		if(it.pt.y < sb.size.y / 2)
			it.pt.y = sb.size.y / 2;
		else if(it.pt.y > gui->wnd_size.y - sb.size.y / 2)
			it.pt.y = gui->wnd_size.y - sb.size.y / 2;

		Rect rect = Rect::Create(Int2(it.pt.x - sb.size.x / 2, it.pt.y - sb.size.y / 2), sb.size);
		gui->DrawItem(tBubble, rect.LeftTop(), sb.size, a1);
		gui->DrawText(GameGui::font_small, sb.text, DTF_CENTER | DTF_VCENTER, a2, rect);
	}
}

//=================================================================================================
void LevelGui::DrawObjectInfo(cstring text, const Vec3& pos)
{
	Rect r;
	if(!gui->DrawText3D(GameGui::font, text, DTF_OUTLINE | DTF_DONT_DRAW, Color::Black, pos, &r))
		return;

	// background
	Rect bkg_rect = { r.Left() - 1, r.Top() - 1,r.Right() + 1,r.Bottom() + 1 };
	gui->DrawArea(Color(0, 0, 0, 255 / 3), bkg_rect);

	// text
	gui->DrawText3D(GameGui::font, text, DTF_OUTLINE, Color::White, pos);
}

//=================================================================================================
void LevelGui::DrawUnitInfo(cstring text, Unit& unit, const Vec3& pos, int alpha)
{
	Rect r;
	if(!gui->DrawText3D(GameGui::font, text, DTF_OUTLINE | DTF_DONT_DRAW, Color::Black, pos, &r))
		return;

	float hpp;
	if(!unit.IsAlive() && !unit.IsFollower())
		hpp = -1.f;
	else
		hpp = max(unit.GetHpp(), 0.f);

	// background
	Rect bkg_rect = { r.Left() - 1, r.Top() - 1, r.Right() + 1, r.Bottom() + 1 };
	if(hpp >= 0.f)
	{
		bkg_rect.p2.y += 4;
		if(unit.IsTeamMember())
			bkg_rect.p2.y += 3;
		if(unit.IsUsingMp())
			bkg_rect.p2.y += 3;
	}
	gui->DrawArea(Color(0, 0, 0, alpha / 3), bkg_rect);

	// text
	Color text_color;
	if(unit.IsFriend(*game->pc->unit))
		text_color = Color(0, 255, 0, alpha);
	else if(unit.IsAlive() && unit.IsEnemy(*game->pc->unit))
		text_color = Color(255, 0, 0, alpha);
	else
		text_color = Color(255, 255, 255, alpha);
	gui->DrawText3D(GameGui::font, text, DTF_OUTLINE, text_color, pos);

	if(hpp >= 0.f)
	{
		// hp
		Color color = Color::Alpha(alpha);
		Rect r2(r.Left(), r.Bottom(), r.Right(), r.Bottom() + 4);
		int sizex = r2.SizeX();
		r2.Right() = r2.Left() + int(hpp * sizex);
		Rect r3 = { 0, 0, int(hpp * 64), 4 };
		gui->DrawSpriteRectPart(tMinihp, r2, r3, color);

		if((game->devmode && Net::IsLocal()) || unit.IsTeamMember())
		{
			// stamina
			float stamina = Clamp(unit.GetStaminap(), 0.f, 1.f);
			r2 += Int2(0, 5);
			r3.Right() = int(stamina * 64);
			r3.Bottom() = 2;
			r2.Right() = r2.Left() + int(stamina * sizex);
			r2.Bottom() = r2.Top() + 2;
			gui->DrawSpriteRectPart(tMinistamina, r2, r3, color);

			// mana
			if(unit.IsUsingMp())
			{
				float mpp = Clamp(unit.GetMpp(), 0.f, 1.f);
				r2 += Int2(0, 3);
				r3.Right() = int(mpp * 64);
				r2.Right() = r2.Left() + int(mpp * sizex);
				gui->DrawSpriteRectPart(tMinimp, r2, r3, color);
			}
		}
	}
}

//=================================================================================================
void LevelGui::Update(float dt)
{
	TooltipGroup group = TooltipGroup::Invalid;
	int id = -1;

	if(game->devmode)
	{
		if(input->PressedRelease(Key::F3))
			debug_info = !debug_info;
	}
	if(input->PressedRelease(Key::F2))
		debug_info2 = !debug_info2;

	Container::Update(dt);

	if(game->cutscene)
		UpdateCutscene(dt);

	UpdatePlayerView(dt);
	UpdateSpeechBubbles(dt);

	game_gui->messages->Update(dt);

	if(!gui->HaveDialog() && !game->dialog_context.dialog_mode && input->Down(Key::Alt))
		use_cursor = true;
	else
		use_cursor = false;

	const bool have_manabar = false;
	float hp_scale = float(gui->wnd_size.x) / 800;
	bool mp_bar = game->pc->unit->IsUsingMp();
	int bar_offset = (mp_bar ? 3 : 2);

	// buffs
	int buffs = game->pc->unit->GetBuffs();

	buff_scale = gui->wnd_size.x / 1024.f;
	float off = buff_scale * 33;
	float buf_posy = float(gui->wnd_size.y - 5) - off - hp_scale * (bar_offset * 18 - 1);
	Int2 buff_size(int(buff_scale * 32), int(buff_scale * 32));

	buff_images.clear();

	for(int i = 0; i < BUFF_COUNT; ++i)
	{
		int buff_bit = 1 << i;
		if(IsSet(buffs, buff_bit))
		{
			auto& info = BuffInfo::info[i];
			buff_images.push_back(BuffImage(Vec2(2, buf_posy), info.img, i));
			buf_posy -= off;
		}
	}

	// sidebar
	int max = (int)SideButtonId::Max;
	sidebar_state[(int)SideButtonId::Inventory] = (game_gui->inventory->inv_mine->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Journal] = (game_gui->journal->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Stats] = (game_gui->stats->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Team] = (game_gui->team->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Minimap] = (game_gui->minimap->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Ability] = (game_gui->ability->visible ? 2 : 0);
	sidebar_state[(int)SideButtonId::Talk] = 0;
	sidebar_state[(int)SideButtonId::Menu] = 0;

	bool anything = use_cursor;
	if(game_gui->inventory->gp_trade->visible || game_gui->book->visible || game_gui->craft->visible)
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
		const Vec2 wndScale(float(gui->wnd_size.x) / 800);

		// for buffs
		for(BuffImage& img : buff_images)
		{
			if(PointInRect(gui->cursor_pos, Int2(img.pos), buff_size))
			{
				group = TooltipGroup::Buff;
				id = img.id;
				break;
			}
		}

		// for health bar
		{
			const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursor_pos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_HP;
			}
			--bar_offset;
		}

		// for stamina bar
		{
			const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursor_pos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_STAMINA;
			}
			--bar_offset;
		}

		// for mana bar
		if(mp_bar)
		{
			const Vec2 pos(0.f, float(gui->wnd_size.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursor_pos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_MANA;
			}
		}

		// shortcuts
		int shortcut_index = GetShortcutIndex();
		if(drag_and_drop == 1)
		{
			if(Int2::Distance(gui->cursor_pos, drag_and_drop_pos) > 3)
				drag_and_drop = 2;
			if(input->Released(Key::LeftButton))
			{
				drag_and_drop = 0;
				game->pc->shortcuts[drag_and_drop_index].trigger = true;
			}
		}
		else if(drag_and_drop == 2)
		{
			if(input->Released(Key::LeftButton))
			{
				drag_and_drop = 0;
				if(shortcut_index != -1)
				{
					if(drag_and_drop_type == -1)
					{
						// drag & drop between game gui shortcuts
						if(shortcut_index != drag_and_drop_index)
						{
							game->pc->SetShortcut(shortcut_index, game->pc->shortcuts[drag_and_drop_index].type, game->pc->shortcuts[drag_and_drop_index].value);
							game->pc->SetShortcut(drag_and_drop_index, Shortcut::TYPE_NONE);
						}
					}
					else
					{
						// drag & drop from actions/inventory
						game->pc->SetShortcut(shortcut_index, (Shortcut::Type)drag_and_drop_type, drag_and_drop_index);
					}
				}
			}
		}
		if(shortcut_index != -1 && game->pc->shortcuts[shortcut_index].type != Shortcut::TYPE_NONE)
		{
			group = TooltipGroup::Shortcut;
			id = shortcut_index;
			if(!gui->HaveDialog())
			{
				if(input->Pressed(Key::LeftButton))
				{
					drag_and_drop = 1;
					drag_and_drop_pos = gui->cursor_pos;
					drag_and_drop_type = -1;
					drag_and_drop_index = shortcut_index;
					drag_and_drop_icon = nullptr;
				}
				else if(input->PressedRelease(Key::RightButton))
				{
					Shortcut& shortcut = game->pc->shortcuts[shortcut_index];
					if(shortcut.type == Shortcut::TYPE_ABILITY && shortcut.ability == PlayerController::data.ability_ready)
						PlayerController::data.ability_ready = nullptr;
					else
					{
						game->pc->SetShortcut(shortcut_index, Shortcut::TYPE_NONE);
						group = TooltipGroup::Invalid;
					}
					drag_and_drop = 0;
				}
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

	if(sidebar > 0.f && !gui->HaveDialog() && show_tooltips)
	{
		int img_size = 76 * gui->wnd_size.x / 1920;
		int offset = img_size + 2;
		int total = offset * max;
		int sposy = gui->wnd_size.y - (gui->wnd_size.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			if(PointInRect(gui->cursor_pos, Int2(int(float(gui->wnd_size.x) - sidebar * offset), sposy - i * offset), Int2(img_size, img_size)))
			{
				group = TooltipGroup::Sidebar;
				id = i;

				if(sidebar_state[i] == 0)
					sidebar_state[i] = 1;
				if(input->PressedRelease(Key::LeftButton))
				{
					switch((SideButtonId)i)
					{
					case SideButtonId::Menu:
						game_gui->ShowMenu();
						use_cursor = false;
						break;
					case SideButtonId::Team:
						ShowPanel(OpenPanel::Team);
						break;
					case SideButtonId::Minimap:
						ShowPanel(OpenPanel::Minimap);
						if(game_gui->minimap->visible)
							use_cursor = true;
						break;
					case SideButtonId::Journal:
						ShowPanel(OpenPanel::Journal);
						break;
					case SideButtonId::Inventory:
						ShowPanel(OpenPanel::Inventory);
						break;
					case SideButtonId::Ability:
						ShowPanel(OpenPanel::Ability);
						break;
					case SideButtonId::Stats:
						ShowPanel(OpenPanel::Stats);
						break;
					case SideButtonId::Talk:
						game_gui->mp_box->visible = !game_gui->mp_box->visible;
						break;
					}
				}

				break;
			}
		}
	}

	// boss
	if(boss)
	{
		if(bossState)
		{
			bossAlpha -= 3.f * dt;
			if(bossAlpha <= 0)
				boss = nullptr;
		}
		else
			bossAlpha = Min(bossAlpha + 3.f * dt, 1.f);
	}

	if(drag_and_drop != 2)
		tooltip.UpdateTooltip(dt, (int)group, id);
	else
		tooltip.anything = false;
}

//=================================================================================================
int LevelGui::GetShortcutIndex()
{
	float wndScale = float(gui->wnd_size.x) / 800;
	int img_size = 76 * gui->wnd_size.x / 1920;
	int offset = img_size + 2;
	Int2 spos(256.f * wndScale, gui->wnd_size.y - offset);
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		Rect r = Rect::Create(spos, Int2(img_size, img_size));
		if(r.IsInside(gui->cursor_pos))
			return i;
		spos.x += offset;
	}
	return -1;
}

//=================================================================================================
void LevelGui::UpdateSpeechBubbles(float dt)
{
	bool removes = false;

	dt *= game->game_speed;

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
						game->ReportError(9, Format("Speech bubble for unit without mesh_inst (unit %s, text \"%.100s\").",
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
bool LevelGui::NeedCursor() const
{
	if(game->dialog_context.dialog_mode || use_cursor)
		return true;
	return Container::NeedCursor();
}

//=================================================================================================
void LevelGui::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		Int2 dsize(gui->wnd_size.x - 256 - 8, 104);
		Int2 offset((gui->wnd_size.x - dsize.x) / 2, 32);
		scrollbar.size = Int2(12, 104);
		scrollbar.global_pos = scrollbar.pos = Int2(dsize.x + offset.x - 16, offset.y);
		dialog_pos = offset;
		dialog_size = dsize;
		tooltip.Clear();
	}
}

//=================================================================================================
void LevelGui::AddSpeechBubble(Unit* unit, cstring text)
{
	assert(unit && text);

	// create new if there isn't already created
	if(!unit->bubble)
	{
		unit->bubble = SpeechBubblePool.Get();
		speech_bbs.push_back(unit->bubble);
	}

	// calculate size
	Int2 s = GameGui::font_small->CalculateSize(text);
	int total = s.x;
	int lines = 1 + total / 400;

	// setup
	unit->bubble->text = text;
	unit->bubble->unit = unit;
	unit->bubble->size = Int2(Max(32, total / lines + 20), s.y * lines + 20);
	unit->bubble->time = 0.f;
	unit->bubble->length = 1.5f + float(strlen(text)) / 20;
	unit->bubble->visible = false;
	unit->bubble->last_pos = unit->GetHeadPoint();

	unit->talking = true;
	unit->talk_timer = 0.f;
}

//=================================================================================================
void LevelGui::AddSpeechBubble(const Vec3& pos, cstring text)
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

	Int2 size = GameGui::font_small->CalculateSize(text);
	int total = size.x;
	int lines = 1 + total / 400;

	sb->text = text;
	sb->unit = nullptr;
	sb->size = Int2(total / lines + 20, size.y * lines + 20);
	sb->time = 0.f;
	sb->length = 1.5f + float(strlen(text)) / 20;
	sb->visible = true;
	sb->last_pos = pos;
}

//=================================================================================================
void LevelGui::Reset()
{
	debug_info = false;
	debug_info2 = false;
	SpeechBubblePool.Free(speech_bbs);
	Event(GuiEvent_Show);
	use_cursor = false;
	sidebar = 0.f;
	unit_views.clear();
	boss = nullptr;
}

//=================================================================================================
bool LevelGui::UpdateChoice()
{
	DialogContext& ctx = game->dialog_context;
	const Int2 dsize(gui->wnd_size.x - 256 - 8, 104);
	const Int2 offset((gui->wnd_size.x - dsize.x) / 2, 32 + 6);
	const int choices = ctx.choices.size();

	// element pod kursorem
	int cursor_choice = -1;
	if(gui->cursor_pos.x >= offset.x && gui->cursor_pos.x < offset.x + dsize.x - 16 && gui->cursor_pos.y >= offset.y && gui->cursor_pos.y < offset.y + dsize.y - 12)
	{
		int w = (gui->cursor_pos.y - offset.y + int(scrollbar.offset)) / GameGui::font->height;
		if(w < choices)
			cursor_choice = w;
	}

	// zmiana zaznaczonego elementu myszk¹
	if(gui->cursor_pos != dialog_cursor_pos)
	{
		dialog_cursor_pos = gui->cursor_pos;
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
		scrollbar.offset = float(GameGui::font->height * (ctx.choice_selected - 2));
		if(scrollbar.offset < 0.f)
			scrollbar.offset = 0.f;
		else if(scrollbar.offset + scrollbar.part > scrollbar.total)
			scrollbar.offset = float(scrollbar.total - scrollbar.part);
	}

	// wybór opcji dialogowej z klawiatury (1,2,3,..,9,0)
	if(GKey.AllowKeyboard() && !input->Down(Key::Shift))
	{
		for(int i = 0; i < min(10, choices); ++i)
		{
			if(input->PressedRelease(Key::N1 + i))
			{
				ctx.choice_selected = i;
				return true;
			}
		}
		if(choices >= 10)
		{
			if(input->PressedRelease(Key::N0))
			{
				ctx.choice_selected = 9;
				return true;
			}
		}
	}

	// wybieranie enterem/esc/spacj¹
	if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG))
		return true;
	else if(ctx.dialog_esc != -1 && GKey.AllowKeyboard() && input->PressedRelease(Key::Escape))
	{
		ctx.choice_selected = ctx.dialog_esc;
		return true;
	}

	// wybieranie klikniêciem
	if(GKey.AllowMouse() && cursor_choice != -1 && input->PressedRelease(Key::LeftButton))
	{
		if(ctx.is_local)
			game->pc->data.wasted_key = Key::LeftButton;
		ctx.choice_selected = cursor_choice;
		return true;
	}

	// aktualizacja paska przewijania
	scrollbar.mouse_focus = focus;
	if(input->Focus() && PointInRect(gui->cursor_pos, dialog_pos, dialog_size) && scrollbar.ApplyMouseWheel())
		dialog_cursor_pos = Int2(-1, -1);
	scrollbar.Update(0.f);

	return false;
}

//=================================================================================================
void LevelGui::UpdateScrollbar(int choices)
{
	scrollbar.part = 104 - 12;
	scrollbar.offset = 0.f;
	scrollbar.total = choices * GameGui::font->height;
	scrollbar.LostFocus();
}

//=================================================================================================
void LevelGui::GetTooltip(TooltipController*, int _group, int id, bool refresh)
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
				tooltip.text = Format("%s (%s)", txMenu, game_gui->controls->GetKeyText(Key::Escape));
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
			case SideButtonId::Ability:
				gk = GK_ABILITY_PANEL;
				break;
			case SideButtonId::Stats:
				gk = GK_STATS;
				break;
			case SideButtonId::Talk:
				gk = GK_TALK_BOX;
				break;
			}

			tooltip.text = game->GetShortcutText(gk);
		}
		break;
	case TooltipGroup::Bar:
		{
			Unit* u = game->pc->unit;
			switch(id)
			{
			case BAR_HP:
				{
					int hp = (int)u->hp;
					if(hp == 0 && u->hp > 0.f)
						hp = 1;
					int hpmax = (int)u->hpmax;
					tooltip.text = Format("%s: %d/%d", txHp, hp, hpmax);
				}
				break;
			case BAR_MANA:
				{
					int mp = (int)u->mp;
					int mpmax = (int)u->mpmax;
					tooltip.text = Format("%s: %d/%d", txMana, mp, mpmax);
				}
				break;
			case BAR_STAMINA:
				{
					int stamina = (int)u->stamina;
					int stamina_max = (int)u->stamina_max;
					tooltip.text = Format("%s: %d/%d", txStamina, stamina, stamina_max);
				}
				break;
			}
		}
		break;
	case TooltipGroup::Shortcut:
		{
			const Shortcut& shortcut = game->pc->shortcuts[id];
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
				case Shortcut::SPECIAL_HEALTH_POTION:
					title = txHealthPotion;
					desc = txHealthPotionDesc;
					break;
				case Shortcut::SPECIAL_MANA_POTION:
					title = txManaPotion;
					desc = txManaPotionDesc;
					break;
				}
			}
			else if(shortcut.type == Shortcut::TYPE_ITEM)
			{
				title = shortcut.item->name.c_str();
				desc = shortcut.item->desc.c_str();
			}
			else if(shortcut.type == Shortcut::TYPE_ABILITY)
			{
				game_gui->ability->GetAbilityTooltip(tooltip, *shortcut.ability);
				tooltip.small_text = Format("%s\n%s", tooltip.text.c_str(), tooltip.small_text.c_str());
				tooltip.text = tooltip.big_text;
				tooltip.big_text.clear();
				tooltip.img = nullptr;
			}

			const GameKey& gk = GKey[GK_SHORTCUT1 + id];
			if(shortcut.type == Shortcut::TYPE_ABILITY)
			{
				if(gk[0] != Key::None)
					tooltip.text = Format("%s (%s)", tooltip.text.c_str(), game_gui->controls->GetKeyText(gk[0]));
			}
			else
			{
				if(gk[0] != Key::None)
					title = Format("%s (%s)", title, game_gui->controls->GetKeyText(gk[0]));
				tooltip.text = title;
				if(desc)
					tooltip.small_text = desc;
				else
					tooltip.small_text.clear();
			}
		}
		break;
	}
}

//=================================================================================================
bool LevelGui::HavePanelOpen() const
{
	return game_gui->stats->visible
		|| game_gui->inventory->inv_mine->visible
		|| game_gui->inventory->gp_trade->visible
		|| game_gui->team->visible
		|| game_gui->journal->visible
		|| game_gui->minimap->visible
		|| game_gui->ability->visible
		|| game_gui->craft->visible;
}

//=================================================================================================
void LevelGui::ClosePanels(bool close_mp_box)
{
	if(game_gui->stats->visible)
		game_gui->stats->Hide();
	if(game_gui->inventory->inv_mine->visible)
		game_gui->inventory->inv_mine->Hide();
	if(game_gui->team->visible)
		game_gui->team->Hide();
	if(game_gui->journal->visible)
		game_gui->journal->Hide();
	if(game_gui->minimap->visible)
		game_gui->minimap->Hide();
	if(game_gui->inventory->gp_trade->visible)
		game_gui->inventory->gp_trade->Hide();
	if(close_mp_box && game_gui->mp_box->visible)
		game_gui->mp_box->visible = false;
	if(game_gui->ability->visible)
		game_gui->ability->Hide();
	if(game_gui->book->visible)
		game_gui->book->Hide();
	if(game_gui->craft->visible)
		game_gui->craft->Hide();
}

//=================================================================================================
void LevelGui::GetGamePanels(vector<GamePanel*>& panels)
{
	panels.push_back(game_gui->inventory->inv_mine);
	panels.push_back(game_gui->stats);
	panels.push_back(game_gui->team);
	panels.push_back(game_gui->journal);
	panels.push_back(game_gui->minimap);
	panels.push_back(game_gui->inventory->inv_trade_mine);
	panels.push_back(game_gui->inventory->inv_trade_other);
	panels.push_back(game_gui->mp_box);
	panels.push_back(game_gui->ability);
}

//=================================================================================================
OpenPanel LevelGui::GetOpenPanel()
{
	if(game_gui->stats->visible)
		return OpenPanel::Stats;
	else if(game_gui->inventory->inv_mine->visible)
		return OpenPanel::Inventory;
	else if(game_gui->team->visible)
		return OpenPanel::Team;
	else if(game_gui->journal->visible)
		return OpenPanel::Journal;
	else if(game_gui->minimap->visible)
		return OpenPanel::Minimap;
	else if(game_gui->inventory->gp_trade->visible)
		return OpenPanel::Trade;
	else if(game_gui->ability->visible)
		return OpenPanel::Ability;
	else if(game_gui->book->visible)
		return OpenPanel::Book;
	else if(game_gui->craft->visible)
		return OpenPanel::Craft;
	else
		return OpenPanel::None;
}

//=================================================================================================
void LevelGui::ShowPanel(OpenPanel to_open, OpenPanel open)
{
	if(open == OpenPanel::Unknown)
		open = GetOpenPanel();

	// close current panel
	switch(open)
	{
	case OpenPanel::None:
		break;
	case OpenPanel::Stats:
		game_gui->stats->Hide();
		break;
	case OpenPanel::Inventory:
		game_gui->inventory->inv_mine->Hide();
		break;
	case OpenPanel::Team:
		game_gui->team->Hide();
		break;
	case OpenPanel::Journal:
		game_gui->journal->Hide();
		break;
	case OpenPanel::Minimap:
		game_gui->minimap->Hide();
		break;
	case OpenPanel::Trade:
		game->OnCloseInventory();
		game_gui->inventory->gp_trade->Hide();
		break;
	case OpenPanel::Ability:
		game_gui->ability->Hide();
		break;
	case OpenPanel::Book:
		game_gui->book->Hide();
		break;
	case OpenPanel::Craft:
		game_gui->craft->Hide();
		break;
	}

	// open new panel
	if(open != to_open)
	{
		switch(to_open)
		{
		case OpenPanel::Stats:
			game_gui->stats->Show();
			break;
		case OpenPanel::Inventory:
			game_gui->inventory->inv_mine->Show();
			break;
		case OpenPanel::Team:
			game_gui->team->Show();
			break;
		case OpenPanel::Journal:
			game_gui->journal->Show();
			break;
		case OpenPanel::Minimap:
			game_gui->minimap->Show();
			break;
		case OpenPanel::Ability:
			game_gui->ability->Show();
			break;
		}
	}
	else
		use_cursor = false;
}

//=================================================================================================
void LevelGui::PositionPanels()
{
	float scale = float(gui->wnd_size.x) / 1024;
	Int2 pos = Int2(int(scale * 48), int(scale * 32));
	Int2 size = Int2(gui->wnd_size.x - pos.x * 2, gui->wnd_size.y - pos.x * 2);

	game_gui->stats->global_pos = game_gui->stats->pos = pos;
	game_gui->stats->size = size;
	game_gui->team->global_pos = game_gui->team->pos = pos;
	game_gui->team->size = size;
	game_gui->inventory->inv_mine->global_pos = game_gui->inventory->inv_mine->pos = pos;
	game_gui->inventory->inv_mine->size = size;
	game_gui->inventory->inv_trade_other->global_pos = game_gui->inventory->inv_trade_other->pos = pos;
	game_gui->inventory->inv_trade_other->size = Int2(size.x, (size.y - 32) / 2);
	game_gui->inventory->inv_trade_mine->global_pos = game_gui->inventory->inv_trade_mine->pos
		= Int2(pos.x, game_gui->inventory->inv_trade_other->pos.y + game_gui->inventory->inv_trade_other->size.y + 16);
	game_gui->inventory->inv_trade_mine->size = game_gui->inventory->inv_trade_other->size;
	game_gui->minimap->size = Int2(size.y, size.y);
	game_gui->minimap->global_pos = game_gui->minimap->pos = Int2((gui->wnd_size.x - game_gui->minimap->size.x) / 2, (gui->wnd_size.y - game_gui->minimap->size.y) / 2);
	game_gui->journal->size = game_gui->minimap->size;
	game_gui->journal->global_pos = game_gui->journal->pos = game_gui->minimap->pos;
	game_gui->mp_box->size = Int2((gui->wnd_size.x - 32) / 2, (gui->wnd_size.y - 64) / 4);
	game_gui->mp_box->global_pos = game_gui->mp_box->pos = Int2(gui->wnd_size.x - pos.x - game_gui->mp_box->size.x, gui->wnd_size.y - pos.x - game_gui->mp_box->size.y);
	game_gui->ability->global_pos = game_gui->ability->pos = pos;
	game_gui->ability->size = size;

	LocalVector<GamePanel*> panels;
	GetGamePanels(panels);
	for(vector<GamePanel*>::iterator it = panels->begin(), end = panels->end(); it != end; ++it)
	{
		(*it)->Event(GuiEvent_Moved);
		(*it)->Event(GuiEvent_Resize);
	}
}

//=================================================================================================
void LevelGui::Save(GameWriter& f) const
{
	f << speech_bbs.size();
	for(const SpeechBubble* p_sb : speech_bbs)
	{
		const SpeechBubble& sb = *p_sb;
		f.WriteString2(sb.text);
		f << (sb.unit ? sb.unit->id : -1);
		f << sb.size;
		f << sb.time;
		f << sb.length;
		f << sb.visible;
		f << sb.last_pos;
	}
}

//=================================================================================================
void LevelGui::Load(GameReader& f)
{
	speech_bbs.resize(f.Read<uint>());
	for(vector<SpeechBubble*>::iterator it = speech_bbs.begin(), end = speech_bbs.end(); it != end; ++it)
	{
		*it = SpeechBubblePool.Get();
		SpeechBubble& sb = **it;
		f.ReadString2(sb.text);
		sb.unit = Unit::GetById(f.Read<int>());
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
void LevelGui::SortUnits()
{
	std::sort(sorted_units.begin(), sorted_units.end(), [](const SortedUnitView& a, const SortedUnitView& b)
	{
		return a.dist > b.dist;
	});
}

//=================================================================================================
void LevelGui::UpdatePlayerView(float dt)
{
	LevelArea& area = *game->pc->unit->area;
	Unit& u = *game->pc->unit;

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
			if(dist < ALERT_RANGE && game_level->camera.frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && game_level->CanSee(u, u2))
				AddUnitView(&u2);
		}
	}

	// extra units
	if(game->pc->data.ability_ready && game->pc->data.ability_ok)
	{
		if(Unit* target = game->pc->data.ability_target; target && target != &u)
			AddUnitView(target);
	}
	if(u.action == A_CAST)
	{
		if(Unit* target = u.act.cast.target; target && target != &u)
			AddUnitView(target);
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
void LevelGui::AddUnitView(Unit* unit)
{
	for(UnitView& view : unit_views)
	{
		if(view.unit == unit)
		{
			view.valid = true;
			view.last_pos = unit->GetUnitTextPos();
			return;
		}
	}

	UnitView& uv = Add1(unit_views);
	uv.valid = true;
	uv.unit = unit;
	uv.time = 0.f;
	uv.last_pos = unit->GetUnitTextPos();
}

//=================================================================================================
void LevelGui::RemoveUnitView(Unit* unit)
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
void LevelGui::StartDragAndDrop(int type, int value, Texture* icon)
{
	drag_and_drop = 2;
	drag_and_drop_type = type;
	drag_and_drop_index = value;
	drag_and_drop_icon = icon;
}

//=================================================================================================
void LevelGui::ResetCutscene()
{
	cutscene_image = nullptr;
	cutscene_next_images.clear();
	cutscene_image_timer = 0;
	cutscene_image_state = CS_NONE;
	cutscene_text.clear();
	cutscene_next_texts.clear();
	cutscene_text_timer = 0;
	cutscene_text_state = CS_NONE;
}

//=================================================================================================
void LevelGui::SetCutsceneImage(Texture* tex, float time)
{
	cutscene_next_images.push_back(std::make_pair(tex, time));
}

//=================================================================================================
void LevelGui::SetCutsceneText(const string& text, float time)
{
	cutscene_next_texts.push_back(std::make_pair(text, time));
}

//=================================================================================================
void LevelGui::UpdateCutscene(float dt)
{
	// is everything shown?
	if(cutscene_image_state == CS_NONE && cutscene_next_images.empty() && cutscene_text_state == CS_NONE && cutscene_next_texts.empty())
	{
		game->CutsceneEnded(false);
		return;
	}

	// cancel cutscene
	if(team->IsLeader() && input->PressedRelease(Key::Escape))
	{
		game->CutsceneEnded(true);
		return;
	}

	// update image
	switch(cutscene_image_state)
	{
	case CS_NONE:
		if(!cutscene_next_images.empty())
		{
			cutscene_image = cutscene_next_images.front().first;
			cutscene_image_timer = 0.f;
			cutscene_image_state = CS_FADE_IN;
		}
		break;
	case CS_FADE_IN:
		cutscene_image_timer += dt * 2;
		if(cutscene_image_timer >= 1.f)
		{
			cutscene_image_timer = cutscene_next_images.front().second;
			cutscene_image_state = CS_WAIT;
			cutscene_next_images.erase(cutscene_next_images.begin());
		}
		break;
	case CS_WAIT:
		cutscene_image_timer -= dt;
		if(cutscene_image_timer <= 0)
		{
			cutscene_image_timer = 1.f;
			cutscene_image_state = CS_FADE_OUT;
		}
		break;
	case CS_FADE_OUT:
		cutscene_image_timer -= dt * 2;
		if(cutscene_image_timer <= 0)
		{
			cutscene_image = nullptr;
			cutscene_image_state = CS_NONE;
		}
		break;
	}

	// update text
	switch(cutscene_text_state)
	{
	case CS_NONE:
		if(!cutscene_next_texts.empty())
		{
			cutscene_text = cutscene_next_texts.front().first;
			cutscene_text_timer = 0.f;
			cutscene_text_state = CS_FADE_IN;
		}
		break;
	case CS_FADE_IN:
		cutscene_text_timer += dt * 2;
		if(cutscene_text_timer >= 1.f)
		{
			cutscene_text_timer = cutscene_next_texts.front().second;
			cutscene_text_state = CS_WAIT;
			cutscene_next_texts.erase(cutscene_next_texts.begin());
		}
		break;
	case CS_WAIT:
		cutscene_text_timer -= dt;
		if(cutscene_text_timer <= 0)
		{
			cutscene_text_timer = 1.f;
			cutscene_text_state = CS_FADE_OUT;
		}
		break;
	case CS_FADE_OUT:
		cutscene_text_timer -= dt * 2;
		if(cutscene_text_timer <= 0)
		{
			cutscene_text.clear();
			cutscene_text_state = CS_NONE;
		}
		break;
	}
}

//=================================================================================================
void LevelGui::DrawCutscene(int fallback_alpha)
{
	if(team->IsLeader())
	{
		const Rect r = { 4,4,200,200 };
		gui->DrawText(game_gui->font, txSkipCutscene, DTF_LEFT | DTF_TOP | DTF_OUTLINE, Color::Alpha(fallback_alpha), r);
	}

	if(cutscene_image)
	{
		const int alpha = GetAlpha(cutscene_image_state, cutscene_image_timer, fallback_alpha);
		Int2 img_size = cutscene_image->GetSize();
		const int max_size = gui->wnd_size.y - 128;
		const Vec2 scale(float(max_size) / img_size.y);
		img_size *= scale.x;
		const Vec2 pos((gui->wnd_size - img_size) / 2);
		const Matrix mat = Matrix::Transform2D(nullptr, 0, &scale, nullptr, 0.f, &pos);
		gui->DrawSprite2(cutscene_image, mat, nullptr, nullptr, Color::Alpha(alpha));
	}

	if(!cutscene_text.empty())
	{
		const int alpha = GetAlpha(cutscene_text_state, cutscene_text_timer, fallback_alpha);
		const Rect r = { 0, gui->wnd_size.y - 64,gui->wnd_size.x, gui->wnd_size.y };
		gui->DrawText(game_gui->font, cutscene_text, DTF_OUTLINE | DTF_CENTER | DTF_VCENTER, Color::Alpha(alpha), r);
	}
}

//=================================================================================================
int LevelGui::GetAlpha(CutsceneState cs, float timer, int fallback_alpha)
{
	switch(cs)
	{
	case CS_NONE:
		return 0;
	case CS_WAIT:
		return fallback_alpha;
	default:
		return int(timer * 255) * fallback_alpha / 255;
	}
}

//=================================================================================================
void LevelGui::SetBoss(Unit* boss, bool instant)
{
	if(boss)
	{
		this->boss = boss;
		bossAlpha = instant ? 1.f : 0.f;
		bossState = false;
	}
	else
	{
		bossState = true;
		if(instant)
			boss = nullptr;
	}
}
