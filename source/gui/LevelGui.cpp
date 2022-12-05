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
LevelGui::LevelGui() : debugInfoSize(0, 0), useCursor(false), boss(nullptr)
{
	scrollbar.parent = this;
	visible = false;

	tooltip.Init(TooltipController::Callback(this, &LevelGui::GetTooltip));
}

//=================================================================================================
LevelGui::~LevelGui()
{
	SpeechBubblePool.Free(speechBubbles);
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
	tCrosshair = resMgr->Load<Texture>("crosshair.png");
	tCrosshair2 = resMgr->Load<Texture>("crosshair2.png");
	tBubble = resMgr->Load<Texture>("bubble.png");
	tDamageLayer = resMgr->Load<Texture>("damage_layer.png");
	tBar = resMgr->Load<Texture>("bar.png");
	tHpBar = resMgr->Load<Texture>("hp_bar.png");
	tPoisonedHpBar = resMgr->Load<Texture>("poisoned_hp_bar.png");
	tStaminaBar = resMgr->Load<Texture>("stamina_bar.png");
	tManaBar = resMgr->Load<Texture>("mana_bar.png");
	tShortcut = resMgr->Load<Texture>("shortcut.png");
	tShortcutHover = resMgr->Load<Texture>("shortcut_hover.png");
	tShortcutDown = resMgr->Load<Texture>("shortcut_down.png");
	tSideButton[(int)SideButtonId::Menu] = resMgr->Load<Texture>("bt_menu.png");
	tSideButton[(int)SideButtonId::Team] = resMgr->Load<Texture>("bt_team.png");
	tSideButton[(int)SideButtonId::Minimap] = resMgr->Load<Texture>("bt_minimap.png");
	tSideButton[(int)SideButtonId::Journal] = resMgr->Load<Texture>("bt_journal.png");
	tSideButton[(int)SideButtonId::Inventory] = resMgr->Load<Texture>("bt_inventory.png");
	tSideButton[(int)SideButtonId::Ability] = resMgr->Load<Texture>("bt_action.png");
	tSideButton[(int)SideButtonId::Stats] = resMgr->Load<Texture>("bt_stats.png");
	tSideButton[(int)SideButtonId::Talk] = resMgr->Load<Texture>("bt_talk.png");
	tMinihp = resMgr->Load<Texture>("minihp.png");
	tMinistamina = resMgr->Load<Texture>("ministamina.png");
	tMinimp = resMgr->Load<Texture>("minimp.png");
	tActionCooldown = resMgr->Load<Texture>("action_cooldown.png");
	tMelee = resMgr->Load<Texture>("sword-brandish.png");
	tRanged = resMgr->Load<Texture>("bow-arrow.png");
	tHealthPotion = resMgr->Load<Texture>("health-potion.png");
	tManaPotion = resMgr->Load<Texture>("mana-potion.png");
	tEmerytura = resMgr->Load<Texture>("emerytura.jpg");
	tEquipped = resMgr->Load<Texture>("equipped.png");
	tDialog = resMgr->Load<Texture>("dialog.png");
	tShortcutAction = resMgr->Load<Texture>("shortcut_action.png");
	tRip = resMgr->Load<Texture>("rip.jpg");
	tBlack = resMgr->Load<Texture>("czern.bmp");

	BuffInfo::LoadImages();
}

//=================================================================================================
void LevelGui::Draw()
{
	if(game->cutscene)
	{
		int fallbackAlpha = DrawFallback();
		DrawCutscene(fallbackAlpha);
	}
	else
	{
		DrawFront();

		Container::Draw();

		DrawBack();

		gameGui->messages->Draw();
	}
}

//=================================================================================================
int LevelGui::DrawFallback()
{
	if(game->fallbackType == FALLBACK::NO)
		return 0;

	int alpha;
	if(game->fallbackTimer < 0.f)
	{
		if(game->fallbackType == FALLBACK::NONE)
			alpha = 255;
		else
			alpha = int((1.f + game->fallbackTimer) * 255);
	}
	else
		alpha = int((1.f - game->fallbackTimer) * 255);

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
	if(game->endOfGame)
	{
		DrawEndOfGameScreen();
		return;
	}

	PlayerController& pc = *game->pc;

	// crosshair
	if(pc.ShouldUseRaytest())
	{
		const Vec2 center(16, 16);
		const Vec2 scale((1.f - pc.data.rangeRatio) + 0.5f);
		const Vec2 pos(0.5f * gui->wndSize.x - 16.f, 0.5f * gui->wndSize.y - 16.f);
		const Matrix mat = Matrix::Transform2D(&center, 0, &scale, nullptr, 0, &pos);
		gui->DrawSprite2(pc.IsAbilityPrepared() ? tCrosshair2 : tCrosshair, mat);
	}

	// taking damage layer (red screen)
	if(pc.dmgc > 0.f)
		gui->DrawSpriteFull(tDamageLayer, Color::Alpha((int)Clamp<float>(pc.dmgc / pc.unit->hp * 5 * 255, 0.f, 255.f)));

	// debug info
	if(debugInfo && !game->devmode)
		debugInfo = false;
	if(debugInfo)
	{
		sorted_units.clear();
		for(Unit* unit : pc.unit->locPart->units)
		{
			if(!unit->IsAlive())
				continue;
			float dist = Vec3::DistanceSquared(gameLevel->camera.from, unit->pos);
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
						u.animation, u.action, strAiState[ai.state], ai.state == AIController::Idle ? Format("(%s)", strAiIdle[ai.st.idle.action]) : "",
						ai.timer, ai.locTimer, order_str[order]);
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
		for(vector<UnitView>::iterator it = unitViews.begin(), end = unitViews.end(); it != end; ++it)
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
				float dist = Vec3::DistanceSquared(gameLevel->camera.from, it->unit->pos);
				sorted_units.push_back({ it->unit, dist, alpha, &it->lastPos });
			}
		}

		SortUnits();

		for(auto& it : sorted_units)
		{
			if(it.unit != boss)
				DrawUnitInfo(it.unit->GetName(), *it.unit, *it.lastPos, it.alpha);
		}
	}

	// text above selected object/units
	switch(game->pc->data.beforePlayer)
	{
	case BP_UNIT:
		if(!debugInfo)
		{
			Unit* u = game->pc->data.beforePlayerPtr.unit;
			bool dont_draw = false;
			for(vector<UnitView>::iterator it = unitViews.begin(), end = unitViews.end(); it != end; ++it)
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
			Vec3 text_pos = game->pc->data.beforePlayerPtr.chest->pos;
			text_pos.y += 0.75f;
			DrawObjectInfo(txChest, text_pos);
		}
		break;
	case BP_DOOR:
		{
			Vec3 text_pos = game->pc->data.beforePlayerPtr.door->pos;
			text_pos.y += 1.75f;
			DrawObjectInfo(game->pc->data.beforePlayerPtr.door->locked == LOCK_NONE ? txDoor : txDoorLocked, text_pos);
		}
		break;
	case BP_ITEM:
		{
			GroundItem& item = *game->pc->data.beforePlayerPtr.item;
			Mesh* mesh;
			if(IsSet(item.item->flags, ITEM_GROUND_MESH))
				mesh = item.item->mesh;
			else
				mesh = gameRes->aBag;
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
			Usable& u = *game->pc->data.beforePlayerPtr.usable;
			Vec3 text_pos = u.pos;
			text_pos.y += u.GetMesh()->head.radius;
			DrawObjectInfo(u.base->name.c_str(), text_pos);
		}
		break;
	}

	// speech bubbles
	DrawSpeechBubbles();

	// dialog box
	if(game->dialogContext.dialogMode)
	{
		const int border = 4;
		Int2 dsize(gui->wndSize.x - 256 - 8, 104);
		Int2 offset((gui->wndSize.x - dsize.x) / 2, 32);
		gui->DrawItem(tDialog, offset, dsize, 0xAAFFFFFF, 16);

		Rect r = Rect::Create(offset, dsize, border);
		if(Any(game->dialogContext.mode, DialogContext::WAIT_CHOICES, DialogContext::WAIT_MP_RESPONSE))
		{
			int off = int(scrollbar.offset);

			// selection
			Rect r_img = Rect::Create(Int2(offset.x + border, offset.y + border + game->dialogContext.choiceSelected * GameGui::font->height - off),
				Int2(dsize.x - border * 2, GameGui::font->height));
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
			for(uint i = 0; i < game->dialogContext.choices.size(); ++i)
			{
				s += game->dialogContext.choices[i].msg;
				s += '\n';
			}
			Rect r2 = { r.Left() + 2, r.Top() - off, r.Right() - 2, r.Bottom() - off };
			gui->DrawText(GameGui::font, s, 0, Color::Black, r2, &r);

			// pasek przewijania
			scrollbar.Draw();
		}
		else if(game->dialogContext.dialogText)
			gui->DrawText(GameGui::font, game->dialogContext.dialogText, DTF_CENTER | DTF_VCENTER, Color::Black, r);
	}

	// health bar
	const Vec2 wndScale(float(gui->wndSize.x) / 800);
	const bool mp_bar = game->pc->unit->IsUsingMp();
	Rect part = { 0, 0, 0, 16 };
	int bar_offset = (mp_bar ? 3 : 2);
	{
		const float hpp = Clamp(pc.unit->GetHpp(), 0.f, 1.f);
		const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
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
		const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
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
		const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
		part.Right() = int(mpp * 256);
		if(part.Right() > 0)
			gui->DrawSprite2(tManaBar, mat, &part, nullptr, Color::White);
		gui->DrawSprite2(tBar, mat, nullptr, nullptr, Color::White);
	}

	// buffs
	for(BuffImage& img : buffImages)
	{
		const Vec2 scale(buffScale);
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &img.pos);
		gui->DrawSprite2(img.tex, mat, nullptr, nullptr, Color::White);
	}

	// shortcuts
	const int img_size = 76 * gui->wndSize.x / 1920;
	const int offset = img_size + 2;
	const Vec2 scale(float(img_size) / 64);
	Int2 spos(256.f * wndScale.x, gui->wndSize.y - offset);
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
			if(dragAndDrop == 2 && dragAndDropType == -1 && dragAndDropIndex == i)
				dragAndDropIcon = icon;

			if(enabled)
			{
				if(equipped)
					gui->DrawSprite2(tEquipped, mat);
				gui->DrawSprite2(icon, mat);
				if(count > 0)
					gui->DrawText(GameGui::fontSmall, Format("%d", count), DTF_RIGHT | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);
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

			if(dragAndDrop == 2 && dragAndDropType == -1 && dragAndDropIndex == i)
				dragAndDropIcon = ability->tex_icon;

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
				gui->DrawText(GameGui::fontSmall, Format("%d/%d", ab->charges, ability->charges), DTF_RIGHT | DTF_BOTTOM | DTF_OUTLINE, Color::White, r);

			// readied ability
			if(game->pc->data.abilityReady == ability)
			{
				pos = Vec2(spos);
				mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
				gui->DrawSprite2(tShortcutAction, mat);
			}
		}

		const GameKey& gk = GKey[GK_SHORTCUT1 + i];
		if(gk[0] != Key::None)
			gui->DrawText(GameGui::fontSmall, gameGui->controls->GetKeyText(gk[0]), DTF_SINGLELINE | DTF_OUTLINE, Color::White, r);

		spos.x += offset;
	}

	// sidebar
	if(sidebar > 0.f)
	{
		int max = (int)SideButtonId::Max;
		int total = offset * max;
		spos.y = gui->wndSize.y - (gui->wndSize.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			Texture* t;
			if(sidebarState[i] == 0)
				t = tShortcut;
			else if(sidebarState[i] == 1)
				t = tShortcutHover;
			else
				t = tShortcutDown;
			const Vec2 pos(float(gui->wndSize.x) - sidebar * offset, float(spos.y - i * offset));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
			gui->DrawSprite2(t, mat, nullptr, nullptr, Color::White);
			gui->DrawSprite2(tSideButton[i], mat, nullptr, nullptr, Color::White);
		}
	}

	if(boss)
	{
		gui->DrawText(GameGui::font, boss->GetName(), DTF_OUTLINE | DTF_CENTER, Color(1.f, 0.f, 0.f, bossAlpha), Rect(0, 5, gui->wndSize.x, 40));
		const float hpp = Clamp(boss->GetHpp(), 0.f, 1.f);
		const Rect part = { 0, 0, int(hpp * 256), 16 };
		const Vec2 pos((float(gui->wndSize.x) - wndScale.x * 256) / 2, 25);
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
	if(dragAndDrop == 2 && dragAndDropIcon)
	{
		const int img_size = 76 * gui->wndSize.x / 1920;
		const Vec2 scale(float(img_size) / dragAndDropIcon->GetSize().x);
		const Vec2 pos(gui->cursorPos + Int2(16, 16));
		const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
		gui->DrawSprite2(dragAndDropIcon, mat);
	}

	// debug info
	if(debugInfo2)
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
		if(Int2::Distance(s, debugInfoSize) < 32)
			debugInfoSize = Int2::Max(s, debugInfoSize);
		else
			debugInfoSize = s;
		gui->DrawItem(tDialog, Int2(0, 0), debugInfoSize + Int2(24, 24), Color::Alpha(128));
		Rect r = { 12, 12, 12 + s.x, 12 + s.y };
		gui->DrawText(GameGui::font, text, 0, Color::Black, r);
	}

	// tooltip
	tooltip.Draw();
}

//=================================================================================================
void LevelGui::DrawDeathScreen()
{
	if(game->deathScreen <= 0)
		return;

	// black screen
	int color;
	if(game->deathScreen == 1)
		color = (int(game->deathFade * 255) << 24) | 0x00FFFFFF;
	else
		color = Color::White;

	if((color & 0xFF000000) != 0)
		gui->DrawSpriteFull(tBlack, color);

	// image & text
	if(game->deathScreen > 1)
	{
		if(game->deathScreen == 2)
			color = (int(game->deathFade * 255) << 24) | 0x00FFFFFF;
		else
			color = Color::White;

		if((color & 0xFF000000) != 0)
		{
			Int2 img_size = tRip->GetSize();
			gui->DrawSprite(tRip, Center(img_size), color);

			cstring text = Format(game->deathSolo ? txDeathAlone : txDeath, game->pc->kills, gameStats->totalKills - game->pc->kills);
			Rect rect = { 0, 0, gui->wndSize.x, gui->wndSize.y - 32 };
			gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
		}
	}
}

//=================================================================================================
void LevelGui::DrawEndOfGameScreen()
{
	// background
	int color;
	if(game->deathFade < 1.f)
		color = (int(game->deathFade * 255) << 24) | 0x00FFFFFF;
	else
		color = Color::White;
	gui->DrawSpriteFull(tBlack, color);

	// image
	Int2 sprite_pos = Center(tEmerytura->GetSize());
	gui->DrawSprite(tEmerytura, sprite_pos, color);

	// text
	cstring text = Format(txGameTimeout, game->pc->kills, gameStats->totalKills - game->pc->kills);
	Rect rect = { 0, 0, gui->wndSize.x, gui->wndSize.y - 32 };
	gui->DrawText(GameGui::font, text, DTF_CENTER | DTF_BOTTOM, color, rect);
}

//=================================================================================================
void LevelGui::DrawSpeechBubbles()
{
	// get list to sort
	LocationPart& locPart = *game->pc->unit->locPart;
	sortedSpeechBubbles.clear();
	for(vector<SpeechBubble*>::iterator it = speechBubbles.begin(), end = speechBubbles.end(); it != end; ++it)
	{
		SpeechBubble& sb = **it;

		Vec3 pos;
		if(sb.unit)
			pos = sb.lastPos = sb.unit->GetHeadPoint();
		else
			pos = sb.lastPos;

		if(Vec3::Distance(game->pc->unit->visual_pos, pos) > ALERT_RANGE || !gameLevel->CanSee(locPart, game->pc->unit->pos, sb.lastPos))
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
				float cam_dist = Vec3::Distance(gameLevel->camera.from, pos);
				sortedSpeechBubbles.push_back({ &sb, cam_dist, pt });
			}
		}
		else
			sb.visible = false;
	}

	// sort
	std::sort(sortedSpeechBubbles.begin(), sortedSpeechBubbles.end(), [](const SortedSpeechBubble& a, const SortedSpeechBubble& b)
	{
		return a.dist > b.dist;
	});

	// draw
	for(auto& it : sortedSpeechBubbles)
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
		else if(it.pt.x > gui->wndSize.x - sb.size.x / 2)
			it.pt.x = gui->wndSize.x - sb.size.x / 2;
		if(it.pt.y < sb.size.y / 2)
			it.pt.y = sb.size.y / 2;
		else if(it.pt.y > gui->wndSize.y - sb.size.y / 2)
			it.pt.y = gui->wndSize.y - sb.size.y / 2;

		Rect rect = Rect::Create(Int2(it.pt.x - sb.size.x / 2, it.pt.y - sb.size.y / 2), sb.size);
		gui->DrawItem(tBubble, rect.LeftTop(), sb.size, a1);
		gui->DrawText(GameGui::fontSmall, sb.text, DTF_CENTER | DTF_VCENTER, a2, rect);
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

	if(input->Down(Key::Control))
	{
		if(game->devmode)
		{
			if(input->PressedRelease(Key::F2))
				debugInfo = !debugInfo;
		}
		if(input->PressedRelease(Key::F1))
			debugInfo2 = !debugInfo2;
	}

	Container::Update(dt);

	if(game->cutscene)
		UpdateCutscene(dt);

	UpdatePlayerView(dt);
	UpdateSpeechBubbles(dt);

	gameGui->messages->Update(dt);

	if(!gui->HaveDialog() && !game->dialogContext.dialogMode && input->Down(Key::Alt))
		useCursor = true;
	else
		useCursor = false;

	const bool have_manabar = false;
	float hp_scale = float(gui->wndSize.x) / 800;
	bool mp_bar = game->pc->unit->IsUsingMp();
	int bar_offset = (mp_bar ? 3 : 2);

	// buffs
	int buffs = game->pc->unit->GetBuffs();

	buffScale = gui->wndSize.x / 1024.f;
	float off = buffScale * 33;
	float buf_posy = float(gui->wndSize.y - 5) - off - hp_scale * (bar_offset * 18 - 1);
	Int2 buff_size(int(buffScale * 32), int(buffScale * 32));

	buffImages.clear();

	for(int i = 0; i < BUFF_COUNT; ++i)
	{
		int buff_bit = 1 << i;
		if(IsSet(buffs, buff_bit))
		{
			auto& info = BuffInfo::info[i];
			buffImages.push_back(BuffImage(Vec2(2, buf_posy), info.img, i));
			buf_posy -= off;
		}
	}

	// sidebar
	int max = (int)SideButtonId::Max;
	sidebarState[(int)SideButtonId::Inventory] = (gameGui->inventory->invMine->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Journal] = (gameGui->journal->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Stats] = (gameGui->stats->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Team] = (gameGui->team->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Minimap] = (gameGui->minimap->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Ability] = (gameGui->ability->visible ? 2 : 0);
	sidebarState[(int)SideButtonId::Talk] = 0;
	sidebarState[(int)SideButtonId::Menu] = 0;

	bool anything = useCursor;
	if(gameGui->inventory->gpTrade->visible || gameGui->book->visible || gameGui->craft->visible)
		anything = true;
	bool show_tooltips = anything;
	if(!anything)
	{
		for(int i = 0; i < (int)SideButtonId::Max; ++i)
		{
			if(sidebarState[i] == 2)
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
		const Vec2 wndScale(float(gui->wndSize.x) / 800);

		// for buffs
		for(BuffImage& img : buffImages)
		{
			if(Rect::IsInside(gui->cursorPos, Int2(img.pos), buff_size))
			{
				group = TooltipGroup::Buff;
				id = img.id;
				break;
			}
		}

		// for health bar
		{
			const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursorPos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_HP;
			}
			--bar_offset;
		}

		// for stamina bar
		{
			const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursorPos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_STAMINA;
			}
			--bar_offset;
		}

		// for mana bar
		if(mp_bar)
		{
			const Vec2 pos(0.f, float(gui->wndSize.y) - wndScale.y * (bar_offset * 18 - 1));
			const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &wndScale, nullptr, 0.f, &pos);
			const Rect r = gui->GetSpriteRect(tBar, mat);
			if(r.IsInside(gui->cursorPos))
			{
				group = TooltipGroup::Bar;
				id = Bar::BAR_MANA;
			}
		}

		// shortcuts
		int shortcut_index = GetShortcutIndex();
		if(dragAndDrop == 1)
		{
			if(Int2::Distance(gui->cursorPos, dragAndDropPos) > 3)
				dragAndDrop = 2;
			if(input->Released(Key::LeftButton))
			{
				dragAndDrop = 0;
				game->pc->shortcuts[dragAndDropIndex].trigger = true;
			}
		}
		else if(dragAndDrop == 2)
		{
			if(input->Released(Key::LeftButton))
			{
				dragAndDrop = 0;
				if(shortcut_index != -1)
				{
					if(dragAndDropType == -1)
					{
						// drag & drop between game gui shortcuts
						if(shortcut_index != dragAndDropIndex)
						{
							game->pc->SetShortcut(shortcut_index, game->pc->shortcuts[dragAndDropIndex].type, game->pc->shortcuts[dragAndDropIndex].value);
							game->pc->SetShortcut(dragAndDropIndex, Shortcut::TYPE_NONE);
						}
					}
					else
					{
						// drag & drop from actions/inventory
						game->pc->SetShortcut(shortcut_index, (Shortcut::Type)dragAndDropType, dragAndDropIndex);
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
					dragAndDrop = 1;
					dragAndDropPos = gui->cursorPos;
					dragAndDropType = -1;
					dragAndDropIndex = shortcut_index;
					dragAndDropIcon = nullptr;
				}
				else if(input->PressedRelease(Key::RightButton))
				{
					Shortcut& shortcut = game->pc->shortcuts[shortcut_index];
					if(shortcut.type == Shortcut::TYPE_ABILITY && shortcut.ability == PlayerController::data.abilityReady)
						PlayerController::data.abilityReady = nullptr;
					else
					{
						game->pc->SetShortcut(shortcut_index, Shortcut::TYPE_NONE);
						group = TooltipGroup::Invalid;
					}
					dragAndDrop = 0;
				}
			}
		}
	}
	else
		dragAndDrop = 0;

	if(anything)
		sidebar += dt * 5;
	else
		sidebar -= dt * 5;
	sidebar = Clamp(sidebar, 0.f, 1.f);

	if(sidebar > 0.f && !gui->HaveDialog() && show_tooltips)
	{
		int img_size = 76 * gui->wndSize.x / 1920;
		int offset = img_size + 2;
		int total = offset * max;
		int sposy = gui->wndSize.y - (gui->wndSize.y - total) / 2 - offset;
		for(int i = 0; i < max; ++i)
		{
			if(Rect::IsInside(gui->cursorPos, Int2(int(float(gui->wndSize.x) - sidebar * offset), sposy - i * offset), Int2(img_size, img_size)))
			{
				group = TooltipGroup::Sidebar;
				id = i;

				if(sidebarState[i] == 0)
					sidebarState[i] = 1;
				if(input->PressedRelease(Key::LeftButton))
				{
					switch((SideButtonId)i)
					{
					case SideButtonId::Menu:
						gameGui->ShowMenu();
						useCursor = false;
						break;
					case SideButtonId::Team:
						ShowPanel(OpenPanel::Team);
						break;
					case SideButtonId::Minimap:
						ShowPanel(OpenPanel::Minimap);
						if(gameGui->minimap->visible)
							useCursor = true;
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
						gameGui->mpBox->visible = !gameGui->mpBox->visible;
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

	if(dragAndDrop != 2)
		tooltip.UpdateTooltip(dt, (int)group, id);
	else
		tooltip.anything = false;
}

//=================================================================================================
int LevelGui::GetShortcutIndex()
{
	float wndScale = float(gui->wndSize.x) / 800;
	int img_size = 76 * gui->wndSize.x / 1920;
	int offset = img_size + 2;
	Int2 spos(256.f * wndScale, gui->wndSize.y - offset);
	for(int i = 0; i < Shortcut::MAX; ++i)
	{
		Rect r = Rect::Create(spos, Int2(img_size, img_size));
		if(r.IsInside(gui->cursorPos))
			return i;
		spos.x += offset;
	}
	return -1;
}

//=================================================================================================
void LevelGui::UpdateSpeechBubbles(float dt)
{
	bool removes = false;

	dt *= game->gameSpeed;

	for(vector<SpeechBubble*>::iterator it = speechBubbles.begin(), end = speechBubbles.end(); it != end; ++it)
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
					if(!sb.unit->meshInst)
					{
						game->ReportError(9, Format("Speech bubble for unit without meshInst (unit %s, text \"%.100s\").",
							sb.unit->GetRealName(), sb.text.c_str()));
					}
					else
						sb.unit->meshInst->needUpdate = true;
				}
				SpeechBubblePool.Free(*it);
				*it = nullptr;
				removes = true;
			}
		}
	}

	if(removes)
		RemoveNullElements(speechBubbles);
}

//=================================================================================================
bool LevelGui::NeedCursor() const
{
	if(game->dialogContext.dialogMode || useCursor)
		return true;
	return Container::NeedCursor();
}

//=================================================================================================
void LevelGui::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		Int2 dsize(gui->wndSize.x - 256 - 8, 104);
		Int2 offset((gui->wndSize.x - dsize.x) / 2, 32);
		scrollbar.size = Int2(12, 104);
		scrollbar.globalPos = scrollbar.pos = Int2(dsize.x + offset.x - 12, offset.y);
		dialogPos = offset;
		dialogSize = dsize;
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
		speechBubbles.push_back(unit->bubble);
	}

	// calculate size
	Int2 s = GameGui::fontSmall->CalculateSize(text);
	int total = s.x;
	int lines = 1 + total / 400;

	// setup
	unit->bubble->text = text;
	unit->bubble->unit = unit;
	unit->bubble->size = Int2(Max(32, total / lines + 20), s.y * lines + 20);
	unit->bubble->time = 0.f;
	unit->bubble->length = 1.5f + float(strlen(text)) / 20;
	unit->bubble->visible = false;
	unit->bubble->lastPos = unit->GetHeadPoint();

	unit->talking = true;
	unit->talk_timer = 0.f;
}

//=================================================================================================
void LevelGui::AddSpeechBubble(const Vec3& pos, cstring text)
{
	assert(text);

	// try to reuse previous bubble
	SpeechBubble* sb = nullptr;
	for(SpeechBubble* bubble : speechBubbles)
	{
		if(!bubble->unit && Vec3::DistanceSquared(pos, bubble->lastPos) < 0.1f)
		{
			sb = bubble;
			break;
		}
	}
	if(!sb)
	{
		sb = SpeechBubblePool.Get();
		speechBubbles.push_back(sb);
	}

	Int2 size = GameGui::fontSmall->CalculateSize(text);
	int total = size.x;
	int lines = 1 + total / 400;

	sb->text = text;
	sb->unit = nullptr;
	sb->size = Int2(total / lines + 20, size.y * lines + 20);
	sb->time = 0.f;
	sb->length = 1.5f + float(strlen(text)) / 20;
	sb->visible = true;
	sb->lastPos = pos;
}

//=================================================================================================
void LevelGui::Reset()
{
	debugInfo = false;
	debugInfo2 = false;
	SpeechBubblePool.Free(speechBubbles);
	Event(GuiEvent_Show);
	useCursor = false;
	sidebar = 0.f;
	unitViews.clear();
	boss = nullptr;
}

//=================================================================================================
bool LevelGui::UpdateChoice()
{
	DialogContext& ctx = game->dialogContext;
	const Int2 dsize(gui->wndSize.x - 256 - 8, 104);
	const Int2 offset((gui->wndSize.x - dsize.x) / 2, 32 + 6);
	const int choices = ctx.choices.size();

	// element pod kursorem
	int cursor_choice = -1;
	if(gui->cursorPos.x >= offset.x && gui->cursorPos.x < offset.x + dsize.x - 16 && gui->cursorPos.y >= offset.y && gui->cursorPos.y < offset.y + dsize.y - 12)
	{
		int w = (gui->cursorPos.y - offset.y + int(scrollbar.offset)) / GameGui::font->height;
		if(w < choices)
			cursor_choice = w;
	}

	// zmiana zaznaczonego elementu myszk¹
	if(gui->cursorPos != dialogCursorPos)
	{
		dialogCursorPos = gui->cursorPos;
		if(cursor_choice != -1)
			ctx.choiceSelected = cursor_choice;
	}

	// strza³ka w górê/dó³
	bool moved = false;
	if(ctx.choiceSelected != 0 && GKey.KeyPressedReleaseAllowed(GK_MOVE_FORWARD))
	{
		--ctx.choiceSelected;
		moved = true;
	}
	if(ctx.choiceSelected != choices - 1 && GKey.KeyPressedReleaseAllowed(GK_MOVE_BACK))
	{
		++ctx.choiceSelected;
		moved = true;
	}
	if(moved && choices > 5)
	{
		scrollbar.offset = float(GameGui::font->height * (ctx.choiceSelected - 2));
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
				ctx.choiceSelected = i;
				return true;
			}
		}
		if(choices >= 10)
		{
			if(input->PressedRelease(Key::N0))
			{
				ctx.choiceSelected = 9;
				return true;
			}
		}
	}

	// wybieranie enterem/esc/spacj¹
	if(GKey.KeyPressedReleaseAllowed(GK_SELECT_DIALOG))
		return true;
	else if(ctx.dialogEsc != -1 && GKey.AllowKeyboard() && input->PressedRelease(Key::Escape))
	{
		ctx.choiceSelected = ctx.dialogEsc;
		return true;
	}

	// wybieranie klikniêciem
	if(GKey.AllowMouse() && cursor_choice != -1 && input->PressedRelease(Key::LeftButton))
	{
		if(ctx.isLocal)
			game->pc->data.wastedKey = Key::LeftButton;
		ctx.choiceSelected = cursor_choice;
		return true;
	}

	// aktualizacja paska przewijania
	scrollbar.mouseFocus = focus;
	if(input->Focus() && Rect::IsInside(gui->cursorPos, dialogPos, dialogSize) && scrollbar.ApplyMouseWheel())
		dialogCursorPos = Int2(-1, -1);
	scrollbar.Update(0.f);

	return false;
}

//=================================================================================================
void LevelGui::UpdateScrollbar(int choices)
{
	scrollbar.part = 104 - 8; // border * 2
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
	tooltip.bigText.clear();
	tooltip.smallText.clear();

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
				tooltip.text = Format("%s (%s)", txMenu, gameGui->controls->GetKeyText(Key::Escape));
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
				gameGui->ability->GetAbilityTooltip(tooltip, *shortcut.ability);
				tooltip.smallText = Format("%s\n%s", tooltip.text.c_str(), tooltip.smallText.c_str());
				tooltip.text = tooltip.bigText;
				tooltip.bigText.clear();
				tooltip.img = nullptr;
			}

			const GameKey& gk = GKey[GK_SHORTCUT1 + id];
			if(shortcut.type == Shortcut::TYPE_ABILITY)
			{
				if(gk[0] != Key::None)
					tooltip.text = Format("%s (%s)", tooltip.text.c_str(), gameGui->controls->GetKeyText(gk[0]));
			}
			else
			{
				if(gk[0] != Key::None)
					title = Format("%s (%s)", title, gameGui->controls->GetKeyText(gk[0]));
				tooltip.text = title;
				if(desc)
					tooltip.smallText = desc;
				else
					tooltip.smallText.clear();
			}
		}
		break;
	}
}

//=================================================================================================
bool LevelGui::HavePanelOpen() const
{
	return gameGui->stats->visible
		|| gameGui->inventory->invMine->visible
		|| gameGui->inventory->gpTrade->visible
		|| gameGui->team->visible
		|| gameGui->journal->visible
		|| gameGui->minimap->visible
		|| gameGui->ability->visible
		|| gameGui->craft->visible;
}

//=================================================================================================
void LevelGui::ClosePanels(bool closeMpBox)
{
	if(gameGui->stats->visible)
		gameGui->stats->Hide();
	if(gameGui->inventory->invMine->visible)
		gameGui->inventory->invMine->Hide();
	if(gameGui->team->visible)
		gameGui->team->Hide();
	if(gameGui->journal->visible)
		gameGui->journal->Hide();
	if(gameGui->minimap->visible)
		gameGui->minimap->Hide();
	if(gameGui->inventory->gpTrade->visible)
		gameGui->inventory->gpTrade->Hide();
	if(closeMpBox && gameGui->mpBox->visible)
		gameGui->mpBox->visible = false;
	if(gameGui->ability->visible)
		gameGui->ability->Hide();
	if(gameGui->book->visible)
		gameGui->book->Hide();
	if(gameGui->craft->visible)
		gameGui->craft->Hide();
}

//=================================================================================================
void LevelGui::GetGamePanels(vector<GamePanel*>& panels)
{
	panels.push_back(gameGui->inventory->invMine);
	panels.push_back(gameGui->stats);
	panels.push_back(gameGui->team);
	panels.push_back(gameGui->journal);
	panels.push_back(gameGui->minimap);
	panels.push_back(gameGui->inventory->invTradeMine);
	panels.push_back(gameGui->inventory->invTradeOther);
	panels.push_back(gameGui->mpBox);
	panels.push_back(gameGui->ability);
}

//=================================================================================================
OpenPanel LevelGui::GetOpenPanel()
{
	if(gameGui->stats->visible)
		return OpenPanel::Stats;
	else if(gameGui->inventory->invMine->visible)
		return OpenPanel::Inventory;
	else if(gameGui->team->visible)
		return OpenPanel::Team;
	else if(gameGui->journal->visible)
		return OpenPanel::Journal;
	else if(gameGui->minimap->visible)
		return OpenPanel::Minimap;
	else if(gameGui->inventory->gpTrade->visible)
		return OpenPanel::Trade;
	else if(gameGui->ability->visible)
		return OpenPanel::Ability;
	else if(gameGui->book->visible)
		return OpenPanel::Book;
	else if(gameGui->craft->visible)
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
		gameGui->stats->Hide();
		break;
	case OpenPanel::Inventory:
		gameGui->inventory->invMine->Hide();
		break;
	case OpenPanel::Team:
		gameGui->team->Hide();
		break;
	case OpenPanel::Journal:
		gameGui->journal->Hide();
		break;
	case OpenPanel::Minimap:
		gameGui->minimap->Hide();
		break;
	case OpenPanel::Trade:
		game->OnCloseInventory();
		gameGui->inventory->gpTrade->Hide();
		break;
	case OpenPanel::Ability:
		gameGui->ability->Hide();
		break;
	case OpenPanel::Book:
		gameGui->book->Hide();
		break;
	case OpenPanel::Craft:
		gameGui->craft->Hide();
		break;
	}

	// open new panel
	if(open != to_open)
	{
		switch(to_open)
		{
		case OpenPanel::Stats:
			gameGui->stats->Show();
			break;
		case OpenPanel::Inventory:
			gameGui->inventory->invMine->Show();
			break;
		case OpenPanel::Team:
			gameGui->team->Show();
			break;
		case OpenPanel::Journal:
			gameGui->journal->Show();
			break;
		case OpenPanel::Minimap:
			gameGui->minimap->Show();
			break;
		case OpenPanel::Ability:
			gameGui->ability->Show();
			break;
		}
	}
	else
		useCursor = false;
}

//=================================================================================================
void LevelGui::PositionPanels()
{
	float scale = float(gui->wndSize.x) / 1024;
	Int2 pos = Int2(int(scale * 48), int(scale * 32));
	Int2 size = Int2(gui->wndSize.x - pos.x * 2, gui->wndSize.y - pos.x * 2);

	gameGui->stats->globalPos = gameGui->stats->pos = pos;
	gameGui->stats->size = size;
	gameGui->team->globalPos = gameGui->team->pos = pos;
	gameGui->team->size = size;
	gameGui->inventory->invMine->globalPos = gameGui->inventory->invMine->pos = pos;
	gameGui->inventory->invMine->size = size;
	gameGui->inventory->invTradeOther->globalPos = gameGui->inventory->invTradeOther->pos = pos;
	gameGui->inventory->invTradeOther->size = Int2(size.x, (size.y - 32) / 2);
	gameGui->inventory->invTradeMine->globalPos = gameGui->inventory->invTradeMine->pos
		= Int2(pos.x, gameGui->inventory->invTradeOther->pos.y + gameGui->inventory->invTradeOther->size.y + 16);
	gameGui->inventory->invTradeMine->size = gameGui->inventory->invTradeOther->size;
	gameGui->minimap->size = Int2(size.y, size.y);
	gameGui->minimap->globalPos = gameGui->minimap->pos = Int2((gui->wndSize.x - gameGui->minimap->size.x) / 2, (gui->wndSize.y - gameGui->minimap->size.y) / 2);
	gameGui->journal->size = gameGui->minimap->size;
	gameGui->journal->globalPos = gameGui->journal->pos = gameGui->minimap->pos;
	gameGui->mpBox->size = Int2((gui->wndSize.x - 32) / 2, (gui->wndSize.y - 64) / 4);
	gameGui->mpBox->globalPos = gameGui->mpBox->pos = Int2(gui->wndSize.x - pos.x - gameGui->mpBox->size.x, gui->wndSize.y - pos.x - gameGui->mpBox->size.y);
	gameGui->ability->globalPos = gameGui->ability->pos = pos;
	gameGui->ability->size = size;

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
	f << speechBubbles.size();
	for(const SpeechBubble* p_sb : speechBubbles)
	{
		const SpeechBubble& sb = *p_sb;
		f.WriteString2(sb.text);
		f << (sb.unit ? sb.unit->id : -1);
		f << sb.size;
		f << sb.time;
		f << sb.length;
		f << sb.visible;
		f << sb.lastPos;
	}
}

//=================================================================================================
void LevelGui::Load(GameReader& f)
{
	speechBubbles.resize(f.Read<uint>());
	for(vector<SpeechBubble*>::iterator it = speechBubbles.begin(), end = speechBubbles.end(); it != end; ++it)
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
		f >> sb.lastPos;
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
	LocationPart& locPart = *game->pc->unit->locPart;
	Unit& u = *game->pc->unit;

	// mark previous views as invalid
	for(vector<UnitView>::iterator it = unitViews.begin(), end = unitViews.end(); it != end; ++it)
		it->valid = false;

	// check units inside player view
	for(vector<Unit*>::iterator it = locPart.units.begin(), end = locPart.units.end(); it != end; ++it)
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
			if(dist < ALERT_RANGE && gameLevel->camera.frustum.SphereToFrustum(u2.visual_pos, u2.GetSphereRadius()) && gameLevel->CanSee(u, u2))
				AddUnitView(&u2);
		}
	}

	// extra units
	if(game->pc->data.abilityReady && game->pc->data.abilityOk)
	{
		if(Unit* target = game->pc->data.abilityTarget; target && target != &u)
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
	for(vector<UnitView>::iterator it = unitViews.begin(), end = unitViews.end(); it != end;)
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
						unitViews.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						unitViews.pop_back();
						end = unitViews.end();
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
						unitViews.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						unitViews.pop_back();
						end = unitViews.end();
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
	for(UnitView& view : unitViews)
	{
		if(view.unit == unit)
		{
			view.valid = true;
			view.lastPos = unit->GetUnitTextPos();
			return;
		}
	}

	UnitView& uv = Add1(unitViews);
	uv.valid = true;
	uv.unit = unit;
	uv.time = 0.f;
	uv.lastPos = unit->GetUnitTextPos();
}

//=================================================================================================
void LevelGui::RemoveUnitView(Unit* unit)
{
	for(vector<UnitView>::iterator it = unitViews.begin(), end = unitViews.end(); it != end; ++it)
	{
		if(it->unit == unit)
		{
			unitViews.erase(it);
			break;
		}
	}
}

//=================================================================================================
void LevelGui::StartDragAndDrop(int type, int value, Texture* icon)
{
	dragAndDrop = 2;
	dragAndDropType = type;
	dragAndDropIndex = value;
	dragAndDropIcon = icon;
}

//=================================================================================================
void LevelGui::ResetCutscene()
{
	cutsceneImage = nullptr;
	cutsceneNextImages.clear();
	cutsceneImageTimer = 0;
	cutsceneImageState = CS_NONE;
	cutsceneText.clear();
	cutsceneNextTexts.clear();
	cutsceneTextTimer = 0;
	cutsceneTextState = CS_NONE;
}

//=================================================================================================
void LevelGui::SetCutsceneImage(Texture* tex, float time)
{
	cutsceneNextImages.push_back(std::make_pair(tex, time));
}

//=================================================================================================
void LevelGui::SetCutsceneText(const string& text, float time)
{
	cutsceneNextTexts.push_back(std::make_pair(text, time));
}

//=================================================================================================
void LevelGui::UpdateCutscene(float dt)
{
	// is everything shown?
	if(cutsceneImageState == CS_NONE && cutsceneNextImages.empty() && cutsceneTextState == CS_NONE && cutsceneNextTexts.empty())
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
	switch(cutsceneImageState)
	{
	case CS_NONE:
		if(!cutsceneNextImages.empty())
		{
			cutsceneImage = cutsceneNextImages.front().first;
			cutsceneImageTimer = 0.f;
			cutsceneImageState = CS_FADE_IN;
		}
		break;
	case CS_FADE_IN:
		cutsceneImageTimer += dt * 2;
		if(cutsceneImageTimer >= 1.f)
		{
			cutsceneImageTimer = cutsceneNextImages.front().second;
			cutsceneImageState = CS_WAIT;
			cutsceneNextImages.erase(cutsceneNextImages.begin());
		}
		break;
	case CS_WAIT:
		cutsceneImageTimer -= dt;
		if(cutsceneImageTimer <= 0)
		{
			cutsceneImageTimer = 1.f;
			cutsceneImageState = CS_FADE_OUT;
		}
		break;
	case CS_FADE_OUT:
		cutsceneImageTimer -= dt * 2;
		if(cutsceneImageTimer <= 0)
		{
			cutsceneImage = nullptr;
			cutsceneImageState = CS_NONE;
		}
		break;
	}

	// update text
	switch(cutsceneTextState)
	{
	case CS_NONE:
		if(!cutsceneNextTexts.empty())
		{
			cutsceneText = cutsceneNextTexts.front().first;
			cutsceneTextTimer = 0.f;
			cutsceneTextState = CS_FADE_IN;
		}
		break;
	case CS_FADE_IN:
		cutsceneTextTimer += dt * 2;
		if(cutsceneTextTimer >= 1.f)
		{
			cutsceneTextTimer = cutsceneNextTexts.front().second;
			cutsceneTextState = CS_WAIT;
			cutsceneNextTexts.erase(cutsceneNextTexts.begin());
		}
		break;
	case CS_WAIT:
		cutsceneTextTimer -= dt;
		if(cutsceneTextTimer <= 0)
		{
			cutsceneTextTimer = 1.f;
			cutsceneTextState = CS_FADE_OUT;
		}
		break;
	case CS_FADE_OUT:
		cutsceneTextTimer -= dt * 2;
		if(cutsceneTextTimer <= 0)
		{
			cutsceneText.clear();
			cutsceneTextState = CS_NONE;
		}
		break;
	}
}

//=================================================================================================
void LevelGui::DrawCutscene(int fallbackAlpha)
{
	if(team->IsLeader())
	{
		const Rect r = { 4,4,200,200 };
		gui->DrawText(gameGui->font, txSkipCutscene, DTF_LEFT | DTF_TOP | DTF_OUTLINE, Color::Alpha(fallbackAlpha), r);
	}

	if(cutsceneImage)
	{
		const int alpha = GetAlpha(cutsceneImageState, cutsceneImageTimer, fallbackAlpha);
		Int2 img_size = cutsceneImage->GetSize();
		const int max_size = gui->wndSize.y - 128;
		const Vec2 scale(float(max_size) / img_size.y);
		img_size *= scale.x;
		const Vec2 pos((gui->wndSize - img_size) / 2);
		const Matrix mat = Matrix::Transform2D(nullptr, 0, &scale, nullptr, 0.f, &pos);
		gui->DrawSprite2(cutsceneImage, mat, nullptr, nullptr, Color::Alpha(alpha));
	}

	if(!cutsceneText.empty())
	{
		const int alpha = GetAlpha(cutsceneTextState, cutsceneTextTimer, fallbackAlpha);
		const Rect r = { 0, gui->wndSize.y - 64,gui->wndSize.x, gui->wndSize.y };
		gui->DrawText(gameGui->font, cutsceneText, DTF_OUTLINE | DTF_CENTER | DTF_VCENTER, Color::Alpha(alpha), r);
	}
}

//=================================================================================================
int LevelGui::GetAlpha(CutsceneState cs, float timer, int fallbackAlpha)
{
	switch(cs)
	{
	case CS_NONE:
		return 0;
	case CS_WAIT:
		return fallbackAlpha;
	default:
		return int(timer * 255) * fallbackAlpha / 255;
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
