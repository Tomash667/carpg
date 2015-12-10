#pragma once

//-----------------------------------------------------------------------------
#include "Gui2.h"
#include "Button.h"
#include "Checkbox.h"

//-----------------------------------------------------------------------------
#define BUTTON_OK 0
#define BUTTON_YES 1
#define BUTTON_NO 2
#define BUTTON_CANCEL 3
#define BUTTON_CHECKED 4
#define BUTTON_UNCHECKED 5
#define BUTTON_CUSTOM 6

//-----------------------------------------------------------------------------
struct Game;

//-----------------------------------------------------------------------------
class Dialog : public Control
{
public:
	explicit Dialog(const DialogInfo& info);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	inline void CloseDialog()
	{
		GUI.CloseDialog(this);
	}

	string name, text;
	GUI_DialogType type;
	DialogEvent event;
	DialogOrder order;
	bool pause, need_delete;

	vector<Button> bts;
	int result;

	static Game* game;
	static TEX tBackground;
};

//-----------------------------------------------------------------------------
class DialogWithCheckbox : public Dialog
{
public:
	explicit DialogWithCheckbox(const DialogInfo& info);
	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

	CheckBox checkbox;
};
