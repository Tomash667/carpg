#include "Pch.h"
#include "GameCore.h"
#include "ActionPanel.h"
#include "Action.h"
#include "Language.h"
#include "ResourceManager.h"
#include "KeyStates.h"

//=================================================================================================
ActionPanel::ActionPanel()
{
	visible = false;

	txActions = Str("actions");
	txCooldown = Str("cooldown");
	txCooldownCharges = Str("cooldownCharges");

	tooltip.Init(TooltipGetText(this, &ActionPanel::GetTooltip));
}

//=================================================================================================
void ActionPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("item_bar.png", tItemBar);
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

	// capition
	Rect rect = {
		pos.x + 8,
		pos.y + 8,
		pos.x + size.x - 16,
		pos.y + size.y - 16
	};
	GUI.DrawText(GUI.fBig, txActions, DTF_TOP | DTF_CENTER, Color::Black, rect);

	// grid
	int count_w = (size.x - 48) / 63;
	int count_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;
	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			Int2 shift(shift_x + x * 63, shift_y + y * 63);
			GUI.DrawSprite(tItemBar, shift);
			uint index = x + y * count_w;
			if(index < actions.size())
			{
				Matrix mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(0.5f, 0.5f), nullptr, 0.f, &Vec2(shift));
				GUI.DrawSprite2(actions[index]->tex->tex, mat);
			}
		}
	}

	// tooltips
	tooltip.Draw();
}

//=================================================================================================
void ActionPanel::Event(GuiEvent e)
{
	GamePanel::Event(e);

	if(e == GuiEvent_Show)
		tooltip.Clear();
}

//=================================================================================================
void ActionPanel::Update(float dt)
{
	GamePanel::Update(dt);

	if(!focus)
		return;

	int group = -1, id = -1;

	int count_w = (size.x - 48) / 63;
	int count_h = (size.y - 64 - 34) / 63;
	int shift_x = pos.x + 12 + (size.x - 48) % 63 / 2;
	int shift_y = pos.y + 48 + (size.y - 64 - 34) % 63 / 2;
	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			uint index = x + y * count_w;
			if(index >= actions.size())
				break;
			if(PointInRect(GUI.cursor_pos, Int2(shift_x + x * 63, shift_y + y * 63), Int2(64, 64)))
			{
				group = 0;
				id = index;
				break;
			}
		}
	}

	tooltip.UpdateTooltip(dt, group, id);

	if(Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Hide();
}

//=================================================================================================
void ActionPanel::GetTooltip(TooltipController*, int group, int id)
{
	if(group == -1)
	{
		tooltip.anything = false;
		return;
	}

	Action& action = *actions[id];
	tooltip.anything = true;
	tooltip.img = action.tex->tex;
	tooltip.big_text = action.name;
	tooltip.text = action.desc;
	if(action.charges == 1)
		tooltip.small_text = Format(txCooldown, action.cooldown);
	else
		tooltip.small_text = Format(txCooldownCharges, action.cooldown, action.charges, action.recharge);
}
