#pragma once

//-----------------------------------------------------------------------------
#include "GamePanel.h"
#include "TooltipController.h"

//-----------------------------------------------------------------------------
class ActionPanel : public GamePanel
{
public:
	ActionPanel();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	void Update(float dt) override;
	void Init(Action* action);

private:
	void DrawGroup(cstring text);
	void UpdateGroup(uint count, int group, int& group_result, int& id_result);
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);
	int ConvertToShortcutSpecial(int group, int id);

	TooltipController tooltip;
	vector<Action*> actions;
	vector<Texture*> images;
	cstring txActions, txCooldown, txCooldownCharges, txAbilities, txOther;
	cstring txMeleeWeapon, txRangedWeapon, txPotion, txMeleeWeaponDesc, txRangedWeaponDesc, txPotionDesc;
	TexturePtr tItemBar, tMelee, tRanged, tPotion;
	Int2 drag_and_drop_pos;
	int grid_offset, drag_and_drop_group, drag_and_drop_index;
	bool drag_and_drop;
};
