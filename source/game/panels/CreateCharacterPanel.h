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

//-----------------------------------------------------------------------------
struct Unit;

//-----------------------------------------------------------------------------
class ValueBar : public Control
{
public:
	int value, min, max;
	string base_text, text;
	Font* font;
	DWORD color;
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
		PickAppearance
	} mode;
	Button bts2[2];
	CheckBox checkbox;
	Unit* unit;
	Class clas;
	Slider2 slider[5];
	ListBox lbClasses;
	TextBox tbClassDesc;
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
	cstring txNext, txGoBack, txCreate, txHardcoreMode, txHair, txMustache, txBeard, txHairColor, txSize, txCharacterCreation, txName;
	ValueBar vb_attrib[(int)Attribute::MAX], vb_skill[(int)Skill::MAX];

private:
	void OnChangeClass(int index);
};
