#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <CheckBox.h>
#include <Slider.h>
#include <ListBox.h>
#include <TextBox.h>
#include <TooltipController.h>
#include <FlowContainer.h>
#include "Class.h"
#include "HumanData.h"
#include "Attribute.h"
#include "Skill.h"
#include "Perk.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
// Show on new game to create player character
class CreateCharacterPanel : public DialogBox
{
	struct OldFlowItem
	{
		bool section;
		int group, id, y;
		float part;

		OldFlowItem(int group, int id, int y) : section(true), group(group), id(id), y(y)
		{
		}

		OldFlowItem(int group, int id, int min, int max, int value, int y) : section(false), group(group), id(id), y(y)
		{
			part = float(value - min) / (max - min);
		}
	};

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
		PickPerk_DisabledButton,
		Hardcore
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
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void Init();
	void Show(bool enterName);
	void ShowRedo(Class* clas, HumanData& hd, CreatedCharacter& cc);

	// results
	CreatedCharacter cc;
	Class* clas;
	string playerName;
	Unit* unit;
	int hairColorIndex, lastHairColorIndex;

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
	void GetTooltip(TooltipController* tooltip, int group, int id, bool refresh);
	void ClassChanged();
	void OnPickSkill(int group, int id);
	void OnPickPerk(int group, int id);
	void RebuildSkillsFlow();
	void RebuildPerksFlow();
	void ResetSkillsPerks();
	void PickAttribute(cstring text, Perk* perk);
	void PickSkill(cstring text, Perk* perk);
	void OnPickAttributeForPerk(int id);
	void OnPickSkillForPerk(int id);
	void UpdateSkillButtons();
	void AddPerk(Perk* perk, int value = -1);
	void CheckSkillsUpdate();
	void UpdateInventory();
	void ResetDoll(bool instant);
	void RandomAppearance();

	Scene* scene;
	Camera* camera;
	Mode mode;
	bool enterName;
	// unit
	DOLL_ANIM anim, anim2;
	float t;
	// controls
	CustomButton customClose, customBt[2];
	Button btCancel, btNext, btBack, btCreate, btRandomSet;
	CheckBox checkbox;
	Slider slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc, tbInfo;
	FlowContainer flowSkills, flowPerks;
	// attribute/skill flow panel
	Int2 flowPos, flowSize;
	Scrollbar flowScroll;
	vector<OldFlowItem> flowItems;
	TooltipController tooltip;
	// data
	bool reset_skills_perks, rotating;
	cstring txHardcoreMode, txHardcoreDesc, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName, txAttributes, txRelatedAttributes,
		txCreateCharWarn, txSkillPoints, txPerkPoints, txPickAttribIncrease, txPickSkillIncrease, txAvailablePerks, txTakenPerks, txCreateCharTooMany,
		txFlawExtraPerk, txPerksRemoved;
	Perk* picked_perk;
	PickItemDialog* pickItemDialog;
	vector<Perk*> availablePerks;
	vector<pair<cstring, int>> takenPerks;
	array<const Item*, SLOT_MAX> items;
	TexturePtr tBox, tPowerBar;
	RenderTarget* rtChar;
};
