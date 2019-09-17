#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"
#include "Container.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
class GamePanel : public Control
{
	friend class GamePanelContainer;

public:
	enum BOX_STATE
	{
		BOX_NOT_VISIBLE,
		BOX_COUNTING,
		BOX_VISIBLE
	};

	static const int INDEX_INVALID = -1;

	GamePanel();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent e) override;
	bool NeedCursor() const override { return true; }

	void DrawBox();
	void UpdateBoxIndex(float dt, int index, int index2 = -1, bool refresh = false);
	virtual void FormatBox(bool refresh) {}

	static TexturePtr tBackground, tDialog;
	uint order;

private:
	void DrawBoxInternal();

protected:
	BOX_STATE box_state;
	float show_timer, box_alpha;
	int last_index, last_index2;
	string box_text, box_text_small;
	Texture* box_img;
	Rect box_big, box_small;
	Int2 box_size, box_pos;
	Int2 box_img_pos, box_img_size;
};

//-----------------------------------------------------------------------------
class GamePanelContainer : public Container
{
public:
	GamePanelContainer();
	void Draw(ControlDrawData*);
	void Update(float dt) override;
	bool NeedCursor() const { return true; }

	void Add(GamePanel* panel);
	void Show();
	void Hide();

	GamePanel* draw_box;

private:
	int order;
	bool lost_focus;
};
