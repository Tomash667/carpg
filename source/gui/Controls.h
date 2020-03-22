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
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	cstring GetKeyText(Key key) { return key_text[(int)key]; }

private:
	void GetCell(int item, int column, Cell& cell);
	void InitKeyText();
	void SelectCell(int item, int column, int button);
	void OnKey(int key);

	Grid grid;
	cstring key_text[255], txResetConfirm, txInfo;
	int picked, picked_n;
	float cursor_tick;
	bool changed;
};
