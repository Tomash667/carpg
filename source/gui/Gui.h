// gui
#pragma once

//-----------------------------------------------------------------------------
struct Game;
struct GUI_Layer;

//-----------------------------------------------------------------------------
typedef void (Game::*DialogDrawEvent)(GUI_Layer*);
typedef void (Game::*DialogEvent)(int);
typedef void (Game::*DialogUpdateEvent)(GUI_Layer*, float);

//-----------------------------------------------------------------------------
#define BUTTON_OK 0
#define BUTTON_YES 1
#define BUTTON_NO 2

//-----------------------------------------------------------------------------
struct GUI_Button
{
	enum State
	{
		NONE,
		FLASH,
		PRESSED,
		DISABLED
	};
	string text;
	State state;
	VEC2 pos;
	INT2 size;
	int id;
	TEX img;

	bool IsInside(const INT2& pt) const
	{
		return (pt.x >= pos.x && pt.y >= pos.y && pt.x <= pos.x+size.x && pt.y <= pos.y+size.y);
	}
	bool IsInside(const INT2& pt, const VEC2& shift) const
	{
		VEC2 rpos = pos + shift;
		return (pt.x >= rpos.x && pt.y >= rpos.y && pt.x <= rpos.x+size.x && pt.y <= rpos.y+size.y);
	}
};

//-----------------------------------------------------------------------------
struct GUI_Layer
{
	string name;
	DialogUpdateEvent eUpdate;
	DialogDrawEvent eDraw;
	int type; // 0 - zwi¹zane z ekwipunkiem, 1 - pozosta³e, 2 - zwi¹zne z dialogiem, 3 - menu
	bool modal, top, dialog;
};

//-----------------------------------------------------------------------------
struct CustomBoxData
{
	string b1_text, b2_text, tick_text;
	GUI_Button::State tick_state;
	bool have_tick, ticked;
};

//-----------------------------------------------------------------------------
struct GUI_Dialog : public GUI_Layer
{
	enum Type
	{
		DIALOG_OK,
		DIALOG_YESNO,
		DIALOG_CUSTOM
	};

	Type dialog_type;
	string text;
	DialogEvent func;
	vector<GUI_Button> buttons;
	INT2 size;
	CustomBoxData* custom;
};

//-----------------------------------------------------------------------------
struct GUI_DialogInfo
{
	string name, text;
	DialogEvent func;
	GUI_Dialog::Type dialog_type;
	CustomBoxData* custom;
	int type;
	bool modal, top;

	GUI_DialogInfo() : custom(nullptr), type(1)
	{
	}
};
