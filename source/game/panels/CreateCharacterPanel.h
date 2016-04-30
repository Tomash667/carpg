// character creation screen
#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
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
#include "FlowContainer2.h"
#include "CreatedCharacter.h"

//-----------------------------------------------------------------------------
struct Unit;
class PickItemDialog;

//-----------------------------------------------------------------------------
struct FlowItem
{
	bool section;
	int group, id, y;
	float part;

	FlowItem(int group, int id, int y) : section(true), group(group), id(id), y(y)
	{

	}

	FlowItem(int group, int id, int min, int max, int value, int y) : section(false), group(group), id(id), y(y)
	{
		part = float(value - min) / (max - min);
	}
};

//-----------------------------------------------------------------------------
class CreateCharacterPanel : public Dialog
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
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	void Init();
	void LoadData();
	void Show(bool enter_name);
	void ShowRedo(Class clas, int hair_index, HumanData& hd, CreatedCharacter& cc);

	// data
	CustomButton custom_x, custom_bt[2];

	// results
	CreatedCharacter cc;
	Class clas;
	string player_name;
	Unit* unit;
	int hair_index;

private:
	enum DOLL_ANIM
	{
		DA_STOI,
		DA_IDZIE,
		DA_ROZGLADA,
		DA_WYJMIJ_BRON,
		DA_SCHOWAJ_BRON,
		DA_WYJMIJ_LUK,
		DA_SCHOWAJ_LUK,
		DA_ATAK,
		DA_STRZAL,
		DA_BLOK,
		DA_BOJOWY
	};

	void SetControls();
	void SetCharacter();
	void OnEnterName(int id);
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
	void OnShowWarning(int id);
	void PickAttribute(cstring text, Perk picked_perk);
	void PickSkill(cstring text, Perk picked_perk, bool positive = true, int multiple = 0);
	void OnPickAttributeForPerk(int id);
	void OnPickSkillForPerk(int id);
	void UpdateSkill(Skill s, int value, bool mod);
	void UpdateSkillButtons();
	void AddPerk(Perk perk, int value = 0, bool apply = true);
	bool ValidatePerk(Perk perk);
	void CheckSkillsUpdate();
	void UpdateInventory();
	void ResetDoll(bool instant);
	
	Mode mode;
	bool enter_name;
	// unit
	DOLL_ANIM anim, anim2;
	float t, dist;
	// controls
	Button btCancel, btNext, btBack, btCreate, btRandomSet;
	CheckBox checkbox;
	Slider2 slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc, tbInfo;
	FlowContainer2 flowSkills, flowPerks;
	// attribute/skill flow panel
	INT2 flow_pos, flow_size;
	Scrollbar flow_scroll;
	vector<FlowItem> flow_items;
	TooltipController tooltip;
	// data
	bool reset_skills_perks, rotating;
	cstring txHardcoreMode, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName, txAttributes, txRelatedAttributes, txCreateCharWarn, txSkillPoints, txPerkPoints,
		txPickAttribIncrease, txPickAttribDecrease, txPickTwoSkillsDecrease, txPickSkillIncrease, txAvailablePerks, txUnavailablePerks, txTakenPerks, txCreateCharTooMany;
	Perk picked_perk;
	PickItemDialog* pickItemDialog;
	int step, step_var, step_var2;
	vector<Perk> available_perks, unavailable_perks;
	vector<std::pair<cstring, int>> taken_perks;
	const Item* items[4];
};
