#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "CheckBox.h"
#include "Slider.h"
#include "Class.h"
#include "ListBox.h"
#include "TextBox.h"
#include "HumanData.h"
#include "Attribute.h"
#include "Skill.h"
#include "Perk.h"
#include "TooltipController.h"
#include "FlowContainer.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
class CreateCharacterPanel : public GameDialogBox
{
public:
	enum class Group
	{
		Section,
		Attribute,
		Skill,
		Perk,
		TakenPerk,
		PickSkill_Button,
		PickPerk_AddButton,
		PickPerk_RemoveButton,
		PickPerk_DisabledButton
	};

	enum class Mode
	{
		PickClass,
		PickSkillPerk,
		PickAppearance
	};

	explicit CreateCharacterPanel(DialogInfo& info);
	~CreateCharacterPanel();
	void LoadLanguage();
	void LoadData();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Init();
	void Show(bool enter_name);
	void ShowRedo(Class clas, HumanData& hd, CreatedCharacter& cc);

	// data
	CustomButton custom_x, custom_bt[2];

	// results
	CreatedCharacter cc;
	Class clas;
	string player_name;
	Unit* unit;
	int hair_color_index, last_hair_color_index;

private:
	enum DOLL_ANIM
	{
		DA_STAND,
		DA_WALK,
		DA_LOOKS_AROUND,
		DA_SHOW_WEAPON,
		DA_HIDE_WEAPON,
		DA_SHOW_BOW,
		DA_HIDE_BOW,
		DA_ATTACK,
		DA_SHOOT,
		DA_BLOCK,
		DA_BATTLE_MODE
	};

	void SetControls();
	void SetCharacter();
	void RenderUnit();
	void UpdateUnit(float dt);
	void OnChangeClass(int index);
	cstring GetText(int group, int id);
	void GetTooltip(TooltipController* tooltip, int group, int id);
	void ClassChanged();
	void OnPickSkill(int group, int id);
	void OnPickPerk(int group, int id);
	void RebuildSkillsFlow();
	void RebuildPerksFlow();
	void ResetSkillsPerks();
	void PickAttribute(cstring text, Perk picked_perk);
	void PickSkill(cstring text, Perk picked_perk);
	void OnPickAttributeForPerk(int id);
	void OnPickSkillForPerk(int id);
	void UpdateSkillButtons();
	void AddPerk(Perk perk, int value = -1);
	bool ValidatePerk(Perk perk);
	void CheckSkillsUpdate();
	void UpdateInventory();
	void ResetDoll(bool instant);
	void RandomAppearance();

	Mode mode;
	bool enter_name;
	// unit
	DOLL_ANIM anim, anim2;
	float t, dist;
	// controls
	Button btCancel, btNext, btBack, btCreate, btRandomSet;
	CheckBox checkbox;
	Slider slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc, tbInfo;
	FlowContainer flowSkills, flowPerks;
	// attribute/skill flow panel
	Int2 flow_pos, flow_size;
	Scrollbar flow_scroll;
	vector<OldFlowItem> flow_items;
	TooltipController tooltip;
	// data
	bool reset_skills_perks, rotating;
	cstring txHardcoreMode, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName, txAttributes, txRelatedAttributes, txCreateCharWarn,
		txSkillPoints, txPerkPoints, txPickAttribIncrease, txPickSkillIncrease, txAvailablePerks, txTakenPerks, txCreateCharTooMany, txFlawExtraPerk,
		txPerksRemoved;
	Perk picked_perk;
	PickItemDialog* pickItemDialog;
	vector<Perk> available_perks;
	vector<pair<cstring, int>> taken_perks;
	const Item* items[SLOT_MAX];
	Texture* tKlasaCecha;
	RenderTarget* rt_char;
};
