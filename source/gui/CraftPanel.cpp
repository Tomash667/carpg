#include "Pch.h"
#include "CraftPanel.h"

#include "Game.h"
#include "GameGui.h"
#include "GameMessages.h"
#include "GameResources.h"
#include "Language.h"
#include "LevelGui.h"
#include "Messenger.h"
#include "PlayerController.h"
#include "PlayerInfo.h"
#include "Unit.h"

#include <GetNumberDialog.h>
#include <ResourceManager.h>
#include <SoundManager.h>

enum Group
{
	G_NONE = -1,
	G_RECIPE,
	G_INGREDIENTS
};

struct RecipeItem : public GuiElement
{
	explicit RecipeItem(Recipe* recipe) : recipe(recipe)
	{
		const Item* item = recipe->result;
		gameRes->PreloadItem(item);
		text = item->name;
		text += "\n(";
		bool first = true;
		for(pair<const Item*, uint>& p : recipe->ingredients)
		{
			if(first)
				first = false;
			else
				text += ", ";
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

//=================================================================================================
CraftPanel::CraftPanel()
{
	messenger->Register(Msg::LearnRecipe, Messenger::Delegate(this, &CraftPanel::OnLearnRecipe));

	focusable = true;
	visible = false;
	tooltip.Init(TooltipController::Callback(this, &CraftPanel::GetTooltip));

	list.SetItemHeight(-1);
	list.SetForceImageSize(Int2(40));
	list.textFlags = DTF_VCENTER;
	list.parent = this;
	list.eventHandler = DialogEvent(this, &CraftPanel::OnSelectionChange);
	list.Initialize();

	button.parent = this;
	button.id = GuiEvent_Custom;

	Add(&left);
	Add(&right);
}

//=================================================================================================
void CraftPanel::LoadData()
{
	tItemBar = resMgr->Load<Texture>("item_bar.png");
	tUsed = resMgr->Load<Texture>("equipped.png");
	sAlchemy = resMgr->Load<Sound>("alchemy.mp3");
}

//=================================================================================================
void CraftPanel::LoadLanguage()
{
	Language::Section s = Language::GetSection("CraftPanel");
	txAlchemy = s.Get("alchemy");
	button.text = s.Get("craft");
	txCraftCount = s.Get("craftCount");
	txIngredients = s.Get("ingredients");
}

//=================================================================================================
void CraftPanel::Draw()
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
	gui->DrawText(GameGui::fontBig, txAlchemy, DTF_TOP | DTF_CENTER, Color::Black, rect);

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
	gui->DrawText(GameGui::fontBig, txIngredients, DTF_TOP | DTF_CENTER, Color::Black, rect);

	// ingredients
	Recipe* recipe = nullptr;
	if(RecipeItem* recipeItem = list.GetItemCast<RecipeItem>())
		recipe = recipeItem->recipe;
	int countW = (right.size.x - 48) / 63;
	int countH = (right.size.y - 66) / 63;
	int shiftX = right.pos.x + 24 + (right.size.x - 48 - countW * 63) / 2;
	int shiftY = right.pos.y + 50 + (right.size.y - 66 - countH * 63) / 2;

	for(int y = 0; y < countH; ++y)
	{
		for(int x = 0; x < countW; ++x)
		{
			Int2 shift(shiftX + x * 63, shiftY + y * 63);
			gui->DrawSprite(tItemBar, shift);
			uint index = x + y * countW;
			if(index < ingredients.size())
			{
				pair<const Item*, uint>& p = ingredients[index];
				if(recipe)
				{
					bool isUsed = false;
					for(pair<const Item*, uint>& p2 : recipe->ingredients)
					{
						if(p.first == p2.first)
						{
							isUsed = true;
							break;
						}
					}

					if(isUsed)
						gui->DrawSprite(tUsed, Int2(shift + Int2(1)));
				}

				gui->DrawSprite(p.first->icon, Int2(shift + Int2(1)));
				if(p.second > 1u)
				{
					Rect rect3 = Rect::Create(Int2(shiftX + x * 63 + 5, shiftY + y * 63 - 3), Int2(64, 63));
					gui->DrawText(GameGui::font, Format("%d", p.second), DTF_BOTTOM | DTF_OUTLINE, Color::White, rect3);
				}
			}
		}
	}

	tooltip.Draw();
}

//=================================================================================================
void CraftPanel::Update(float dt)
{
	if(focus && input->Focus() && input->PressedRelease(Key::Escape))
	{
		Hide();
		return;
	}

	Container::Update(dt);

	int group = G_NONE, id = -1;

	// recipes
	list.mouseFocus = focus;
	list.focus = focus;
	list.Update(dt);
	if(int hover = list.GetHover(); hover != -1)
	{
		group = G_RECIPE;
		id = hover;
	}

	button.mouseFocus = focus;
	button.Update(dt);

	// ingredients
	if(focus)
	{
		int countW = (right.size.x - 48) / 63;
		int countH = (right.size.y - 66) / 63;
		int shiftX = right.pos.x + 24 + (right.size.x - 48 - countW * 63) / 2;
		int shiftY = right.pos.y + 50 + (right.size.y - 66 - countH * 63) / 2;

		for(int y = 0; y < countH; ++y)
		{
			for(int x = 0; x < countW; ++x)
			{
				uint index = x + y * countW;
				if(index >= ingredients.size())
					break;
				if(Rect::IsInside(gui->cursorPos, Int2(shiftX + x * 63, shiftY + y * 63), Int2(64, 64)))
				{
					group = G_INGREDIENTS;
					id = index;
					break;
				}
			}
		}
	}
	tooltip.UpdateTooltip(dt, group, id);
}

//=================================================================================================
void CraftPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		Vec2 scale = Vec2(gui->wndSize) / Vec2(1024, 768);
		left.pos = Int2(64, 64) * scale;
		left.size = Int2(300, 600) * scale;
		list.size = Int2(left.size.x - 12 * 2, left.size.y - 100 - 12 * 2);
		list.pos = Int2(12, 50);
		list.globalPos = left.pos + list.pos;
		list.Event(GuiEvent_Resize);
		button.pos = Int2(20, left.size.y - 48 - 20);
		button.globalPos = left.pos + button.pos;
		button.size = Int2(left.size.x - 40, 48);

		right.pos = Int2(1024 - 300 - 60, 64) * scale;
		right.size = Int2(300, 600) * scale;

		if(e == GuiEvent_Show)
		{
			left.Event(GuiEvent_Show);
			right.Event(GuiEvent_Show);

			SetRecipes(false);
			SetIngredients();
			if(list.GetItems().empty())
				button.state = Button::DISABLED;
			else
			{
				list.Select(0);
				Recipe* recipe = list.GetItemCast<RecipeItem>()->recipe;
				button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
			}
			tooltip.Clear();
		}
	}
	else if(e == GuiEvent_Hide)
	{
		if(game->pc)
			game->pc->unit->StopUsingUsable();
	}
	else if(e == GuiEvent_Custom)
	{
		Recipe* recipe = list.GetItemCast<RecipeItem>()->recipe;
		uint max = HaveIngredients(recipe);
		if(max == 1 || input->Down(Key::Control))
		{
			// craft one
			counter = 1;
			OnCraft(BUTTON_OK);
		}
		else if(input->Down(Key::Shift))
		{
			// craft max
			counter = max;
			OnCraft(BUTTON_OK);
		}
		else
		{
			counter = 1;
			GetNumberDialogParams params(counter, 1, max);
			params.parent = this;
			params.event = DialogEvent(this, &CraftPanel::OnCraft);
			params.text = txCraftCount;
			GetNumberDialog::Show(params);
		}
	}
}

//=================================================================================================
void CraftPanel::SetRecipes(bool rememberRecipe)
{
	Recipe* prevRecipe = nullptr;
	if(rememberRecipe && list.GetIndex() != -1)
		prevRecipe = list.GetItemCast<RecipeItem>()->recipe;

	list.Reset();
	for(MemorizedRecipe& recipe : game->pc->recipes)
	{
		RecipeItem* item = new RecipeItem(recipe.recipe);
		list.Add(item);
	}

	vector<RecipeItem*>& items = list.GetItemsCast<RecipeItem>();
	std::sort(items.begin(), items.end(), [](RecipeItem* a, RecipeItem* b)
	{
		return a->recipe->order < b->recipe->order;
	});

	if(prevRecipe)
	{
		int index = 0;
		for(RecipeItem* item : items)
		{
			if(item->recipe == prevRecipe)
			{
				list.Select(index);
				list.ScrollTo(index);
				break;
			}
			++index;
		}
	}
}

//=================================================================================================
void CraftPanel::SetIngredients()
{
	ingredients.clear();
	for(ItemSlot& slot : game->pc->unit->items)
	{
		if(IsSet(slot.item->flags, ITEM_INGREDIENT))
		{
			gameRes->PreloadItem(slot.item);
			ingredients.push_back(std::make_pair(slot.item, slot.count));
		}
	}
}

//=================================================================================================
void CraftPanel::GetTooltip(TooltipController* tooltip, int group, int id, bool refresh)
{
	const Item* item;
	uint count;
	if(group == G_RECIPE)
	{
		Recipe* recipe = list.GetItems()[id]->Cast<RecipeItem>()->recipe;
		item = recipe->result;
		count = 1u;
	}
	else
	{
		pair<const Item*, uint>& p = ingredients[id];
		item = p.first;
		count = p.second;
	}

	tooltip->anything = true;
	GetItemString(tooltip->text, item, nullptr, count);
	tooltip->smallText = item->desc;
	tooltip->img = item->icon;
}

//=================================================================================================
void CraftPanel::OnCraft(int id)
{
	if(id == BUTTON_CANCEL || counter == 0)
		return;
	Recipe* recipe = list.GetItemCast<RecipeItem>()->recipe;
	if(Net::IsLocal())
	{
		DoCraft(*game->pc, recipe, counter);
		AfterCraft();
	}
	else
	{
		NetChange& c = Net::PushChange(NetChange::CRAFT);
		c.recipe = recipe;
		c.count = counter;
	}
}

//=================================================================================================
void CraftPanel::DoCraft(PlayerController& player, Recipe* recipe, uint count)
{
	uint teamCount = count;
	for(pair<const Item*, uint>& p : recipe->ingredients)
	{
		uint ingredientTeamCount = player.unit->GetItemTeamCount(p.first) / p.second;
		teamCount = min(teamCount, ingredientTeamCount);
		player.unit->RemoveItem(p.first, p.second * count);
	}
	player.unit->AddItem2(recipe->result, count, teamCount);
	float value = ((float)recipe->skill + 25) / 25.f * 1000 * count;
	player.Train(TrainWhat::Craft, value, 0);
}

//=================================================================================================
void CraftPanel::OnSelectionChange(int index)
{
	Recipe* recipe = list.GetItemCast<RecipeItem>()->recipe;
	button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
}

//=================================================================================================
uint CraftPanel::HaveIngredients(Recipe* recipe)
{
	assert(recipe);
	uint maxCount = 999u;
	for(pair<const Item*, uint>& p : recipe->ingredients)
	{
		bool ok = false;
		for(pair<const Item*, uint>& p2 : ingredients)
		{
			if(p.first == p2.first)
			{
				if(p2.second >= p.second)
				{
					ok = true;
					maxCount = min(p2.second / p.second, maxCount);
					break;
				}
				return 0;
			}
		}
		if(!ok)
			return 0;
	}
	return maxCount;
}

//=================================================================================================
void CraftPanel::Show()
{
	gameGui->levelGui->ClosePanels();
	Control::Show();
}

//=================================================================================================
bool CraftPanel::DoPlayerCraft(PlayerController& player, Recipe* recipe, uint count)
{
	assert(recipe && count > 0);
	for(pair<const Item*, uint>& p : recipe->ingredients)
	{
		if((uint)player.unit->GetItemCount(p.first) < p.second * count)
			return false;
	}

	DoCraft(player, recipe, count);
	player.playerInfo->PushChange(NetChangePlayer::AFTER_CRAFT);
	return true;
}

//=================================================================================================
void CraftPanel::AfterCraft()
{
	Recipe* recipe = list.GetItemCast<RecipeItem>()->recipe;
	soundMgr->PlaySound2d(sAlchemy);
	SetIngredients();
	button.state = HaveIngredients(recipe) >= 1u ? Button::NONE : Button::DISABLED;
}

//=================================================================================================
void CraftPanel::OnLearnRecipe()
{
	gameGui->messages->AddGameMsg3(GMS_LEARNED_RECIPE);
	if(visible)
		SetRecipes(true);
}
