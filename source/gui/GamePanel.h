#pragma once

//-----------------------------------------------------------------------------
#include <Control.h>
#include <Container.h>
#include <MenuList.h>

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

	void Draw() override;
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
	BOX_STATE boxState;
	float showTimer, boxAlpha;
	int lastIndex, lastIndex2;
	string boxText, boxTextSmall;
	Texture* boxImg;
	Rect boxBig, boxSmall;
	Int2 boxSize, boxPos;
	Int2 boxImgPos, boxImgSize;
};

//-----------------------------------------------------------------------------
class GamePanelContainer : public Container
{
public:
	GamePanelContainer();
	void Draw();
	void Update(float dt) override;
	bool NeedCursor() const { return true; }

	void Add(GamePanel* panel);
	void Show();
	void Hide();

	GamePanel* drawBox;

private:
	int order;
	bool lostFocus;
};
