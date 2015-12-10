#pragma once

//-----------------------------------------------------------------------------
#include "Dialog2.h"
#include "Grid.h"

//-----------------------------------------------------------------------------
class Controls : public Dialog
{
public:
	explicit Controls(const DialogInfo& info);
	void Draw(ControlDrawData* cdd = nullptr);
	void Update(float dt);
	void Event(GuiEvent e);

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
