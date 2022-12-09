#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include <TooltipController.h>

//-----------------------------------------------------------------------------
// Panel with player abilities and some special shortcuts (use healing/mana potion, take weapon)
class AbilityPanel : public GamePanel
{
public:
	AbilityPanel();
	void LoadLanguage();
	void LoadData();
	void Draw() override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;
	void Refresh();
	void GetAbilityTooltip(TooltipController& tooltip, Ability& ability);

private:
	void DrawGroup(cstring text);
	void UpdateGroup(uint count, int group, int& groupResult, int& idResult);
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);
	pair<int, int> ConvertToShortcut(int group, int id);

	TooltipController tooltip;
	vector<Ability*> abilities;
	vector<Texture*> images;
	cstring txTitle, txCooldown, txCooldownCharges, txCost, txAbilities, txOther, txMana, txStamina;
	cstring txMeleeWeapon, txRangedWeapon, txHealthPotion, txManaPotion, txMeleeWeaponDesc, txRangedWeaponDesc, txHealthPotionDesc, txManaPotionDesc;
	TexturePtr tItemBar, tMelee, tRanged, tHealthPotion, tManaPotion;
	Int2 dragAndDropPos;
	int grid_offset, dragAndDropGroup, dragAndDropIndex;
	bool dragAndDrop;
};
