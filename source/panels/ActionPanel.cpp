#include "Pch.h"
#include "GameCore.h"
#include "ActionPanel.h"
#include "LevelGui.h"
#include "GameGui.h"
#include "Action.h"
#include "Language.h"
#include "ResourceManager.h"
#include "GameKeys.h"
#include "PlayerController.h"
#include "Game.h"

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
	O_POTION
};

//=================================================================================================
ActionPanel::ActionPanel()
{
	visible = false;
	tooltip.Init(TooltipGetText(this, &ActionPanel::GetTooltip));
}

//=================================================================================================
void ActionPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("ActionPanel");
	txActions = s.Get("actions");
	txCooldown = s.Get("cooldown");
	txCooldownCharges = s.Get("cooldownCharges");
	txAbilities = s.Get("abilities");
	txOther = s.Get("other");

	Language::Section s2 = Language::GetSection("LevelGui");
	txMeleeWeapon = s2.Get("meleeWeapon");
	txRangedWeapon = s2.Get("rangedWeapon");
	txPotion = s2.Get("potion");
	txMeleeWeaponDesc = s2.Get("meleeWeaponDesc");
	txRangedWeaponDesc = s2.Get("rangedWeaponDesc");
	txPotionDesc = s2.Get("potionDesc");
}

//=================================================================================================
void ActionPanel::LoadData()
{
	tItemBar = res_mgr->Load<Texture>("item_bar.png");
	tMelee = res_mgr->Load<Texture>("sword-brandish.png");
	tRanged = res_mgr->Load<Texture>("bow-arrow.png");
	tPotion = res_mgr->Load<Texture>("health-potion.png");
}

//=================================================================================================
void ActionPanel::Init(Action* action)
{
	actions.clear();
	if(action)
		actions.push_back(action);
}

//=================================================================================================
void ActionPanel::Draw(ControlDrawData*)
{
	GamePanel::Draw();
	grid_offset = 0;

	// capition
	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	gui->DrawText(GameGui::font_big, txActions, DTF_TOP | DTF_CENTER, Color::Black, rect);

	// abilities grid group
	if(!actions.empty())
	{
		images.clear();
		images.push_back(actions[0]->tex);
		DrawGroup(txAbilities);
	}

	// other grid group
	images.clear();
	images.push_back(tMelee);
	images.push_back(tRanged);
	images.push_back(tPotion);
	DrawGroup(txOther);

	// tooltips
	tooltip.Draw();
}

//=================================================================================================
void ActionPanel::DrawGroup(cstring text)
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
void ActionPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Show)
	{
		drag_and_drop = false;
		tooltip.Clear();
	}
}

//=================================================================================================
void ActionPanel::Update(float dt)
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
			int value = ConvertToShortcutSpecial(drag_and_drop_group, drag_and_drop_index);
			Texture* icon = nullptr;
			switch(value)
			{
			case Shortcut::SPECIAL_MELEE_WEAPON:
				icon = tMelee;
				break;
			case Shortcut::SPECIAL_RANGED_WEAPON:
				icon = tRanged;
				break;
			case Shortcut::SPECIAL_HEALING_POTION:
				icon = tPotion;
				break;
			case Shortcut::SPECIAL_ACTION:
				icon = actions[0]->tex;
				break;
			}
			game_gui->level_gui->StartDragAndDrop(Shortcut::TYPE_SPECIAL, value, icon);
			drag_and_drop = false;
		}
		if(input->Released(Key::LeftButton))
			drag_and_drop = false;
	}

	int group = G_NONE, id = -1;
	grid_offset = 0;
	if(!actions.empty())
		UpdateGroup(1, G_ACTION, group, id);
	UpdateGroup(3, G_OTHER, group, id);
	tooltip.UpdateTooltip(dt, group, id);

	if(input->Focus())
	{
		if(IsInside(gui->cursor_pos))
		{
			for(int i = 0; i < Shortcut::MAX; ++i)
			{
				if(GKey.PressedRelease((GAME_KEYS)(GK_SHORTCUT1 + i)) && group != G_NONE)
				{
					int value = ConvertToShortcutSpecial(group, id);
					game->pc->SetShortcut(i, Shortcut::TYPE_SPECIAL, value);
				}
			}
		}

		if(input->PressedRelease(Key::Escape))
			Hide();
	}
}

//=================================================================================================
void ActionPanel::UpdateGroup(uint count, int group, int& group_result, int& id_result)
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
void ActionPanel::GetTooltip(TooltipController*, int group, int id)
{
	if(group == G_NONE)
	{
		tooltip.anything = false;
		return;
	}

	if(group == G_ACTION)
	{
		Action& action = *actions[id];
		tooltip.anything = true;
		tooltip.img = action.tex;
		tooltip.big_text = action.name;
		tooltip.text = action.desc;
		if(action.charges == 1)
			tooltip.small_text = Format(txCooldown, action.cooldown);
		else
			tooltip.small_text = Format(txCooldownCharges, action.cooldown, action.charges, action.recharge);
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
		case O_POTION:
			tooltip.img = tPotion;
			tooltip.big_text = txPotion;
			tooltip.text = txPotionDesc;
			break;
		}
	}
}

//=================================================================================================
int ActionPanel::ConvertToShortcutSpecial(int group, int id)
{
	if(group == G_ACTION)
		return Shortcut::SPECIAL_ACTION;
	else
	{
		assert(group == G_OTHER);
		switch(id)
		{
		default:
			assert(0);
		case O_MELEE_WEAPON:
			return Shortcut::SPECIAL_MELEE_WEAPON;
		case O_RANGED_WEAPON:
			return Shortcut::SPECIAL_RANGED_WEAPON;
		case O_POTION:
			return Shortcut::SPECIAL_HEALING_POTION;
		}
	}
}
