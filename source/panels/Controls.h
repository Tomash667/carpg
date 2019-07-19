#pragma once

//-----------------------------------------------------------------------------
#include "GameDialogBox.h"
#include "Grid.h"

//-----------------------------------------------------------------------------
class Controls : public GameDialogBox
{
public:
	explicit Controls(const DialogInfo& info);
	void LoadLanguage();
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	void GetCell(int item, int column, Cell& cell);
	void InitKeyText();
	void SelectCell(int item, int column, int button);
	void OnKey(int key);

	Grid grid;
	cstring key_text[255];
	int picked, picked_n;
	float cursor_tick;
	bool changed;
};
