#pragma once

//-----------------------------------------------------------------------------
#include <DialogBox.h>
#include <Grid.h>

//-----------------------------------------------------------------------------
// Keyboard & mouse controls, view or change
class Controls : public DialogBox
{
public:
	explicit Controls(const DialogInfo& info);
	void LoadLanguage();
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	cstring GetKeyText(Key key) { return txKey[(int)key]; }

private:
	void GetCell(int item, int column, Cell& cell);
	void InitKeyText();
	void SelectCell(int item, int column, int button);
	void OnKey(Key key);

	Grid grid;
	cstring txKey[255], txResetConfirm, txInfo, txTitle;
	int pickedKey, pickedIndex;
	float cursorTick;
	bool changed;
};
