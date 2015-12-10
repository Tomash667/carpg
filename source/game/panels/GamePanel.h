#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Container.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
#define BOX_NOT_VISIBLE 0
#define BOX_COUNTING 1
#define BOX_VISIBLE 2

//-----------------------------------------------------------------------------
#define INDEX_INVALID -1

//-----------------------------------------------------------------------------
class GamePanel : public Control
{
	friend class GamePanelContainer;

public:
	GamePanel();

	void Draw(ControlDrawData* cdd=nullptr);
	void Update(float dt);
	void Event(GuiEvent e);
	bool NeedCursor() const { return true; }

	void DrawBox();
	void UpdateBoxIndex(float dt, int index, int index2 = -1);
	virtual void FormatBox() {}

	//INT2 min_size;
	uint order;

	static TEX tBackground;

private:
	void DrawBoxInternal();

	/*bool resizing, draging, shift_size; // zmiana rozmiaru z shiftem zrobiona tylko dla lewego dolnego rogu
	int move_what;
	INT2 move_offset;*/

protected:
	int box_state;
	float show_timer, box_alpha;
	int last_index, last_index2;
	string box_text, box_text_small;
	TEX box_img;
	RECT box_big, box_small;
	INT2 box_size, box_pos;
	INT2 box_img_pos, box_img_size;
};

//-----------------------------------------------------------------------------
class GamePanelContainer : public Container
{
public:
	GamePanelContainer();
	void Draw(ControlDrawData* cdd/* =nullptr */);
	void Update(float dt);
	bool NeedCursor() const { return true; }

	void Add(GamePanel* panel);
	void Show();
	void Hide();

	GamePanel* draw_box;

private:
	int order;
	bool lost_focus;
};
