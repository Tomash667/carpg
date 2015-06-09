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
#include "TooltipController.h"
#include "FlowContainer2.h"

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
struct FlowItem
{
	enum class Type
	{
		Section,
		Attribute,
		Skill
	};

	Type type;
	int id, y;
	float part;

	FlowItem(int id, int y) : type(Type::Section), id(id), y(y)
	{

	}

	FlowItem(Type type, int id, int min, int max, int value, int y) : type(type), id(id), y(y)
	{
		part = float(value - min) / (max - min);
	}
};

//-----------------------------------------------------------------------------
class CreateCharacterPanel : public Dialog
{
public:
	CreateCharacterPanel(DialogInfo& info);
	~CreateCharacterPanel();
	void Draw(ControlDrawData* cdd = NULL);
	void Update(float dt);
	void Event(GuiEvent e);

	void InitInventory();
	void OnEnterName(int id);
	void RenderUnit();
	void UpdateUnit(float dt);
	void Random(Class clas = Class::RANDOM);
	void Redo(Class clas, HumanData& hd);
	void Init();

	// config
	bool enter_name;

	// data
	enum Mode
	{
		PickClass,
		PickSkillPerk,
		PickAppearance
	} mode;
	Unit* unit;
	Class clas;
	string name;
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
	} anim, anim2;
	float t, dist;
	int height;
	cstring txHardcoreMode, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName, txAttributes, txRelatedAttributes;

private:
	void OnChangeClass(int index);
	cstring GetText(FlowItem::Type type, int id);
	void GetTooltip(TooltipController* tooltip, int group, int id);
	void ClassChanged();
	void OnPickSkill(int group, int id);
	void OnPickPerk(int group, int id);
	
	// controls
	Button btCancel, btNext, btBack, btCreate;
	CustomButton custom_x, custom_bt[2];
	CheckBox checkbox;
	Slider2 slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc;
	FlowContainer2 flowSkills, flowPerks;
	// attribute/skill flow panel
	INT2 flow_pos, flow_size;
	Scrollbar flow_scroll;
	vector<FlowItem> flow_items;
	TooltipController tooltip;
};
