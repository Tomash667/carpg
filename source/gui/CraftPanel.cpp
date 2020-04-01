#include "Pch.h"
#include "CraftPanel.h"
#include "Language.h"
#include "PlayerController.h"
#include "Unit.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "ResourceManager.h"
#include "GetNumberDialog.h"
#include "SoundManager.h"
#include "PlayerInfo.h"

struct RecipeItem : public GuiElement
{
	RecipeItem(Recipe* recipe) : recipe(recipe)
	{
		const Item* item = recipe->result;
		game_res->PreloadItem(item);
		text = item->name;
		text += "\n(";
		bool first = true;
		for(pair<const Item*, uint>& p : recipe->items)
		{
			if(first)
				first = false;
			else
				text += "\n";
			if(p.second > 1u)
				text += Format("%ux ", p.second);
			text += p.first->name;
		}
		text += ")";
		tex = item->icon;
	}
	cstring ToString() override
	{
		return text.c_str();
	}

	Recipe* recipe;
	string text;
};

CraftPanel::CraftPanel()
{
	focusable = true;
	visible = false;
	tooltip.Init(TooltipController::Callback(this, &CraftPanel::GetTooltip));

	list.SetItemHeight(50);
	list.SetForceImageSize(Int2(40));
	list.flags = DTF_VCENTER;
	list.parent = this;
	list.event_handler = DialogEvent(this, &CraftPanel::OnSelectionChange);
	list.Initialize();

	button.parent = this;
	button.id = GuiEvent_Custom;

	lastListSize = 0;
	lastListIndex = 0;

	Add(&left);
	Add(&right);
}

void CraftPanel::LoadData()
{
	tItemBar = res_mgr->Load<Texture>("item_bar.png");
	sAlchemy = res_mgr->Load<Sound>("alchemy.mp3");
}

void CraftPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("CraftPanel");
	txAlchemy = s.Get("alchemy");
	button.text = s.Get("craft");
	txCraftCount = s.Get("craftCount");
	txIngredients = s.Get("ingredients");
}

void CraftPanel::Draw(ControlDrawData*)
{
	//============ LEFT PANEL ===============
	left.Draw();

	// title
	Rect rect = {
		left.pos.x + 8,
		left.pos.y + 8,
		left.pos.x + left.size.x - 16,
		left.pos.y + left.size.y - 16
	};
	gui->DrawText(GameGui::font_big, txAlchemy, DTF_TOP | DTF_CENTER, Color::Black, rect);

	list.Draw();
	button.Draw();

	//============ RIGHT PANEL ===============
	right.Draw();

	// title
	rect = {
		right.pos.x + 8,
		right.pos.y + 8,
		right.pos.x + right.size.x - 16,
		right.pos.y + right.size.y - 16
	};
	gui->DrawText(GameGui::font_big, txIngredients, DTF_TOP | DTF_CENTER, Color::Black, rect);

	// ingredients
	int count_w = (right.size.x - 48) / 63;
	int count_h = (right.size.y - 48) / 63;
	int shift_x = right.pos.x + 24 + (right.size.x - 48) % 63 / 2;
	int shift_y = right.pos.y + 50 + (right.size.y - 48) % 63 / 2;

	for(int y = 0; y < count_h; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			Int2 shift(shift_x + x * 63, shift_y + y * 63);
			gui->DrawSprite(tItemBar, shift);
			uint index = x + y * count_w;
			if(index < ingredients.size())
			{
				pair<const Item*, uint>& p = ingredients[index];
				gui->DrawSprite(p.first->icon, Int2(shift + Int2(1)));
				if(p.second > 1u)
				{
					Rect rect3 = Rect::Create(Int2(shift_x + x * 63 + 2, shift_y + y * 63), Int2(64, 63));
					gui->DrawText(GameGui::font, Format("%d", p.second), DTF_BOTTOM, Color::Black, rect3);
				}
			}
		}
	}

	tooltip.Draw();
}

void CraftPanel::Update(float dt)
{
	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
	{
		Hide();
		return;
	}

	int new_skill = game->pc->unit->Get(SkillId::ALCHEMY);
	if(skill != new_skill)
	{
		skill = new_skill;
		SetRecipes();
	}

	Container::Update(dt);

	list.mouse_focus = focus;
	list.Update(dt);

	button.mouse_focus = focus;
	button.Update(dt);

	// ingredients
	int group = -1, id = -1;
	if(focus)
	{
		int count_w = (right.size.x - 48) / 63;
		int count_h = (right.size.y - 48) / 63;
		int shift_x = right.pos.x + 24 + (right.size.x - 48) % 63 / 2;
		int shift_y = right.pos.y + 50 + (right.size.y - 48) % 63 / 2;

		for(int y = 0; y < count_h; ++y)
		{
			for(int x = 0; x < count_w; ++x)
			{
				uint index = x + y * count_w;
				if(index >= ingredients.size())
					break;
				if(PointInRect(gui->cursor_pos, Int2(shift_x + x * 63, shift_y + y * 63), Int2(64, 64)))
				{
					group = 0;
					id = index;
					break;
				}
			}
		}
	}
	tooltip.UpdateTooltip(dt, group, id);
}

void CraftPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		Vec2 scale = Vec2(gui->wnd_size) / Vec2(1024, 768);
		left.pos = Int2(64, 64) * scale;
		left.size = Int2(300, 600) * scale;
		list.size = Int2(left.size.x - 12 * 2, left.size.y - 100 - 12 * 2);
		list.pos = Int2(12, 50);
		list.global_pos = left.pos + list.pos;
		list.Event(GuiEvent_Resize);
		button.pos = Int2(20, left.size.y - 48 - 20);
		button.global_pos = left.pos + button.pos;
		button.size = Int2(left.size.x - 40, 48);

		right.pos = Int2(1024 - 300 - 60, 64) * scale;
		right.size = Int2(300, 600) * scale;

		if(e == GuiEvent_Show)
		{
			left.Event(GuiEvent_Show);
			right.Event(GuiEvent_Show);

			skill = game->pc->unit->Get(SkillId::ALCHEMY);
			SetRecipes();
			SetIngredients();
			if(list.GetItems().empty())
				button.state = Button::DISABLED;
			else
			{
				list.Select(0);
				lastListIndex = 0;
				lastListSize = list.GetCount();
				Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
				button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
			}
			tooltip.Clear();
		}
	}
	else if(e == GuiEvent_GainFocus)
	{
		if(list.GetItems().empty())
			button.state = Button::DISABLED;
		else
		{
			if(lastListSize > list.GetCount())
			{
				list.Select(0);
				lastListIndex = 0;
			}
			else
				list.Select(lastListIndex);

			Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
			button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
		}
	}
	else if(e == GuiEvent_Hide)
	{
		if(game->pc)
			game->pc->unit->StopUsingUsable();
	}
	else if(e == GuiEvent_Custom)
	{
		lastListIndex = list.GetIndex();
		Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
		uint max = HaveIngredients(recipe);
		counter = 1;
		GetNumberDialog::Show(this, delegate<void(int)>(this, &CraftPanel::OnCraft), txCraftCount, 1, max, &counter);
	}
}

void CraftPanel::SetRecipes()
{
	list.Reset();
	for(Recipe* recipe : Recipe::recipes)
	{
		if(skill >= recipe->skill)
		{
			RecipeItem* item = new RecipeItem(recipe);
			list.Add(item);
		}
	}
}

void CraftPanel::SetIngredients()
{
	ingredients.clear();
	for(ItemSlot& slot : game->pc->unit->items)
	{
		if(IsSet(slot.item->flags, ITEM_INGREDIENT))
		{
			game_res->PreloadItem(slot.item);
			ingredients.push_back(std::make_pair(slot.item, slot.count));
		}
	}
}

void CraftPanel::GetTooltip(TooltipController* tooltip, int group, int id, bool refresh)
{
	pair<const Item*, uint>& p = ingredients[id];
	tooltip->anything = true;
	GetItemString(tooltip->text, p.first, nullptr, p.second);
	tooltip->small_text = p.first->desc;
	tooltip->img = p.first->icon;
}

void CraftPanel::OnCraft(int id)
{
	if(id == BUTTON_CANCEL || counter == 0)
		return;
	Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
	if(Net::IsLocal())
	{
		for(pair<const Item*, uint>& p : recipe->items)
			game->pc->unit->RemoveItem(p.first, p.second * counter);
		game->pc->unit->AddItem2(recipe->result, counter, 0u);
		float value = ((float)recipe->skill + 25) / 25.f * 1000 * counter;
		game->pc->Train(TrainWhat::Craft, value, 0);
		sound_mgr->PlaySound2d(sAlchemy);
		SetIngredients();
		button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
	}
	else
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CRAFT;
		c.recipe = recipe;
		c.count = counter;
	}
}

void CraftPanel::OnSelectionChange(int index)
{
	Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
	button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
}

uint CraftPanel::HaveIngredients(Recipe* recipe)
{
	assert(recipe);
	uint max_count = 999u;
	for(pair<const Item*, uint>& p : recipe->items)
	{
		bool ok = false;
		for(pair<const Item*, uint>& p2 : ingredients)
		{
			if(p.first == p2.first)
			{
				if(p2.second >= p.second)
				{
					ok = true;
					max_count = min(p2.second / p.second, max_count);
					break;
				}
				return 0;
			}
		}
		if(!ok)
			return 0;
	}
	return max_count;
}

bool CraftPanel::DoPlayerCraft(PlayerController& player, Recipe* recipe, uint count)
{
	assert(recipe && count > 0);
	for(pair<const Item*, uint>& p : recipe->items)
	{
		if((uint)player.unit->CountItem(p.first) < p.second * count)
			return false;
	}

	for(pair<const Item*, uint>& p : recipe->items)
		player.unit->RemoveItem(p.first, p.second * count);
	player.unit->AddItem2(recipe->result, count, 0u);
	float value = ((float)recipe->skill + 25) / 25.f * 1000 * count;
	player.Train(TrainWhat::Craft, value, 0);

	NetChangePlayer& c = Add1(player.player_info->changes);
	c.type = NetChangePlayer::AFTER_CRAFT;

	return true;
}

void CraftPanel::AfterCraft()
{
	Recipe* recipe = static_cast<RecipeItem*>(list.GetItem())->recipe;
	sound_mgr->PlaySound2d(sAlchemy);
	SetIngredients();
	button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
}
