#include "Pch.h"
#include "AbilityPanel.h"

#include "Ability.h"
#include "Game.h"
#include "GameGui.h"
#include "GameKeys.h"
#include "Language.h"
#include "LevelGui.h"
#include "PlayerController.h"
#include "Unit.h"

#include <ResourceManager.h>

enum TooltipGroup
{
	G_NONE = -1,
	G_ACTION,
	G_OTHER
};

enum OtherAction
{
	O_MELEE_WEAPON,
	O_RANGED_WEAPON,
	O_HEALTH_POTION,
	O_MANA_POTION
};

//=================================================================================================
AbilityPanel::AbilityPanel()
{
	visible = false;
	tooltip.Init(TooltipController::Callback(this, &AbilityPanel::GetTooltip));
}

//=================================================================================================
void AbilityPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("AbilityPanel");
	txTitle = s.Get("title");
	txCooldown = s.Get("cooldown");
	txCooldownCharges = s.Get("cooldownCharges");
	txCost = s.Get("cost");
	txAbilities = s.Get("abilities");
	txOther = s.Get("other");
	txMana = s.Get("mana");
	txStamina = s.Get("stamina");

	Language::Section s2 = Language::GetSection("LevelGui");
	txMeleeWeapon = s2.Get("meleeWeapon");
	txRangedWeapon = s2.Get("rangedWeapon");
	txHealthPotion = s2.Get("healthPotion");
	txManaPotion = s2.Get("manaPotion");
	txMeleeWeaponDesc = s2.Get("meleeWeaponDesc");
	txRangedWeaponDesc = s2.Get("rangedWeaponDesc");
	txHealthPotionDesc = s2.Get("healthPotionDesc");
	txManaPotionDesc = s2.Get("manaPotionDesc");
}

//=================================================================================================
void AbilityPanel::LoadData()
{
	tItemBar = res_mgr->Load<Texture>("item_bar.png");
	tMelee = res_mgr->Load<Texture>("sword-brandish.png");
	tRanged = res_mgr->Load<Texture>("bow-arrow.png");
	tHealthPotion = res_mgr->Load<Texture>("health-potion.png");
	tManaPotion = res_mgr->Load<Texture>("mana-potion.png");
}

//=================================================================================================
void AbilityPanel::Refresh()
{
	abilities.clear();
	for(PlayerAbility& ab : game->pc->abilities)
		abilities.push_back(ab.ability);
}

//=================================================================================================
void AbilityPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();
	grid_offset = 0;

	// title
	Rect rect = {
		pos.x,
		pos.y + 10,
		pos.x + size.x,
		pos.y + size.y
	};
	gui->DrawText(GameGui::font_big, txTitle, DTF_TOP | DTF_CENTER, Color::Black, rect);

	// abilities grid group
	if(!abilities.empty())
	{
		images.clear();
		for(Ability* ability : abilities)
			images.push_back(ability->tex_icon);
		DrawGroup(txAbilities);
	}

	// other grid group
	images.clear();
	images.push_back(tMelee);
	images.push_back(tRanged);
	images.push_back(tHealthPotion);
	images.push_back(tManaPotion);
	DrawGroup(txOther);

	// tooltips
	tooltip.Draw();
}

//=================================================================================================
void AbilityPanel::DrawGroup(cstring text)
{
	int count_w = (size.x - 48) / 63;
	int count_h = images.size() / count_w + 1;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2 + grid_offset;

	gui->DrawText(GameGui::font_big, text, DTF_LEFT, Color::Black, Rect(shift_x, shift_y, shift_x + 400, shift_y + 50));
	shift_y += 40;

	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			Int2 shift(shift_x + x * 63, shift_y + y * 63);
			gui->DrawSprite(tItemBar, shift);
			uint index = x + y * count_w;
			if(index < images.size())
			{
				const float ratio = 62.f / 128.f;
				Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(ratio, ratio), nullptr, 0.f, &Vec2(shift + Int2(1, 1)));
				gui->DrawSprite2(images[index], mat);
			}
		}
	}

	grid_offset += count_h * 63 + 40;
}

//=================================================================================================
void AbilityPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Show)
	{
		drag_and_drop = false;
		tooltip.Clear();
	}
}

//=================================================================================================
void AbilityPanel::Update(float dt)
{
	GamePanel::Update(dt);

	if(!focus)
	{
		drag_and_drop = false;
		return;
	}

	if(game_gui->level_gui->IsDragAndDrop())
	{
		tooltip.anything = false;
		return;
	}

	if(drag_and_drop)
	{
		if(Int2::Distance(gui->cursor_pos, drag_and_drop_pos) > 3)
		{
			pair<int, int> shortcut = ConvertToShortcut(drag_and_drop_group, drag_and_drop_index);
			Texture* icon = nullptr;
			if(shortcut.first == Shortcut::TYPE_ABILITY)
			{
				Ability* ability = reinterpret_cast<Ability*>(shortcut.second);
				icon = ability->tex_icon;
			}
			else
			{
				switch(shortcut.second)
				{
				case Shortcut::SPECIAL_MELEE_WEAPON:
					icon = tMelee;
					break;
				case Shortcut::SPECIAL_RANGED_WEAPON:
					icon = tRanged;
					break;
				case Shortcut::SPECIAL_HEALTH_POTION:
					icon = tHealthPotion;
					break;
				case Shortcut::SPECIAL_MANA_POTION:
					icon = tManaPotion;
					break;
				}
			}
			game_gui->level_gui->StartDragAndDrop(shortcut.first, shortcut.second, icon);
			drag_and_drop = false;
		}
		if(input->Released(Key::LeftButton))
			drag_and_drop = false;
	}

	int group = G_NONE, id = -1;
	grid_offset = 0;
	if(!abilities.empty())
		UpdateGroup(abilities.size(), G_ACTION, group, id);
	UpdateGroup(4, G_OTHER, group, id);
	tooltip.UpdateTooltip(dt, group, id);

	if(input->Focus())
	{
		if(IsInside(gui->cursor_pos))
		{
			for(int i = 0; i < Shortcut::MAX; ++i)
			{
				if(GKey.PressedRelease((GAME_KEYS)(GK_SHORTCUT1 + i)) && group != G_NONE)
				{
					pair<int, int> shortcut = ConvertToShortcut(group, id);
					game->pc->SetShortcut(i, (Shortcut::Type)shortcut.first, shortcut.second);
				}
			}
		}

		if(input->PressedRelease(Key::Escape))
			Hide();
	}
}

//=================================================================================================
void AbilityPanel::UpdateGroup(uint count, int group, int& group_result, int& id_result)
{
	int count_w = (size.x - 48) / 63;
	int count_h = count / count_w + 1;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2 + grid_offset + 40;

	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			uint index = x + y * count_w;
			if(index >= count)
				break;
			if(PointInRect(gui->cursor_pos, Int2(shift_x + x * 63, shift_y + y * 63), Int2(64, 64)))
			{
				group_result = group;
				id_result = index;
				if(input->Focus() && input->Pressed(Key::LeftButton))
				{
					drag_and_drop = true;
					drag_and_drop_pos = gui->cursor_pos;
					drag_and_drop_group = group;
					drag_and_drop_index = index;
				}
				break;
			}
		}
	}

	grid_offset += count_h * 63 + 40;
}

//=================================================================================================
void AbilityPanel::GetTooltip(TooltipController*, int group, int id, bool refresh)
{
	if(group == G_NONE)
	{
		tooltip.anything = false;
		return;
	}

	if(group == G_ACTION)
	{
		Ability& ability = *abilities[id];
		GetAbilityTooltip(tooltip, ability);
	}
	else
	{
		tooltip.anything = true;
		tooltip.small_text.clear();
		switch(id)
		{
		case O_MELEE_WEAPON:
			tooltip.img = tMelee;
			tooltip.big_text = txMeleeWeapon;
			tooltip.text = txMeleeWeaponDesc;
			break;
		case O_RANGED_WEAPON:
			tooltip.img = tRanged;
			tooltip.big_text = txRangedWeapon;
			tooltip.text = txRangedWeaponDesc;
			break;
		case O_HEALTH_POTION:
			tooltip.img = tHealthPotion;
			tooltip.big_text = txHealthPotion;
			tooltip.text = txHealthPotionDesc;
			break;
		case O_MANA_POTION:
			tooltip.img = tManaPotion;
			tooltip.big_text = txManaPotion;
			tooltip.text = txManaPotionDesc;
			break;
		}
	}
}

//=================================================================================================
void AbilityPanel::GetAbilityTooltip(TooltipController& tooltip, Ability& ability)
{
	tooltip.anything = true;
	tooltip.img = ability.tex_icon;
	tooltip.big_text = ability.name;

	tooltip.text = ability.desc;
	uint pos = tooltip.text.find("{power}", 0);
	if(pos != string::npos)
		tooltip.text.replace(pos, 7, Format("%d", (int)game->pc->unit->GetAbilityPower(ability)));

	if(ability.charges == 1)
		tooltip.small_text = Format(txCooldown, ability.cooldown.x);
	else
		tooltip.small_text = Format(txCooldownCharges, ability.cooldown.x, ability.charges, ability.recharge);
	if(ability.mana > 0.f || ability.stamina > 0.f)
	{
		tooltip.small_text += '\n';
		tooltip.small_text += txCost;
		if(ability.mana > 0.f)
			tooltip.small_text += Format("$cb%g %s$c-", ability.mana, txMana);
		if(ability.stamina > 0.f)
		{
			if(ability.mana > 0.f)
				tooltip.small_text += ", ";
			tooltip.small_text += Format("$cy%g %s$c-", ability.stamina, txStamina);
		}
	}
}

//=================================================================================================
pair<int, int> AbilityPanel::ConvertToShortcut(int group, int id)
{
	pair<int, int> shortcut;
	if(group == G_ACTION)
	{
		shortcut.first = Shortcut::TYPE_ABILITY;
		shortcut.second = reinterpret_cast<int>(abilities[id]);
	}
	else
	{
		assert(group == G_OTHER);
		shortcut.first = Shortcut::TYPE_SPECIAL;
		switch(id)
		{
		default:
			assert(0);
		case O_MELEE_WEAPON:
			shortcut.second = Shortcut::SPECIAL_MELEE_WEAPON;
			break;
		case O_RANGED_WEAPON:
			shortcut.second = Shortcut::SPECIAL_RANGED_WEAPON;
			break;
		case O_HEALTH_POTION:
			shortcut.second = Shortcut::SPECIAL_HEALTH_POTION;
			break;
		case O_MANA_POTION:
			shortcut.second = Shortcut::SPECIAL_MANA_POTION;
			break;
		}
	}
	return shortcut;
}
