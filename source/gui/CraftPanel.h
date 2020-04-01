#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include <Button.h>
#include <ListBox.h>
#include <TooltipController.h>

//-----------------------------------------------------------------------------
// Panel shown to allow player to craft potion
class CraftPanel : public Container
{
public:
	CraftPanel();
	void LoadData();
	void LoadLanguage();
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	bool DoPlayerCraft(PlayerController& player, Recipe* recipe, uint count);
	void AfterCraft();

private:
	void SetIngredients();
	void SetRecipes();
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);
	void OnCraft(int id);
	void OnSelectionChange(int index);
	uint HaveIngredients(Recipe* recipe);

	vector<pair<const Item*, uint>> ingredients;
	int skill, counter;
	uint lastListIndex, lastListSize;
	GamePanel left, right;
	ListBox list;
	Button button;
	TooltipController tooltip;
	TexturePtr tItemBar;
	SoundPtr sAlchemy;
	cstring txAlchemy, txCraftCount, txIngredients;
};
