#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class Scrollbar;

//-----------------------------------------------------------------------------
class TextBox : public Control, public OnCharHandler
{
public:
	explicit TextBox(bool with_scrollbar = false, bool is_new = false);
	~TextBox();

	// from Control
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

	void AddScrollbar();
	void Move(const INT2& global_pos);
	void Add(cstring str);
	void CalculateOffset(bool center);
	void Reset();
	void UpdateScrollbar();
	void UpdateSize(const INT2& pos, const INT2& size);
	void SetText(cstring text);
	string& GetText() { return text; }
	void SelectAll();
	void SetBackground(TEX t) { tBackground = t; }
	TEX GetBackground() { return tBackground; }
	bool IsReadonly() const { return readonly; }
	void SetReadonly(bool new_readonly) { readonly = new_readonly; }

	static TEX tBox;
	int limit, num_min, num_max;
	cstring label;
	Scrollbar* scrollbar;
	bool numeric, multiline;

private:
	void ValidateNumber();
	void GetCaretPos(int x, int& index, int& pos);
	void CalculateSelection(int new_index, int new_pos);
	void CalculateSelection(int index1, int pos1, int index2, int pos2);
	void DeleteSelection();
	int IndexToPos(int index);

	string text;
	float caret_blink, offset_move;
	int caret_index, caret_pos, select_start_index, select_end_index, select_start_pos, select_end_pos, select_fixed_index, offset;
	TEX tBackground;
	bool added, with_scrollbar, down, readonly;
};
