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
	tItemBar = resMgr->Load<Texture>("item_bar.png");
	tMelee = resMgr->Load<Texture>("sword-brandish.png");
	tRanged = resMgr->Load<Texture>("bow-arrow.png");
	tHealthPotion = resMgr->Load<Texture>("health-potion.png");
	tManaPotion = resMgr->Load<Texture>("mana-potion.png");
}

//=================================================================================================
void AbilityPanel::Refresh()
{
	abilities.clear();
	for(PlayerAbility& ab : game->pc->abilities)
		abilities.push_back(ab.ability);
}

//=================================================================================================
void AbilityPanel::Draw()
{
	GamePanel::Draw();
	grid_offset = 0;

	// title
	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	gui->DrawText(GameGui::fontBig, txTitle, DTF_TOP | DTF_CENTER, Color::Black, rect);

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

	gui->DrawText(GameGui::fontBig, text, DTF_LEFT, Color::Black, Rect(shift_x, shift_y, shift_x + 400, shift_y + 50));
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
				const Vec2 scale(ratio, ratio);
				const Vec2 pos(shift + Int2(1, 1));
				const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
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
		dragAndDrop = false;
		tooltip.Clear();
	}
}

//=================================================================================================
void AbilityPanel::Update(float dt)
{
	GamePanel::Update(dt);

	if(!focus)
	{
		dragAndDrop = false;
		return;
	}

	if(game_gui->levelGui->IsDragAndDrop())
	{
		tooltip.anything = false;
		return;
	}

	if(dragAndDrop)
	{
		if(Int2::Distance(gui->cursorPos, dragAndDropPos) > 3)
		{
			pair<int, int> shortcut = ConvertToShortcut(dragAndDropGroup, dragAndDropIndex);
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
			game_gui->levelGui->StartDragAndDrop(shortcut.first, shortcut.second, icon);
			dragAndDrop = false;
		}
		if(input->Released(Key::LeftButton))
			dragAndDrop = false;
	}

	int group = G_NONE, id = -1;
	grid_offset = 0;
	if(!abilities.empty())
		UpdateGroup(abilities.size(), G_ACTION, group, id);
	UpdateGroup(4, G_OTHER, group, id);
	tooltip.UpdateTooltip(dt, group, id);

	if(input->Focus())
	{
		if(IsInside(gui->cursorPos))
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
void AbilityPanel::UpdateGroup(uint count, int group, int& groupResult, int& idResult)
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
			if(Rect::IsInside(gui->cursorPos, Int2(shift_x + x * 63, shift_y + y * 63), Int2(64, 64)))
			{
				groupResult = group;
				idResult = index;
				if(input->Focus() && input->Pressed(Key::LeftButton))
				{
					dragAndDrop = true;
					dragAndDropPos = gui->cursorPos;
					dragAndDropGroup = group;
					dragAndDropIndex = index;
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
		tooltip.smallText.clear();
		switch(id)
		{
		case O_MELEE_WEAPON:
			tooltip.img = tMelee;
			tooltip.bigText = txMeleeWeapon;
			tooltip.text = txMeleeWeaponDesc;
			break;
		case O_RANGED_WEAPON:
			tooltip.img = tRanged;
			tooltip.bigText = txRangedWeapon;
			tooltip.text = txRangedWeaponDesc;
			break;
		case O_HEALTH_POTION:
			tooltip.img = tHealthPotion;
			tooltip.bigText = txHealthPotion;
			tooltip.text = txHealthPotionDesc;
			break;
		case O_MANA_POTION:
			tooltip.img = tManaPotion;
			tooltip.bigText = txManaPotion;
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
	tooltip.bigText = ability.name;

	tooltip.text = ability.desc;
	uint pos = tooltip.text.find("{power}", 0);
	if(pos != string::npos)
		tooltip.text.replace(pos, 7, Format("%d", (int)game->pc->unit->GetAbilityPower(ability)));

	if(ability.charges == 1)
		tooltip.smallText = Format(txCooldown, ability.cooldown.x);
	else
		tooltip.smallText = Format(txCooldownCharges, ability.cooldown.x, ability.charges, ability.recharge);
	if(ability.mana > 0.f || ability.stamina > 0.f)
	{
		tooltip.smallText += '\n';
		tooltip.smallText += txCost;
		if(ability.mana > 0.f)
			tooltip.smallText += Format("$cb%g %s$c-", ability.mana, txMana);
		if(ability.stamina > 0.f)
		{
			if(ability.mana > 0.f)
				tooltip.smallText += ", ";
			tooltip.smallText += Format("$cy%g %s$c-", ability.stamina, txStamina);
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
