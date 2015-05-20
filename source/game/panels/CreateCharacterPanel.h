#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "CheckBox.h"
#include "Slider.h"
#include "Class.h"

//-----------------------------------------------------------------------------
struct Unit;

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
};
